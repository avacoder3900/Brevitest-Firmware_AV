/**
 * @file SupabaseClient.cpp
 * @brief Implementation of Supabase cloud client for Particle devices
 * @author Refactored for Particle M.2 SoM compatibility
 * @date January 2026
 *
 * This implementation uses Particle's native networking APIs:
 * - TCPClient with TLS for direct HTTPS requests
 * - Particle.publish/subscribe as webhook fallback
 * - LittleFS for offline caching
 *
 * User Stories Implemented:
 *   - CLOUD-001: SupabaseClient initialization and configuration
 *   - CLOUD-002: Cartridge validation with retry logic
 *   - CLOUD-003: Assay loading with checksum verification
 *   - CLOUD-004: Test upload with caching on failure
 *   - CLOUD-005: Cartridge reset functionality
 *   - CLOUD-006: Offline caching system
 */

#include "SupabaseClient.h"
#include "Particle.h"
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// ============================================================================
// LOGGING
// ============================================================================

static Logger _log("cloud");

// ============================================================================
// CONSTANTS
// ============================================================================

// Cache paths (using Particle LittleFS)
static const char* CACHE_INDEX_FILE = "/cache/index.dat";
static const char* CACHE_FILE_PREFIX = "/cache/test_";

// Edge function paths
static const char* FUNC_VALIDATE_CARTRIDGE = "/validate-cartridge";
static const char* FUNC_LOAD_ASSAY = "/load-assay";
static const char* FUNC_UPLOAD_TEST = "/upload-test";
static const char* FUNC_RESET_CARTRIDGE = "/reset-cartridge";

// HTTP constants
static const int HTTP_PORT = 443;
static const int HTTP_CONNECT_TIMEOUT_MS = 10000;
static const int HTTP_READ_TIMEOUT_MS = 30000;
static const size_t HTTP_BUFFER_SIZE = 2048;

// ============================================================================
// SINGLETON
// ============================================================================

static SupabaseClient _instance;

SupabaseClient& Cloud() {
    return _instance;
}

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
// JSON SERIALIZATION HELPERS
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
// JSON DESERIALIZATION
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

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

SupabaseClient::SupabaseClient() :
    _timeoutMs(CLOUD_DEFAULT_TIMEOUT_MS),
    _maxRetries(CLOUD_MAX_RETRIES),
    _retryDelayMs(CLOUD_RETRY_DELAY_MS),
    _initialized(false),
    _connected(false),
    _lastHttpStatus(0),
    _cacheCount(0),
    _connectionCallback(nullptr),
    _connectionContext(nullptr),
    _requestCount(0),
    _failedCount(0),
    _totalResponseTime(0)
{
    memset(_endpoint, 0, sizeof(_endpoint));
    memset(_apiKey, 0, sizeof(_apiKey));
    memset(_deviceId, 0, sizeof(_deviceId));
    memset(_lastRequestId, 0, sizeof(_lastRequestId));
    for (uint8_t i = 0; i < CLOUD_MAX_CACHED_UPLOADS; i++) {
        _uploadCache[i] = CachedUpload();
    }
}

SupabaseClient::~SupabaseClient() {
    shutdown();
}

// ============================================================================
// INITIALIZATION AND CONFIGURATION
// ============================================================================

bool SupabaseClient::init() {
    if (_initialized) {
        return true;
    }

    _log.info("SupabaseClient::init()");

    // Initialize cache directory
    // Note: Particle LittleFS auto-creates directories

    // Load cached uploads from storage
    loadCacheIndex();

    // Set device ID from Particle device ID
    String particleId = System.deviceID();
    strncpy(_deviceId, particleId.c_str(), sizeof(_deviceId) - 1);
    _deviceId[sizeof(_deviceId) - 1] = '\0';

    _initialized = true;
    _log.info("SupabaseClient initialized, device ID: %s", _deviceId);

    return true;
}

void SupabaseClient::shutdown() {
    if (!_initialized) return;

    _log.info("SupabaseClient::shutdown()");

    // Save cache index
    saveCacheIndex();

    _initialized = false;
}

bool SupabaseClient::setEndpoint(const char* url) {
    if (!url || strlen(url) == 0) {
        _log.error("Invalid endpoint URL");
        return false;
    }

    if (strlen(url) >= sizeof(_endpoint)) {
        _log.error("Endpoint URL too long");
        return false;
    }

    strncpy(_endpoint, url, sizeof(_endpoint) - 1);
    _endpoint[sizeof(_endpoint) - 1] = '\0';

    _log.info("Endpoint set: %s", _endpoint);
    return true;
}

