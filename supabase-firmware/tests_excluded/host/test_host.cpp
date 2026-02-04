/**
 * @file test_host.cpp
 * @brief Host-based unit tests for Brevitest firmware core logic
 * @details These tests run on your desktop computer without hardware.
 *          Compile with: g++ -std=c++11 -I../../src test_host.cpp ../../src/StateMachine.cpp ../../src/DataTypes.cpp -o test_host
 *          Run with: ./test_host
 *
 * Tests cover:
 *   - State machine transition validation
 *   - Data structure sizes (binary compatibility)
 *   - CRC32 checksum calculation
 *   - Base64 encoding/decoding
 *   - State history circular buffer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Include firmware headers (these work without Particle SDK when PARTICLE is not defined)
#include "DataTypes.h"
#include "StateMachine.h"

//==============================================================================
// TEST FRAMEWORK (minimal)
//==============================================================================

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("  Running: %s... ", #name); \
    tests_run++; \
    try { \
        test_##name(); \
        tests_passed++; \
        printf("PASSED\n"); \
    } catch (...) { \
        tests_failed++; \
        printf("FAILED (exception)\n"); \
    } \
} while(0)

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        printf("\n    ASSERTION FAILED: %s (line %d)\n", #expr, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ(a, b) do { \
    if ((a) != (b)) { \
        printf("\n    ASSERTION FAILED: %s == %s (line %d)\n", #a, #b, __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

#define ASSERT_STREQ(a, b) do { \
    if (strcmp((a), (b)) != 0) { \
        printf("\n    ASSERTION FAILED: strcmp(%s, %s) (line %d)\n    got: '%s' vs '%s'\n", #a, #b, __LINE__, (a), (b)); \
        tests_failed++; \
        return; \
    } \
} while(0)

//==============================================================================
// DATA STRUCTURE SIZE TESTS
//==============================================================================

TEST(spectrophotometer_reading_size) {
    // PRD Requirement: BrevitestSpectrophotometerReading must be exactly 32 bytes
    ASSERT_EQ(sizeof(BrevitestSpectrophotometerReading), 32);
}

TEST(test_record_size) {
    // PRD Requirement: BrevitestTestRecord must be exactly 9668 bytes
    ASSERT_EQ(sizeof(BrevitestTestRecord), 9668);
}

TEST(test_record_header_layout) {
    BrevitestTestRecord record;
    initTestRecord(&record);

    // Verify default values
    ASSERT_EQ(record.data_format_code, 'J');
    ASSERT_EQ(record.astep, 999);  // SPECTRO_ASTEP_DEFAULT
    ASSERT_EQ(record.atime, 49);   // SPECTRO_ATIME_DEFAULT
    ASSERT_EQ(record.again, 7);    // SPECTRO_AGAIN_DEFAULT
    ASSERT_EQ(record.number_of_readings, 0);
}

//==============================================================================
// ENUM STRING CONVERSION TESTS
//==============================================================================

TEST(device_mode_to_string) {
    ASSERT_STREQ(deviceModeToString(DeviceMode::IDLE), "IDLE");
    ASSERT_STREQ(deviceModeToString(DeviceMode::INITIALIZING), "INITIALIZING");
    ASSERT_STREQ(deviceModeToString(DeviceMode::HEATING), "HEATING");
    ASSERT_STREQ(deviceModeToString(DeviceMode::BARCODE_SCANNING), "BARCODE_SCANNING");
    ASSERT_STREQ(deviceModeToString(DeviceMode::VALIDATING_CARTRIDGE), "VALIDATING_CARTRIDGE");
    ASSERT_STREQ(deviceModeToString(DeviceMode::VALIDATING_MAGNETOMETER), "VALIDATING_MAGNETOMETER");
    ASSERT_STREQ(deviceModeToString(DeviceMode::RUNNING_TEST), "RUNNING_TEST");
    ASSERT_STREQ(deviceModeToString(DeviceMode::UPLOADING_RESULTS), "UPLOADING_RESULTS");
    ASSERT_STREQ(deviceModeToString(DeviceMode::RESETTING_CARTRIDGE), "RESETTING_CARTRIDGE");
    ASSERT_STREQ(deviceModeToString(DeviceMode::STRESS_TESTING), "STRESS_TESTING");
    ASSERT_STREQ(deviceModeToString(DeviceMode::ERROR_STATE), "ERROR_STATE");
}

TEST(test_state_to_string) {
    ASSERT_STREQ(testStateToString(TestState::NOT_STARTED), "NOT_STARTED");
    ASSERT_STREQ(testStateToString(TestState::RUNNING), "RUNNING");
    ASSERT_STREQ(testStateToString(TestState::COMPLETED), "COMPLETED");
    ASSERT_STREQ(testStateToString(TestState::CANCELLED), "CANCELLED");
    ASSERT_STREQ(testStateToString(TestState::UPLOADED), "UPLOADED");
}

TEST(cartridge_state_to_string) {
    ASSERT_STREQ(cartridgeStateToString(CartridgeState::NOT_INSERTED), "NOT_INSERTED");
    ASSERT_STREQ(cartridgeStateToString(CartridgeState::DETECTED), "DETECTED");
    ASSERT_STREQ(cartridgeStateToString(CartridgeState::VALIDATED), "VALIDATED");
    ASSERT_STREQ(cartridgeStateToString(CartridgeState::INVALID), "INVALID");
}

//==============================================================================
// STATE MACHINE TRANSITION VALIDATION TESTS
//==============================================================================

TEST(transition_same_state_invalid) {
    // PRD Requirement: Same state is not a valid transition
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::IDLE));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::HEATING));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::ERROR_STATE, DeviceMode::ERROR_STATE));
}

TEST(transition_always_allow_error_state) {
    // PRD Requirement: Can ALWAYS transition to ERROR_STATE from any state
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::ERROR_STATE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::ERROR_STATE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::RUNNING_TEST, DeviceMode::ERROR_STATE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::ERROR_STATE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::ERROR_STATE));
}

TEST(transition_always_allow_idle) {
    // PRD Requirement: Can ALWAYS transition to IDLE from any state
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::RUNNING_TEST, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::ERROR_STATE, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::UPLOADING_RESULTS, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::STRESS_TESTING, DeviceMode::IDLE));
}

TEST(transition_initializing_valid) {
    // INITIALIZING -> IDLE or HEATING only
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::INITIALIZING, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::INITIALIZING, DeviceMode::HEATING));

    // Invalid targets from INITIALIZING
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::INITIALIZING, DeviceMode::RUNNING_TEST));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::INITIALIZING, DeviceMode::BARCODE_SCANNING));
}

TEST(transition_idle_valid) {
    // IDLE -> BARCODE_SCANNING, STRESS_TESTING, HEATING, UPLOADING_RESULTS
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::BARCODE_SCANNING));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::STRESS_TESTING));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::HEATING));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::UPLOADING_RESULTS));

    // Invalid targets from IDLE
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::RUNNING_TEST));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::VALIDATING_CARTRIDGE));
}

TEST(transition_heating_valid) {
    // HEATING -> IDLE, BARCODE_SCANNING, STRESS_TESTING
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::BARCODE_SCANNING));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::STRESS_TESTING));

    // Invalid targets from HEATING
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::RUNNING_TEST));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::UPLOADING_RESULTS));
}

TEST(transition_barcode_scanning_valid) {
    // BARCODE_SCANNING -> VALIDATING_CARTRIDGE, VALIDATING_MAGNETOMETER, STRESS_TESTING, IDLE
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::VALIDATING_CARTRIDGE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::VALIDATING_MAGNETOMETER));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::STRESS_TESTING));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::IDLE));

    // Invalid targets from BARCODE_SCANNING
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::RUNNING_TEST));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::UPLOADING_RESULTS));
}

TEST(transition_validating_cartridge_valid) {
    // VALIDATING_CARTRIDGE -> RUNNING_TEST, RESETTING_CARTRIDGE, IDLE
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::RUNNING_TEST));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::RESETTING_CARTRIDGE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::IDLE));

    // Invalid targets
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::BARCODE_SCANNING));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::HEATING));
}

TEST(transition_running_test_valid) {
    // RUNNING_TEST -> UPLOADING_RESULTS, IDLE
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::RUNNING_TEST, DeviceMode::UPLOADING_RESULTS));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::RUNNING_TEST, DeviceMode::IDLE));

    // Invalid targets
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::RUNNING_TEST, DeviceMode::BARCODE_SCANNING));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::RUNNING_TEST, DeviceMode::VALIDATING_CARTRIDGE));
}

//==============================================================================
// STATE MACHINE INSTANCE TESTS
//==============================================================================

TEST(state_machine_init) {
    StateMachine sm;
    ASSERT_FALSE(sm.isInitialized());

    ASSERT_TRUE(sm.init());
    ASSERT_TRUE(sm.isInitialized());

    // Should start in INITIALIZING
    ASSERT_EQ(sm.getCurrentMode(), DeviceMode::INITIALIZING);
}

TEST(state_machine_valid_transition) {
    StateMachine sm;
    sm.init();

    // INITIALIZING -> IDLE should work
    ErrorCode result = sm.setMode(DeviceMode::IDLE);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    ASSERT_EQ(sm.getCurrentMode(), DeviceMode::IDLE);
    ASSERT_EQ(sm.getPreviousMode(), DeviceMode::INITIALIZING);
}

TEST(state_machine_invalid_transition) {
    StateMachine sm;
    sm.init();

    // INITIALIZING -> RUNNING_TEST should fail
    ErrorCode result = sm.setMode(DeviceMode::RUNNING_TEST);
    ASSERT_EQ(result, ErrorCode::ERR_INVALID_STATE);

    // Should still be in INITIALIZING
    ASSERT_EQ(sm.getCurrentMode(), DeviceMode::INITIALIZING);
}

TEST(state_machine_force_mode) {
    StateMachine sm;
    sm.init();

    // Force an invalid transition
    ErrorCode result = sm.forceMode(DeviceMode::RUNNING_TEST);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    ASSERT_EQ(sm.getCurrentMode(), DeviceMode::RUNNING_TEST);
}

TEST(state_machine_reset_to_idle) {
    StateMachine sm;
    sm.init();
    sm.forceMode(DeviceMode::RUNNING_TEST);
    sm.setTestState(TestState::RUNNING);
    sm.setCartridgeState(CartridgeState::VALIDATED);
    sm.setCurrentCartridgeId("test-uuid-12345");

    sm.resetToIdle();

    ASSERT_EQ(sm.getCurrentMode(), DeviceMode::IDLE);
    ASSERT_EQ(sm.getTestState(), TestState::NOT_STARTED);
    ASSERT_EQ(sm.getCartridgeState(), CartridgeState::NOT_INSERTED);
    ASSERT_STREQ(sm.getCurrentCartridgeId(), "");
}

TEST(state_machine_history_circular_buffer) {
    StateMachine sm;
    sm.init();

    // Make 60 transitions (buffer is 50)
    for (int i = 0; i < 60; i++) {
        sm.forceMode(DeviceMode::IDLE);
        sm.forceMode(DeviceMode::HEATING);
    }

    // Should have exactly 50 entries (circular buffer limit)
    ASSERT_EQ(sm.getTransitionCount(), 50);
}

TEST(state_machine_cartridge_id) {
    StateMachine sm;
    sm.init();

    const char* testId = "550e8400-e29b-41d4-a716-446655440000";
    sm.setCurrentCartridgeId(testId);

    ASSERT_STREQ(sm.getCurrentCartridgeId(), testId);

    sm.clearCurrentCartridgeId();
    ASSERT_STREQ(sm.getCurrentCartridgeId(), "");
}

TEST(state_machine_cartridge_state) {
    StateMachine sm;
    sm.init();

    ASSERT_EQ(sm.getCartridgeState(), CartridgeState::NOT_INSERTED);
    ASSERT_FALSE(sm.hasCartridge());

    sm.onCartridgeInserted();
    ASSERT_EQ(sm.getCartridgeState(), CartridgeState::DETECTED);
    ASSERT_TRUE(sm.hasCartridge());

    sm.setCartridgeState(CartridgeState::VALIDATED);
    ASSERT_TRUE(sm.isCartridgeValidated());

    sm.onCartridgeRemoved();
    ASSERT_EQ(sm.getCartridgeState(), CartridgeState::NOT_INSERTED);
    ASSERT_FALSE(sm.hasCartridge());
}

TEST(state_machine_test_state) {
    StateMachine sm;
    sm.init();

    ASSERT_EQ(sm.getTestState(), TestState::NOT_STARTED);
    ASSERT_TRUE(sm.canStartNewTest());

    // NOT_STARTED -> RUNNING
    ASSERT_EQ(sm.setTestState(TestState::RUNNING), ErrorCode::SUCCESS);
    ASSERT_TRUE(sm.isTestInProgress());

    // RUNNING -> COMPLETED
    ASSERT_EQ(sm.setTestState(TestState::COMPLETED), ErrorCode::SUCCESS);
    ASSERT_FALSE(sm.isTestInProgress());
    ASSERT_TRUE(sm.hasResultsPendingUpload());
}

//==============================================================================
// CRC32 TESTS
//==============================================================================

TEST(crc32_known_value) {
    // "123456789" has known CRC32 = 0xCBF43926
    const uint8_t data[] = "123456789";
    uint32_t crc = calculateCRC32(data, 9);
    ASSERT_EQ(crc, 0xCBF43926);
}

TEST(crc32_empty) {
    uint32_t crc = calculateCRC32(nullptr, 0);
    ASSERT_EQ(crc, 0);
}

TEST(crc32_single_byte) {
    const uint8_t data[] = {0x00};
    uint32_t crc = calculateCRC32(data, 1);
    ASSERT_EQ(crc, 0xD202EF8D);
}

TEST(test_record_checksum) {
    BrevitestTestRecord record;
    initTestRecord(&record);

    strcpy(record.cartridge_id, "test-cartridge-id");
    strcpy(record.assay_id, "ASSAY001");
    record.start_time = 1706500000;
    record.duration = 120;
    record.number_of_readings = 10;

    // Calculate and set checksum
    record.checksum = calculateTestRecordChecksum(&record);

    // Verify checksum
    ASSERT_TRUE(verifyTestRecordChecksum(&record));

    // Modify data and verify checksum fails
    record.duration = 121;
    ASSERT_FALSE(verifyTestRecordChecksum(&record));
}

//==============================================================================
// BASE64 TESTS
//==============================================================================

TEST(base64_encode_basic) {
    const uint8_t data[] = "Hello";
    char output[32];

    size_t len = base64Encode(data, 5, output, sizeof(output));
    ASSERT_TRUE(len > 0);
    ASSERT_STREQ(output, "SGVsbG8=");
}

TEST(base64_decode_basic) {
    const char* input = "SGVsbG8=";
    uint8_t output[32];

    size_t len = base64Decode(input, strlen(input), output, sizeof(output));
    ASSERT_EQ(len, 5);
    output[len] = '\0';
    ASSERT_STREQ((char*)output, "Hello");
}

TEST(base64_roundtrip) {
    const uint8_t original[] = "Test data for base64 roundtrip!";
    char encoded[128];
    uint8_t decoded[64];

    size_t encLen = base64Encode(original, strlen((char*)original), encoded, sizeof(encoded));
    ASSERT_TRUE(encLen > 0);

    size_t decLen = base64Decode(encoded, encLen, decoded, sizeof(decoded));
    ASSERT_EQ(decLen, strlen((char*)original));

    decoded[decLen] = '\0';
    ASSERT_STREQ((char*)decoded, (char*)original);
}

//==============================================================================
// SERIALIZATION TESTS
//==============================================================================

TEST(serialize_test_record) {
    BrevitestTestRecord original;
    initTestRecord(&original);

    strcpy(original.cartridge_id, "test-uuid");
    strcpy(original.assay_id, "ASSAY001");
    original.start_time = 1706500000;
    original.duration = 60;
    original.number_of_readings = 5;

    // Add a reading
    original.reading[0].number = 0;
    original.reading[0].channel = 'A';
    original.reading[0].f1 = 1234;
    original.reading[0].f2 = 5678;

    // Serialize
    uint8_t buffer[10000];
    size_t serializedSize = serializeTestRecord(&original, buffer, sizeof(buffer));
    ASSERT_EQ(serializedSize, 9668);

    // Deserialize
    BrevitestTestRecord restored;
    ASSERT_TRUE(deserializeTestRecord(buffer, serializedSize, &restored));

    // Verify
    ASSERT_STREQ(restored.cartridge_id, original.cartridge_id);
    ASSERT_STREQ(restored.assay_id, original.assay_id);
    ASSERT_EQ(restored.start_time, original.start_time);
    ASSERT_EQ(restored.duration, original.duration);
    ASSERT_EQ(restored.reading[0].f1, 1234);
    ASSERT_EQ(restored.reading[0].f2, 5678);
}

//==============================================================================
// MAIN
//==============================================================================

int main(int argc, char** argv) {
    printf("\n========================================\n");
    printf("Brevitest Firmware Host Unit Tests\n");
    printf("========================================\n\n");

    printf("Data Structure Tests:\n");
    RUN_TEST(spectrophotometer_reading_size);
    RUN_TEST(test_record_size);
    RUN_TEST(test_record_header_layout);

    printf("\nEnum Conversion Tests:\n");
    RUN_TEST(device_mode_to_string);
    RUN_TEST(test_state_to_string);
    RUN_TEST(cartridge_state_to_string);

    printf("\nState Transition Validation Tests:\n");
    RUN_TEST(transition_same_state_invalid);
    RUN_TEST(transition_always_allow_error_state);
    RUN_TEST(transition_always_allow_idle);
    RUN_TEST(transition_initializing_valid);
    RUN_TEST(transition_idle_valid);
    RUN_TEST(transition_heating_valid);
    RUN_TEST(transition_barcode_scanning_valid);
    RUN_TEST(transition_validating_cartridge_valid);
    RUN_TEST(transition_running_test_valid);

    printf("\nState Machine Instance Tests:\n");
    RUN_TEST(state_machine_init);
    RUN_TEST(state_machine_valid_transition);
    RUN_TEST(state_machine_invalid_transition);
    RUN_TEST(state_machine_force_mode);
    RUN_TEST(state_machine_reset_to_idle);
    RUN_TEST(state_machine_history_circular_buffer);
    RUN_TEST(state_machine_cartridge_id);
    RUN_TEST(state_machine_cartridge_state);
    RUN_TEST(state_machine_test_state);

    printf("\nCRC32 Tests:\n");
    RUN_TEST(crc32_known_value);
    RUN_TEST(crc32_empty);
    RUN_TEST(crc32_single_byte);
    RUN_TEST(test_record_checksum);

    printf("\nBase64 Tests:\n");
    RUN_TEST(base64_encode_basic);
    RUN_TEST(base64_decode_basic);
    RUN_TEST(base64_roundtrip);

    printf("\nSerialization Tests:\n");
    RUN_TEST(serialize_test_record);

    printf("\n========================================\n");
    printf("Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(" (%d FAILED)", tests_failed);
    }
    printf("\n========================================\n\n");

    return tests_failed > 0 ? 1 : 0;
}
