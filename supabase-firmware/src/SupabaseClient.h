/**
 * @file SupabaseClient.h
 * @brief HTTPS client for Supabase Edge Function communication
 * @author Agent DELTA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module provides the cloud communication layer for the Brevitest device,
 * replacing the Particle Pub/Sub system with direct HTTPS requests to
 * Supabase Edge Functions.
 *
 * Architecture:
 * - Device → HTTPS POST → Edge Function → PostgreSQL
 * - Device ← HTTPS Response ← Edge Function
 *
 * Key Features:
 * - Synchronous HTTPS requests with configurable timeout
 * - Automatic retry logic (3 attempts, 5s backoff)
 * - Request ID correlation for tracking
 * - Offline caching for failed uploads
 * - WiFi connectivity management
 *
 * User Stories Implemented:
 *   - CLOUD-001: SupabaseClient class with init/config
 *   - CLOUD-002: validateCartridge() function
 *   - CLOUD-003: loadAssay() function
 *   - CLOUD-004: uploadTest() function with retry
 *   - CLOUD-005: resetCartridge() function
 *   - CLOUD-006: Offline caching system
 */

#ifndef SUPABASE_CLIENT_H
#define SUPABASE_CLIENT_H

#include "Particle.h"
#include "CloudProtocol.h"
#include "DataTypes.h"

// ============================================================================
// CALLBACK TYPES
// ============================================================================

/**
 * @brief Callback for connection state changes
 * @param connected Current connection state
 * @param userContext User-provided context pointer
 */
typedef void (*ConnectionStateCallback)(bool connected, void* userContext);

// ============================================================================
// SUPABASE CLIENT CLASS
// ============================================================================

/**
 * @class SupabaseClient
 * @brief HTTPS client for Supabase Edge Function communication
 *
 * This class manages all cloud communication for the Brevitest device,
 * providing a clean interface for cartridge validation, assay loading,
 * test uploads, and cartridge reset operations.
 *
 * Example Usage:
 * @code
 * SupabaseClient cloud;
 *
 * // Initialize with endpoint and credentials
 * cloud.init();
 * cloud.setEndpoint("https://your-project.supabase.co/functions/v1");
 * cloud.setApiKey("your-anon-key");
 * cloud.setDeviceId("device-123");
 *
 * // Validate a cartridge
 * ValidateCartridgeResponse response;
 * CloudStatus status = cloud.validateCartridge("cartridge-uuid", response);
 * if (status == CloudStatus::SUCCESS && response.isValid) {
 *     // Load the assay
 *     LoadAssayResponse assayResponse;
 *     cloud.loadAssay(response.assayId, assayResponse);
 * }
 * @endcode
 */
class SupabaseClient {
public:
    // ========================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ========================================================================

    /**
     * @brief Default constructor
     */
    SupabaseClient();

    /**
     * @brief Destructor
     */
    ~SupabaseClient();

    // ========================================================================
    // INITIALIZATION AND CONFIGURATION (CLOUD-001)
    // ========================================================================

    /**
     * @brief Initialize the HTTP client
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Shutdown and cleanup the client
     */
    void shutdown();

    /**
     * @brief Set the Supabase project endpoint URL
     * @param url Base URL (e.g., "https://project.supabase.co/functions/v1")
     * @return true if URL is valid
     */
    bool setEndpoint(const char* url);

    /**
     * @brief Get the current endpoint URL
     * @return Pointer to endpoint URL string
     */
    const char* getEndpoint() const;

    /**
     * @brief Set the API key for authentication
     * @param apiKey Supabase anon or service role key
     * @return true if key is valid
     */
    bool setApiKey(const char* apiKey);

    /**
     * @brief Set the device identifier
     * @param deviceId Unique device identifier
     * @return true if device ID is valid
     */
    bool setDeviceId(const char* deviceId);

    /**
     * @brief Get the current device ID
     * @return Pointer to device ID string
     */
    const char* getDeviceId() const;

    /**
     * @brief Set request timeout
     * @param timeoutMs Timeout in milliseconds (default 45000)
     */
    void setTimeout(uint32_t timeoutMs);

    /**
     * @brief Get current request timeout
     * @return Timeout in milliseconds
     */
    uint32_t getTimeout() const;

    /**
     * @brief Set retry parameters
     * @param maxRetries Maximum number of retry attempts (default 3)
     * @param retryDelayMs Base delay between retries (default 5000ms)
     */
    void setRetryPolicy(uint8_t maxRetries, uint32_t retryDelayMs);

    // ========================================================================
    // CONNECTION MANAGEMENT
    // ========================================================================

    /**
     * @brief Check if WiFi is connected
     * @return true if WiFi connection is active
     */
    bool isWiFiConnected() const;

    /**
     * @brief Check if cloud services are reachable
     * @return true if endpoint is reachable
     */
    bool isConnected();

    /**
     * @brief Set connection state callback
     * @param callback Function to call on connection state change
     * @param context User context to pass to callback
     */
    void setConnectionCallback(ConnectionStateCallback callback, void* context);

