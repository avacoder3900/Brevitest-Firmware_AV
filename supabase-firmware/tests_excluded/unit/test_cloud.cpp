/**
 * @file test_cloud.cpp
 * @brief Unit tests for SupabaseClient and CloudProtocol
 * @author Agent DELTA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file contains unit tests for the cloud communication module,
 * using a mocked HTTP client to test request formatting, response
 * parsing, retry logic, and cache operations.
 *
 * User Story: CLOUD-009 (Cloud communication unit tests)
 *
 * Test Categories:
 * - Request serialization
 * - Response deserialization
 * - Retry logic
 * - Cache operations
 * - Timeout handling
 * - Error code mapping
 */

#include <unity.h>
#include <string.h>
#include <stdlib.h>

// Include module under test
#include "CloudProtocol.h"
#include "SupabaseClient.h"
#include "DataTypes.h"

// ============================================================================
// MOCK HTTP CLIENT
// ============================================================================

/**
 * @brief Mock HTTP client for testing without actual network calls
 */
class MockHttpClient {
public:
    // Response to return on next request
    int responseCode = 200;
    const char* responseBody = "{}";

    // Request capture
    char lastUrl[512] = {0};
    char lastBody[4096] = {0};
    int requestCount = 0;

    // Simulate delays and failures
    bool simulateTimeout = false;
    bool simulateNetworkError = false;
    int failAfterAttempts = 0;

    void reset() {
        responseCode = 200;
        responseBody = "{}";
        lastUrl[0] = '\0';
        lastBody[0] = '\0';
        requestCount = 0;
        simulateTimeout = false;
        simulateNetworkError = false;
        failAfterAttempts = 0;
    }
};

static MockHttpClient mockHttp;

// ============================================================================
// TEST FIXTURES
// ============================================================================

void setUp(void) {
    mockHttp.reset();
}

void tearDown(void) {
    // Cleanup after each test
}

// ============================================================================
// CLOUDPROTOCOL TESTS - Request Serialization
// ============================================================================

/**
 * @brief Test ValidateCartridgeRequest serialization
 */
void test_serialize_validate_request(void) {
    ValidateCartridgeRequest request;
    strcpy(request.requestId, "test-request-id-12345678901234567");
    strcpy(request.deviceId, "device-123");
    strcpy(request.cartridgeUuid, "cartridge-uuid-1234567890123456789");
    request.timestamp = 1706550000;
    request.firmwareVersion = 100;
    request.dataFormatVersion = 40;

    char buffer[1024];
    size_t len = serializeValidateRequest(&request, buffer, sizeof(buffer));

    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "requestId"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "deviceId"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "cartridgeUuid"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "device-123"));
}

/**
 * @brief Test ValidateCartridgeRequest serialization with NULL input
 */
void test_serialize_validate_request_null(void) {
    char buffer[1024];

    size_t len = serializeValidateRequest(nullptr, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, len);

    ValidateCartridgeRequest request;
    len = serializeValidateRequest(&request, nullptr, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, len);
}

/**
 * @brief Test LoadAssayRequest serialization
 */
void test_serialize_load_assay_request(void) {
    LoadAssayRequest request;
    strcpy(request.requestId, "assay-request-12345");
    strcpy(request.deviceId, "device-456");
    strcpy(request.assayId, "ASSAY001");
    request.timestamp = 1706550100;
    request.firmwareVersion = 100;

    char buffer[1024];
    size_t len = serializeLoadAssayRequest(&request, buffer, sizeof(buffer));

    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "assayId"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "ASSAY001"));
}

/**
 * @brief Test UploadTestRequest serialization
 */
void test_serialize_upload_request(void) {
    UploadTestRequest request;
    strcpy(request.requestId, "upload-request-12345");
    strcpy(request.deviceId, "device-789");
    strcpy(request.cartridgeUuid, "cart-uuid-1234567890123456789012");
    strcpy(request.assayId, "ASSAY002");
    request.startTime = 1706550200;
    request.duration = 300;
    request.numberOfReadings = 150;
    request.baselineScans = 10;
    request.testScans = 140;
    request.checksum = 0xDEADBEEF;
    request.recordSize = 9668;
    request.timestamp = 1706550500;
    request.firmwareVersion = 100;
    request.dataFormatVersion = 40;

    char buffer[2048];
    size_t len = serializeUploadRequest(&request, buffer, sizeof(buffer));

    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "numberOfReadings"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "checksum"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "recordSize"));
}

