/**
 * @file CloudProtocol.h
 * @brief Request/response structures and async callbacks for Supabase cloud communication
 * @date February 2026
 *
 * All data structures and callback types for communication between the Brevitest
 * device and Supabase Edge Functions. All cloud operations are non-blocking via
 * async callbacks - NO blocking delay() in any cloud operation path.
 *
 * Protocol Overview:
 * - All requests use HTTPS POST to Edge Function endpoints
 * - Request bodies are JSON-encoded
 * - Responses are JSON with standard status/error fields
 * - Request ID correlation for tracking and retry logic
 * - Async callback pattern: caller provides callback, gets notified on completion
 * - Retry: 3 attempts, 5s exponential backoff, 45s timeout
 *
 * User Stories Implemented:
 *   - GAMMA-001: Protocol structures with async callback typedefs
 *   - CLOUD-001: Protocol structures for SupabaseClient
 *   - CLOUD-002: Validate cartridge request/response
 *   - CLOUD-003: Load assay request/response
 *   - CLOUD-004: Upload test request/response
 *   - CLOUD-005: Reset cartridge request/response
 */

#ifndef CLOUD_PROTOCOL_H
#define CLOUD_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>
#include "DataTypes.h"

// ============================================================================
// CLOUD COMMUNICATION CONSTANTS
// ============================================================================

/** @brief Maximum length of API endpoint URL */
#define CLOUD_MAX_URL_LENGTH 256

/** @brief Maximum length of API key */
#define CLOUD_MAX_API_KEY_LENGTH 128

/** @brief Maximum length of request ID (UUID format) */
#define CLOUD_REQUEST_ID_LENGTH 36

/** @brief Maximum length of error message */
#define CLOUD_MAX_ERROR_LENGTH 256

/** @brief Default request timeout in milliseconds */
#define CLOUD_DEFAULT_TIMEOUT_MS 45000

/** @brief Number of retry attempts for failed requests */
#define CLOUD_MAX_RETRIES 3

/** @brief Base delay between retries in milliseconds */
#define CLOUD_RETRY_DELAY_MS 5000

/** @brief Maximum cached uploads for offline operation */
#define CLOUD_MAX_CACHED_UPLOADS 50

/** @brief Maximum JSON payload size for requests */
#define CLOUD_MAX_REQUEST_SIZE 1024

/** @brief Maximum JSON payload size for responses */
#define CLOUD_MAX_RESPONSE_SIZE 8192

// ============================================================================
// CLOUD OPERATION STATUS
// ============================================================================

/**
 * @brief Cloud operation result status codes
 * @details Standard status codes returned by Edge Functions
 */
enum class CloudStatus : int16_t {
    SUCCESS = 0,                    ///< Operation completed successfully

    // Network errors (1-99)
    ERR_NO_CONNECTION = 1,          ///< No network connection
    ERR_TIMEOUT = 2,                ///< Request timed out
    ERR_DNS_FAILED = 3,             ///< DNS resolution failed
    ERR_TLS_FAILED = 4,             ///< TLS/SSL handshake failed
    ERR_HTTP_ERROR = 5,             ///< HTTP protocol error

    // Authentication errors (100-199)
    ERR_UNAUTHORIZED = 100,         ///< Invalid or missing API key
    ERR_FORBIDDEN = 101,            ///< Device not authorized for operation
    ERR_DEVICE_NOT_FOUND = 102,     ///< Device ID not registered

    // Validation errors (200-299)
    ERR_CARTRIDGE_NOT_FOUND = 200,  ///< Cartridge UUID not in database
    ERR_CARTRIDGE_USED = 201,       ///< Cartridge already used
    ERR_CARTRIDGE_EXPIRED = 202,    ///< Cartridge past expiration date
    ERR_CARTRIDGE_INVALID = 203,    ///< Cartridge validation failed
    ERR_ASSAY_NOT_FOUND = 204,      ///< Associated assay not found
    ERR_ASSAY_INACTIVE = 205,       ///< Assay is not active

    // Upload errors (300-399)
    ERR_UPLOAD_FAILED = 300,        ///< Test upload failed
    ERR_CHECKSUM_MISMATCH = 301,    ///< Data checksum verification failed
    ERR_DUPLICATE_UPLOAD = 302,     ///< Test already uploaded
    ERR_INVALID_FORMAT = 303,       ///< Invalid data format

