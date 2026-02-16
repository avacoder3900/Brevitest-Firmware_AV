/**
 * @file test_host.cpp
 * @brief Host-based unit tests for Brevitest firmware core logic
 * @details These tests run on your desktop computer without hardware.
 *
 * Build (from tests_excluded/host/):
 *   export PATH="/c/ProgramData/mingw64/mingw64/bin:$PATH"
 *   g++ -std=c++11 -I. -I../../src test_host.cpp \
 *       ../../src/StateMachine.cpp ../../src/DataTypes.cpp \
 *       ../../src/BCODEInterpreter.cpp ../../src/CloudProtocol.cpp \
 *       -o test_host.exe
 *
 * Run:  ./test_host.exe
 *
 * Tests cover:
 *   - Data structure sizes (binary compatibility)
 *   - Enum string conversions
 *   - State machine transitions and state management
 *   - CRC32 checksum calculation
 *   - Base64 encoding/decoding
 *   - Test record serialization
 *   - BCODE interpreter: parsing, execution, repeat blocks, error handling
 *   - CloudProtocol: JSON serialization and deserialization
 *   - Request ID generation (UUID v4 format)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Include firmware headers (mock Particle.h is in this directory via -I.)
#include "DataTypes.h"
#include "StateMachine.h"
#include "BCODEInterpreter.h"
#include "CloudProtocol.h"

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

#define ASSERT_CONTAINS(haystack, needle) do { \
    if (strstr((haystack), (needle)) == nullptr) { \
        printf("\n    ASSERTION FAILED: '%s' not found in '%s' (line %d)\n", (needle), (haystack), __LINE__); \
        tests_failed++; \
        return; \
    } \
} while(0)

//==============================================================================
// BCODE TEST HELPERS
//==============================================================================

// Track callback invocations for BCODE tests
static int cb_stage_move_count = 0;
static int32_t cb_last_microns = 0;
static uint16_t cb_last_step_delay = 0;
static int cb_spectro_config_count = 0;
static int cb_baseline_scan_count = 0;
static int cb_test_scan_count = 0;
static int cb_delay_loop_count = 0;

static void reset_bcode_callbacks() {
    cb_stage_move_count = 0;
    cb_last_microns = 0;
    cb_last_step_delay = 0;
    cb_spectro_config_count = 0;
    cb_baseline_scan_count = 0;
    cb_test_scan_count = 0;
    cb_delay_loop_count = 0;
}

static bool mock_stage_move(int32_t microns, uint16_t step_delay) {
    cb_stage_move_count++;
    cb_last_microns = microns;
    cb_last_step_delay = step_delay;
    return true;
}

static bool mock_spectro_config(uint8_t gain, uint16_t astep, uint8_t atime) {
    (void)gain; (void)astep; (void)atime;
    cb_spectro_config_count++;
    return true;
}

static uint16_t mock_spectro_scan(uint16_t num_scans, bool is_baseline) {
    if (is_baseline) cb_baseline_scan_count++;
    else cb_test_scan_count++;
    return num_scans;
}

static bool mock_delay_loop() {
    cb_delay_loop_count++;
    return true;  // continue execution
}

static bool mock_delay_loop_cancel() {
    cb_delay_loop_count++;
    return false;  // cancel execution
}

//==============================================================================
// DATA STRUCTURE SIZE TESTS
//==============================================================================

TEST(spectrophotometer_reading_size) {
    ASSERT_EQ(sizeof(BrevitestSpectrophotometerReading), 32);
}

TEST(test_record_size) {
    ASSERT_EQ(sizeof(BrevitestTestRecord), 9668);
}

TEST(test_record_header_layout) {
    BrevitestTestRecord record;
    initTestRecord(&record);
    ASSERT_EQ(record.data_format_code, 'J');
    ASSERT_EQ(record.astep, 999);
    ASSERT_EQ(record.atime, 49);
    ASSERT_EQ(record.again, 7);
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
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::IDLE));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::HEATING));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::ERROR_STATE, DeviceMode::ERROR_STATE));
}

TEST(transition_always_allow_error_state) {
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::ERROR_STATE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::ERROR_STATE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::RUNNING_TEST, DeviceMode::ERROR_STATE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::ERROR_STATE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::ERROR_STATE));
}

TEST(transition_always_allow_idle) {
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::RUNNING_TEST, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::ERROR_STATE, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::UPLOADING_RESULTS, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::STRESS_TESTING, DeviceMode::IDLE));
}

TEST(transition_initializing_valid) {
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::INITIALIZING, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::INITIALIZING, DeviceMode::HEATING));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::INITIALIZING, DeviceMode::RUNNING_TEST));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::INITIALIZING, DeviceMode::BARCODE_SCANNING));
}

TEST(transition_idle_valid) {
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::BARCODE_SCANNING));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::STRESS_TESTING));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::HEATING));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::UPLOADING_RESULTS));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::RUNNING_TEST));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::VALIDATING_CARTRIDGE));
}

TEST(transition_heating_valid) {
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::IDLE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::BARCODE_SCANNING));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::STRESS_TESTING));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::RUNNING_TEST));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::HEATING, DeviceMode::UPLOADING_RESULTS));
}

TEST(transition_barcode_scanning_valid) {
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::VALIDATING_CARTRIDGE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::VALIDATING_MAGNETOMETER));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::STRESS_TESTING));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::IDLE));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::RUNNING_TEST));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::BARCODE_SCANNING, DeviceMode::UPLOADING_RESULTS));
}

TEST(transition_validating_cartridge_valid) {
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::RUNNING_TEST));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::RESETTING_CARTRIDGE));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::IDLE));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::BARCODE_SCANNING));
    ASSERT_FALSE(StateMachine::isValidTransition(DeviceMode::VALIDATING_CARTRIDGE, DeviceMode::HEATING));
}

TEST(transition_running_test_valid) {
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::RUNNING_TEST, DeviceMode::UPLOADING_RESULTS));
    ASSERT_TRUE(StateMachine::isValidTransition(DeviceMode::RUNNING_TEST, DeviceMode::IDLE));
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
    ASSERT_EQ(sm.getCurrentMode(), DeviceMode::INITIALIZING);
}

TEST(state_machine_valid_transition) {
    StateMachine sm;
    sm.init();
    ErrorCode result = sm.setMode(DeviceMode::IDLE);
    ASSERT_EQ(result, ErrorCode::SUCCESS);
    ASSERT_EQ(sm.getCurrentMode(), DeviceMode::IDLE);
    ASSERT_EQ(sm.getPreviousMode(), DeviceMode::INITIALIZING);
}

TEST(state_machine_invalid_transition) {
    StateMachine sm;
    sm.init();
    ErrorCode result = sm.setMode(DeviceMode::RUNNING_TEST);
    ASSERT_EQ(result, ErrorCode::ERR_INVALID_STATE);
    ASSERT_EQ(sm.getCurrentMode(), DeviceMode::INITIALIZING);
}

TEST(state_machine_force_mode) {
    StateMachine sm;
    sm.init();
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
    for (int i = 0; i < 60; i++) {
        sm.forceMode(DeviceMode::IDLE);
        sm.forceMode(DeviceMode::HEATING);
    }
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
    ASSERT_EQ(sm.setTestState(TestState::RUNNING), ErrorCode::SUCCESS);
    ASSERT_TRUE(sm.isTestInProgress());
    ASSERT_EQ(sm.setTestState(TestState::COMPLETED), ErrorCode::SUCCESS);
    ASSERT_FALSE(sm.isTestInProgress());
    ASSERT_TRUE(sm.hasResultsPendingUpload());
}

//==============================================================================
// CRC32 TESTS
//==============================================================================

TEST(crc32_known_value) {
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
    record.checksum = calculateTestRecordChecksum(&record);
    ASSERT_TRUE(verifyTestRecordChecksum(&record));
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
    original.reading[0].number = 0;
    original.reading[0].channel = 'A';
    original.reading[0].f1 = 1234;
    original.reading[0].f2 = 5678;

    uint8_t buffer[10000];
    size_t serializedSize = serializeTestRecord(&original, buffer, sizeof(buffer));
    ASSERT_EQ(serializedSize, 9668);

    BrevitestTestRecord restored;
    ASSERT_TRUE(deserializeTestRecord(buffer, serializedSize, &restored));
    ASSERT_STREQ(restored.cartridge_id, original.cartridge_id);
    ASSERT_STREQ(restored.assay_id, original.assay_id);
    ASSERT_EQ(restored.start_time, original.start_time);
    ASSERT_EQ(restored.duration, original.duration);
    ASSERT_EQ(restored.reading[0].f1, 1234);
    ASSERT_EQ(restored.reading[0].f2, 5678);
}

//==============================================================================
// BCODE INTERPRETER TESTS
//==============================================================================

TEST(bcode_load_valid) {
    BCODEInterpreter interp;
    const char* bcode = "0:|1:1000|99:";
    ASSERT_TRUE(interp.loadBCODE(bcode, strlen(bcode)));
    ASSERT_TRUE(interp.isBCODELoaded());
    ASSERT_EQ(interp.getBCODELength(), (uint16_t)strlen(bcode));
    ASSERT_EQ(interp.getState(), InterpreterState::READY);
}

TEST(bcode_load_null) {
    BCODEInterpreter interp;
    ASSERT_FALSE(interp.loadBCODE(nullptr, 0));
    ASSERT_EQ(interp.getState(), InterpreterState::ERROR);
}

TEST(bcode_load_empty) {
    BCODEInterpreter interp;
    ASSERT_FALSE(interp.loadBCODE("", 0));
    ASSERT_EQ(interp.getState(), InterpreterState::ERROR);
}

TEST(bcode_simple_start_end) {
    // "0:|99:" = START_TEST, END_TEST
    BCODEInterpreter interp;
    const char* bcode = "0:|99:";
    interp.loadBCODE(bcode, strlen(bcode));
    interp.startExecution();
    ASSERT_EQ(interp.getState(), InterpreterState::RUNNING);

    // Execute START_TEST (opcode 0)
    bool more = interp.executeNextInstruction();
    ASSERT_TRUE(more);
    ASSERT_EQ(interp.getCurrentInstruction().opcode, BCODEOpcode::START_TEST);

    // Execute END_TEST (opcode 99)
    more = interp.executeNextInstruction();
    ASSERT_FALSE(more);
    ASSERT_EQ(interp.getState(), InterpreterState::COMPLETED);
}

TEST(bcode_parse_move) {
    // "0:|2:5000,300|99:"
    BCODEInterpreter interp;
    const char* bcode = "0:|2:5000,300|99:";
    reset_bcode_callbacks();
    interp.loadBCODE(bcode, strlen(bcode));
    interp.setStageMoveCallback(mock_stage_move);
    interp.startExecution();

    interp.executeNextInstruction(); // START_TEST
    interp.executeNextInstruction(); // MOVE_MICRONS

    ASSERT_EQ(cb_stage_move_count, 1);
    ASSERT_EQ(cb_last_microns, 5000);
    ASSERT_EQ(cb_last_step_delay, 300);
}

TEST(bcode_parse_sensor_params) {
    // "0:|10:7,999,49|99:"
    BCODEInterpreter interp;
    const char* bcode = "0:|10:7,999,49|99:";
    reset_bcode_callbacks();
    interp.loadBCODE(bcode, strlen(bcode));
    interp.setSpectroConfigCallback(mock_spectro_config);
    interp.startExecution();

    interp.executeNextInstruction(); // START
    interp.executeNextInstruction(); // SET_SENSOR_PARAMS

    ASSERT_EQ(cb_spectro_config_count, 1);
}

TEST(bcode_baseline_and_test_scans) {
    // "0:|11:5|14:10|99:"
    BCODEInterpreter interp;
    const char* bcode = "0:|11:5|14:10|99:";
    reset_bcode_callbacks();
    interp.loadBCODE(bcode, strlen(bcode));
    interp.setSpectroScanCallback(mock_spectro_scan);
    interp.startExecution();

    interp.executeNextInstruction(); // START
    interp.executeNextInstruction(); // BASELINE_SCANS(5)
    interp.executeNextInstruction(); // TEST_SCANS(10)

    ASSERT_EQ(cb_baseline_scan_count, 1);
    ASSERT_EQ(cb_test_scan_count, 1);
}

TEST(bcode_repeat_block) {
    // "0:|20:3|2:1000,200|21:|99:" = START, REPEAT(3) { MOVE }, END
    BCODEInterpreter interp;
    const char* bcode = "0:|20:3|2:1000,200|21:|99:";
    reset_bcode_callbacks();
    interp.loadBCODE(bcode, strlen(bcode));
    interp.setStageMoveCallback(mock_stage_move);
    interp.startExecution();

    // Run to completion
    while (interp.getState() == InterpreterState::RUNNING) {
        interp.executeNextInstruction();
    }

    ASSERT_EQ(interp.getState(), InterpreterState::COMPLETED);
    // MOVE should have been called 3 times (repeat 3 iterations)
    ASSERT_EQ(cb_stage_move_count, 3);
}

TEST(bcode_nested_repeat) {
    // "0:|20:2|20:3|2:100,50|21:|21:|99:"
    // Outer loop 2x, inner loop 3x = 6 moves
    BCODEInterpreter interp;
    const char* bcode = "0:|20:2|20:3|2:100,50|21:|21:|99:";
    reset_bcode_callbacks();
    interp.loadBCODE(bcode, strlen(bcode));
    interp.setStageMoveCallback(mock_stage_move);
    interp.startExecution();

    while (interp.getState() == InterpreterState::RUNNING) {
        interp.executeNextInstruction();
    }

    ASSERT_EQ(interp.getState(), InterpreterState::COMPLETED);
    ASSERT_EQ(cb_stage_move_count, 6);
}

TEST(bcode_missing_params_error) {
    // "0:|2:5000|99:" = MOVE with only 1 param (needs 2)
    BCODEInterpreter interp;
    const char* bcode = "0:|2:5000|99:";
    interp.loadBCODE(bcode, strlen(bcode));
    interp.startExecution();

    interp.executeNextInstruction(); // START_TEST
    bool ok = interp.executeNextInstruction(); // MOVE - should fail

    ASSERT_FALSE(ok);
    ASSERT_EQ(interp.getState(), InterpreterState::ERROR);
    ASSERT_EQ(interp.getLastError(), ErrorCode::ERR_BCODE_INVALID);
}

TEST(bcode_cancel_during_delay) {
    // "0:|1:5000|99:" with a callback that cancels
    BCODEInterpreter interp;
    const char* bcode = "0:|1:5000|99:";
    reset_bcode_callbacks();
    interp.loadBCODE(bcode, strlen(bcode));
    interp.setDelayLoopCallback(mock_delay_loop_cancel);
    interp.startExecution();

    interp.executeNextInstruction(); // START_TEST
    bool ok = interp.executeNextInstruction(); // DELAY - should be cancelled

    ASSERT_FALSE(ok);
    ASSERT_EQ(interp.getState(), InterpreterState::CANCELLED);
}

TEST(bcode_pause_resume) {
    BCODEInterpreter interp;
    const char* bcode = "0:|2:1000,200|99:";
    interp.loadBCODE(bcode, strlen(bcode));
    interp.startExecution();
    ASSERT_EQ(interp.getState(), InterpreterState::RUNNING);

    interp.pauseExecution();
    ASSERT_EQ(interp.getState(), InterpreterState::PAUSED);

    // Cannot execute while paused
    ASSERT_FALSE(interp.executeNextInstruction());

    interp.resumeExecution();
    ASSERT_EQ(interp.getState(), InterpreterState::RUNNING);
}

TEST(bcode_opcode_to_string) {
    ASSERT_STREQ(bcodeOpcodeToString(BCODEOpcode::START_TEST), "START_TEST");
    ASSERT_STREQ(bcodeOpcodeToString(BCODEOpcode::DELAY), "DELAY");
    ASSERT_STREQ(bcodeOpcodeToString(BCODEOpcode::MOVE_MICRONS), "MOVE_MICRONS");
    ASSERT_STREQ(bcodeOpcodeToString(BCODEOpcode::END_TEST), "END_TEST");
    ASSERT_STREQ(bcodeOpcodeToString(BCODEOpcode::REPEAT_BEGIN), "REPEAT_BEGIN");
    ASSERT_STREQ(bcodeOpcodeToString(BCODEOpcode::REPEAT_END), "REPEAT_END");
    ASSERT_STREQ(bcodeOpcodeToString(BCODEOpcode::INVALID), "INVALID");
}

TEST(bcode_interpreter_state_to_string) {
    ASSERT_STREQ(interpreterStateToString(InterpreterState::IDLE), "IDLE");
    ASSERT_STREQ(interpreterStateToString(InterpreterState::READY), "READY");
    ASSERT_STREQ(interpreterStateToString(InterpreterState::RUNNING), "RUNNING");
    ASSERT_STREQ(interpreterStateToString(InterpreterState::COMPLETED), "COMPLETED");
    ASSERT_STREQ(interpreterStateToString(InterpreterState::ERROR), "ERROR");
    ASSERT_STREQ(interpreterStateToString(InterpreterState::CANCELLED), "CANCELLED");
}

//==============================================================================
// CLOUD PROTOCOL TESTS
//==============================================================================

TEST(cloud_serialize_validate_request) {
    ValidateCartridgeRequest req;
    strcpy(req.requestId, "aaaaaaaa-bbbb-4ccc-dddd-eeeeeeeeeeee");
    strcpy(req.deviceId, "device-001");
    req.timestamp = 1706500000;
    strcpy(req.cartridgeUuid, "550e8400-e29b-41d4-a716-446655440000");

    char buffer[CLOUD_MAX_REQUEST_SIZE];
    size_t len = serializeValidateRequest(&req, buffer, sizeof(buffer));

    ASSERT_TRUE(len > 0);
    ASSERT_CONTAINS(buffer, "\"requestId\":\"aaaaaaaa-bbbb-4ccc-dddd-eeeeeeeeeeee\"");
    ASSERT_CONTAINS(buffer, "\"deviceId\":\"device-001\"");
    ASSERT_CONTAINS(buffer, "\"cartridgeUuid\":\"550e8400-e29b-41d4-a716-446655440000\"");
    ASSERT_CONTAINS(buffer, "\"firmwareVersion\":200");
}

TEST(cloud_serialize_upload_request) {
    UploadTestRequest req;
    strcpy(req.requestId, "req-upload-001");
    strcpy(req.deviceId, "dev-001");
    req.timestamp = 1706500000;
    strcpy(req.cartridgeUuid, "cart-uuid-001");
    strcpy(req.assayId, "ASSAY001");
    req.startTime = 1706500100;
    req.duration = 120;
    req.numberOfReadings = 50;
    req.baselineScans = 5;
    req.testScans = 10;
    req.checksum = 0xDEADBEEF;
    req.recordSize = 9668;

    char buffer[CLOUD_MAX_REQUEST_SIZE];
    size_t len = serializeUploadRequest(&req, buffer, sizeof(buffer));

    ASSERT_TRUE(len > 0);
    ASSERT_CONTAINS(buffer, "\"duration\":120");
    ASSERT_CONTAINS(buffer, "\"numberOfReadings\":50");
    ASSERT_CONTAINS(buffer, "\"recordSize\":9668");
}

TEST(cloud_deserialize_validate_response) {
    const char* json = "{"
        "\"requestId\":\"test-req-001\","
        "\"status\":0,"
        "\"errorMessage\":\"\","
        "\"serverTimestamp\":1706500500,"
        "\"isValid\":true,"
        "\"cartridgeUuid\":\"cart-uuid-validated\","
        "\"assayId\":\"ASSAY002\""
        "}";

    ValidateCartridgeResponse resp;
    ASSERT_TRUE(deserializeValidateResponse(json, &resp));

    ASSERT_STREQ(resp.requestId, "test-req-001");
    ASSERT_EQ(resp.status, CloudStatus::SUCCESS);
    ASSERT_TRUE(resp.isValid);
    ASSERT_STREQ(resp.cartridgeUuid, "cart-uuid-validated");
    ASSERT_STREQ(resp.assayId, "ASSAY002");
    ASSERT_EQ(resp.serverTimestamp, (uint32_t)1706500500);
}

TEST(cloud_deserialize_validate_response_error) {
    const char* json = "{"
        "\"requestId\":\"req-err\","
        "\"status\":201,"
        "\"errorMessage\":\"Cartridge already used\","
        "\"isValid\":false,"
        "\"cartridgeUuid\":\"\","
        "\"assayId\":\"\""
        "}";

    ValidateCartridgeResponse resp;
    ASSERT_TRUE(deserializeValidateResponse(json, &resp));

    ASSERT_EQ(resp.status, CloudStatus::ERR_CARTRIDGE_USED);
    ASSERT_FALSE(resp.isValid);
    ASSERT_STREQ(resp.errorMessage, "Cartridge already used");
}

TEST(cloud_deserialize_load_assay_response) {
    const char* json = "{"
        "\"requestId\":\"req-assay\","
        "\"status\":0,"
        "\"errorMessage\":\"\","
        "\"serverTimestamp\":1706501000,"
        "\"assayId\":\"ASSAY003\","
        "\"duration\":60000,"
        "\"bcodeLength\":25,"
        "\"bcodeChecksum\":12345678,"
        "\"bcode\":\"0:|1:1000|2:5000,300|99:\""
        "}";

    LoadAssayResponse resp;
    ASSERT_TRUE(deserializeLoadAssayResponse(json, &resp));

    ASSERT_STREQ(resp.assayId, "ASSAY003");
    ASSERT_EQ(resp.duration, 60000);
    ASSERT_EQ(resp.bcodeLength, 25);
    ASSERT_EQ(resp.bcodeChecksum, (uint32_t)12345678);
    ASSERT_STREQ(resp.bcode, "0:|1:1000|2:5000,300|99:");
}

TEST(cloud_deserialize_upload_response) {
    const char* json = "{"
        "\"requestId\":\"req-upload\","
        "\"status\":0,"
        "\"serverTimestamp\":1706501500,"
        "\"cartridgeUuid\":\"cart-uploaded\","
        "\"testResultId\":\"result-uuid-123\","
        "\"acknowledged\":true"
        "}";

    UploadTestResponse resp;
    ASSERT_TRUE(deserializeUploadResponse(json, &resp));

    ASSERT_TRUE(resp.acknowledged);
    ASSERT_STREQ(resp.testResultId, "result-uuid-123");
    ASSERT_STREQ(resp.cartridgeUuid, "cart-uploaded");
}

TEST(cloud_deserialize_reset_response) {
    const char* json = "{"
        "\"requestId\":\"req-reset\","
        "\"status\":0,"
        "\"cartridgeUuid\":\"cart-reset-001\","
        "\"wasReset\":true"
        "}";

    ResetCartridgeResponse resp;
    ASSERT_TRUE(deserializeResetResponse(json, &resp));

    ASSERT_TRUE(resp.wasReset);
    ASSERT_STREQ(resp.cartridgeUuid, "cart-reset-001");
}

TEST(cloud_deserialize_null_handling) {
    ASSERT_FALSE(deserializeValidateResponse(nullptr, nullptr));
    ASSERT_FALSE(deserializeLoadAssayResponse(nullptr, nullptr));
    ASSERT_FALSE(deserializeUploadResponse(nullptr, nullptr));
    ASSERT_FALSE(deserializeResetResponse(nullptr, nullptr));
}

TEST(cloud_generate_request_id_format) {
    srand(42); // Deterministic
    char id[CLOUD_REQUEST_ID_LENGTH + 1];
    generateRequestId(id);

    // Check length
    ASSERT_EQ(strlen(id), 36);

    // Check hyphen positions
    ASSERT_EQ(id[8], '-');
    ASSERT_EQ(id[13], '-');
    ASSERT_EQ(id[18], '-');
    ASSERT_EQ(id[23], '-');

    // Check version nibble (position 14 = '4')
    ASSERT_EQ(id[14], '4');

    // Check variant nibble (position 19 = 8, 9, a, or b)
    ASSERT_TRUE(id[19] == '8' || id[19] == '9' || id[19] == 'a' || id[19] == 'b');
}

TEST(cloud_status_to_string) {
    ASSERT_STREQ(cloudStatusToString(CloudStatus::SUCCESS), "SUCCESS");
    ASSERT_STREQ(cloudStatusToString(CloudStatus::ERR_NO_CONNECTION), "NO_CONNECTION");
    ASSERT_STREQ(cloudStatusToString(CloudStatus::ERR_CARTRIDGE_USED), "CARTRIDGE_USED");
    ASSERT_STREQ(cloudStatusToString(CloudStatus::ERR_UPLOAD_FAILED), "UPLOAD_FAILED");
    ASSERT_STREQ(cloudStatusToString(CloudStatus::ERR_SERVER_ERROR), "SERVER_ERROR");
}

TEST(cloud_serialize_roundtrip_validate) {
    // Serialize a request, then deserialize a matching response
    ValidateCartridgeRequest req;
    strcpy(req.requestId, "roundtrip-001");
    strcpy(req.deviceId, "dev-test");
    req.timestamp = 1706500000;
    strcpy(req.cartridgeUuid, "550e8400-e29b-41d4-a716-446655440000");

    char reqBuffer[CLOUD_MAX_REQUEST_SIZE];
    serializeValidateRequest(&req, reqBuffer, sizeof(reqBuffer));

    // Verify the serialized JSON contains expected fields
    ASSERT_CONTAINS(reqBuffer, "\"cartridgeUuid\":\"550e8400-e29b-41d4-a716-446655440000\"");

    // Simulate server response
    const char* responseJson = "{"
        "\"requestId\":\"roundtrip-001\","
        "\"status\":0,"
        "\"isValid\":true,"
        "\"cartridgeUuid\":\"550e8400-e29b-41d4-a716-446655440000\","
        "\"assayId\":\"TEST0001\""
        "}";

    ValidateCartridgeResponse resp;
    ASSERT_TRUE(deserializeValidateResponse(responseJson, &resp));
    ASSERT_STREQ(resp.requestId, req.requestId);
    ASSERT_TRUE(resp.isValid);
    ASSERT_STREQ(resp.cartridgeUuid, req.cartridgeUuid);
}

//==============================================================================
// MAIN
//==============================================================================

int main(int argc, char** argv) {
    (void)argc; (void)argv;

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

    printf("\nBCODE Interpreter Tests:\n");
    RUN_TEST(bcode_load_valid);
    RUN_TEST(bcode_load_null);
    RUN_TEST(bcode_load_empty);
    RUN_TEST(bcode_simple_start_end);
    RUN_TEST(bcode_parse_move);
    RUN_TEST(bcode_parse_sensor_params);
    RUN_TEST(bcode_baseline_and_test_scans);
    RUN_TEST(bcode_repeat_block);
    RUN_TEST(bcode_nested_repeat);
    RUN_TEST(bcode_missing_params_error);
    RUN_TEST(bcode_cancel_during_delay);
    RUN_TEST(bcode_pause_resume);
    RUN_TEST(bcode_opcode_to_string);
    RUN_TEST(bcode_interpreter_state_to_string);

    printf("\nCloud Protocol Tests:\n");
    RUN_TEST(cloud_serialize_validate_request);
    RUN_TEST(cloud_serialize_upload_request);
    RUN_TEST(cloud_deserialize_validate_response);
    RUN_TEST(cloud_deserialize_validate_response_error);
    RUN_TEST(cloud_deserialize_load_assay_response);
    RUN_TEST(cloud_deserialize_upload_response);
    RUN_TEST(cloud_deserialize_reset_response);
    RUN_TEST(cloud_deserialize_null_handling);
    RUN_TEST(cloud_generate_request_id_format);
    RUN_TEST(cloud_status_to_string);
    RUN_TEST(cloud_serialize_roundtrip_validate);

    printf("\n========================================\n");
    printf("Results: %d/%d passed", tests_passed, tests_run);
    if (tests_failed > 0) {
        printf(" (%d FAILED)", tests_failed);
    }
    printf("\n========================================\n\n");

    return tests_failed > 0 ? 1 : 0;
}
