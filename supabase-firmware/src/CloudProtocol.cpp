/**
 * @file CloudProtocol.cpp
 * @brief Implementation of JSON serialization/deserialization for cloud protocol
 * @date February 2026
 *
 * Extracted from SupabaseClient.cpp for clean separation of concerns.
 * These functions handle all JSON encoding/decoding for Supabase Edge Function
 * communication. Uses manual sprintf/strstr-based parsing (no external JSON library).
 *
 * User Stories Implemented:
 *   - GAMMA-001: Protocol structures with async callback typedefs
 *   - CLOUD-001 through CLOUD-005: Serialize/deserialize for all endpoints
 */

#include "CloudProtocol.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ============================================================================
// CLOUD STATUS TO STRING
// ============================================================================

const char* cloudStatusToString(CloudStatus status) {
    switch (status) {
        case CloudStatus::SUCCESS:              return "SUCCESS";
        case CloudStatus::ERR_NO_CONNECTION:    return "NO_CONNECTION";
        case CloudStatus::ERR_TIMEOUT:          return "TIMEOUT";
        case CloudStatus::ERR_DNS_FAILED:       return "DNS_FAILED";
        case CloudStatus::ERR_TLS_FAILED:       return "TLS_FAILED";
        case CloudStatus::ERR_HTTP_ERROR:       return "HTTP_ERROR";
        case CloudStatus::ERR_UNAUTHORIZED:     return "UNAUTHORIZED";
        case CloudStatus::ERR_FORBIDDEN:        return "FORBIDDEN";
        case CloudStatus::ERR_DEVICE_NOT_FOUND: return "DEVICE_NOT_FOUND";
        case CloudStatus::ERR_CARTRIDGE_NOT_FOUND: return "CARTRIDGE_NOT_FOUND";
        case CloudStatus::ERR_CARTRIDGE_USED:   return "CARTRIDGE_USED";
        case CloudStatus::ERR_CARTRIDGE_EXPIRED: return "CARTRIDGE_EXPIRED";
        case CloudStatus::ERR_CARTRIDGE_INVALID: return "CARTRIDGE_INVALID";
        case CloudStatus::ERR_ASSAY_NOT_FOUND:  return "ASSAY_NOT_FOUND";
        case CloudStatus::ERR_ASSAY_INACTIVE:   return "ASSAY_INACTIVE";
        case CloudStatus::ERR_UPLOAD_FAILED:    return "UPLOAD_FAILED";
        case CloudStatus::ERR_CHECKSUM_MISMATCH: return "CHECKSUM_MISMATCH";
        case CloudStatus::ERR_DUPLICATE_UPLOAD: return "DUPLICATE_UPLOAD";
        case CloudStatus::ERR_INVALID_FORMAT:   return "INVALID_FORMAT";
        case CloudStatus::ERR_SERVER_ERROR:     return "SERVER_ERROR";
        case CloudStatus::ERR_DATABASE_ERROR:   return "DATABASE_ERROR";
        case CloudStatus::ERR_SERVICE_UNAVAILABLE: return "SERVICE_UNAVAILABLE";
        case CloudStatus::ERR_INVALID_REQUEST:  return "INVALID_REQUEST";
        case CloudStatus::ERR_MISSING_FIELD:    return "MISSING_FIELD";
        case CloudStatus::ERR_PARSE_ERROR:      return "PARSE_ERROR";
        case CloudStatus::ERR_CACHE_FULL:       return "CACHE_FULL";
        default:                                return "UNKNOWN";
    }
}

// ============================================================================
// REQUEST ID GENERATION
// ============================================================================

void generateRequestId(char* buffer) {
    // Generate UUID v4 format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    static const char hex[] = "0123456789abcdef";

    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            buffer[i] = '-';
        } else if (i == 14) {
            buffer[i] = '4';  // UUID version 4
        } else if (i == 19) {
            buffer[i] = hex[(rand() & 0x3) | 0x8];  // Variant bits
        } else {
            buffer[i] = hex[rand() & 0xF];
        }
    }
    buffer[36] = '\0';
}

// ============================================================================
// JSON SERIALIZATION
// ============================================================================

size_t serializeValidateRequest(const ValidateCartridgeRequest* request,
                                char* buffer, size_t bufferSize) {
    return snprintf(buffer, bufferSize,
        "{"
        "\"requestId\":\"%s\","
        "\"deviceId\":\"%s\","
        "\"timestamp\":%lu,"
        "\"firmwareVersion\":%d,"
        "\"dataFormatVersion\":%d,"
        "\"cartridgeUuid\":\"%s\""
        "}",
        request->requestId,
        request->deviceId,
        (unsigned long)request->timestamp,
        request->firmwareVersion,
        request->dataFormatVersion,
        request->cartridgeUuid);
}

