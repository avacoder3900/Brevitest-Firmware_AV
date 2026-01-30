/**
 * @file test_state_machine.cpp
 * @brief Unit tests for State Machine module
 * @author Agent GAMMA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file contains comprehensive unit tests for the StateMachine module.
 * Tests cover all user stories:
 *   - SM-001: DeviceState class
 *   - SM-002: State transition validation
 *   - SM-003: Transition history tracking
 *   - SM-004: setMode() with validation
 *   - SM-005: Test state management
 *   - SM-006: Cartridge state management
 *   - SM-007: State query functions
 *
 * Test Categories:
 *   - Initialization tests
 *   - Valid transition tests (all valid paths)
 *   - Invalid transition tests (rejection)
 *   - History tracking tests
 *   - Cartridge state integration
 *   - Test state integration
 *   - Force transition functionality
 *   - Error handling
 *   - Observer pattern
 *
 * Usage:
 *   Run via Particle device test framework or compile with mock hardware
 *   includes for PC-based testing.
 */

#include "Particle.h"
#include "StateMachine.h"
#include "DataTypes.h"

// ============================================================================
// TEST FRAMEWORK MACROS
// ============================================================================
// Simple test framework that works on Particle devices

static int _testsPassed = 0;
static int _testsFailed = 0;

#define TEST_ASSERT(condition, message) do { \
    if (condition) { \
        _testsPassed++; \
        Log.info("PASS: %s", message); \
    } else { \
        _testsFailed++; \
        Log.error("FAIL: %s (line %d)", message, __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_EQUAL(expected, actual, message) do { \
    if ((expected) == (actual)) { \
        _testsPassed++; \
        Log.info("PASS: %s", message); \
    } else { \
        _testsFailed++; \
        Log.error("FAIL: %s - expected %d, got %d (line %d)", message, (int)(expected), (int)(actual), __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_STR_EQUAL(expected, actual, message) do { \
    if (strcmp((expected), (actual)) == 0) { \
        _testsPassed++; \
        Log.info("PASS: %s", message); \
    } else { \
        _testsFailed++; \
        Log.error("FAIL: %s - expected '%s', got '%s' (line %d)", message, (expected), (actual), __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_NOT_NULL(ptr, message) do { \
    if ((ptr) != nullptr) { \
        _testsPassed++; \
        Log.info("PASS: %s", message); \
    } else { \
        _testsFailed++; \
        Log.error("FAIL: %s - pointer is NULL (line %d)", message, __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_NULL(ptr, message) do { \
    if ((ptr) == nullptr) { \
        _testsPassed++; \
        Log.info("PASS: %s", message); \
    } else { \
        _testsFailed++; \
        Log.error("FAIL: %s - pointer is not NULL (line %d)", message, __LINE__); \
    } \
} while(0)

// ============================================================================
// TEST HELPER: Reset state machine between tests
// ============================================================================

static StateMachine testMachine;

void resetTestMachine() {
    // Create fresh machine
    testMachine = StateMachine();
    testMachine.init();
    testMachine.clearHistory();
}

// ============================================================================
// INITIALIZATION TESTS (SM-001)
// ============================================================================

void test_initialization() {
    Log.info("=== Testing Initialization (SM-001) ===");

    StateMachine sm;

    // Before init
    TEST_ASSERT(!sm.isInitialized(), "Should not be initialized before init()");

    // Initialize
    bool initResult = sm.init();
    TEST_ASSERT(initResult, "init() should return true");
    TEST_ASSERT(sm.isInitialized(), "Should be initialized after init()");

    // Initial state should be INITIALIZING
    TEST_ASSERT_EQUAL((int)DeviceMode::INITIALIZING, (int)sm.getCurrentMode(),
                      "Initial mode should be INITIALIZING");

    // Test state should be NOT_STARTED
    TEST_ASSERT_EQUAL((int)TestState::NOT_STARTED, (int)sm.getTestState(),
                      "Initial test state should be NOT_STARTED");

    // Cartridge state should be NOT_INSERTED
    TEST_ASSERT_EQUAL((int)CartridgeState::NOT_INSERTED, (int)sm.getCartridgeState(),
                      "Initial cartridge state should be NOT_INSERTED");

    // Double init should be safe
    bool reinitResult = sm.init();
    TEST_ASSERT(reinitResult, "Re-init should return true");
}

void test_initial_state_queries() {
    Log.info("=== Testing Initial State Queries (SM-007) ===");

    resetTestMachine();

    TEST_ASSERT(!testMachine.isIdle(), "Should not be idle initially");
    TEST_ASSERT(testMachine.isInitializing(), "Should be initializing initially");
    TEST_ASSERT(!testMachine.isHeating(), "Should not be heating initially");
    TEST_ASSERT(!testMachine.isRunningTest(), "Should not be running test initially");
    TEST_ASSERT(!testMachine.isInErrorState(), "Should not be in error state initially");
    TEST_ASSERT(!testMachine.hasCartridge(), "Should not have cartridge initially");
    TEST_ASSERT(!testMachine.isCloudOperationInProgress(), "No cloud op initially");
}

// ============================================================================
// VALID TRANSITION TESTS (SM-002)
// ============================================================================

void test_valid_transitions_from_initializing() {
    Log.info("=== Testing Valid Transitions from INITIALIZING ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::INITIALIZING);

    // INITIALIZING -> IDLE (normal startup)
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::IDLE),
                "INITIALIZING -> IDLE should be valid");

    // INITIALIZING -> HEATING (temp not ready)
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::HEATING),
                "INITIALIZING -> HEATING should be valid");

    // INITIALIZING -> ERROR (always allowed)
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::ERROR_STATE),
                "INITIALIZING -> ERROR_STATE should be valid");
}

void test_valid_transitions_from_idle() {
    Log.info("=== Testing Valid Transitions from IDLE ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::IDLE);

    // IDLE -> BARCODE_SCANNING
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::BARCODE_SCANNING),
                "IDLE -> BARCODE_SCANNING should be valid");

    // IDLE -> STRESS_TESTING
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::STRESS_TESTING),
                "IDLE -> STRESS_TESTING should be valid");

    // IDLE -> HEATING
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::HEATING),
                "IDLE -> HEATING should be valid");

    // IDLE -> UPLOADING_RESULTS (cached results)
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::UPLOADING_RESULTS),
                "IDLE -> UPLOADING_RESULTS should be valid");
}