    /**
     * @brief Process connection monitoring (call from loop)
     * @note Should be called periodically to update connection state
     */
    void processConnection();

    // ========================================================================
    // CARTRIDGE VALIDATION (CLOUD-002)
    // ========================================================================

    /**
     * @brief Validate a cartridge with the cloud server
     * @param cartridgeUuid Barcode UUID from cartridge
     * @param response Output response structure
     * @return CloudStatus indicating result
     *
     * This function sends a validation request to the validate-cartridge
     * Edge Function and waits for the response. It implements automatic
     * retry logic with exponential backoff.
     *
     * On success, response.isValid indicates if cartridge is valid,
     * and response.assayId contains the associated assay identifier.
     */
    CloudStatus validateCartridge(const char* cartridgeUuid,
                                  ValidateCartridgeResponse& response);

    /**
     * @brief Validate cartridge asynchronously
     * @param cartridgeUuid Barcode UUID from cartridge
     * @param callback Function to call with result
     * @param context User context to pass to callback
     * @return true if request was initiated
     */
    bool validateCartridgeAsync(const char* cartridgeUuid,
                                ValidateCartridgeCallback callback,
                                void* context);

    /**
     * @brief Get the last validation request ID
     * @return Request ID string
     */
    const char* getLastValidationRequestId() const;

    // ========================================================================
    // ASSAY LOADING (CLOUD-003)
    // ========================================================================

    /**
     * @brief Load an assay from the cloud server
     * @param assayId Assay identifier to load
     * @param response Output response structure containing BCODE
     * @return CloudStatus indicating result
     *
     * This function downloads the assay definition including BCODE
     * instructions from the load-assay Edge Function. It handles
     * potentially large responses (up to 5KB BCODE).
     *
     * The response includes checksum for verification.
     */
    CloudStatus loadAssay(const char* assayId,
                          LoadAssayResponse& response);

    /**
     * @brief Load assay asynchronously
     * @param assayId Assay identifier to load
     * @param callback Function to call with result
     * @param context User context to pass to callback
     * @return true if request was initiated
     */
    bool loadAssayAsync(const char* assayId,
                        LoadAssayCallback callback,
                        void* context);

    /**
     * @brief Verify loaded assay checksum
     * @param response Assay response to verify
     * @return true if checksum matches
     */
    bool verifyAssayChecksum(const LoadAssayResponse& response);

    // ========================================================================
    // TEST UPLOAD (CLOUD-004)
    // ========================================================================

    /**
     * @brief Upload test results to the cloud server
     * @param testRecord Complete test record to upload
     * @param response Output response structure
     * @return CloudStatus indicating result
     *
     * This function uploads the complete test record (9668 bytes)
     * to the upload-test Edge Function. It handles retry logic
     * and will cache failed uploads for later retry.
     */
    CloudStatus uploadTest(const BrevitestTestRecord* testRecord,
                           UploadTestResponse& response);

    /**
     * @brief Upload test asynchronously
     * @param testRecord Complete test record to upload
     * @param callback Function to call with result
     * @param context User context to pass to callback
     * @return true if request was initiated
     */
    bool uploadTestAsync(const BrevitestTestRecord* testRecord,
                         UploadTestCallback callback,
                         void* context);

    /**
     * @brief Upload test from cached file
     * @param cacheIndex Index of cached upload to send
     * @param response Output response structure
     * @return CloudStatus indicating result
     */
    CloudStatus uploadCachedTest(uint8_t cacheIndex,
                                 UploadTestResponse& response);

    // ========================================================================
    // CARTRIDGE RESET (CLOUD-005)
    // ========================================================================

    /**
     * @brief Reset a cartridge status on the server
     * @param cartridgeUuid Barcode UUID of cartridge to reset
     * @param response Output response structure
     * @return CloudStatus indicating result
     *
     * This function marks a cartridge as unused on the server,
     * allowing it to be validated and used again.
     */
    CloudStatus resetCartridge(const char* cartridgeUuid,
                               ResetCartridgeResponse& response);

    /**
     * @brief Reset cartridge asynchronously
     * @param cartridgeUuid Barcode UUID of cartridge to reset
     * @param callback Function to call with result
     * @param context User context to pass to callback
     * @return true if request was initiated
     */
    bool resetCartridgeAsync(const char* cartridgeUuid,
                             ResetCartridgeCallback callback,
                             void* context);

    // ========================================================================
    // OFFLINE CACHING (CLOUD-006)
    // ========================================================================

    /**
     * @brief Get number of cached uploads
     * @return Number of uploads in cache
     */
    uint8_t getCacheCount() const;

    /**
     * @brief Get information about a cached upload
     * @param index Cache index (0 to getCacheCount()-1)
     * @param info Output structure for cache info
     * @return true if index is valid
     */
    bool getCacheInfo(uint8_t index, CachedUpload& info) const;

    /**
     * @brief Clear a specific cached upload
     * @param index Cache index to clear
     * @return true if cleared successfully
     */
    bool clearCacheEntry(uint8_t index);

