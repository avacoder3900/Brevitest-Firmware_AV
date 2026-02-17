/**
 * @file load-assay/index.ts
 * @brief Supabase Edge Function for assay loading
 * @author Agent DELTA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This Edge Function retrieves an assay definition including BCODE
 * instructions from the database for download to the device.
 *
 * User Story: CLOUD-007 (load-assay function)
 *
 * Request:
 * POST /load-assay
 * {
 *   "requestId": "uuid",
 *   "deviceId": "string",
 *   "assayId": "string (8 chars)",
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
 *   "assayId": "string",
 *   "duration": number (milliseconds),
 *   "bcode": "string (BCODE instructions)",
 *   "checksum": number (CRC32 of BCODE)
 * }
 */

import { serve } from "https://deno.land/std@0.168.0/http/server.ts";
import { createClient } from "https://esm.sh/@supabase/supabase-js@2";

// Error codes matching CloudProtocol.h
const CloudStatus = {
  SUCCESS: 0,
  ERR_UNAUTHORIZED: 100,
  ERR_DEVICE_NOT_FOUND: 102,
  ERR_ASSAY_NOT_FOUND: 204,
  ERR_ASSAY_INACTIVE: 205,
  ERR_SERVER_ERROR: 400,
  ERR_DATABASE_ERROR: 401,
  ERR_INVALID_REQUEST: 500,
  ERR_MISSING_FIELD: 501,
};

interface LoadAssayRequest {
  requestId: string;
  deviceId: string;
  assayId: string;
  timestamp: number;
  firmwareVersion: number;
}

interface LoadAssayResponse {
  requestId: string;
  status: string;
  errorCode?: number;
  errorMessage?: string;
  assayId: string;
  duration: number;
  bcode: string;
  checksum: number;
  serverTimestamp?: number;
}