void test_valid_transitions_from_heating() {
    Log.info("=== Testing Valid Transitions from HEATING ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::HEATING);

    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::IDLE),
                "HEATING -> IDLE should be valid");
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::BARCODE_SCANNING),
                "HEATING -> BARCODE_SCANNING should be valid");
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::STRESS_TESTING),
                "HEATING -> STRESS_TESTING should be valid");
}

void test_valid_transitions_from_barcode_scanning() {
    Log.info("=== Testing Valid Transitions from BARCODE_SCANNING ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::BARCODE_SCANNING);

    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::VALIDATING_CARTRIDGE),
                "BARCODE_SCANNING -> VALIDATING_CARTRIDGE should be valid");
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::VALIDATING_MAGNETOMETER),
                "BARCODE_SCANNING -> VALIDATING_MAGNETOMETER should be valid");
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::STRESS_TESTING),
                "BARCODE_SCANNING -> STRESS_TESTING should be valid");
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::IDLE),
                "BARCODE_SCANNING -> IDLE should be valid");
}

void test_valid_transitions_from_validating_cartridge() {
    Log.info("=== Testing Valid Transitions from VALIDATING_CARTRIDGE ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::VALIDATING_CARTRIDGE);

    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::RUNNING_TEST),
                "VALIDATING_CARTRIDGE -> RUNNING_TEST should be valid");
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::RESETTING_CARTRIDGE),
                "VALIDATING_CARTRIDGE -> RESETTING_CARTRIDGE should be valid");
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::IDLE),
                "VALIDATING_CARTRIDGE -> IDLE should be valid");
}

void test_valid_transitions_from_running_test() {
    Log.info("=== Testing Valid Transitions from RUNNING_TEST ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::RUNNING_TEST);

    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::UPLOADING_RESULTS),
                "RUNNING_TEST -> UPLOADING_RESULTS should be valid");
    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::IDLE),
                "RUNNING_TEST -> IDLE should be valid");
}

void test_valid_transitions_from_uploading_results() {
    Log.info("=== Testing Valid Transitions from UPLOADING_RESULTS ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::UPLOADING_RESULTS);

    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::IDLE),
                "UPLOADING_RESULTS -> IDLE should be valid");
}

