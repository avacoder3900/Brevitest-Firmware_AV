/**
 * @file validate-cartridge/index.ts
 * @brief Supabase Edge Function for cartridge validation
 * @author Agent DELTA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This Edge Function validates a cartridge UUID against the database,
 * checking status, expiration, and associated assay availability.
 *
 * User Story: CLOUD-007 (validate-cartridge function)
 *
 * Request:
 * POST /validate-cartridge
 * {
 *   "requestId": "uuid",
 *   "deviceId": "string",
 *   "cartridgeUuid": "string (36 chars)",
 *   "timestamp": number,
 *   "firmwareVersion": number,
 *   "dataFormatVersion": number
 * }
 *
 * Response:
 * {
 *   "requestId": "uuid",
 *   "status": "SUCCESS" | "ERROR",
 *   "errorCode": number (optional),
 *   "errorMessage": "string (optional)",
 *   "isValid": boolean,
 *   "cartridgeUuid": "string",
 *   "assayId": "string (8 chars)"
 * }
 */

import { serve } from "https://deno.land/std@0.168.0/http/server.ts";
import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

// Error codes matching CloudProtocol.h
const CloudStatus = {
  SUCCESS: 0,
  ERR_UNAUTHORIZED: 100,
  ERR_DEVICE_NOT_FOUND: 102,
  ERR_CARTRIDGE_NOT_FOUND: 200,
  ERR_CARTRIDGE_USED: 201,
  ERR_CARTRIDGE_EXPIRED: 202,
  ERR_CARTRIDGE_INVALID: 203,
  ERR_ASSAY_NOT_FOUND: 204,
  ERR_ASSAY_INACTIVE: 205,
  ERR_SERVER_ERROR: 400,
  ERR_DATABASE_ERROR: 401,
  ERR_INVALID_REQUEST: 500,
  ERR_MISSING_FIELD: 501,
};

interface ValidateRequest {
  requestId: string;
  deviceId: string;
  cartridgeUuid: string;
  timestamp: number;
  firmwareVersion: number;
  dataFormatVersion: number;
}

interface ValidateResponse {
  requestId: string;
  status: string;
  errorCode?: number;
  errorMessage?: string;
  isValid: boolean;
  cartridgeUuid: string;
  assayId: string;
  serverTimestamp?: number;
}