size_t serializeLoadAssayRequest(const LoadAssayRequest* request,
                                 char* buffer, size_t bufferSize) {
    return snprintf(buffer, bufferSize,
        "{"
        "\"requestId\":\"%s\","
        "\"deviceId\":\"%s\","
        "\"timestamp\":%lu,"
        "\"firmwareVersion\":%d,"
        "\"dataFormatVersion\":%d,"
        "\"assayId\":\"%s\""
        "}",
        request->requestId,
        request->deviceId,
        (unsigned long)request->timestamp,
        request->firmwareVersion,
        request->dataFormatVersion,
        request->assayId);
}

size_t serializeUploadRequest(const UploadTestRequest* request,
                              char* buffer, size_t bufferSize) {
    return snprintf(buffer, bufferSize,
        "{"
        "\"requestId\":\"%s\","
        "\"deviceId\":\"%s\","
        "\"timestamp\":%lu,"
        "\"firmwareVersion\":%d,"
        "\"dataFormatVersion\":%d,"
        "\"cartridgeUuid\":\"%s\","
        "\"assayId\":\"%s\","
        "\"startTime\":%lu,"
        "\"duration\":%d,"
        "\"numberOfReadings\":%d,"
        "\"baselineScans\":%d,"
        "\"testScans\":%d,"
        "\"checksum\":%lu,"
        "\"recordSize\":%d"
        "}",
        request->requestId,
        request->deviceId,
        (unsigned long)request->timestamp,
        request->firmwareVersion,
        request->dataFormatVersion,
        request->cartridgeUuid,
        request->assayId,
        (unsigned long)request->startTime,
        request->duration,
        request->numberOfReadings,
        request->baselineScans,
        request->testScans,
        (unsigned long)request->checksum,
        request->recordSize);
}

size_t serializeResetRequest(const ResetCartridgeRequest* request,
                             char* buffer, size_t bufferSize) {
    return snprintf(buffer, bufferSize,
        "{"
        "\"requestId\":\"%s\","
        "\"deviceId\":\"%s\","
        "\"timestamp\":%lu,"
        "\"firmwareVersion\":%d,"
        "\"dataFormatVersion\":%d,"
        "\"cartridgeUuid\":\"%s\""
        "}",
        request->requestId,
        request->deviceId,
        (unsigned long)request->timestamp,
        request->firmwareVersion,
        request->dataFormatVersion,
        request->cartridgeUuid);
}

// ============================================================================
// SIMPLE JSON PARSER HELPERS
// ============================================================================

static bool findJsonString(const char* json, const char* key, char* value, size_t maxLen) {
    char searchKey[64];
    snprintf(searchKey, sizeof(searchKey), "\"%s\":", key);

    const char* pos = strstr(json, searchKey);
    if (!pos) return false;

    pos += strlen(searchKey);
    while (*pos == ' ' || *pos == '\t') pos++;

    if (*pos != '"') return false;
    pos++;

    size_t i = 0;
    while (*pos && *pos != '"' && i < maxLen - 1) {
        if (*pos == '\\' && *(pos+1)) {
            pos++;  // Skip escape
        }
        value[i++] = *pos++;
    }
    value[i] = '\0';
    return true;
}

static bool findJsonBool(const char* json, const char* key, bool* value) {
    char searchKey[64];
    snprintf(searchKey, sizeof(searchKey), "\"%s\":", key);

    const char* pos = strstr(json, searchKey);
    if (!pos) return false;

    pos += strlen(searchKey);
    while (*pos == ' ' || *pos == '\t') pos++;

    if (strncmp(pos, "true", 4) == 0) {
        *value = true;
        return true;
    } else if (strncmp(pos, "false", 5) == 0) {
        *value = false;
        return true;
    }
    return false;
}

static bool findJsonInt(const char* json, const char* key, int32_t* value) {
    char searchKey[64];
    snprintf(searchKey, sizeof(searchKey), "\"%s\":", key);

    const char* pos = strstr(json, searchKey);
    if (!pos) return false;

    pos += strlen(searchKey);
    while (*pos == ' ' || *pos == '\t') pos++;

    char numBuf[32];
    size_t i = 0;
    if (*pos == '-') numBuf[i++] = *pos++;
    while (*pos >= '0' && *pos <= '9' && i < sizeof(numBuf) - 1) {
        numBuf[i++] = *pos++;
    }
    numBuf[i] = '\0';

    if (i > 0) {
        *value = atol(numBuf);
        return true;
    }
    return false;
}