// CRC32 calculation for BCODE checksum
function crc32(str: string): number {
  let crc = 0xFFFFFFFF;
  const table = new Uint32Array(256);

  // Generate CRC32 table
  for (let i = 0; i < 256; i++) {
    let c = i;
    for (let j = 0; j < 8; j++) {
      c = (c & 1) ? (0xEDB88320 ^ (c >>> 1)) : (c >>> 1);
    }
    table[i] = c;
  }

  // Calculate CRC
  for (let i = 0; i < str.length; i++) {
    crc = table[(crc ^ str.charCodeAt(i)) & 0xFF] ^ (crc >>> 8);
  }

  return (crc ^ 0xFFFFFFFF) >>> 0;
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
    const request: LoadAssayRequest = await req.json();

    // Validate required fields
    if (!request.requestId || !request.deviceId || !request.assayId) {
      return new Response(
        JSON.stringify({
          requestId: request.requestId || "",
          status: "ERROR",
          errorCode: CloudStatus.ERR_MISSING_FIELD,
          errorMessage: "Missing required fields: requestId, deviceId, or assayId",
          assayId: request.assayId || "",
          duration: 0,
          bcode: "",
          checksum: 0,
        } as LoadAssayResponse),
        {
          status: 400,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    console.log(`Load assay: ${request.assayId} (request: ${request.requestId})`);

    // Create Supabase client with service role
    const supabaseUrl = Deno.env.get("SUPABASE_URL") ?? "";
    const supabaseServiceKey = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY") ?? "";
    const supabase = createClient(supabaseUrl, supabaseServiceKey);

    // Verify device exists
    const { data: device, error: deviceError } = await supabase
      .from("devices")
      .select("id, device_id")
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
          assayId: request.assayId,
          duration: 0,
          bcode: "",
          checksum: 0,
        } as LoadAssayResponse),
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
        firmware_version: request.firmwareVersion
      })
      .eq("id", device.id);

    // Look up assay
    const { data: assay, error: assayError } = await supabase
      .from("assays")
      .select("assay_id, name, duration, bcode, bcode_length, checksum, is_active")
      .eq("assay_id", request.assayId)
      .single();

    if (assayError || !assay) {
      console.error(`Assay not found: ${request.assayId}`);

      // Log event
      await supabase.from("device_events").insert({
        device_id: device.id,
        event_type: "load_assay",
        success: false,
        error_message: "Assay not found",
        event_data: { requestId: request.requestId, assayId: request.assayId }
      });

      return new Response(
        JSON.stringify({
          requestId: request.requestId,
          status: "ERROR",
          errorCode: CloudStatus.ERR_ASSAY_NOT_FOUND,
          errorMessage: "Assay not found in database",
          assayId: request.assayId,
          duration: 0,
          bcode: "",
          checksum: 0,
        } as LoadAssayResponse),
        {
          status: 404,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Check if assay is active
    if (!assay.is_active) {
      console.error(`Assay inactive: ${request.assayId}`);

      await supabase.from("device_events").insert({
        device_id: device.id,
        event_type: "load_assay",
        success: false,
        error_message: "Assay is inactive",
        event_data: { requestId: request.requestId, assayId: request.assayId }
      });

      return new Response(
        JSON.stringify({
          requestId: request.requestId,
          status: "ERROR",
          errorCode: CloudStatus.ERR_ASSAY_INACTIVE,
          errorMessage: "Assay is not active",
          assayId: request.assayId,
          duration: 0,
          bcode: "",
          checksum: 0,
        } as LoadAssayResponse),
        {
          status: 409,
          headers: { ...corsHeaders, "Content-Type": "application/json" },
        }
      );
    }

    // Decode BCODE from bytea (stored as base64 or hex in Supabase)
    let bcodeString: string;
    if (assay.bcode) {
      // If bcode is a Uint8Array or buffer, decode it
      if (typeof assay.bcode === 'object' && assay.bcode.data) {
        // Supabase returns bytea as { data: number[] }
        bcodeString = String.fromCharCode(...assay.bcode.data);
      } else if (typeof assay.bcode === 'string') {
        // If it's already a string (might be hex encoded)
        if (assay.bcode.startsWith('\\x')) {
          // Hex encoded - decode it
          const hexString = assay.bcode.slice(2);
          const bytes = [];
          for (let i = 0; i < hexString.length; i += 2) {
            bytes.push(parseInt(hexString.substr(i, 2), 16));
          }
          bcodeString = String.fromCharCode(...bytes);
        } else {
          bcodeString = assay.bcode;
        }
      } else {
        bcodeString = String(assay.bcode);
      }
    } else {
      bcodeString = "";
    }

    // Verify checksum
    const calculatedChecksum = crc32(bcodeString);

    // Log successful load
    await supabase.from("device_events").insert({
      device_id: device.id,
      event_type: "load_assay",
      success: true,
      event_data: {
        requestId: request.requestId,
        assayId: request.assayId,
        assayName: assay.name,
        bcodeLength: bcodeString.length,
        duration: assay.duration
      }
    });

    console.log(`Assay loaded: ${request.assayId} (${assay.name}, ${bcodeString.length} bytes)`);

    return new Response(
      JSON.stringify({
        requestId: request.requestId,
        status: "SUCCESS",
        assayId: assay.assay_id,
        duration: assay.duration,
        bcode: bcodeString,
        checksum: assay.checksum || calculatedChecksum,
        serverTimestamp: Math.floor(Date.now() / 1000),
      } as LoadAssayResponse),
      {
        status: 200,
        headers: { ...corsHeaders, "Content-Type": "application/json" },
      }
    );

  } catch (error) {
    console.error("Load assay error:", error);
    return new Response(
      JSON.stringify({
        requestId: "",
        status: "ERROR",
        errorCode: CloudStatus.ERR_SERVER_ERROR,
        errorMessage: error instanceof Error ? error.message : "Internal server error",
        assayId: "",
        duration: 0,
        bcode: "",
        checksum: 0,
      } as LoadAssayResponse),
      {
        status: 500,
        headers: { "Content-Type": "application/json" },
      }
    );
  }
});