void test_valid_transitions_from_error_state() {
    Log.info("=== Testing Valid Transitions from ERROR_STATE ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::ERROR_STATE);

    TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::IDLE),
                "ERROR_STATE -> IDLE should be valid");
}

void test_always_allowed_transitions() {
    Log.info("=== Testing Always-Allowed Transitions ===");

    // ERROR_STATE should be reachable from any state
    DeviceMode modes[] = {
        DeviceMode::IDLE,
        DeviceMode::HEATING,
        DeviceMode::BARCODE_SCANNING,
        DeviceMode::VALIDATING_CARTRIDGE,
        DeviceMode::RUNNING_TEST,
        DeviceMode::UPLOADING_RESULTS,
        DeviceMode::STRESS_TESTING
    };

    for (int i = 0; i < 7; i++) {
        resetTestMachine();
        testMachine.forceMode(modes[i]);
        TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::ERROR_STATE),
                    "Any state -> ERROR_STATE should be valid");
    }

    // IDLE should be reachable from any state (reset mechanism)
    for (int i = 0; i < 7; i++) {
        resetTestMachine();
        testMachine.forceMode(modes[i]);
        TEST_ASSERT(testMachine.canTransitionTo(DeviceMode::IDLE),
                    "Any state -> IDLE should be valid");
    }
}

// ============================================================================
// INVALID TRANSITION TESTS (SM-002)
// ============================================================================

void test_invalid_transitions() {
    Log.info("=== Testing Invalid Transitions ===");

    // IDLE -> RUNNING_TEST (skips validation)
    resetTestMachine();
    testMachine.forceMode(DeviceMode::IDLE);
    TEST_ASSERT(!testMachine.canTransitionTo(DeviceMode::RUNNING_TEST),
                "IDLE -> RUNNING_TEST should be invalid");

    // IDLE -> VALIDATING_CARTRIDGE (skips barcode scan)
    TEST_ASSERT(!testMachine.canTransitionTo(DeviceMode::VALIDATING_CARTRIDGE),
                "IDLE -> VALIDATING_CARTRIDGE should be invalid");

    // HEATING -> RUNNING_TEST (skips validation)
    resetTestMachine();
    testMachine.forceMode(DeviceMode::HEATING);
    TEST_ASSERT(!testMachine.canTransitionTo(DeviceMode::RUNNING_TEST),
                "HEATING -> RUNNING_TEST should be invalid");

    // BARCODE_SCANNING -> RUNNING_TEST (skips validation)
    resetTestMachine();
    testMachine.forceMode(DeviceMode::BARCODE_SCANNING);
    TEST_ASSERT(!testMachine.canTransitionTo(DeviceMode::RUNNING_TEST),
                "BARCODE_SCANNING -> RUNNING_TEST should be invalid");

    // UPLOADING_RESULTS -> RUNNING_TEST
    resetTestMachine();
    testMachine.forceMode(DeviceMode::UPLOADING_RESULTS);
    TEST_ASSERT(!testMachine.canTransitionTo(DeviceMode::RUNNING_TEST),
                "UPLOADING_RESULTS -> RUNNING_TEST should be invalid");

    // Same state transition is invalid
    resetTestMachine();
    testMachine.forceMode(DeviceMode::IDLE);
    TEST_ASSERT(!testMachine.canTransitionTo(DeviceMode::IDLE),
                "IDLE -> IDLE should be invalid (same state)");
}