    /**
     * @brief Clear all cached uploads
     * @return Number of entries cleared
     */
    uint8_t clearCache();

    /**
     * @brief Process cached uploads (send when connected)
     * @param maxToProcess Maximum number to process in this call
     * @return Number of uploads successfully sent
     *
     * Call periodically from main loop to retry cached uploads.
     */
    uint8_t processCachedUploads(uint8_t maxToProcess = 1);

    /**
     * @brief Check if cache is full
     * @return true if no more uploads can be cached
     */
    bool isCacheFull() const;

    // ========================================================================
    // DIAGNOSTICS
    // ========================================================================

    /**
     * @brief Get last HTTP status code
     * @return HTTP status code from last request
     */
    int getLastHttpStatus() const;

    /**
     * @brief Get total request count
     * @return Number of requests made
     */
    uint32_t getRequestCount() const;

    /**
     * @brief Get failed request count
     * @return Number of failed requests
     */
    uint32_t getFailedCount() const;

    /**
     * @brief Get average response time
     * @return Average response time in milliseconds
     */
    uint32_t getAverageResponseTime() const;

    /**
     * @brief Reset diagnostic counters
     */
    void resetDiagnostics();

private:
    // ========================================================================
    // PRIVATE METHODS
    // ========================================================================

    /**
     * @brief Build full URL for an endpoint
     * @param function Edge function name
     * @param buffer Output buffer
     * @param bufferSize Size of output buffer
     * @return true if URL built successfully
     */
    bool buildUrl(const char* function, char* buffer, size_t bufferSize);

    /**
     * @brief Send HTTP POST request
     * @param url Full request URL
     * @param body Request body (JSON)
     * @param responseBuffer Buffer for response
     * @param responseSize Size of response buffer
     * @return HTTP status code, or negative on error
     */
    int sendPost(const char* url, const char* body,
                 char* responseBuffer, size_t responseSize);

    /**
     * @brief Send HTTP POST with binary data
     * @param url Full request URL
     * @param metadata JSON metadata
     * @param binaryData Binary payload
     * @param binarySize Size of binary payload
     * @param responseBuffer Buffer for response
     * @param responseSize Size of response buffer
     * @return HTTP status code, or negative on error
     */
    int sendPostBinary(const char* url, const char* metadata,
                       const uint8_t* binaryData, size_t binarySize,
                       char* responseBuffer, size_t responseSize);

    /**
     * @brief Execute request with retry logic
     * @param url Request URL
     * @param body Request body
     * @param responseBuffer Response buffer
     * @param responseSize Response buffer size
     * @return Final HTTP status code
     */
    int executeWithRetry(const char* url, const char* body,
                         char* responseBuffer, size_t responseSize);

    /**
     * @brief Cache a failed upload
     * @param testRecord Test record to cache
     * @return true if cached successfully
     */
    bool cacheUpload(const BrevitestTestRecord* testRecord);

    /**
     * @brief Load cached uploads from filesystem
     * @return Number of cached uploads loaded
     */
    uint8_t loadCacheIndex();

    /**
     * @brief Save cache index to filesystem
     * @return true if saved successfully
     */
    bool saveCacheIndex();

    /**
     * @brief Update connection state
     * @param connected New connection state
     */
    void updateConnectionState(bool connected);

    // ========================================================================
    // PRIVATE MEMBERS
    // ========================================================================

    // Configuration
    char _endpoint[CLOUD_MAX_URL_LENGTH];          ///< Supabase endpoint URL
    char _apiKey[CLOUD_MAX_API_KEY_LENGTH];        ///< API key for auth
    char _deviceId[DEVICE_UUID_LENGTH + 1];        ///< Device identifier
    uint32_t _timeoutMs;                           ///< Request timeout
    uint8_t _maxRetries;                           ///< Max retry attempts
    uint32_t _retryDelayMs;                        ///< Base retry delay

    // State
    bool _initialized;                             ///< Client initialized
    bool _connected;                               ///< Connection state
    int _lastHttpStatus;                           ///< Last HTTP response code
    char _lastRequestId[CLOUD_REQUEST_ID_LENGTH + 1]; ///< Last request ID

    // Cache
    CachedUpload _uploadCache[CLOUD_MAX_CACHED_UPLOADS]; ///< Upload cache
    uint8_t _cacheCount;                           ///< Number of cached items

    // Callbacks
    ConnectionStateCallback _connectionCallback;   ///< Connection callback
    void* _connectionContext;                      ///< Connection callback context

    // Diagnostics
    uint32_t _requestCount;                        ///< Total requests
    uint32_t _failedCount;                         ///< Failed requests
    uint32_t _totalResponseTime;                   ///< Total response time (ms)

    // HTTP client (platform-specific)
    // Note: Implementation depends on Particle HTTPClient
};

// ============================================================================
// SINGLETON ACCESSOR
// ============================================================================

/**
 * @brief Get the global SupabaseClient instance
 * @return Reference to singleton SupabaseClient
 */
SupabaseClient& Cloud();

#endif // SUPABASE_CLIENT_H
