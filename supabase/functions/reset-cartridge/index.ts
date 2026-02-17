/**
 * @file reset-cartridge/index.ts
 * @brief Supabase Edge Function for cartridge reset
 * @author Agent DELTA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This Edge Function resets a cartridge status back to 'unused',
 * allowing it to be validated and used again. This is typically
 * used during testing and development.
 *
 * User Story: CLOUD-007 (reset-cartridge function)
 *
 * Request:
 * POST /reset-cartridge
 * {
 *   "requestId": "uuid",
 *   "deviceId": "string",
 *   "cartridgeUuid": "string (36 chars)",
 *   "timestamp": number,
 *   "firmwareVersion": number
 * }
 *
 * Response:
 * {
 *   "requestId": "uuid",
 *   "status": "SUCCESS" | "ERROR",
 *   "errorCode": number (optional),
 *   "errorMessage": "string (optional)",
 *   "cartridgeUuid": "string",
 *   "wasReset": boolean
 * }
 */

import { serve } from "https://deno.land/std@0.168.0/http/server.ts";
import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

// Error codes matching CloudProtocol.h
const CloudStatus = {
  SUCCESS: 0,
  ERR_UNAUTHORIZED: 100,
  ERR_FORBIDDEN: 101,
  ERR_DEVICE_NOT_FOUND: 102,
  ERR_CARTRIDGE_NOT_FOUND: 200,
  ERR_SERVER_ERROR: 400,
  ERR_DATABASE_ERROR: 401,
  ERR_INVALID_REQUEST: 500,
  ERR_MISSING_FIELD: 501,
};

interface ResetCartridgeRequest {
  requestId: string;
  deviceId: string;
  cartridgeUuid: string;
  timestamp: number;
  firmwareVersion: number;
}

interface ResetCartridgeResponse {
  requestId: string;
  status: string;
  errorCode?: number;
  errorMessage?: string;
  cartridgeUuid: string;
  wasReset: boolean;
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
    const request: ResetCartridgeRequest = await req.json();

    // Validate required fields
    if (!request.requestId || !request.deviceId || !request.cartridgeUuid) {
      return new Response(
        JSON.stringify({
          requestId: request.requestId || "",
          status: "ERROR",
          errorCode: CloudStatus.ERR_MISSING_FIELD,
          errorMessage: "Missing required fields: requestId, deviceId, or cartridgeUuid",
          cartridgeUuid: request.cartridgeUuid || "",
          wasReset: false,
        } as ResetCartridgeResponse),
        {
          status: 400,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    console.log(`Reset cartridge: ${request.cartridgeUuid} (request: ${request.requestId})`);

    // Create Supabase client with service role
    const supabaseUrl = Deno.env.get("SUPABASE_URL") ?? "";
    const supabaseServiceKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") ?? "";
    const supabase = createClient(supabaseUrl, supabaseServiceKey);

    // Verify device exists
    const { data: device, error: deviceError } = await supabase
      .from("devices")
      .select("id, device_id, metadata")
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
          cartridgeUuid: request.cartridgeUuid,
          wasReset: false,
        } as ResetCartridgeResponse),
        {
          status: 401,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Check if device is authorized for reset operations
    // This can be controlled via device metadata or a separate permissions table
    const allowReset = device.metadata?.allow_reset ?? true;  // Default to allowing for dev
    if (!allowReset) {
      console.error(`Device not authorized for reset: ${request.deviceId}`);

      await supabase.from("device_events").insert({
        device_id: device.id,
        event_type: "reset",
        cartridge_uuid: request.cartridgeUuid,
        success: false,
        error_message: "Device not authorized for cartridge reset",
        event_data: { requestId: request.requestId }
      });

      return new Response(
        JSON.stringify({
          requestId: request.requestId,
          status: "ERROR",
          errorCode: CloudStatus.ERR_FORBIDDEN,
          errorMessage: "Device not authorized for cartridge reset",
          cartridgeUuid: request.cartridgeUuid,
          wasReset: false,
        } as ResetCartridgeResponse),
        {
          status: 403,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Update device last_seen
    await supabase
      .from("devices")
      .update({
        last_seen: new Date().toISOString(),
        firmware_version: request.firmwareVersion
      })
      .eq("id", device.id);

    // Look up cartridge
    const { data: cartridge, error: cartridgeError } = await supabase
      .from("cartridges")
      .select("id, cartridge_uuid, status, test_result_id")
      .eq("cartridge_uuid", request.cartridgeUuid)
      .single();

    if (cartridgeError || !cartridge) {
      console.error(`Cartridge not found: ${request.cartridgeUuid}`);

      await supabase.from("device_events").insert({
        device_id: device.id,
        event_type: "reset",
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
          cartridgeUuid: request.cartridgeUuid,
          wasReset: false,
        } as ResetCartridgeResponse),
        {
          status: 404,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Record previous status for logging
    const previousStatus = cartridge.status;

    // Delete associated test result if exists (optional - can be configured)
    // For now, keep test results but unlink from cartridge
    if (cartridge.test_result_id) {
      // Option 1: Delete test result
      // await supabase.from("test_results").delete().eq("id", cartridge.test_result_id);

      // Option 2: Archive test result (set status to 'archived')
      await supabase
        .from("test_results")
        .update({ status: "archived" })
        .eq("id", cartridge.test_result_id);

      console.log(`Archived test result: ${cartridge.test_result_id}`);
    }

    // Reset cartridge status
    const { error: updateError } = await supabase
      .from("cartridges")
      .update({
        status: "unused",
        validation_count: 0,
        last_validated_at: null,
        last_validated_by: null,
        test_result_id: null,
      })
      .eq("id", cartridge.id);

    if (updateError) {
      console.error("Failed to reset cartridge:", updateError);

      await supabase.from("device_events").insert({
        device_id: device.id,
        event_type: "reset",
        cartridge_uuid: request.cartridgeUuid,
        success: false,
        error_message: updateError.message,
        event_data: { requestId: request.requestId }
      });

      return new Response(
        JSON.stringify({
          requestId: request.requestId,
          status: "ERROR",
          errorCode: CloudStatus.ERR_DATABASE_ERROR,
          errorMessage: `Database error: ${updateError.message}`,
          cartridgeUuid: request.cartridgeUuid,
          wasReset: false,
        } as ResetCartridgeResponse),
        {
          status: 500,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Log successful reset
    await supabase.from("device_events").insert({
      device_id: device.id,
      event_type: "reset",
      cartridge_uuid: request.cartridgeUuid,
      success: true,
      event_data: {
        requestId: request.requestId,
        previousStatus: previousStatus,
        hadTestResult: !!cartridge.test_result_id
      }
    });

    console.log(`Cartridge reset: ${request.cartridgeUuid} (was: ${previousStatus})`);

    return new Response(
      JSON.stringify({
        requestId: request.requestId,
        status: "SUCCESS",
        cartridgeUuid: request.cartridgeUuid,
        wasReset: true,
        serverTimestamp: Math.floor(Date.now() / 1000),
      } as ResetCartridgeResponse),
      {
        status: 200,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );

  } catch (error) {
    console.error("Reset cartridge error:", error);
    return new Response(
      JSON.stringify({
        requestId: "",
        status: "ERROR",
        errorCode: CloudStatus.ERR_SERVER_ERROR,
        errorMessage: error instanceof Error ? error.message : "Internal server error",
        cartridgeUuid: "",
        wasReset: false,
      } as ResetCartridgeResponse),
      {
        status: 500,
        headers: { "Content-Type": "application/json" },
      }
    );
  }
});