    // Server errors (400-499)
    ERR_SERVER_ERROR = 400,         ///< Internal server error
    ERR_DATABASE_ERROR = 401,       ///< Database operation failed
    ERR_SERVICE_UNAVAILABLE = 402,  ///< Service temporarily unavailable

    // Client errors (500-599)
    ERR_INVALID_REQUEST = 500,      ///< Malformed request
    ERR_MISSING_FIELD = 501,        ///< Required field missing
    ERR_PARSE_ERROR = 502,          ///< JSON parse error
    ERR_CACHE_FULL = 503            ///< Local cache is full
};

/**
 * @brief Convert CloudStatus to human-readable string
 * @param status Cloud status code
 * @return Constant string representation
 */
const char* cloudStatusToString(CloudStatus status);

// ============================================================================
// REQUEST STRUCTURES
// ============================================================================

/**
 * @brief Base structure for all cloud requests
 * @details Contains common fields for request tracking and authentication
 */
struct CloudRequestBase {
    char requestId[CLOUD_REQUEST_ID_LENGTH + 1];  ///< Unique request ID (UUID)
    char deviceId[DEVICE_UUID_LENGTH + 1];        ///< Device identifier
    uint32_t timestamp;                            ///< Request timestamp (Unix)
    uint8_t firmwareVersion;                       ///< Firmware version
    uint8_t dataFormatVersion;                     ///< Data format version

    /** @brief Default constructor */
    CloudRequestBase() :
        timestamp(0),
        firmwareVersion(FIRMWARE_VERSION),
        dataFormatVersion(DATA_FORMAT_VERSION)
    {
        requestId[0] = '\0';
        deviceId[0] = '\0';
    }
};

/**
 * @brief Cartridge validation request
 * @details Sent to validate-cartridge Edge Function
 */
struct ValidateCartridgeRequest : public CloudRequestBase {
    char cartridgeUuid[BARCODE_UUID_LENGTH + 1];  ///< Cartridge barcode UUID

    /** @brief Default constructor */
    ValidateCartridgeRequest() : CloudRequestBase() {
        cartridgeUuid[0] = '\0';
    }
};

/**
 * @brief Load assay request
 * @details Sent to load-assay Edge Function
 */
struct LoadAssayRequest : public CloudRequestBase {
    char assayId[ASSAY_UUID_LENGTH + 1];          ///< Assay identifier to load

    /** @brief Default constructor */
    LoadAssayRequest() : CloudRequestBase() {
        assayId[0] = '\0';
    }
};

/**
 * @brief Test upload request
 * @details Sent to upload-test Edge Function
 * @note The actual test record is sent as binary payload
 */
struct UploadTestRequest : public CloudRequestBase {
    char cartridgeUuid[BARCODE_UUID_LENGTH + 1];  ///< Cartridge barcode UUID
    char assayId[ASSAY_UUID_LENGTH + 1];          ///< Assay identifier
    uint32_t startTime;                            ///< Test start timestamp
    uint16_t duration;                             ///< Test duration (seconds)
    uint16_t numberOfReadings;                     ///< Number of spectro readings
    uint16_t baselineScans;                        ///< Number of baseline scans
    uint16_t testScans;                            ///< Number of test scans
    uint32_t checksum;                             ///< CRC32 checksum of record
    uint16_t recordSize;                           ///< Size of binary record

    /** @brief Default constructor */
    UploadTestRequest() : CloudRequestBase(),
        startTime(0),
        duration(0),
        numberOfReadings(0),
        baselineScans(0),
        testScans(0),
        checksum(0),
        recordSize(0)
    {
        cartridgeUuid[0] = '\0';
        assayId[0] = '\0';
    }
};

/**
 * @brief Cartridge reset request
 * @details Sent to reset-cartridge Edge Function
 */
struct ResetCartridgeRequest : public CloudRequestBase {
    char cartridgeUuid[BARCODE_UUID_LENGTH + 1];  ///< Cartridge barcode UUID

    /** @brief Default constructor */
    ResetCartridgeRequest() : CloudRequestBase() {
        cartridgeUuid[0] = '\0';
    }
};