serve(async (req: Request): Promise<Response> => {
  // CORS headers
  const corsHeaders = {
    "Access-Control-Allow-Origin": "*",
    "Access-Control-Allow-Headers": "authorization, x-client-info, apikey, content-type",
  };

  // Handle CORS preflight
  if (req.method === "OPTIONS") {
    return new Response("ok", { headers: corsHeaders });
  }

  try {
    // Parse request
    const request: ValidateRequest = await req.json();

    // Validate required fields
    if (!request.requestId || !request.deviceId || !request.cartridgeUuid) {
      return new Response(
        JSON.stringify({
          requestId: request.requestId || "",
          status: "ERROR",
          errorCode: CloudStatus.ERR_MISSING_FIELD,
          errorMessage: "Missing required fields: requestId, deviceId, or cartridgeUuid",
          isValid: false,
          cartridgeUuid: request.cartridgeUuid || "",
          assayId: "",
        } as ValidateResponse),
        {
          status: 400,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    console.log(`Validate cartridge: ${request.cartridgeUuid} (request: ${request.requestId})`);

    // Create Supabase client with service role
    const supabaseUrl = Deno.env.get("SUPABASE_URL") ?? "";
    const supabaseServiceKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") ?? "";
    const supabase = createClient(supabaseUrl, supabaseServiceKey);

    // Verify device exists and is authorized
    const { data: device, error: deviceError } = await supabase
      .from("devices")
      .select("id, device_id, api_key")
      .eq("device_id", request.deviceId)
      .single();

    if (deviceError || !device) {
      console.error(`Device not found: ${request.deviceId}`);
      return new Response(
        JSON.stringify({
          requestId: request.requestId,
          status: "ERROR",
          errorCode: CloudStatus.ERR_DEVICE_NOT_FOUND,
          errorMessage: "Device not registered",
          isValid: false,
          cartridgeUuid: request.cartridgeUuid,
          assayId: "",
        } as ValidateResponse),
        {
          status: 401,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Update device last_seen
    await supabase
      .from("devices")
      .update({
        last_seen: new Date().toISOString(),
        firmware_version: request.firmwareVersion,
        data_format_version: request.dataFormatVersion
      })
      .eq("id", device.id);

    // Look up cartridge
    const { data: cartridge, error: cartridgeError } = await supabase
      .from("cartridges")
      .select("id, cartridge_uuid, assay_id, status, expiration_date, validation_count")
      .eq("cartridge_uuid", request.cartridgeUuid)
      .single();

    if (cartridgeError || !cartridge) {
      console.error(`Cartridge not found: ${request.cartridgeUuid}`);

      // Log event
      await supabase.from("device_events").insert({
        device_id: device.id,
        event_type: "validate",
        cartridge_uuid: request.cartridgeUuid,
        success: false,
        error_message: "Cartridge not found",
        event_data: { requestId: request.requestId }
      });

      return new Response(
        JSON.stringify({
          requestId: request.requestId,
          status: "ERROR",
          errorCode: CloudStatus.ERR_CARTRIDGE_NOT_FOUND,
          errorMessage: "Cartridge not found in database",
          isValid: false,
          cartridgeUuid: request.cartridgeUuid,
          assayId: "",
        } as ValidateResponse),
        {
          status: 404,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Check cartridge status
    if (cartridge.status === "used") {
      console.error(`Cartridge already used: ${request.cartridgeUuid}`);

      await supabase.from("device_events").insert({
        device_id: device.id,
        event_type: "validate",
        cartridge_uuid: request.cartridgeUuid,
        success: false,
        error_message: "Cartridge already used",
        event_data: { requestId: request.requestId, status: cartridge.status }
      });

      return new Response(
        JSON.stringify({
          requestId: request.requestId,
          status: "ERROR",
          errorCode: CloudStatus.ERR_CARTRIDGE_USED,
          errorMessage: "Cartridge has already been used",
          isValid: false,
          cartridgeUuid: request.cartridgeUuid,
          assayId: cartridge.assay_id || "",
        } as ValidateResponse),
        {
          status: 409,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Check expiration
    if (cartridge.expiration_date) {
      const expDate = new Date(cartridge.expiration_date);
      const now = new Date();
      if (expDate < now) {
        console.error(`Cartridge expired: ${request.cartridgeUuid} (exp: ${cartridge.expiration_date})`);

        // Update status to expired
        await supabase
          .from("cartridges")
          .update({ status: "expired" })
          .eq("id", cartridge.id);

        await supabase.from("device_events").insert({
          device_id: device.id,
          event_type: "validate",
          cartridge_uuid: request.cartridgeUuid,
          success: false,
          error_message: "Cartridge expired",
          event_data: { requestId: request.requestId, expirationDate: cartridge.expiration_date }
        });

        return new Response(
          JSON.stringify({
            requestId: request.requestId,
            status: "ERROR",
            errorCode: CloudStatus.ERR_CARTRIDGE_EXPIRED,
            errorMessage: "Cartridge has expired",
            isValid: false,
            cartridgeUuid: request.cartridgeUuid,
            assayId: cartridge.assay_id || "",
          } as ValidateResponse),
          {
            status: 409,
            headers: { ...corsHeaders, "Content-Type": "application/json" },
          }
        );
      }
    }

    // Verify associated assay exists and is active
    if (!cartridge.assay_id) {
      console.error(`No assay associated with cartridge: ${request.cartridgeUuid}`);

      await supabase.from("device_events").insert({
        device_id: device.id,
        event_type: "validate",
        cartridge_uuid: request.cartridgeUuid,
        success: false,
        error_message: "No associated assay",
        event_data: { requestId: request.requestId }
      });

      return new Response(
        JSON.stringify({
          requestId: request.requestId,
          status: "ERROR",
          errorCode: CloudStatus.ERR_ASSAY_NOT_FOUND,
          errorMessage: "No assay associated with cartridge",
          isValid: false,
          cartridgeUuid: request.cartridgeUuid,
          assayId: "",
        } as ValidateResponse),
        {
          status: 404,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    const { data: assay, error: assayError } = await supabase
      .from("assays")
      .select("assay_id, is_active")
      .eq("assay_id", cartridge.assay_id)
      .single();

    if (assayError || !assay) {
      console.error(`Assay not found: ${cartridge.assay_id}`);

      await supabase.from("device_events").insert({
        device_id: device.id,
        event_type: "validate",
        cartridge_uuid: request.cartridgeUuid,
        success: false,
        error_message: "Associated assay not found",
        event_data: { requestId: request.requestId, assayId: cartridge.assay_id }
      });

      return new Response(
        JSON.stringify({
          requestId: request.requestId,
          status: "ERROR",
          errorCode: CloudStatus.ERR_ASSAY_NOT_FOUND,
          errorMessage: "Associated assay not found",
          isValid: false,
          cartridgeUuid: request.cartridgeUuid,
          assayId: cartridge.assay_id,
        } as ValidateResponse),
        {
          status: 404,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    if (!assay.is_active) {
      console.error(`Assay inactive: ${cartridge.assay_id}`);

      await supabase.from("device_events").insert({
        device_id: device.id,
        event_type: "validate",
        cartridge_uuid: request.cartridgeUuid,
        success: false,
        error_message: "Associated assay is inactive",
        event_data: { requestId: request.requestId, assayId: cartridge.assay_id }
      });

      return new Response(
        JSON.stringify({
          requestId: request.requestId,
          status: "ERROR",
          errorCode: CloudStatus.ERR_ASSAY_INACTIVE,
          errorMessage: "Associated assay is inactive",
          isValid: false,
          cartridgeUuid: request.cartridgeUuid,
          assayId: cartridge.assay_id,
        } as ValidateResponse),
        {
          status: 409,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Validation successful - update cartridge
    await supabase
      .from("cartridges")
      .update({
        status: "validated",
        validation_count: (cartridge.validation_count || 0) + 1,
        last_validated_at: new Date().toISOString(),
        last_validated_by: device.id,
      })
      .eq("id", cartridge.id);

    // Log successful validation
    await supabase.from("device_events").insert({
      device_id: device.id,
      event_type: "validate",
      cartridge_uuid: request.cartridgeUuid,
      success: true,
      event_data: {
        requestId: request.requestId,
        assayId: cartridge.assay_id,
        validationCount: (cartridge.validation_count || 0) + 1
      }
    });

    console.log(`Validation successful: ${request.cartridgeUuid} -> ${cartridge.assay_id}`);

    return new Response(
      JSON.stringify({
        requestId: request.requestId,
        status: "SUCCESS",
        isValid: true,
        cartridgeUuid: request.cartridgeUuid,
        assayId: cartridge.assay_id,
        serverTimestamp: Math.floor(Date.now() / 1000),
      } as ValidateResponse),
      {
        status: 200,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );

  } catch (error) {
    console.error("Validation error:", error);
    return new Response(
      JSON.stringify({
        requestId: "",
        status: "ERROR",
        errorCode: CloudStatus.ERR_SERVER_ERROR,
        errorMessage: error instanceof Error ? error.message : "Internal server error",
        isValid: false,
        cartridgeUuid: "",
        assayId: "",
      } as ValidateResponse),
      {
        status: 500,
        headers: { "Content-Type": "application/json" },
      }
    );
  }
});