void test_setMode_validation() {
    Log.info("=== Testing setMode() Validation (SM-004) ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::IDLE);

    // Valid transition should succeed
    ErrorCode result = testMachine.setMode(DeviceMode::BARCODE_SCANNING);
    TEST_ASSERT_EQUAL((int)ErrorCode::SUCCESS, (int)result,
                      "Valid setMode() should return SUCCESS");
    TEST_ASSERT_EQUAL((int)DeviceMode::BARCODE_SCANNING, (int)testMachine.getCurrentMode(),
                      "Mode should be BARCODE_SCANNING after valid transition");

    // Invalid transition should fail
    result = testMachine.setMode(DeviceMode::UPLOADING_RESULTS);
    TEST_ASSERT_EQUAL((int)ErrorCode::ERR_INVALID_STATE, (int)result,
                      "Invalid setMode() should return ERR_INVALID_STATE");
    TEST_ASSERT_EQUAL((int)DeviceMode::BARCODE_SCANNING, (int)testMachine.getCurrentMode(),
                      "Mode should remain BARCODE_SCANNING after invalid transition");
}

// ============================================================================
// HISTORY TRACKING TESTS (SM-003)
// ============================================================================

void test_history_tracking() {
    Log.info("=== Testing History Tracking (SM-003) ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::IDLE);

    // Initially no history (after clear)
    TEST_ASSERT_EQUAL(0, testMachine.getTransitionCount(),
                      "History should be empty after clear");

    // Perform some transitions
    testMachine.setMode(DeviceMode::BARCODE_SCANNING);
    TEST_ASSERT_EQUAL(1, testMachine.getTransitionCount(),
                      "History count should be 1 after first transition");

    testMachine.setMode(DeviceMode::VALIDATING_CARTRIDGE);
    TEST_ASSERT_EQUAL(2, testMachine.getTransitionCount(),
                      "History count should be 2 after second transition");

    // Check most recent transition
    const StateTransitionEntry* entry = testMachine.getTransition(0);
    TEST_ASSERT_NOT_NULL(entry, "getTransition(0) should return non-null");
    TEST_ASSERT_EQUAL((int)DeviceMode::BARCODE_SCANNING, (int)entry->from_mode,
                      "Most recent from_mode should be BARCODE_SCANNING");
    TEST_ASSERT_EQUAL((int)DeviceMode::VALIDATING_CARTRIDGE, (int)entry->to_mode,
                      "Most recent to_mode should be VALIDATING_CARTRIDGE");

    // Check second most recent
    entry = testMachine.getTransition(1);
    TEST_ASSERT_NOT_NULL(entry, "getTransition(1) should return non-null");
    TEST_ASSERT_EQUAL((int)DeviceMode::IDLE, (int)entry->from_mode,
                      "Second entry from_mode should be IDLE");
}

void test_history_circular_buffer() {
    Log.info("=== Testing History Circular Buffer ===");

    resetTestMachine();

    // Fill the buffer beyond capacity
    for (int i = 0; i < 60; i++) {
        // Alternate between two states to generate transitions
        if (i % 2 == 0) {
            testMachine.forceMode(DeviceMode::IDLE);
        } else {
            testMachine.forceMode(DeviceMode::HEATING);
        }
    }

    // Count should be capped at buffer size
    TEST_ASSERT_EQUAL(TRANSITION_HISTORY_SIZE, testMachine.getTransitionCount(),
                      "History count should be capped at buffer size");

    // Should still be able to get entries
    const StateTransitionEntry* entry = testMachine.getTransition(0);
    TEST_ASSERT_NOT_NULL(entry, "Should be able to get most recent entry");

    // Out of bounds should return null
    entry = testMachine.getTransition(TRANSITION_HISTORY_SIZE);
    TEST_ASSERT_NULL(entry, "Out of bounds index should return null");
}

void test_history_cartridge_filtering() {
    Log.info("=== Testing History Cartridge Filtering ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::IDLE);

    // Set cartridge ID and do transitions
    testMachine.setCurrentCartridgeId("test-cartridge-001");
    testMachine.setMode(DeviceMode::BARCODE_SCANNING);
    testMachine.setMode(DeviceMode::VALIDATING_CARTRIDGE);

    // Change cartridge and do more transitions
    testMachine.setCurrentCartridgeId("test-cartridge-002");
    testMachine.setMode(DeviceMode::RUNNING_TEST);
    testMachine.setMode(DeviceMode::UPLOADING_RESULTS);

    // Filter by first cartridge
    StateTransitionEntry entries[10];
    int count = testMachine.getHistoryByCartridge("test-cartridge-001", entries, 10);
    TEST_ASSERT_EQUAL(2, count, "Should find 2 transitions for first cartridge");

    // Filter by second cartridge
    count = testMachine.getHistoryByCartridge("test-cartridge-002", entries, 10);
    TEST_ASSERT_EQUAL(2, count, "Should find 2 transitions for second cartridge");

    // Filter by non-existent cartridge
    count = testMachine.getHistoryByCartridge("non-existent", entries, 10);
    TEST_ASSERT_EQUAL(0, count, "Should find 0 transitions for non-existent cartridge");
}

void test_history_clear() {
    Log.info("=== Testing History Clear ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::IDLE);
    testMachine.setMode(DeviceMode::HEATING);
    testMachine.setMode(DeviceMode::IDLE);

    TEST_ASSERT(testMachine.getTransitionCount() > 0,
                "Should have history before clear");

    testMachine.clearHistory();

    TEST_ASSERT_EQUAL(0, testMachine.getTransitionCount(),
                      "History should be empty after clear");
}

// ============================================================================
// TEST STATE MANAGEMENT (SM-005)
// ============================================================================

void test_test_state_transitions() {
    Log.info("=== Testing Test State Transitions (SM-005) ===");

    resetTestMachine();

    // Initial state
    TEST_ASSERT_EQUAL((int)TestState::NOT_STARTED, (int)testMachine.getTestState(),
                      "Initial test state should be NOT_STARTED");
    TEST_ASSERT(testMachine.canStartNewTest(), "Should be able to start new test");
    TEST_ASSERT(!testMachine.isTestInProgress(), "Test should not be in progress");

    // NOT_STARTED -> RUNNING
    ErrorCode result = testMachine.setTestState(TestState::RUNNING);
    TEST_ASSERT_EQUAL((int)ErrorCode::SUCCESS, (int)result,
                      "NOT_STARTED -> RUNNING should succeed");
    TEST_ASSERT(testMachine.isTestInProgress(), "Test should be in progress");

    // RUNNING -> COMPLETED
    result = testMachine.setTestState(TestState::COMPLETED);
    TEST_ASSERT_EQUAL((int)ErrorCode::SUCCESS, (int)result,
                      "RUNNING -> COMPLETED should succeed");
    TEST_ASSERT(!testMachine.isTestInProgress(), "Test should not be in progress");

    // COMPLETED -> UPLOAD_PENDING
    result = testMachine.setTestState(TestState::UPLOAD_PENDING);
    TEST_ASSERT_EQUAL((int)ErrorCode::SUCCESS, (int)result,
                      "COMPLETED -> UPLOAD_PENDING should succeed");
    TEST_ASSERT(testMachine.hasResultsPendingUpload(), "Should have results pending");

    // UPLOAD_PENDING -> UPLOAD_IN_PROGRESS
    result = testMachine.setTestState(TestState::UPLOAD_IN_PROGRESS);
    TEST_ASSERT_EQUAL((int)ErrorCode::SUCCESS, (int)result,
                      "UPLOAD_PENDING -> UPLOAD_IN_PROGRESS should succeed");

    // UPLOAD_IN_PROGRESS -> UPLOADED
    result = testMachine.setTestState(TestState::UPLOADED);
    TEST_ASSERT_EQUAL((int)ErrorCode::SUCCESS, (int)result,
                      "UPLOAD_IN_PROGRESS -> UPLOADED should succeed");
    TEST_ASSERT(testMachine.canStartNewTest(), "Should be able to start new test");
}

void test_test_state_cancellation() {
    Log.info("=== Testing Test State Cancellation ===");

    resetTestMachine();

    // Start a test
    testMachine.setTestState(TestState::RUNNING);

    // RUNNING -> CANCELLED
    ErrorCode result = testMachine.setTestState(TestState::CANCELLED);
    TEST_ASSERT_EQUAL((int)ErrorCode::SUCCESS, (int)result,
                      "RUNNING -> CANCELLED should succeed");
    TEST_ASSERT_EQUAL((int)TestState::CANCELLED, (int)testMachine.getTestState(),
                      "Test state should be CANCELLED");

    // CANCELLED -> NOT_STARTED (reset)
    result = testMachine.setTestState(TestState::NOT_STARTED);
    TEST_ASSERT_EQUAL((int)ErrorCode::SUCCESS, (int)result,
                      "CANCELLED -> NOT_STARTED should succeed");
}

void test_test_state_invalid_transitions() {
    Log.info("=== Testing Test State Invalid Transitions ===");

    resetTestMachine();

    // NOT_STARTED -> COMPLETED (skips RUNNING)
    ErrorCode result = testMachine.setTestState(TestState::COMPLETED);
    TEST_ASSERT_EQUAL((int)ErrorCode::ERR_INVALID_STATE, (int)result,
                      "NOT_STARTED -> COMPLETED should fail");

    // NOT_STARTED -> UPLOADED (skips everything)
    result = testMachine.setTestState(TestState::UPLOADED);
    TEST_ASSERT_EQUAL((int)ErrorCode::ERR_INVALID_STATE, (int)result,
                      "NOT_STARTED -> UPLOADED should fail");
}

// ============================================================================
// CARTRIDGE STATE MANAGEMENT (SM-006)
// ============================================================================

void test_cartridge_state_management() {
    Log.info("=== Testing Cartridge State Management (SM-006) ===");

    resetTestMachine();

    // Initial state
    TEST_ASSERT(!testMachine.hasCartridge(), "Should not have cartridge initially");
    TEST_ASSERT_STR_EQUAL("", testMachine.getCurrentCartridgeId(),
                          "Cartridge ID should be empty initially");

    // Simulate cartridge insertion
    testMachine.onCartridgeInserted();
    TEST_ASSERT(testMachine.hasCartridge(), "Should have cartridge after insertion");
    TEST_ASSERT_EQUAL((int)CartridgeState::DETECTED, (int)testMachine.getCartridgeState(),
                      "Cartridge state should be DETECTED");

    // Set cartridge ID
    testMachine.setCurrentCartridgeId("ABC-123-XYZ");
    TEST_ASSERT_STR_EQUAL("ABC-123-XYZ", testMachine.getCurrentCartridgeId(),
                          "Cartridge ID should be set");

    // Validate cartridge
    testMachine.setCartridgeState(CartridgeState::VALIDATED);
    TEST_ASSERT(testMachine.isCartridgeValidated(), "Cartridge should be validated");

    // Simulate cartridge removal
    testMachine.onCartridgeRemoved();
    TEST_ASSERT(!testMachine.hasCartridge(), "Should not have cartridge after removal");
    TEST_ASSERT_STR_EQUAL("", testMachine.getCurrentCartridgeId(),
                          "Cartridge ID should be cleared after removal");
}

void test_cartridge_removal_cancels_test() {
    Log.info("=== Testing Cartridge Removal Cancels Test ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::RUNNING_TEST);
    testMachine.setTestState(TestState::RUNNING);
    testMachine.onCartridgeInserted();

    // Remove cartridge while test is running
    testMachine.onCartridgeRemoved();

    TEST_ASSERT_EQUAL((int)TestState::CANCELLED, (int)testMachine.getTestState(),
                      "Test should be CANCELLED after cartridge removal");
}

// ============================================================================
// FORCE TRANSITION TESTS
// ============================================================================

void test_force_transition() {
    Log.info("=== Testing Force Transition ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::IDLE);

    // Force an otherwise invalid transition
    ErrorCode result = testMachine.forceMode(DeviceMode::RUNNING_TEST);
    TEST_ASSERT_EQUAL((int)ErrorCode::SUCCESS, (int)result,
                      "forceMode() should always succeed");
    TEST_ASSERT_EQUAL((int)DeviceMode::RUNNING_TEST, (int)testMachine.getCurrentMode(),
                      "Mode should be RUNNING_TEST after force");

    // Previous mode should be tracked
    TEST_ASSERT_EQUAL((int)DeviceMode::IDLE, (int)testMachine.getPreviousMode(),
                      "Previous mode should be IDLE");
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

void test_error_handling() {
    Log.info("=== Testing Error Handling ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::IDLE);

    // Set error
    testMachine.setError(ErrorCode::ERR_HEATER_FAULT, "Heater temperature fault");

    TEST_ASSERT(testMachine.isInErrorState(), "Should be in error state");
    TEST_ASSERT_EQUAL((int)ErrorCode::ERR_HEATER_FAULT, (int)testMachine.getLastErrorCode(),
                      "Error code should be ERR_HEATER_FAULT");
    TEST_ASSERT_STR_EQUAL("Heater temperature fault", testMachine.getLastErrorMessage(),
                          "Error message should be set");

    // Clear error
    testMachine.clearError();

    TEST_ASSERT(!testMachine.isInErrorState(), "Should not be in error state");
    TEST_ASSERT(testMachine.isIdle(), "Should be IDLE after clearing error");
    TEST_ASSERT_EQUAL((int)ErrorCode::SUCCESS, (int)testMachine.getLastErrorCode(),
                      "Error code should be SUCCESS after clear");
}

// ============================================================================
// CLOUD OPERATION TRACKING TESTS
// ============================================================================

void test_cloud_operation_tracking() {
    Log.info("=== Testing Cloud Operation Tracking ===");

    resetTestMachine();

    // Initially no operation
    TEST_ASSERT(!testMachine.isCloudOperationPending(), "No cloud op initially");
    TEST_ASSERT(!testMachine.isCloudOperationTimeout(), "Should not timeout without op");

    // Start operation
    testMachine.startCloudOperation();
    TEST_ASSERT(testMachine.isCloudOperationPending(), "Cloud op should be pending");

    // Elapsed should be tracked
    delay(100);
    TEST_ASSERT(testMachine.getCloudOperationElapsed() >= 100,
                "Elapsed time should be >= 100ms");

    // End operation
    testMachine.endCloudOperation();
    TEST_ASSERT(!testMachine.isCloudOperationPending(), "Cloud op should not be pending");
    TEST_ASSERT_EQUAL(0, (int)testMachine.getCloudOperationElapsed(),
                      "Elapsed should be 0 when no operation");
}

// ============================================================================
// STATE QUERY TESTS (SM-007)
// ============================================================================

void test_state_query_functions() {
    Log.info("=== Testing State Query Functions (SM-007) ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::IDLE);

    // Test getCurrentModeString
    TEST_ASSERT_STR_EQUAL("IDLE", testMachine.getCurrentModeString(),
                          "Mode string should be IDLE");

    // Test JSON output
    char buffer[512];
    size_t len = testMachine.getStateAsJson(buffer, sizeof(buffer));
    TEST_ASSERT(len > 0, "getStateAsJson should return non-zero length");
    TEST_ASSERT(strstr(buffer, "\"mode\":\"IDLE\"") != nullptr,
                "JSON should contain mode");

    // Test canAcceptCartridge
    TEST_ASSERT(testMachine.canAcceptCartridge(), "Should accept cartridge when IDLE");
    testMachine.onCartridgeInserted();
    TEST_ASSERT(!testMachine.canAcceptCartridge(), "Should not accept when cartridge present");
}

void test_reset_to_idle() {
    Log.info("=== Testing Reset to IDLE ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::RUNNING_TEST);
    testMachine.setTestState(TestState::RUNNING);
    testMachine.setCartridgeState(CartridgeState::VALIDATED);
    testMachine.setCurrentCartridgeId("test-cartridge");
    testMachine.startCloudOperation();
    testMachine.setError(ErrorCode::ERR_TEST_FAILED, "Test error");

    // Reset
    testMachine.resetToIdle();

    // Verify all state is reset
    TEST_ASSERT(testMachine.isIdle(), "Should be IDLE after reset");
    TEST_ASSERT_EQUAL((int)TestState::NOT_STARTED, (int)testMachine.getTestState(),
                      "Test state should be NOT_STARTED");
    TEST_ASSERT_EQUAL((int)CartridgeState::NOT_INSERTED, (int)testMachine.getCartridgeState(),
                      "Cartridge state should be NOT_INSERTED");
    TEST_ASSERT_STR_EQUAL("", testMachine.getCurrentCartridgeId(),
                          "Cartridge ID should be empty");
    TEST_ASSERT(!testMachine.isCloudOperationPending(), "Cloud op should be cleared");
    TEST_ASSERT_EQUAL((int)ErrorCode::SUCCESS, (int)testMachine.getLastErrorCode(),
                      "Error should be cleared");
}

// ============================================================================
// OBSERVER PATTERN TESTS
// ============================================================================

static int observerCallCount = 0;
static DeviceMode lastObservedFrom = DeviceMode::INITIALIZING;
static DeviceMode lastObservedTo = DeviceMode::INITIALIZING;

void testObserverCallback(DeviceMode from, DeviceMode to) {
    observerCallCount++;
    lastObservedFrom = from;
    lastObservedTo = to;
}

void test_observer_pattern() {
    Log.info("=== Testing Observer Pattern ===");

    resetTestMachine();
    testMachine.forceMode(DeviceMode::IDLE);
    observerCallCount = 0;

    // Register callback
    bool registered = testMachine.registerStateChangeCallback(testObserverCallback);
    TEST_ASSERT(registered, "Callback should be registered");

    // Trigger transition
    testMachine.setMode(DeviceMode::HEATING);

    TEST_ASSERT_EQUAL(1, observerCallCount, "Observer should be called once");
    TEST_ASSERT_EQUAL((int)DeviceMode::IDLE, (int)lastObservedFrom,
                      "Observer should see from=IDLE");
    TEST_ASSERT_EQUAL((int)DeviceMode::HEATING, (int)lastObservedTo,
                      "Observer should see to=HEATING");

    // Unregister
    bool unregistered = testMachine.unregisterStateChangeCallback(testObserverCallback);
    TEST_ASSERT(unregistered, "Callback should be unregistered");

    // Trigger another transition
    testMachine.setMode(DeviceMode::IDLE);
    TEST_ASSERT_EQUAL(1, observerCallCount, "Observer should not be called after unregister");
}

// ============================================================================
// STATIC VALIDATION FUNCTION TEST
// ============================================================================

void test_static_validation() {
    Log.info("=== Testing Static isValidTransition() ===");

    // Test the static validation method
    TEST_ASSERT(StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::BARCODE_SCANNING),
                "Static: IDLE -> BARCODE_SCANNING should be valid");
    TEST_ASSERT(!StateMachine::isValidTransition(DeviceMode::IDLE, DeviceMode::RUNNING_TEST),
                "Static: IDLE -> RUNNING_TEST should be invalid");
    TEST_ASSERT(StateMachine::isValidTransition(DeviceMode::RUNNING_TEST, DeviceMode::ERROR_STATE),
                "Static: Any -> ERROR_STATE should be valid");
}

// ============================================================================
// TEST RUNNER
// ============================================================================

/**
 * @brief Run all state machine unit tests
 * @return Number of failed tests
 */
int runStateMachineTests() {
    _testsPassed = 0;
    _testsFailed = 0;

    Log.info("========================================");
    Log.info("    State Machine Unit Tests Starting");
    Log.info("========================================");

    // Initialization tests (SM-001)
    test_initialization();
    test_initial_state_queries();

    // Valid transition tests (SM-002)
    test_valid_transitions_from_initializing();
    test_valid_transitions_from_idle();
    test_valid_transitions_from_heating();
    test_valid_transitions_from_barcode_scanning();
    test_valid_transitions_from_validating_cartridge();
    test_valid_transitions_from_running_test();
    test_valid_transitions_from_uploading_results();
    test_valid_transitions_from_error_state();
    test_always_allowed_transitions();

    // Invalid transition tests (SM-002)
    test_invalid_transitions();
    test_setMode_validation();

    // History tracking tests (SM-003)
    test_history_tracking();
    test_history_circular_buffer();
    test_history_cartridge_filtering();
    test_history_clear();

    // Test state management (SM-005)
    test_test_state_transitions();
    test_test_state_cancellation();
    test_test_state_invalid_transitions();

    // Cartridge state management (SM-006)
    test_cartridge_state_management();
    test_cartridge_removal_cancels_test();

    // Force transition
    test_force_transition();

    // Error handling
    test_error_handling();

    // Cloud operation tracking
    test_cloud_operation_tracking();

    // State query functions (SM-007)
    test_state_query_functions();
    test_reset_to_idle();

    // Observer pattern
    test_observer_pattern();

    // Static validation
    test_static_validation();

    // Summary
    Log.info("========================================");
    Log.info("    State Machine Unit Tests Complete");
    Log.info("    Passed: %d", _testsPassed);
    Log.info("    Failed: %d", _testsFailed);
    Log.info("========================================");

    return _testsFailed;
}

// ============================================================================
// STANDALONE TEST ENTRY POINT
// ============================================================================
// Uncomment the following to run tests standalone on device

/*
void setup() {
    Serial.begin(115200);
    waitFor(Serial.isConnected, 10000);
    delay(1000);

    int failures = runStateMachineTests();

    if (failures == 0) {
        Log.info("ALL STATE MACHINE TESTS PASSED!");
    } else {
        Log.error("STATE MACHINE TESTS FAILED: %d failures", failures);
    }
}

void loop() {
    // Do nothing after tests complete
    delay(10000);
}
*/