static bool findJsonUint(const char* json, const char* key, uint32_t* value) {
    int32_t temp;
    if (findJsonInt(json, key, &temp)) {
        *value = (uint32_t)temp;
        return true;
    }
    return false;
}

// ============================================================================
// STATUS CODE PARSING
// ============================================================================

static CloudStatus parseStatusCode(int32_t code) {
    if (code == 0) return CloudStatus::SUCCESS;
    if (code >= 1 && code <= 99) return (CloudStatus)code;
    if (code >= 100 && code <= 199) return (CloudStatus)code;
    if (code >= 200 && code <= 299) return (CloudStatus)code;
    if (code >= 300 && code <= 399) return (CloudStatus)code;
    if (code >= 400 && code <= 499) return (CloudStatus)code;
    if (code >= 500 && code <= 599) return (CloudStatus)code;
    return CloudStatus::ERR_SERVER_ERROR;
}

// ============================================================================
// JSON DESERIALIZATION
// ============================================================================

bool deserializeValidateResponse(const char* json, ValidateCartridgeResponse* response) {
    if (!json || !response) return false;

    findJsonString(json, "requestId", response->requestId, sizeof(response->requestId));

    int32_t statusCode = 0;
    if (findJsonInt(json, "status", &statusCode)) {
        response->status = parseStatusCode(statusCode);
    }

    findJsonString(json, "errorMessage", response->errorMessage, sizeof(response->errorMessage));
    findJsonUint(json, "serverTimestamp", &response->serverTimestamp);
    findJsonBool(json, "isValid", &response->isValid);
    findJsonString(json, "cartridgeUuid", response->cartridgeUuid, sizeof(response->cartridgeUuid));
    findJsonString(json, "assayId", response->assayId, sizeof(response->assayId));

    return true;
}

bool deserializeLoadAssayResponse(const char* json, LoadAssayResponse* response) {
    if (!json || !response) return false;

    findJsonString(json, "requestId", response->requestId, sizeof(response->requestId));

    int32_t statusCode = 0;
    if (findJsonInt(json, "status", &statusCode)) {
        response->status = parseStatusCode(statusCode);
    }

    findJsonString(json, "errorMessage", response->errorMessage, sizeof(response->errorMessage));
    findJsonUint(json, "serverTimestamp", &response->serverTimestamp);
    findJsonString(json, "assayId", response->assayId, sizeof(response->assayId));

    int32_t duration = 0;
    if (findJsonInt(json, "duration", &duration)) {
        response->duration = duration;
    }

    int32_t bcodeLen = 0;
    if (findJsonInt(json, "bcodeLength", &bcodeLen)) {
        response->bcodeLength = (uint16_t)bcodeLen;
    }

    findJsonUint(json, "bcodeChecksum", &response->bcodeChecksum);
    findJsonString(json, "bcode", response->bcode, sizeof(response->bcode));

    return true;
}

bool deserializeUploadResponse(const char* json, UploadTestResponse* response) {
    if (!json || !response) return false;

    findJsonString(json, "requestId", response->requestId, sizeof(response->requestId));

    int32_t statusCode = 0;
    if (findJsonInt(json, "status", &statusCode)) {
        response->status = parseStatusCode(statusCode);
    }

    findJsonString(json, "errorMessage", response->errorMessage, sizeof(response->errorMessage));
    findJsonUint(json, "serverTimestamp", &response->serverTimestamp);
    findJsonString(json, "cartridgeUuid", response->cartridgeUuid, sizeof(response->cartridgeUuid));
    findJsonString(json, "testResultId", response->testResultId, sizeof(response->testResultId));
    findJsonBool(json, "acknowledged", &response->acknowledged);

    return true;
}

bool deserializeResetResponse(const char* json, ResetCartridgeResponse* response) {
    if (!json || !response) return false;

    findJsonString(json, "requestId", response->requestId, sizeof(response->requestId));

    int32_t statusCode = 0;
    if (findJsonInt(json, "status", &statusCode)) {
        response->status = parseStatusCode(statusCode);
    }

    findJsonString(json, "errorMessage", response->errorMessage, sizeof(response->errorMessage));
    findJsonUint(json, "serverTimestamp", &response->serverTimestamp);
    findJsonString(json, "cartridgeUuid", response->cartridgeUuid, sizeof(response->cartridgeUuid));
    findJsonBool(json, "wasReset", &response->wasReset);

    return true;
}