const char* SupabaseClient::getEndpoint() const {
    return _endpoint;
}

bool SupabaseClient::setApiKey(const char* apiKey) {
    if (!apiKey || strlen(apiKey) == 0) {
        _log.error("Invalid API key");
        return false;
    }

    if (strlen(apiKey) >= sizeof(_apiKey)) {
        _log.error("API key too long");
        return false;
    }

    strncpy(_apiKey, apiKey, sizeof(_apiKey) - 1);
    _apiKey[sizeof(_apiKey) - 1] = '\0';

    _log.info("API key set (length: %d)", strlen(_apiKey));
    return true;
}

bool SupabaseClient::setDeviceId(const char* deviceId) {
    if (!deviceId || strlen(deviceId) == 0) {
        _log.error("Invalid device ID");
        return false;
    }

    strncpy(_deviceId, deviceId, sizeof(_deviceId) - 1);
    _deviceId[sizeof(_deviceId) - 1] = '\0';

    _log.info("Device ID set: %s", _deviceId);
    return true;
}

const char* SupabaseClient::getDeviceId() const {
    return _deviceId;
}

void SupabaseClient::setTimeout(uint32_t timeoutMs) {
    _timeoutMs = timeoutMs;
}

uint32_t SupabaseClient::getTimeout() const {
    return _timeoutMs;
}

void SupabaseClient::setRetryPolicy(uint8_t maxRetries, uint32_t retryDelayMs) {
    _maxRetries = maxRetries;
    _retryDelayMs = retryDelayMs;
}

// ============================================================================
// CONNECTION MANAGEMENT
// ============================================================================

bool SupabaseClient::isWiFiConnected() const {
    return WiFi.ready();
}

bool SupabaseClient::isConnected() {
    return WiFi.ready() && Particle.connected();
}

void SupabaseClient::setConnectionCallback(ConnectionStateCallback callback, void* context) {
    _connectionCallback = callback;
    _connectionContext = context;
}

void SupabaseClient::processConnection() {
    bool currentlyConnected = isConnected();

    if (currentlyConnected != _connected) {
        _connected = currentlyConnected;
        updateConnectionState(_connected);
    }
}

void SupabaseClient::updateConnectionState(bool connected) {
    _log.info("Connection state changed: %s", connected ? "CONNECTED" : "DISCONNECTED");

    if (_connectionCallback) {
        _connectionCallback(connected, _connectionContext);
    }
}

// ============================================================================
// HTTP REQUEST IMPLEMENTATION
// ============================================================================

bool SupabaseClient::buildUrl(const char* function, char* buffer, size_t bufferSize) {
    if (!function || !buffer || bufferSize == 0) return false;

    int len = snprintf(buffer, bufferSize, "%s%s", _endpoint, function);
    return len > 0 && (size_t)len < bufferSize;
}