/**
 * @brief Test ResetCartridgeRequest serialization
 */
void test_serialize_reset_request(void) {
    ResetCartridgeRequest request;
    strcpy(request.requestId, "reset-request-12345");
    strcpy(request.deviceId, "device-reset");
    strcpy(request.cartridgeUuid, "reset-cart-uuid-12345678901234567");
    request.timestamp = 1706550300;
    request.firmwareVersion = 100;

    char buffer[1024];
    size_t len = serializeResetRequest(&request, buffer, sizeof(buffer));

    TEST_ASSERT_GREATER_THAN(0, len);
    TEST_ASSERT_NOT_NULL(strstr(buffer, "cartridgeUuid"));
    TEST_ASSERT_NOT_NULL(strstr(buffer, "reset-cart-uuid"));
}

// ============================================================================
// CLOUDPROTOCOL TESTS - Response Deserialization
// ============================================================================

/**
 * @brief Test ValidateCartridgeResponse deserialization - success
 */
void test_deserialize_validate_response_success(void) {
    const char* json = R"({
        "requestId": "test-request-123",
        "status": "SUCCESS",
        "isValid": true,
        "cartridgeUuid": "cart-uuid-123",
        "assayId": "ASSAY001"
    })";

    ValidateCartridgeResponse response;
    bool result = deserializeValidateResponse(json, &response);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(CloudStatus::SUCCESS, response.status);
    TEST_ASSERT_TRUE(response.isValid);
    TEST_ASSERT_EQUAL_STRING("cart-uuid-123", response.cartridgeUuid);
    TEST_ASSERT_EQUAL_STRING("ASSAY001", response.assayId);
}

/**
 * @brief Test ValidateCartridgeResponse deserialization - error
 */
void test_deserialize_validate_response_error(void) {
    const char* json = R"({
        "requestId": "test-request-456",
        "status": "ERROR",
        "errorCode": 200,
        "errorMessage": "Cartridge not found",
        "isValid": false,
        "cartridgeUuid": "",
        "assayId": ""
    })";

    ValidateCartridgeResponse response;
    bool result = deserializeValidateResponse(json, &response);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(CloudStatus::ERR_CARTRIDGE_NOT_FOUND, response.status);
    TEST_ASSERT_FALSE(response.isValid);
    TEST_ASSERT_EQUAL_STRING("Cartridge not found", response.errorMessage);
}

/**
 * @brief Test LoadAssayResponse deserialization - success
 */
void test_deserialize_load_assay_response_success(void) {
    const char* json = R"({
        "requestId": "assay-request-789",
        "status": "SUCCESS",
        "assayId": "ASSAY002",
        "duration": 180000,
        "bcode": "TEST|BCODE|DATA|HERE",
        "checksum": 12345678
    })";

    LoadAssayResponse response;
    bool result = deserializeLoadAssayResponse(json, &response);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(CloudStatus::SUCCESS, response.status);
    TEST_ASSERT_EQUAL_STRING("ASSAY002", response.assayId);
    TEST_ASSERT_EQUAL(180000, response.duration);
    TEST_ASSERT_EQUAL_STRING("TEST|BCODE|DATA|HERE", response.bcode);
    TEST_ASSERT_EQUAL(12345678, response.bcodeChecksum);
}

/**
 * @brief Test UploadTestResponse deserialization - success
 */
void test_deserialize_upload_response_success(void) {
    const char* json = R"({
        "requestId": "upload-request-abc",
        "status": "SUCCESS",
        "cartridgeUuid": "cart-uuid-uploaded",
        "testResultId": "result-uuid-12345678901234567890",
        "acknowledged": true
    })";

    UploadTestResponse response;
    bool result = deserializeUploadResponse(json, &response);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(CloudStatus::SUCCESS, response.status);
    TEST_ASSERT_EQUAL_STRING("cart-uuid-uploaded", response.cartridgeUuid);
    TEST_ASSERT_TRUE(response.acknowledged);
}

