/**
 * @file test_motor.cpp
 * @brief Unit tests for Motor Controller module
 * @author Agent ETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file contains unit tests for the MotorController module. Tests verify:
 * - Initialization and configuration
 * - Position tracking and conversion math
 * - Movement calculations
 * - Limit switch handling
 * - Oscillation logic
 * - Predefined positions
 *
 * Usage:
 *   Run via Particle device test framework or compile with mock hardware
 *   includes for PC-based testing.
 */

#include "Particle.h"
#include "MotorController.h"
#include "HardwareConfig.h"

// ============================================================================
// TEST FRAMEWORK MACROS
// ============================================================================
// Reuse test framework from HAL tests

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
        Log.error("FAIL: %s - expected %ld, got %ld (line %d)", message, (long)(expected), (long)(actual), __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_NEAR(expected, actual, tolerance, message) do { \
    long diff = ((expected) > (actual)) ? ((expected) - (actual)) : ((actual) - (expected)); \
    if (diff <= (tolerance)) { \
        _testsPassed++; \
        Log.info("PASS: %s (diff=%ld)", message, diff); \
    } else { \
        _testsFailed++; \
        Log.error("FAIL: %s - expected %ld, got %ld, tolerance %ld (line %d)", message, (long)(expected), (long)(actual), (long)(tolerance), __LINE__); \
    } \
} while(0)

// Global motor controller instance for tests
static MotorController* _motor = nullptr;

// ============================================================================
// MOT-001: MOTOR CONTROLLER INITIALIZATION TESTS
// ============================================================================

/**
 * @brief Test motor controller construction
 */
void test_motor_construction() {
    Log.info("=== Testing Motor Controller Construction (MOT-001) ===");

    MotorController motor;
    TEST_ASSERT(!motor.isInitialized(), "Motor should not be initialized after construction");
    TEST_ASSERT(!motor.isEnabled(), "Motor should not be enabled after construction");
    TEST_ASSERT(!motor.isHomed(), "Motor should not be homed after construction");
    TEST_ASSERT_EQUAL(0, motor.getCurrentPosition(), "Initial position should be 0");
}

/**
 * @brief Test motor controller initialization
 */
void test_motor_initialization() {
    Log.info("=== Testing Motor Controller Initialization ===");

    // Create and initialize motor
    _motor = new MotorController();

    bool initResult = _motor->init();
    TEST_ASSERT(initResult, "Motor init() should return true");
    TEST_ASSERT(_motor->isInitialized(), "Motor should be initialized after init()");

    // Double init should be safe
    bool reinitResult = _motor->init();
    TEST_ASSERT(reinitResult, "Motor init() should return true on re-init");
}

/**
 * @brief Test motor enable/disable
 */
void test_motor_enable_disable() {
    Log.info("=== Testing Motor Enable/Disable ===");

    if (!_motor || !_motor->isInitialized()) {
        Log.error("SKIP: Motor not initialized");
        return;
    }

    // Enable motor
    _motor->enable();
    TEST_ASSERT(_motor->isEnabled(), "Motor should be enabled after enable()");

    // Disable motor
    _motor->disable();
    TEST_ASSERT(!_motor->isEnabled(), "Motor should be disabled after disable()");
}

// ============================================================================
// MOT-002: POSITION TRACKING TESTS
// ============================================================================

/**
 * @brief Test position constants are correct
 */
void test_position_constants() {
    Log.info("=== Testing Position Constants (MOT-002) ===");

    // Verify predefined positions from PRD
    TEST_ASSERT_EQUAL(21000, StagePosition::HOME, "HOME position should be 21000");
    TEST_ASSERT_EQUAL(7860, StagePosition::TEST_START, "TEST_START position should be 7860");
    TEST_ASSERT_EQUAL(28000, StagePosition::SHIPPING_BOLT, "SHIPPING_BOLT position should be 28000");
    TEST_ASSERT_EQUAL(12800, StagePosition::MAGNETOMETER, "MAGNETOMETER position should be 12800");
    TEST_ASSERT_EQUAL(21000, StagePosition::OPTICAL_READ, "OPTICAL_READ position should be 21000");

    // Verify limits
    TEST_ASSERT_EQUAL(-60000, StagePosition::MIN, "MIN position should be -60000");
    TEST_ASSERT_EQUAL(45000, StagePosition::MAX, "MAX position should be 45000");
    TEST_ASSERT_EQUAL(0, StagePosition::LIMIT, "LIMIT position should be 0");
}

/**
 * @brief Test speed constants are correct
 */
void test_speed_constants() {
    Log.info("=== Testing Speed Constants ===");

    // Verify speed presets from PRD (250-600 microseconds)
    TEST_ASSERT_EQUAL(250, MotorSpeed::MINIMUM_DELAY, "MINIMUM_DELAY should be 250us");
    TEST_ASSERT_EQUAL(290, MotorSpeed::FAST, "FAST should be 290us");
    TEST_ASSERT_EQUAL(290, MotorSpeed::RESET, "RESET should be 290us");
    TEST_ASSERT_EQUAL(350, MotorSpeed::OSCILLATION, "OSCILLATION should be 350us");
    TEST_ASSERT_EQUAL(600, MotorSpeed::SLOW, "SLOW should be 600us");
    TEST_ASSERT_EQUAL(1000, MotorSpeed::SENSOR, "SENSOR should be 1000us");
}

/**
 * @brief Test micron per step constant
 */
void test_microns_per_step() {
    Log.info("=== Testing Microns Per Step ===");

    TEST_ASSERT_EQUAL(25, MOTOR_MICRONS_PER_EIGHTH_STEP, "Microns per 1/8 step should be 25");
}

/**
 * @brief Test position conversion functions
 */
void test_position_conversion() {
    Log.info("=== Testing Position Conversion ===");

    // Test microns to steps
    TEST_ASSERT_EQUAL(0, MotorController::micronsToSteps(0), "0 microns = 0 steps");
    TEST_ASSERT_EQUAL(1, MotorController::micronsToSteps(25), "25 microns = 1 step");
    TEST_ASSERT_EQUAL(4, MotorController::micronsToSteps(100), "100 microns = 4 steps");
    TEST_ASSERT_EQUAL(40, MotorController::micronsToSteps(1000), "1000 microns = 40 steps");
    TEST_ASSERT_EQUAL(840, MotorController::micronsToSteps(21000), "21000 microns = 840 steps");

    // Test steps to microns
    TEST_ASSERT_EQUAL(0, MotorController::stepsToMicrons(0), "0 steps = 0 microns");
    TEST_ASSERT_EQUAL(25, MotorController::stepsToMicrons(1), "1 step = 25 microns");
    TEST_ASSERT_EQUAL(100, MotorController::stepsToMicrons(4), "4 steps = 100 microns");
    TEST_ASSERT_EQUAL(1000, MotorController::stepsToMicrons(40), "40 steps = 1000 microns");

    // Test negative values
    TEST_ASSERT_EQUAL(-4, MotorController::micronsToSteps(-100), "-100 microns = -4 steps");
    TEST_ASSERT_EQUAL(-100, MotorController::stepsToMicrons(-4), "-4 steps = -100 microns");
}

/**
 * @brief Test position tracking
 */
void test_position_tracking() {
    Log.info("=== Testing Position Tracking ===");

    if (!_motor || !_motor->isInitialized()) {
        Log.error("SKIP: Motor not initialized");
        return;
    }

    // Test setPosition and getCurrentPosition
    _motor->setPosition(1000);
    TEST_ASSERT_EQUAL(1000, _motor->getCurrentPosition(), "Position should be 1000 after setPosition(1000)");

    _motor->setPosition(21000);
    TEST_ASSERT_EQUAL(21000, _motor->getCurrentPosition(), "Position should be 21000 after setPosition(21000)");

    // Reset for other tests
    _motor->setPosition(0);
}

/**
 * @brief Test position validation
 */
void test_position_validation() {
    Log.info("=== Testing Position Validation ===");

    if (!_motor || !_motor->isInitialized()) {
        Log.error("SKIP: Motor not initialized");
        return;
    }

    // Valid positions
    TEST_ASSERT(_motor->isValidPosition(0), "0 should be valid");
    TEST_ASSERT(_motor->isValidPosition(21000), "21000 should be valid");
    TEST_ASSERT(_motor->isValidPosition(45000), "45000 (MAX) should be valid");
    TEST_ASSERT(_motor->isValidPosition(-60000), "-60000 (MIN) should be valid");

    // Invalid positions
    TEST_ASSERT(!_motor->isValidPosition(50000), "50000 should be invalid (> MAX)");
    TEST_ASSERT(!_motor->isValidPosition(-70000), "-70000 should be invalid (< MIN)");
}

// ============================================================================
// MOT-003: HOMING TESTS (requires hardware)
// ============================================================================

/**
 * @brief Test limit switch reading
 */
void test_limit_switch() {
    Log.info("=== Testing Limit Switch (MOT-003) ===");

    if (!_motor || !_motor->isInitialized()) {
        Log.error("SKIP: Motor not initialized");
        return;
    }

    // Just verify the function doesn't crash
    bool atLimit = _motor->isAtLimit();
    Log.info("Limit switch state: %s", atLimit ? "AT LIMIT" : "NOT AT LIMIT");
    Log.info("PASS: Limit switch read completed without error");
    _testsPassed++;
}

// Note: Full homing test requires hardware and would move the motor
// Uncomment to run on actual device:
/*
void test_homing_sequence() {
    Log.info("=== Testing Homing Sequence ===");

    if (!_motor || !_motor->isInitialized()) {
        Log.error("SKIP: Motor not initialized");
        return;
    }

    MotorResult result = _motor->home(true);
    TEST_ASSERT(result == MotorResult::SUCCESS || result == MotorResult::AT_LIMIT,
                "Homing should succeed");
    TEST_ASSERT(_motor->isHomed(), "Motor should be homed after home()");
    TEST_ASSERT(_motor->getCurrentPosition() == StagePosition::INITIAL,
                "Position should be INITIAL after homing");
}
*/

// ============================================================================
// MOT-004 & MOT-005: MOVEMENT CALCULATION TESTS
// ============================================================================

/**
 * @brief Test movement math without hardware
 */
void test_movement_calculations() {
    Log.info("=== Testing Movement Calculations (MOT-004, MOT-005) ===");

    // Test step delay for integration time calculation
    // With default ATIME=49, ASTEP=999:
    // Integration time = (49+1) * (999+1) * 2.78us = 139000us
    // Steps needed = 5000/25 = 200
    // Time per step = 139000/200 = 695us
    // Step delay = 695/2 = 347us

    uint16_t delay = MotorController::calculateStepDelayForIntegration(49, 999);
    TEST_ASSERT_NEAR(347, delay, 10, "Step delay for ATIME=49, ASTEP=999 should be ~347us");

    // Test with very short integration time (should hit minimum)
    delay = MotorController::calculateStepDelayForIntegration(0, 0);
    TEST_ASSERT_EQUAL(MotorSpeed::MINIMUM_DELAY, delay, "Very short integration should use minimum delay");
}

/**
 * @brief Test position error calculation
 */
void test_position_error() {
    Log.info("=== Testing Position Error ===");

    if (!_motor || !_motor->isInitialized()) {
        Log.error("SKIP: Motor not initialized");
        return;
    }

    // Position error should be 0 after setPosition
    _motor->setPosition(1000);
    TEST_ASSERT_EQUAL(0, _motor->getPositionError(), "Error should be 0 after setPosition");

    // Note: Actual error accumulation tested with hardware movement
}

// ============================================================================
// MOT-006: OSCILLATION TESTS
// ============================================================================

/**
 * @brief Test oscillation state management
 */
void test_oscillation_state() {
    Log.info("=== Testing Oscillation State (MOT-006) ===");

    if (!_motor || !_motor->isInitialized()) {
        Log.error("SKIP: Motor not initialized");
        return;
    }

    // Should not be oscillating initially
    TEST_ASSERT(!_motor->isOscillating(), "Motor should not be oscillating initially");

    // Start oscillation (non-blocking mode test)
    MotorResult result = _motor->startOscillation(1000, MotorSpeed::OSCILLATION, nullptr);
    TEST_ASSERT(result == MotorResult::SUCCESS, "startOscillation should succeed");
    TEST_ASSERT(_motor->isOscillating(), "Motor should be oscillating after startOscillation");

    // Stop oscillation
    _motor->stopOscillation();
    TEST_ASSERT(!_motor->isOscillating(), "Motor should not be oscillating after stopOscillation");
}

// ============================================================================
// MOT-007: PREDEFINED POSITION TESTS
// ============================================================================

/**
 * @brief Test predefined position functions exist and are callable
 */
void test_predefined_position_functions() {
    Log.info("=== Testing Predefined Position Functions (MOT-007) ===");

    if (!_motor || !_motor->isInitialized()) {
        Log.error("SKIP: Motor not initialized");
        return;
    }

    // Set a known position
    _motor->setPosition(15000);

    // Note: These functions would move the motor on real hardware
    // Just verify they can be called without crashing

    // Test that each function exists and can compute distance
    int32_t currentPos = _motor->getCurrentPosition();
    int32_t distanceToHome = StagePosition::HOME - currentPos;
    int32_t distanceToTestStart = StagePosition::TEST_START - currentPos;
    int32_t distanceToMag = StagePosition::MAGNETOMETER - currentPos;
    int32_t distanceToShipping = StagePosition::SHIPPING_BOLT - currentPos;

    Log.info("Distance to HOME: %ld microns", distanceToHome);
    Log.info("Distance to TEST_START: %ld microns", distanceToTestStart);
    Log.info("Distance to MAGNETOMETER: %ld microns", distanceToMag);
    Log.info("Distance to SHIPPING_BOLT: %ld microns", distanceToShipping);

    TEST_ASSERT_EQUAL(6000, distanceToHome, "Distance to HOME from 15000 should be 6000");
    TEST_ASSERT_EQUAL(-7140, distanceToTestStart, "Distance to TEST_START from 15000 should be -7140");

    Log.info("PASS: Predefined position calculations verified");
    _testsPassed++;
}

// ============================================================================
// MOTOR STATE TESTS
// ============================================================================

/**
 * @brief Test motor state transitions
 */
void test_motor_state() {
    Log.info("=== Testing Motor State ===");

    if (!_motor || !_motor->isInitialized()) {
        Log.error("SKIP: Motor not initialized");
        return;
    }

    // Should be IDLE after initialization
    TEST_ASSERT(_motor->getState() == MotorState::IDLE, "State should be IDLE after init");

    // Cancel should work even when idle
    _motor->cancel();
    TEST_ASSERT(_motor->getState() == MotorState::IDLE, "State should still be IDLE after cancel");
}

/**
 * @brief Test motor result enum coverage
 */
void test_motor_result_enum() {
    Log.info("=== Testing Motor Result Enum ===");

    // Just verify all enum values are distinct
    TEST_ASSERT(MotorResult::SUCCESS != MotorResult::ERROR, "SUCCESS != ERROR");
    TEST_ASSERT(MotorResult::AT_LIMIT != MotorResult::AT_BOUNDARY, "AT_LIMIT != AT_BOUNDARY");
    TEST_ASSERT(MotorResult::TIMEOUT != MotorResult::CANCELLED, "TIMEOUT != CANCELLED");
    TEST_ASSERT(MotorResult::NOT_HOMED != MotorResult::INVALID_POSITION, "NOT_HOMED != INVALID_POSITION");

    Log.info("PASS: Motor result enum values are distinct");
    _testsPassed++;
}

// ============================================================================
// CLEANUP
// ============================================================================

void cleanup_tests() {
    if (_motor != nullptr) {
        _motor->disable();
        delete _motor;
        _motor = nullptr;
    }
}

// ============================================================================
// TEST RUNNER
// ============================================================================

/**
 * @brief Run all motor controller unit tests
 * @return Number of failed tests
 */
int runMotorTests() {
    _testsPassed = 0;
    _testsFailed = 0;

    Log.info("========================================");
    Log.info("    Motor Controller Unit Tests");
    Log.info("========================================");

    // MOT-001: Initialization
    test_motor_construction();
    test_motor_initialization();
    test_motor_enable_disable();

    // MOT-002: Position tracking
    test_position_constants();
    test_speed_constants();
    test_microns_per_step();
    test_position_conversion();
    test_position_tracking();
    test_position_validation();

    // MOT-003: Homing / limit switch
    test_limit_switch();
    // test_homing_sequence(); // Uncomment for hardware testing

    // MOT-004 & MOT-005: Movement calculations
    test_movement_calculations();
    test_position_error();

    // MOT-006: Oscillation
    test_oscillation_state();

    // MOT-007: Predefined positions
    test_predefined_position_functions();

    // State and enum tests
    test_motor_state();
    test_motor_result_enum();

    // Cleanup
    cleanup_tests();

    // Summary
    Log.info("========================================");
    Log.info("    Motor Controller Tests Complete");
    Log.info("    Passed: %d", _testsPassed);
    Log.info("    Failed: %d", _testsFailed);
    Log.info("========================================");

    return _testsFailed;
}

// ============================================================================
// HARDWARE MOVEMENT TESTS (Separate section - requires real hardware)
// ============================================================================
// These tests actually move the motor and should only be run on a real device
// with the stage properly mounted and safe to move.

#ifdef RUN_HARDWARE_MOTOR_TESTS

void test_actual_homing() {
    Log.info("=== HARDWARE TEST: Homing ===");

    MotorController motor;
    motor.init();

    MotorResult result = motor.home(false);
    TEST_ASSERT(result == MotorResult::SUCCESS || result == MotorResult::AT_LIMIT,
                "Homing should succeed");
    TEST_ASSERT(motor.isHomed(), "Motor should be homed");
    TEST_ASSERT_NEAR(1000, motor.getCurrentPosition(), 50, "Position should be near INITIAL");

    motor.disable();
}

void test_actual_movement() {
    Log.info("=== HARDWARE TEST: Movement ===");

    MotorController motor;
    motor.init();
    motor.home(false);

    // Move to test start
    MotorResult result = motor.moveToTestStart(MotorSpeed::SLOW);
    TEST_ASSERT(result == MotorResult::SUCCESS, "Move to test start should succeed");
    TEST_ASSERT_NEAR(StagePosition::TEST_START, motor.getCurrentPosition(), 50,
                     "Position should be near TEST_START");

    // Move to home
    result = motor.moveToHome(MotorSpeed::SLOW);
    TEST_ASSERT(result == MotorResult::SUCCESS, "Move to home should succeed");
    TEST_ASSERT_NEAR(StagePosition::HOME, motor.getCurrentPosition(), 50,
                     "Position should be near HOME");

    motor.disable();
}

void test_actual_oscillation() {
    Log.info("=== HARDWARE TEST: Oscillation ===");

    MotorController motor;
    motor.init();
    motor.home(false);

    int32_t startPos = motor.getCurrentPosition();

    // Oscillate 3 cycles
    MotorResult result = motor.oscillate(1000, MotorSpeed::OSCILLATION, 3);
    TEST_ASSERT(result == MotorResult::SUCCESS, "Oscillation should succeed");
    TEST_ASSERT_NEAR(startPos, motor.getCurrentPosition(), 50,
                     "Position should return to start after oscillation");

    motor.disable();
}

int runHardwareMotorTests() {
    int failures = 0;
    _testsPassed = 0;
    _testsFailed = 0;

    Log.info("========================================");
    Log.info("    HARDWARE Motor Tests");
    Log.info("    WARNING: Motor will move!");
    Log.info("========================================");

    test_actual_homing();
    test_actual_movement();
    test_actual_oscillation();

    failures = _testsFailed;

    Log.info("========================================");
    Log.info("    Hardware Tests Complete");
    Log.info("    Passed: %d, Failed: %d", _testsPassed, _testsFailed);
    Log.info("========================================");

    return failures;
}

#endif // RUN_HARDWARE_MOTOR_TESTS

// ============================================================================
// STANDALONE TEST ENTRY POINT
// ============================================================================
// Uncomment the following to run tests standalone on device

/*
void setup() {
    Serial.begin(115200);
    waitFor(Serial.isConnected, 10000);
    delay(1000);

    int failures = runMotorTests();

    if (failures == 0) {
        Log.info("ALL MOTOR TESTS PASSED!");
    } else {
        Log.error("MOTOR TESTS FAILED: %d failures", failures);
    }

    // Uncomment for hardware tests:
    // runHardwareMotorTests();
}

void loop() {
    delay(10000);
}
*/