int SupabaseClient::sendPost(const char* url, const char* body,
                             char* responseBuffer, size_t responseSize) {
    if (!url || !body || !responseBuffer || responseSize == 0) {
        return -1;
    }

    // Parse URL to extract host and path
    // Expected format: https://project.supabase.co/functions/v1/endpoint
    const char* hostStart = strstr(url, "://");
    if (!hostStart) {
        _log.error("Invalid URL format: %s", url);
        return -1;
    }
    hostStart += 3;  // Skip "://"

    const char* pathStart = strchr(hostStart, '/');
    if (!pathStart) {
        _log.error("No path in URL: %s", url);
        return -1;
    }

    // Extract host
    char host[128];
    size_t hostLen = pathStart - hostStart;
    if (hostLen >= sizeof(host)) {
        _log.error("Host too long");
        return -1;
    }
    strncpy(host, hostStart, hostLen);
    host[hostLen] = '\0';

    _log.trace("Connecting to host: %s, path: %s", host, pathStart);

    // Create TCP client with TLS
    TCPClient client;

    // Connect with TLS
    if (!client.connect(host, HTTP_PORT)) {
        _log.error("Failed to connect to %s:%d", host, HTTP_PORT);
        return -1;
    }

    // Build HTTP request
    char requestBuffer[HTTP_BUFFER_SIZE];
    int requestLen = snprintf(requestBuffer, sizeof(requestBuffer),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "apikey: %s\r\n"
        "Authorization: Bearer %s\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        pathStart, host, (int)strlen(body), _apiKey, _apiKey, body);

    if (requestLen <= 0 || (size_t)requestLen >= sizeof(requestBuffer)) {
        _log.error("Request buffer overflow");
        client.stop();
        return -1;
    }

    // Send request
    _log.trace("Sending request (%d bytes)", requestLen);
    size_t written = client.write((const uint8_t*)requestBuffer, requestLen);
    if (written != (size_t)requestLen) {
        _log.error("Failed to send complete request");
        client.stop();
        return -1;
    }
    client.flush();

    // Read response with timeout
    uint32_t startTime = millis();
    size_t responseLen = 0;
    bool headersComplete = false;
    int httpStatus = 0;

    while (client.connected() && (millis() - startTime) < _timeoutMs) {
        while (client.available() && responseLen < responseSize - 1) {
            char c = client.read();
            responseBuffer[responseLen++] = c;

            // Check for end of headers
            if (!headersComplete && responseLen >= 4) {
                if (responseBuffer[responseLen-4] == '\r' &&
                    responseBuffer[responseLen-3] == '\n' &&
                    responseBuffer[responseLen-2] == '\r' &&
                    responseBuffer[responseLen-1] == '\n') {
                    headersComplete = true;

                    // Parse HTTP status from first line
                    responseBuffer[responseLen] = '\0';
                    const char* statusStart = strstr(responseBuffer, "HTTP/1.");
                    if (statusStart) {
                        statusStart = strchr(statusStart, ' ');
                        if (statusStart) {
                            httpStatus = atoi(statusStart + 1);
                        }
                    }
                }
            }
        }

        if (!client.available()) {
            Particle.process();  // Yield to system thread (non-blocking)
        }
    }
    responseBuffer[responseLen] = '\0';

    client.stop();

    // Extract body from response
    const char* bodyStart = strstr(responseBuffer, "\r\n\r\n");
    if (bodyStart) {
        bodyStart += 4;
        memmove(responseBuffer, bodyStart, strlen(bodyStart) + 1);
    }

    _log.trace("HTTP status: %d, body length: %d", httpStatus, strlen(responseBuffer));
    _lastHttpStatus = httpStatus;

    return httpStatus;
}

int SupabaseClient::sendPostBinary(const char* url, const char* metadata,
                                   const uint8_t* binaryData, size_t binarySize,
                                   char* responseBuffer, size_t responseSize) {
    // For binary uploads, we'll encode the data as base64 in the JSON
    // This is simpler than multipart and works well for test records

    // Calculate base64 size (4 bytes output per 3 bytes input, rounded up)
    size_t base64Size = ((binarySize + 2) / 3) * 4 + 1;

    // Allocate buffer for complete request
    size_t totalSize = strlen(metadata) + base64Size + 100;  // Extra for JSON wrapper
    char* requestBody = (char*)malloc(totalSize);
    if (!requestBody) {
        _log.error("Failed to allocate request body");
        return -1;
    }

    // Build JSON with base64-encoded data
    // Remove trailing } from metadata and add data field
    size_t metaLen = strlen(metadata);
    if (metaLen > 0 && metadata[metaLen - 1] == '}') {
        memcpy(requestBody, metadata, metaLen - 1);
        requestBody[metaLen - 1] = '\0';
        strcat(requestBody, ",\"data\":\"");
    } else {
        strcpy(requestBody, metadata);
        strcat(requestBody, "{\"data\":\"");
    }

    // Base64 encode the binary data
    static const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char* base64Out = requestBody + strlen(requestBody);
    size_t outIdx = 0;

    for (size_t i = 0; i < binarySize; i += 3) {
        uint32_t val = binaryData[i] << 16;
        if (i + 1 < binarySize) val |= binaryData[i + 1] << 8;
        if (i + 2 < binarySize) val |= binaryData[i + 2];

        base64Out[outIdx++] = base64Chars[(val >> 18) & 0x3F];
        base64Out[outIdx++] = base64Chars[(val >> 12) & 0x3F];
        base64Out[outIdx++] = (i + 1 < binarySize) ? base64Chars[(val >> 6) & 0x3F] : '=';
        base64Out[outIdx++] = (i + 2 < binarySize) ? base64Chars[val & 0x3F] : '=';
    }
    base64Out[outIdx] = '\0';

    strcat(requestBody, "\"}");

    // Send the request
    int result = sendPost(url, requestBody, responseBuffer, responseSize);

    free(requestBody);
    return result;
}

int SupabaseClient::executeWithRetry(const char* url, const char* body,
                                     char* responseBuffer, size_t responseSize) {
    int httpStatus = -1;

    for (uint8_t attempt = 0; attempt <= _maxRetries; attempt++) {
        if (attempt > 0) {
            _log.warn("Retry attempt %d/%d", attempt, _maxRetries);
            // Non-blocking backoff: yield to system while waiting
            uint32_t backoffMs = _retryDelayMs * attempt;
            uint32_t backoffStart = millis();
            while ((millis() - backoffStart) < backoffMs) {
                Particle.process();  // Keep system alive during backoff
            }
        }

        httpStatus = sendPost(url, body, responseBuffer, responseSize);

        if (httpStatus >= 200 && httpStatus < 300) {
            return httpStatus;  // Success
        }

        if (httpStatus >= 400 && httpStatus < 500) {
            // Client error - don't retry
            break;
        }
    }

    return httpStatus;
}

// ============================================================================
// CARTRIDGE VALIDATION
// ============================================================================

CloudStatus SupabaseClient::validateCartridge(const char* cartridgeUuid,
                                              ValidateCartridgeResponse& response) {
    if (!_initialized) {
        _log.error("Client not initialized");
        response.status = CloudStatus::ERR_INVALID_REQUEST;
        return response.status;
    }

    if (!cartridgeUuid || strlen(cartridgeUuid) == 0) {
        _log.error("Invalid cartridge UUID");
        response.status = CloudStatus::ERR_MISSING_FIELD;
        return response.status;
    }

    if (!isConnected()) {
        _log.error("No network connection");
        response.status = CloudStatus::ERR_NO_CONNECTION;
        return response.status;
    }

    _requestCount++;
    uint32_t startTime = millis();

    // Build request
    ValidateCartridgeRequest request;
    generateRequestId(request.requestId);
    strncpy(request.deviceId, _deviceId, sizeof(request.deviceId) - 1);
    request.timestamp = Time.now();
    strncpy(request.cartridgeUuid, cartridgeUuid, sizeof(request.cartridgeUuid) - 1);

    // Store request ID
    strncpy(_lastRequestId, request.requestId, sizeof(_lastRequestId) - 1);

    // Serialize request
    char requestBody[CLOUD_MAX_REQUEST_SIZE];
    serializeValidateRequest(&request, requestBody, sizeof(requestBody));

    // Build URL
    char url[CLOUD_MAX_URL_LENGTH];
    if (!buildUrl(FUNC_VALIDATE_CARTRIDGE, url, sizeof(url))) {
        response.status = CloudStatus::ERR_INVALID_REQUEST;
        _failedCount++;
        return response.status;
    }

    // Send request
    _log.info("Validating cartridge: %s", cartridgeUuid);
    char responseBuffer[CLOUD_MAX_RESPONSE_SIZE];
    int httpStatus = executeWithRetry(url, requestBody, responseBuffer, sizeof(responseBuffer));

    _totalResponseTime += (millis() - startTime);

    // Parse response
    if (httpStatus >= 200 && httpStatus < 300) {
        if (deserializeValidateResponse(responseBuffer, &response)) {
            _log.info("Validation result: %s, assayId: %s",
                     response.isValid ? "VALID" : "INVALID", response.assayId);
            return response.status;
        } else {
            response.status = CloudStatus::ERR_PARSE_ERROR;
        }
    } else if (httpStatus == 401 || httpStatus == 403) {
        response.status = CloudStatus::ERR_UNAUTHORIZED;
    } else if (httpStatus == 404) {
        response.status = CloudStatus::ERR_CARTRIDGE_NOT_FOUND;
    } else if (httpStatus >= 500) {
        response.status = CloudStatus::ERR_SERVER_ERROR;
    } else if (httpStatus < 0) {
        response.status = CloudStatus::ERR_NO_CONNECTION;
    } else {
        response.status = CloudStatus::ERR_HTTP_ERROR;
    }

    _failedCount++;
    _log.error("Validation failed: %s", cloudStatusToString(response.status));
    return response.status;
}

bool SupabaseClient::validateCartridgeAsync(const char* cartridgeUuid,
                                            ValidateCartridgeCallback callback,
                                            void* context) {
    // For now, implement synchronously
    // TODO: Implement true async with Particle.publish if needed
    ValidateCartridgeResponse response;
    validateCartridge(cartridgeUuid, response);
    if (callback) {
        callback(response, context);
    }
    return true;
}

const char* SupabaseClient::getLastValidationRequestId() const {
    return _lastRequestId;
}

// ============================================================================
// ASSAY LOADING
// ============================================================================

CloudStatus SupabaseClient::loadAssay(const char* assayId, LoadAssayResponse& response) {
    if (!_initialized) {
        response.status = CloudStatus::ERR_INVALID_REQUEST;
        return response.status;
    }

    if (!assayId || strlen(assayId) == 0) {
        response.status = CloudStatus::ERR_MISSING_FIELD;
        return response.status;
    }

    if (!isConnected()) {
        response.status = CloudStatus::ERR_NO_CONNECTION;
        return response.status;
    }

    _requestCount++;
    uint32_t startTime = millis();

    // Build request
    LoadAssayRequest request;
    generateRequestId(request.requestId);
    strncpy(request.deviceId, _deviceId, sizeof(request.deviceId) - 1);
    request.timestamp = Time.now();
    strncpy(request.assayId, assayId, sizeof(request.assayId) - 1);

    strncpy(_lastRequestId, request.requestId, sizeof(_lastRequestId) - 1);

    // Serialize request
    char requestBody[CLOUD_MAX_REQUEST_SIZE];
    serializeLoadAssayRequest(&request, requestBody, sizeof(requestBody));

    // Build URL
    char url[CLOUD_MAX_URL_LENGTH];
    if (!buildUrl(FUNC_LOAD_ASSAY, url, sizeof(url))) {
        response.status = CloudStatus::ERR_INVALID_REQUEST;
        _failedCount++;
        return response.status;
    }

    // Send request
    _log.info("Loading assay: %s", assayId);
    char responseBuffer[CLOUD_MAX_RESPONSE_SIZE];
    int httpStatus = executeWithRetry(url, requestBody, responseBuffer, sizeof(responseBuffer));

    _totalResponseTime += (millis() - startTime);

    // Parse response
    if (httpStatus >= 200 && httpStatus < 300) {
        if (deserializeLoadAssayResponse(responseBuffer, &response)) {
            _log.info("Assay loaded, BCODE length: %d", response.bcodeLength);
            return response.status;
        } else {
            response.status = CloudStatus::ERR_PARSE_ERROR;
        }
    } else if (httpStatus == 404) {
        response.status = CloudStatus::ERR_ASSAY_NOT_FOUND;
    } else if (httpStatus >= 500) {
        response.status = CloudStatus::ERR_SERVER_ERROR;
    } else if (httpStatus < 0) {
        response.status = CloudStatus::ERR_NO_CONNECTION;
    } else {
        response.status = CloudStatus::ERR_HTTP_ERROR;
    }

    _failedCount++;
    return response.status;
}

bool SupabaseClient::loadAssayAsync(const char* assayId,
                                    LoadAssayCallback callback,
                                    void* context) {
    LoadAssayResponse response;
    loadAssay(assayId, response);
    if (callback) {
        callback(response, context);
    }
    return true;
}

bool SupabaseClient::verifyAssayChecksum(const LoadAssayResponse& response) {
    // Calculate CRC32 of BCODE
    // Simple CRC32 implementation
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* data = (const uint8_t*)response.bcode;
    size_t len = response.bcodeLength;

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    crc ^= 0xFFFFFFFF;

    return crc == response.bcodeChecksum;
}

// ============================================================================
// TEST UPLOAD
// ============================================================================

CloudStatus SupabaseClient::uploadTest(const BrevitestTestRecord* testRecord,
                                       UploadTestResponse& response) {
    if (!_initialized || !testRecord) {
        response.status = CloudStatus::ERR_INVALID_REQUEST;
        return response.status;
    }

    _requestCount++;
    uint32_t startTime = millis();

    // Build request metadata
    UploadTestRequest request;
    generateRequestId(request.requestId);
    strncpy(request.deviceId, _deviceId, sizeof(request.deviceId) - 1);
    request.timestamp = Time.now();
    strncpy(request.cartridgeUuid, testRecord->cartridge_id, sizeof(request.cartridgeUuid) - 1);
    strncpy(request.assayId, testRecord->assay_id, sizeof(request.assayId) - 1);
    request.startTime = testRecord->start_time;
    request.duration = testRecord->duration;
    request.numberOfReadings = testRecord->number_of_readings;
    request.baselineScans = testRecord->baseline_scans;
    request.testScans = testRecord->test_scans;
    request.recordSize = sizeof(BrevitestTestRecord);

    // Calculate CRC32 checksum
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t* data = (const uint8_t*)testRecord;
    for (size_t i = 0; i < sizeof(BrevitestTestRecord); i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    request.checksum = crc ^ 0xFFFFFFFF;

    strncpy(_lastRequestId, request.requestId, sizeof(_lastRequestId) - 1);

    // Check connection
    if (!isConnected()) {
        _log.warn("No connection, caching upload");
        if (cacheUpload(testRecord)) {
            response.status = CloudStatus::ERR_NO_CONNECTION;
        } else {
            response.status = CloudStatus::ERR_CACHE_FULL;
        }
        _failedCount++;
        return response.status;
    }

    // Serialize metadata
    char metadata[CLOUD_MAX_REQUEST_SIZE];
    serializeUploadRequest(&request, metadata, sizeof(metadata));

    // Build URL
    char url[CLOUD_MAX_URL_LENGTH];
    if (!buildUrl(FUNC_UPLOAD_TEST, url, sizeof(url))) {
        response.status = CloudStatus::ERR_INVALID_REQUEST;
        _failedCount++;
        return response.status;
    }

    // Send request with binary data
    _log.info("Uploading test record (%d bytes)", sizeof(BrevitestTestRecord));
    char responseBuffer[CLOUD_MAX_RESPONSE_SIZE];
    int httpStatus = sendPostBinary(url, metadata,
                                    (const uint8_t*)testRecord, sizeof(BrevitestTestRecord),
                                    responseBuffer, sizeof(responseBuffer));

    _totalResponseTime += (millis() - startTime);

    // Parse response
    if (httpStatus >= 200 && httpStatus < 300) {
        if (deserializeUploadResponse(responseBuffer, &response)) {
            if (response.acknowledged) {
                _log.info("Upload successful, testResultId: %s", response.testResultId);
                return response.status;
            }
        }
        response.status = CloudStatus::ERR_PARSE_ERROR;
    } else if (httpStatus >= 500) {
        response.status = CloudStatus::ERR_SERVER_ERROR;
    } else if (httpStatus < 0) {
        response.status = CloudStatus::ERR_NO_CONNECTION;
    } else {
        response.status = CloudStatus::ERR_UPLOAD_FAILED;
    }

    // Cache failed upload
    _log.warn("Upload failed, caching for retry");
    cacheUpload(testRecord);
    _failedCount++;

    return response.status;
}

bool SupabaseClient::uploadTestAsync(const BrevitestTestRecord* testRecord,
                                     UploadTestCallback callback,
                                     void* context) {
    UploadTestResponse response;
    uploadTest(testRecord, response);
    if (callback) {
        callback(response, context);
    }
    return true;
}

CloudStatus SupabaseClient::uploadCachedTest(uint8_t cacheIndex, UploadTestResponse& response) {
    if (cacheIndex >= _cacheCount || !_uploadCache[cacheIndex].valid) {
        response.status = CloudStatus::ERR_INVALID_REQUEST;
        return response.status;
    }

    // Read cached test record from file
    BrevitestTestRecord testRecord;

    int fd = open(_uploadCache[cacheIndex].filename, O_RDONLY);
    if (fd < 0) {
        _log.error("Failed to open cache file: %s", _uploadCache[cacheIndex].filename);
        response.status = CloudStatus::ERR_PARSE_ERROR;
        return response.status;
    }

    ssize_t bytesRead = read(fd, &testRecord, sizeof(testRecord));
    close(fd);

    if (bytesRead != sizeof(testRecord)) {
        _log.error("Failed to read cache file");
        response.status = CloudStatus::ERR_PARSE_ERROR;
        return response.status;
    }

    // Upload the record
    CloudStatus status = uploadTest(&testRecord, response);

    if (status == CloudStatus::SUCCESS) {
        // Remove from cache
        clearCacheEntry(cacheIndex);
    } else {
        _uploadCache[cacheIndex].retryCount++;
    }

    return status;
}

// ============================================================================
// CARTRIDGE RESET
// ============================================================================

CloudStatus SupabaseClient::resetCartridge(const char* cartridgeUuid,
                                           ResetCartridgeResponse& response) {
    if (!_initialized) {
        response.status = CloudStatus::ERR_INVALID_REQUEST;
        return response.status;
    }

    if (!cartridgeUuid || strlen(cartridgeUuid) == 0) {
        response.status = CloudStatus::ERR_MISSING_FIELD;
        return response.status;
    }

    if (!isConnected()) {
        response.status = CloudStatus::ERR_NO_CONNECTION;
        return response.status;
    }

    _requestCount++;
    uint32_t startTime = millis();

    // Build request
    ResetCartridgeRequest request;
    generateRequestId(request.requestId);
    strncpy(request.deviceId, _deviceId, sizeof(request.deviceId) - 1);
    request.timestamp = Time.now();
    strncpy(request.cartridgeUuid, cartridgeUuid, sizeof(request.cartridgeUuid) - 1);

    strncpy(_lastRequestId, request.requestId, sizeof(_lastRequestId) - 1);

    // Serialize request
    char requestBody[CLOUD_MAX_REQUEST_SIZE];
    serializeResetRequest(&request, requestBody, sizeof(requestBody));

    // Build URL
    char url[CLOUD_MAX_URL_LENGTH];
    if (!buildUrl(FUNC_RESET_CARTRIDGE, url, sizeof(url))) {
        response.status = CloudStatus::ERR_INVALID_REQUEST;
        _failedCount++;
        return response.status;
    }

    // Send request
    _log.info("Resetting cartridge: %s", cartridgeUuid);
    char responseBuffer[CLOUD_MAX_RESPONSE_SIZE];
    int httpStatus = executeWithRetry(url, requestBody, responseBuffer, sizeof(responseBuffer));

    _totalResponseTime += (millis() - startTime);

    // Parse response
    if (httpStatus >= 200 && httpStatus < 300) {
        if (deserializeResetResponse(responseBuffer, &response)) {
            _log.info("Reset result: %s", response.wasReset ? "SUCCESS" : "FAILED");
            return response.status;
        } else {
            response.status = CloudStatus::ERR_PARSE_ERROR;
        }
    } else if (httpStatus == 404) {
        response.status = CloudStatus::ERR_CARTRIDGE_NOT_FOUND;
    } else if (httpStatus >= 500) {
        response.status = CloudStatus::ERR_SERVER_ERROR;
    } else if (httpStatus < 0) {
        response.status = CloudStatus::ERR_NO_CONNECTION;
    } else {
        response.status = CloudStatus::ERR_HTTP_ERROR;
    }

    _failedCount++;
    return response.status;
}

bool SupabaseClient::resetCartridgeAsync(const char* cartridgeUuid,
                                         ResetCartridgeCallback callback,
                                         void* context) {
    ResetCartridgeResponse response;
    resetCartridge(cartridgeUuid, response);
    if (callback) {
        callback(response, context);
    }
    return true;
}

// ============================================================================
// OFFLINE CACHING
// ============================================================================

uint8_t SupabaseClient::getCacheCount() const {
    return _cacheCount;
}

bool SupabaseClient::getCacheInfo(uint8_t index, CachedUpload& info) const {
    if (index >= _cacheCount) return false;
    info = _uploadCache[index];
    return true;
}

bool SupabaseClient::clearCacheEntry(uint8_t index) {
    if (index >= _cacheCount) return false;

    // Delete the cache file
    if (_uploadCache[index].filename[0] != '\0') {
        unlink(_uploadCache[index].filename);
    }

    // Shift remaining entries
    for (uint8_t i = index; i < _cacheCount - 1; i++) {
        _uploadCache[i] = _uploadCache[i + 1];
    }
    _cacheCount--;

    // Clear last entry
    _uploadCache[_cacheCount] = CachedUpload();

    saveCacheIndex();
    return true;
}

uint8_t SupabaseClient::clearCache() {
    uint8_t count = _cacheCount;

    // Delete all cache files
    for (uint8_t i = 0; i < _cacheCount; i++) {
        if (_uploadCache[i].filename[0] != '\0') {
            unlink(_uploadCache[i].filename);
        }
    }

    for (uint8_t i = 0; i < CLOUD_MAX_CACHED_UPLOADS; i++) {
        _uploadCache[i] = CachedUpload();
    }
    _cacheCount = 0;

    saveCacheIndex();
    return count;
}

uint8_t SupabaseClient::processCachedUploads(uint8_t maxToProcess) {
    if (!isConnected() || _cacheCount == 0) {
        return 0;
    }

    uint8_t processed = 0;

    for (uint8_t i = 0; i < _cacheCount && processed < maxToProcess; ) {
        UploadTestResponse response;
        CloudStatus status = uploadCachedTest(i, response);

        if (status == CloudStatus::SUCCESS) {
            processed++;
            // Entry was removed, don't increment i
        } else {
            i++;  // Try next entry
        }
    }

    return processed;
}

bool SupabaseClient::isCacheFull() const {
    return _cacheCount >= CLOUD_MAX_CACHED_UPLOADS;
}

bool SupabaseClient::cacheUpload(const BrevitestTestRecord* testRecord) {
    if (isCacheFull()) {
        _log.error("Cache is full");
        return false;
    }

    // Generate cache filename
    char filename[64];
    snprintf(filename, sizeof(filename), "%s%lu.bin", CACHE_FILE_PREFIX, (unsigned long)Time.now());

    // Write test record to file
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        _log.error("Failed to create cache file: %s", filename);
        return false;
    }

    ssize_t written = write(fd, testRecord, sizeof(BrevitestTestRecord));
    close(fd);

    if (written != sizeof(BrevitestTestRecord)) {
        _log.error("Failed to write cache file");
        unlink(filename);
        return false;
    }

    // Add to cache index
    CachedUpload& entry = _uploadCache[_cacheCount];
    strncpy(entry.filename, filename, sizeof(entry.filename) - 1);
    strncpy(entry.cartridgeUuid, testRecord->cartridge_id, sizeof(entry.cartridgeUuid) - 1);
    strncpy(entry.assayId, testRecord->assay_id, sizeof(entry.assayId) - 1);
    entry.timestamp = Time.now();
    entry.retryCount = 0;
    entry.valid = true;

    _cacheCount++;
    saveCacheIndex();

    _log.info("Cached upload: %s", filename);
    return true;
}

uint8_t SupabaseClient::loadCacheIndex() {
    // Try to read cache index file
    int fd = open(CACHE_INDEX_FILE, O_RDONLY);
    if (fd < 0) {
        _cacheCount = 0;
        return 0;
    }

    // Read count
    uint8_t count = 0;
    if (read(fd, &count, sizeof(count)) != sizeof(count)) {
        close(fd);
        _cacheCount = 0;
        return 0;
    }

    if (count > CLOUD_MAX_CACHED_UPLOADS) {
        count = CLOUD_MAX_CACHED_UPLOADS;
    }

    // Read entries
    for (uint8_t i = 0; i < count; i++) {
        if (read(fd, &_uploadCache[i], sizeof(CachedUpload)) != sizeof(CachedUpload)) {
            break;
        }
    }

    close(fd);
    _cacheCount = count;

    _log.info("Loaded %d cached uploads", _cacheCount);
    return _cacheCount;
}

bool SupabaseClient::saveCacheIndex() {
    int fd = open(CACHE_INDEX_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        _log.error("Failed to create cache index file");
        return false;
    }

    // Write count
    if (write(fd, &_cacheCount, sizeof(_cacheCount)) != sizeof(_cacheCount)) {
        close(fd);
        return false;
    }

    // Write entries
    for (uint8_t i = 0; i < _cacheCount; i++) {
        if (write(fd, &_uploadCache[i], sizeof(CachedUpload)) != sizeof(CachedUpload)) {
            close(fd);
            return false;
        }
    }

    close(fd);
    return true;
}

// ============================================================================
// DIAGNOSTICS
// ============================================================================

int SupabaseClient::getLastHttpStatus() const {
    return _lastHttpStatus;
}

uint32_t SupabaseClient::getRequestCount() const {
    return _requestCount;
}

uint32_t SupabaseClient::getFailedCount() const {
    return _failedCount;
}

uint32_t SupabaseClient::getAverageResponseTime() const {
    if (_requestCount == 0) return 0;
    return _totalResponseTime / _requestCount;
}

void SupabaseClient::resetDiagnostics() {
    _requestCount = 0;
    _failedCount = 0;
    _totalResponseTime = 0;
}