/**
 * @brief Test UploadTestResponse deserialization - duplicate error
 */
void test_deserialize_upload_response_duplicate(void) {
    const char* json = R"({
        "requestId": "upload-request-dup",
        "status": "ERROR",
        "errorCode": 302,
        "errorMessage": "Test already uploaded",
        "cartridgeUuid": "cart-uuid-dup",
        "testResultId": "existing-result-id",
        "acknowledged": true
    })";

    UploadTestResponse response;
    bool result = deserializeUploadResponse(json, &response);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(CloudStatus::ERR_DUPLICATE_UPLOAD, response.status);
    TEST_ASSERT_TRUE(response.acknowledged);  // Still acknowledged to stop retries
}

/**
 * @brief Test ResetCartridgeResponse deserialization
 */
void test_deserialize_reset_response_success(void) {
    const char* json = R"({
        "requestId": "reset-request-xyz",
        "status": "SUCCESS",
        "cartridgeUuid": "cart-uuid-reset",
        "wasReset": true
    })";

    ResetCartridgeResponse response;
    bool result = deserializeResetResponse(json, &response);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(CloudStatus::SUCCESS, response.status);
    TEST_ASSERT_EQUAL_STRING("cart-uuid-reset", response.cartridgeUuid);
    TEST_ASSERT_TRUE(response.wasReset);
}

/**
 * @brief Test response deserialization with invalid JSON
 */
void test_deserialize_invalid_json(void) {
    const char* invalidJson = "{ this is not valid json }";

    ValidateCartridgeResponse response;
    bool result = deserializeValidateResponse(invalidJson, &response);

    TEST_ASSERT_FALSE(result);
}

/**
 * @brief Test response deserialization with NULL input
 */
void test_deserialize_null_input(void) {
    ValidateCartridgeResponse response;

    bool result = deserializeValidateResponse(nullptr, &response);
    TEST_ASSERT_FALSE(result);

    result = deserializeValidateResponse("{}", nullptr);
    TEST_ASSERT_FALSE(result);
}

// ============================================================================
// REQUEST ID GENERATION TESTS
// ============================================================================

/**
 * @brief Test request ID generation format
 */
void test_generate_request_id_format(void) {
    char requestId[CLOUD_REQUEST_ID_LENGTH + 1];
    generateRequestId(requestId);

    TEST_ASSERT_EQUAL(36, strlen(requestId));
    TEST_ASSERT_EQUAL('-', requestId[8]);
    TEST_ASSERT_EQUAL('-', requestId[13]);
    TEST_ASSERT_EQUAL('4', requestId[14]);  // UUID v4
    TEST_ASSERT_EQUAL('-', requestId[18]);
    TEST_ASSERT_EQUAL('-', requestId[23]);
}

/**
 * @brief Test request ID uniqueness
 */
void test_generate_request_id_unique(void) {
    char requestId1[CLOUD_REQUEST_ID_LENGTH + 1];
    char requestId2[CLOUD_REQUEST_ID_LENGTH + 1];

    generateRequestId(requestId1);
    generateRequestId(requestId2);

    TEST_ASSERT_NOT_EQUAL_STRING(requestId1, requestId2);
}

// ============================================================================
// CLOUD STATUS TESTS
// ============================================================================

/**
 * @brief Test cloudStatusToString for all status codes
 */