// ============================================================================
// RESPONSE STRUCTURES
// ============================================================================

/**
 * @brief Base structure for all cloud responses
 * @details Contains common fields for response tracking and error handling
 */
struct CloudResponseBase {
    char requestId[CLOUD_REQUEST_ID_LENGTH + 1];  ///< Matching request ID
    CloudStatus status;                            ///< Operation status
    char errorMessage[CLOUD_MAX_ERROR_LENGTH];    ///< Error description (if any)
    uint32_t serverTimestamp;                      ///< Server timestamp

    /** @brief Default constructor */
    CloudResponseBase() :
        status(CloudStatus::SUCCESS),
        serverTimestamp(0)
    {
        requestId[0] = '\0';
        errorMessage[0] = '\0';
    }

    /** @brief Check if response indicates success */
    bool isSuccess() const { return status == CloudStatus::SUCCESS; }
};

/**
 * @brief Cartridge validation response
 * @details Response from validate-cartridge Edge Function
 */
struct ValidateCartridgeResponse : public CloudResponseBase {
    bool isValid;                                  ///< Cartridge validation result
    char cartridgeUuid[BARCODE_UUID_LENGTH + 1];  ///< Confirmed cartridge UUID
    char assayId[ASSAY_UUID_LENGTH + 1];          ///< Associated assay ID

    /** @brief Default constructor */
    ValidateCartridgeResponse() : CloudResponseBase(),
        isValid(false)
    {
        cartridgeUuid[0] = '\0';
        assayId[0] = '\0';
    }
};

/**
 * @brief Load assay response
 * @details Response from load-assay Edge Function
 */
struct LoadAssayResponse : public CloudResponseBase {
    char assayId[ASSAY_UUID_LENGTH + 1];          ///< Confirmed assay ID
    int32_t duration;                              ///< Expected test duration (ms)
    uint16_t bcodeLength;                          ///< Length of BCODE data
    uint32_t bcodeChecksum;                        ///< CRC32 checksum of BCODE
    char bcode[BCODE_CAPACITY];                   ///< BCODE instruction buffer

    /** @brief Default constructor */
    LoadAssayResponse() : CloudResponseBase(),
        duration(0),
        bcodeLength(0),
        bcodeChecksum(0)
    {
        assayId[0] = '\0';
        bcode[0] = '\0';
    }
};

/**
 * @brief Test upload response
 * @details Response from upload-test Edge Function
 */
struct UploadTestResponse : public CloudResponseBase {
    char cartridgeUuid[BARCODE_UUID_LENGTH + 1];  ///< Confirmed cartridge UUID
    char testResultId[CLOUD_REQUEST_ID_LENGTH + 1]; ///< Server-assigned test ID
    bool acknowledged;                             ///< Upload acknowledged

    /** @brief Default constructor */
    UploadTestResponse() : CloudResponseBase(),
        acknowledged(false)
    {
        cartridgeUuid[0] = '\0';
        testResultId[0] = '\0';
    }
};

/**
 * @brief Cartridge reset response
 * @details Response from reset-cartridge Edge Function
 */
struct ResetCartridgeResponse : public CloudResponseBase {
    char cartridgeUuid[BARCODE_UUID_LENGTH + 1];  ///< Confirmed cartridge UUID
    bool wasReset;                                 ///< Reset successful flag

    /** @brief Default constructor */
    ResetCartridgeResponse() : CloudResponseBase(),
        wasReset(false)
    {
        cartridgeUuid[0] = '\0';
    }
};

// ============================================================================
// CACHED UPLOAD STRUCTURE
// ============================================================================

/**
 * @brief Structure for cached offline uploads
 * @details Stores test records that failed to upload for later retry
 */
struct CachedUpload {
    char filename[64];                             ///< Cache file name
    char cartridgeUuid[BARCODE_UUID_LENGTH + 1];  ///< Cartridge UUID
    char assayId[ASSAY_UUID_LENGTH + 1];          ///< Assay ID
    uint32_t timestamp;                            ///< Cache timestamp
    uint8_t retryCount;                            ///< Number of retry attempts
    bool valid;                                    ///< Entry is valid