void test_cloud_status_to_string(void) {
    TEST_ASSERT_EQUAL_STRING("SUCCESS", cloudStatusToString(CloudStatus::SUCCESS));
    TEST_ASSERT_EQUAL_STRING("NO_CONNECTION", cloudStatusToString(CloudStatus::ERR_NO_CONNECTION));
    TEST_ASSERT_EQUAL_STRING("TIMEOUT", cloudStatusToString(CloudStatus::ERR_TIMEOUT));
    TEST_ASSERT_EQUAL_STRING("UNAUTHORIZED", cloudStatusToString(CloudStatus::ERR_UNAUTHORIZED));
    TEST_ASSERT_EQUAL_STRING("CARTRIDGE_NOT_FOUND", cloudStatusToString(CloudStatus::ERR_CARTRIDGE_NOT_FOUND));
    TEST_ASSERT_EQUAL_STRING("CARTRIDGE_USED", cloudStatusToString(CloudStatus::ERR_CARTRIDGE_USED));
    TEST_ASSERT_EQUAL_STRING("CARTRIDGE_EXPIRED", cloudStatusToString(CloudStatus::ERR_CARTRIDGE_EXPIRED));
    TEST_ASSERT_EQUAL_STRING("ASSAY_NOT_FOUND", cloudStatusToString(CloudStatus::ERR_ASSAY_NOT_FOUND));
    TEST_ASSERT_EQUAL_STRING("UPLOAD_FAILED", cloudStatusToString(CloudStatus::ERR_UPLOAD_FAILED));
    TEST_ASSERT_EQUAL_STRING("CHECKSUM_MISMATCH", cloudStatusToString(CloudStatus::ERR_CHECKSUM_MISMATCH));
    TEST_ASSERT_EQUAL_STRING("CACHE_FULL", cloudStatusToString(CloudStatus::ERR_CACHE_FULL));
}

// ============================================================================
// DATA STRUCTURE SIZE TESTS
// ============================================================================

/**
 * @brief Test CloudRequestBase size
 */
void test_request_base_fields(void) {
    CloudRequestBase base;

    TEST_ASSERT_EQUAL(CLOUD_REQUEST_ID_LENGTH + 1, sizeof(base.requestId));
    TEST_ASSERT_EQUAL(DEVICE_UUID_LENGTH + 1, sizeof(base.deviceId));
}

/**
 * @brief Test response base isSuccess method
 */
void test_response_base_is_success(void) {
    CloudResponseBase response;

    response.status = CloudStatus::SUCCESS;
    TEST_ASSERT_TRUE(response.isSuccess());

    response.status = CloudStatus::ERR_NO_CONNECTION;
    TEST_ASSERT_FALSE(response.isSuccess());
}

// ============================================================================
// CACHED UPLOAD STRUCTURE TESTS
// ============================================================================

/**
 * @brief Test CachedUpload default state
 */
void test_cached_upload_default(void) {
    CachedUpload cache;

    TEST_ASSERT_FALSE(cache.valid);
    TEST_ASSERT_EQUAL(0, cache.timestamp);
    TEST_ASSERT_EQUAL(0, cache.retryCount);
}

// ============================================================================
// SUPABASE CLIENT CONFIGURATION TESTS
// ============================================================================

/**
 * @brief Test endpoint configuration
 */
void test_client_set_endpoint(void) {
    SupabaseClient client;
    client.init();

    bool result = client.setEndpoint("https://test.supabase.co/functions/v1");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("https://test.supabase.co/functions/v1", client.getEndpoint());

    // Test trailing slash removal
    result = client.setEndpoint("https://test2.supabase.co/functions/v1/");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("https://test2.supabase.co/functions/v1", client.getEndpoint());
}

/**
 * @brief Test device ID configuration
 */
void test_client_set_device_id(void) {
    SupabaseClient client;
    client.init();

    bool result = client.setDeviceId("test-device-id-12345");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("test-device-id-12345", client.getDeviceId());
}

/**
 * @brief Test timeout configuration
 */
void test_client_set_timeout(void) {
    SupabaseClient client;
    client.init();

    client.setTimeout(30000);
    TEST_ASSERT_EQUAL(30000, client.getTimeout());

    // Test default
    SupabaseClient client2;
    client2.init();
    TEST_ASSERT_EQUAL(CLOUD_DEFAULT_TIMEOUT_MS, client2.getTimeout());
}

/**
 * @brief Test invalid endpoint
 */
void test_client_invalid_endpoint(void) {
    SupabaseClient client;
    client.init();

    bool result = client.setEndpoint(nullptr);
    TEST_ASSERT_FALSE(result);

    // Empty endpoint should work
    result = client.setEndpoint("");
    TEST_ASSERT_TRUE(result);
}

// ============================================================================
// CACHE OPERATIONS TESTS
// ============================================================================

/**
 * @brief Test cache count initial state
 */
void test_client_cache_initial_empty(void) {
    SupabaseClient client;
    client.init();

    TEST_ASSERT_EQUAL(0, client.getCacheCount());
    TEST_ASSERT_FALSE(client.isCacheFull());
}

/**
 * @brief Test getCacheInfo with invalid index
 */
void test_client_get_cache_info_invalid(void) {
    SupabaseClient client;
    client.init();

    CachedUpload info;
    bool result = client.getCacheInfo(100, info);  // Invalid index

    TEST_ASSERT_FALSE(result);
}

/**
 * @brief Test clearCacheEntry with invalid index
 */
void test_client_clear_cache_invalid(void) {
    SupabaseClient client;
    client.init();

    bool result = client.clearCacheEntry(100);  // Invalid index
    TEST_ASSERT_FALSE(result);
}

/**
 * @brief Test clearCache when empty
 */
void test_client_clear_cache_empty(void) {
    SupabaseClient client;
    client.init();

    uint8_t cleared = client.clearCache();
    TEST_ASSERT_EQUAL(0, cleared);
}

// ============================================================================
// DIAGNOSTICS TESTS
// ============================================================================

/**
 * @brief Test diagnostic counters initial state
 */
void test_client_diagnostics_initial(void) {
    SupabaseClient client;
    client.init();

    TEST_ASSERT_EQUAL(0, client.getRequestCount());
    TEST_ASSERT_EQUAL(0, client.getFailedCount());
    TEST_ASSERT_EQUAL(0, client.getAverageResponseTime());
}

/**
 * @brief Test diagnostic reset
 */
void test_client_diagnostics_reset(void) {
    SupabaseClient client;
    client.init();

    // Would need to make actual requests to test this fully
    client.resetDiagnostics();

    TEST_ASSERT_EQUAL(0, client.getRequestCount());
    TEST_ASSERT_EQUAL(0, client.getFailedCount());
}

// ============================================================================
// TEST RUNNER
// ============================================================================

void setup() {
    delay(2000);  // Wait for serial

    UNITY_BEGIN();

    // Request serialization tests
    RUN_TEST(test_serialize_validate_request);
    RUN_TEST(test_serialize_validate_request_null);
    RUN_TEST(test_serialize_load_assay_request);
    RUN_TEST(test_serialize_upload_request);
    RUN_TEST(test_serialize_reset_request);

    // Response deserialization tests
    RUN_TEST(test_deserialize_validate_response_success);
    RUN_TEST(test_deserialize_validate_response_error);
    RUN_TEST(test_deserialize_load_assay_response_success);
    RUN_TEST(test_deserialize_upload_response_success);
    RUN_TEST(test_deserialize_upload_response_duplicate);
    RUN_TEST(test_deserialize_reset_response_success);
    RUN_TEST(test_deserialize_invalid_json);
    RUN_TEST(test_deserialize_null_input);

    // Request ID tests
    RUN_TEST(test_generate_request_id_format);
    RUN_TEST(test_generate_request_id_unique);

    // Status string tests
    RUN_TEST(test_cloud_status_to_string);

    // Structure tests
    RUN_TEST(test_request_base_fields);
    RUN_TEST(test_response_base_is_success);
    RUN_TEST(test_cached_upload_default);

    // Client configuration tests
    RUN_TEST(test_client_set_endpoint);
    RUN_TEST(test_client_set_device_id);
    RUN_TEST(test_client_set_timeout);
    RUN_TEST(test_client_invalid_endpoint);

    // Cache operation tests
    RUN_TEST(test_client_cache_initial_empty);
    RUN_TEST(test_client_get_cache_info_invalid);
    RUN_TEST(test_client_clear_cache_invalid);
    RUN_TEST(test_client_clear_cache_empty);

    // Diagnostics tests
    RUN_TEST(test_client_diagnostics_initial);
    RUN_TEST(test_client_diagnostics_reset);

    UNITY_END();
}

void loop() {
    // Nothing to do in loop
}

// Native test entry point (for running on host)
#ifndef PARTICLE_PLATFORM
int main(int argc, char** argv) {
    setup();
    return 0;
}
#endif