    /** @brief Default constructor */
    CachedUpload() :
        timestamp(0),
        retryCount(0),
        valid(false)
    {
        filename[0] = '\0';
        cartridgeUuid[0] = '\0';
        assayId[0] = '\0';
    }
};

// ============================================================================
// ASYNC CALLBACK TYPEDEFS
// ============================================================================

/**
 * @brief Callback for cartridge validation completion
 * @param response The validation response (check response.isSuccess())
 */
typedef void (*ValidateCartridgeCallback)(const ValidateCartridgeResponse& response);

/**
 * @brief Callback for assay load completion
 * @param response The load assay response (check response.isSuccess())
 */
typedef void (*LoadAssayCallback)(const LoadAssayResponse& response);

/**
 * @brief Callback for test upload completion
 * @param response The upload response (check response.isSuccess())
 */
typedef void (*UploadTestCallback)(const UploadTestResponse& response);

/**
 * @brief Callback for cartridge reset completion
 * @param response The reset response (check response.isSuccess())
 */
typedef void (*ResetCartridgeCallback)(const ResetCartridgeResponse& response);

/**
 * @brief Generic cloud operation callback (for WiFi status, etc.)
 * @param status Operation result status
 * @param message Optional status message (may be nullptr)
 */
typedef void (*CloudStatusCallback)(CloudStatus status, const char* message);

// ============================================================================
// JSON SERIALIZATION HELPERS
// ============================================================================

/**
 * @brief Serialize ValidateCartridgeRequest to JSON string
 * @param request Request structure
 * @param buffer Output buffer
 * @param bufferSize Size of output buffer
 * @return Number of bytes written, 0 on error
 */
size_t serializeValidateRequest(const ValidateCartridgeRequest* request,
                                char* buffer, size_t bufferSize);

/**
 * @brief Deserialize JSON to ValidateCartridgeResponse
 * @param json JSON string
 * @param response Output response structure
 * @return true on success, false on parse error
 */
bool deserializeValidateResponse(const char* json,
                                 ValidateCartridgeResponse* response);

/**
 * @brief Serialize LoadAssayRequest to JSON string
 * @param request Request structure
 * @param buffer Output buffer
 * @param bufferSize Size of output buffer
 * @return Number of bytes written, 0 on error
 */
size_t serializeLoadAssayRequest(const LoadAssayRequest* request,
                                 char* buffer, size_t bufferSize);

/**
 * @brief Deserialize JSON to LoadAssayResponse
 * @param json JSON string
 * @param response Output response structure
 * @return true on success, false on parse error
 */
bool deserializeLoadAssayResponse(const char* json,
                                  LoadAssayResponse* response);

/**
 * @brief Serialize UploadTestRequest to JSON string (metadata only)
 * @param request Request structure
 * @param buffer Output buffer
 * @param bufferSize Size of output buffer
 * @return Number of bytes written, 0 on error
 * @note Binary test record is sent separately
 */
size_t serializeUploadRequest(const UploadTestRequest* request,
                              char* buffer, size_t bufferSize);

/**
 * @brief Deserialize JSON to UploadTestResponse
 * @param json JSON string
 * @param response Output response structure
 * @return true on success, false on parse error
 */
bool deserializeUploadResponse(const char* json,
                               UploadTestResponse* response);

/**
 * @brief Serialize ResetCartridgeRequest to JSON string
 * @param request Request structure
 * @param buffer Output buffer
 * @param bufferSize Size of output buffer
 * @return Number of bytes written, 0 on error
 */
size_t serializeResetRequest(const ResetCartridgeRequest* request,
                             char* buffer, size_t bufferSize);

/**
 * @brief Deserialize JSON to ResetCartridgeResponse
 * @param json JSON string
 * @param response Output response structure
 * @return true on success, false on parse error
 */
bool deserializeResetResponse(const char* json,
                              ResetCartridgeResponse* response);

// ============================================================================
// REQUEST ID GENERATION
// ============================================================================

/**
 * @brief Generate a unique request ID (UUID v4 format)
 * @param buffer Output buffer (must be at least CLOUD_REQUEST_ID_LENGTH + 1)
 */
void generateRequestId(char* buffer);

#endif // CLOUD_PROTOCOL_H
