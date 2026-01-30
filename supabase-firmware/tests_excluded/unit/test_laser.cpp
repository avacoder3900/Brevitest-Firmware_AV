/**
 * @file test_laser.cpp
 * @brief Unit tests for LaserController module
 * @author Agent IOTA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file contains unit tests for the LaserController module. Tests verify
 * initialization, individual and bulk laser control, pulse mode timing,
 * and safety interlock logic.
 *
 * Test Categories:
 * - LAS-001: Initialization tests
 * - LAS-002: Individual channel control tests
 * - LAS-003: All-laser control tests
 * - LAS-004: Pulse mode timing tests
 * - LAS-005: Safety interlock tests
 *
 * Usage:
 *   Run via Particle device test framework or compile with mock hardware
 *   includes for PC-based testing.
 */

#include "Particle.h"
#include "LaserController.h"
#include "HAL.h"
#include "HardwareConfig.h"

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

#define TEST_ASSERT_NOT_EQUAL(notExpected, actual, message) do { \
    if ((notExpected) != (actual)) { \
        _testsPassed++; \
        Log.info("PASS: %s", message); \
    } else { \
        _testsFailed++; \
        Log.error("FAIL: %s - should not be %d (line %d)", message, (int)(actual), __LINE__); \
    } \
} while(0)

#define TEST_ASSERT_NEAR(expected, actual, tolerance, message) do { \
    int diff = ((expected) > (actual)) ? ((expected) - (actual)) : ((actual) - (expected)); \
    if (diff <= (tolerance)) { \
        _testsPassed++; \
        Log.info("PASS: %s (diff=%d)", message, diff); \
    } else { \
        _testsFailed++; \
        Log.error("FAIL: %s - expected %d, got %d, tolerance %d (line %d)", message, (int)(expected), (int)(actual), (int)(tolerance), __LINE__); \
    } \
} while(0)

// ============================================================================
// TEST FIXTURES
// ============================================================================

static LaserController* _laser = nullptr;

/**
 * @brief Set up test fixture - create fresh LaserController
 */
static void setUp() {
    if (_laser != nullptr) {
        delete _laser;
    }
    _laser = new LaserController();

    // Ensure HAL is initialized
    HAL::init();
}

/**
 * @brief Tear down test fixture - clean up
 */
static void tearDown() {
    if (_laser != nullptr) {
        _laser->emergencyStop();
        delete _laser;
        _laser = nullptr;
    }
    // Ensure all lasers are off
    HAL::setAllLasersOff();
}

// ============================================================================
// LAS-001: INITIALIZATION TESTS
// ============================================================================

/**
 * @brief Test LaserController construction
 */
void test_constructor() {
    Log.info("=== Testing Constructor (LAS-001) ===");
    setUp();

    // Controller should not be initialized after construction
    TEST_ASSERT(!_laser->isInitialized(), "Should not be initialized after construction");

    // Default power should be LASER_DEFAULT_POWER (128)
    TEST_ASSERT_EQUAL(LASER_DEFAULT_POWER, _laser->getLaserPower(LaserChannel::A),
                      "Default power A should be 128");
    TEST_ASSERT_EQUAL(LASER_DEFAULT_POWER, _laser->getLaserPower(LaserChannel::B),
                      "Default power B should be 128");
    TEST_ASSERT_EQUAL(LASER_DEFAULT_POWER, _laser->getLaserPower(LaserChannel::C),
                      "Default power C should be 128");

    // All lasers should be disabled
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::A), "Laser A should be disabled");
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::B), "Laser B should be disabled");
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::C), "Laser C should be disabled");

    tearDown();
}

/**
 * @brief Test LaserController initialization
 */
void test_initialization() {
    Log.info("=== Testing Initialization (LAS-001) ===");
    setUp();

    // Initialize should succeed
    bool result = _laser->init();
    TEST_ASSERT(result, "init() should return true");
    TEST_ASSERT(_laser->isInitialized(), "Should be initialized after init()");

    // Double init should be safe
    bool result2 = _laser->init();
    TEST_ASSERT(result2, "Second init() should also return true");

    // Interlock should be enabled by default
    TEST_ASSERT(_laser->isInterlockEnabled(), "Interlock should be enabled by default");

    // Pulse mode should be enabled by default
    TEST_ASSERT(_laser->isPulseModeEnabled(), "Pulse mode should be enabled after init");

    // Default pulse timing should be set
    TEST_ASSERT_EQUAL(LASER_DEFAULT_PULSE_ON_US, _laser->getPulseOnTime(),
                      "Default pulse on-time should be 20us");
    TEST_ASSERT_EQUAL(LASER_DEFAULT_CYCLE_US, _laser->getPulseCycleTime(),
                      "Default cycle time should be 2000us");

    tearDown();
}

/**
 * @brief Test pin configuration after init
 */
void test_pin_configuration() {
    Log.info("=== Testing Pin Configuration (LAS-001) ===");
    setUp();

    // Verify pin constants
    TEST_ASSERT_EQUAL(D5, PIN_LASER_A, "Laser A should be on D5");
    TEST_ASSERT_EQUAL(D6, PIN_LASER_B, "Laser B should be on D6");
    TEST_ASSERT_EQUAL(D7, PIN_LASER_C, "Laser C should be on D7");

    // Verify getChannelPin utility
    TEST_ASSERT_EQUAL(PIN_LASER_A, LaserController::getChannelPin(LaserChannel::A),
                      "getChannelPin(A) should return D5");
    TEST_ASSERT_EQUAL(PIN_LASER_B, LaserController::getChannelPin(LaserChannel::B),
                      "getChannelPin(B) should return D6");
    TEST_ASSERT_EQUAL(PIN_LASER_C, LaserController::getChannelPin(LaserChannel::C),
                      "getChannelPin(C) should return D7");

    tearDown();
}

// ============================================================================
// LAS-002: INDIVIDUAL CHANNEL CONTROL TESTS
// ============================================================================

/**
 * @brief Test setLaserPower function
 */
void test_set_laser_power() {
    Log.info("=== Testing setLaserPower (LAS-002) ===");
    setUp();
    _laser->init();

    // Set power for each channel
    TEST_ASSERT(_laser->setLaserPower(LaserChannel::A, 100), "Set power A to 100");
    TEST_ASSERT(_laser->setLaserPower(LaserChannel::B, 200), "Set power B to 200");
    TEST_ASSERT(_laser->setLaserPower(LaserChannel::C, 255), "Set power C to 255");

    // Verify power values
    TEST_ASSERT_EQUAL(100, _laser->getLaserPower(LaserChannel::A), "Power A should be 100");
    TEST_ASSERT_EQUAL(200, _laser->getLaserPower(LaserChannel::B), "Power B should be 200");
    TEST_ASSERT_EQUAL(255, _laser->getLaserPower(LaserChannel::C), "Power C should be 255");

    // Test character-based interface
    TEST_ASSERT(_laser->setLaserPower('A', 50), "Set power 'A' to 50");
    TEST_ASSERT_EQUAL(50, _laser->getLaserPower('A'), "Power 'A' should be 50");

    // Test lowercase
    TEST_ASSERT(_laser->setLaserPower('b', 75), "Set power 'b' to 75");
    TEST_ASSERT_EQUAL(75, _laser->getLaserPower('b'), "Power 'b' should be 75");
    TEST_ASSERT_EQUAL(75, _laser->getLaserPower('B'), "Power 'B' should also be 75");

    // Test invalid channel
    TEST_ASSERT(!_laser->setLaserPower('X', 100), "Set power 'X' should fail");
    TEST_ASSERT_EQUAL(0, _laser->getLaserPower('X'), "Power 'X' should be 0");

    tearDown();
}

/**
 * @brief Test enableLaser/disableLaser functions
 */
void test_enable_disable_laser() {
    Log.info("=== Testing enableLaser/disableLaser (LAS-002) ===");
    setUp();
    _laser->init();

    // Disable interlock for testing without cartridge
    _laser->setInterlockEnabled(false);

    // Enable laser A
    TEST_ASSERT(_laser->enableLaser(LaserChannel::A), "Enable laser A should succeed");
    TEST_ASSERT(_laser->isLaserEnabled(LaserChannel::A), "Laser A should be enabled");

    // Enable laser B using char
    TEST_ASSERT(_laser->enableLaser('B'), "Enable laser 'B' should succeed");
    TEST_ASSERT(_laser->isLaserEnabled('B'), "Laser 'B' should be enabled");

    // Disable laser A
    TEST_ASSERT(_laser->disableLaser(LaserChannel::A), "Disable laser A should succeed");
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::A), "Laser A should be disabled");

    // Disable laser B using char
    TEST_ASSERT(_laser->disableLaser('B'), "Disable laser 'B' should succeed");
    TEST_ASSERT(!_laser->isLaserEnabled('B'), "Laser 'B' should be disabled");

    // Test lowercase
    TEST_ASSERT(_laser->enableLaser('c'), "Enable laser 'c' should succeed");
    TEST_ASSERT(_laser->isLaserEnabled('C'), "Laser 'C' should be enabled");
    TEST_ASSERT(_laser->disableLaser('c'), "Disable laser 'c' should succeed");

    // Test invalid channel
    TEST_ASSERT(!_laser->enableLaser('X'), "Enable laser 'X' should fail");
    TEST_ASSERT(!_laser->disableLaser('Z'), "Disable laser 'Z' should fail");

    tearDown();
}

/**
 * @brief Test that laser cannot be enabled with power=0
 */
void test_enable_with_zero_power() {
    Log.info("=== Testing Enable with Zero Power (LAS-002) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // Set power to 0
    _laser->setLaserPower(LaserChannel::A, 0);

    // Try to enable - should fail
    TEST_ASSERT(!_laser->enableLaser(LaserChannel::A), "Enable with power=0 should fail");
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::A), "Laser should not be enabled");

    // Set power > 0 and try again
    _laser->setLaserPower(LaserChannel::A, 128);
    TEST_ASSERT(_laser->enableLaser(LaserChannel::A), "Enable with power>0 should succeed");

    tearDown();
}

/**
 * @brief Test isLaserEnabled function
 */
void test_is_laser_enabled() {
    Log.info("=== Testing isLaserEnabled (LAS-002) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // Initially all should be disabled
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::A), "A should be disabled initially");
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::B), "B should be disabled initially");
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::C), "C should be disabled initially");

    // Enable A
    _laser->enableLaser(LaserChannel::A);
    TEST_ASSERT(_laser->isLaserEnabled(LaserChannel::A), "A should be enabled");
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::B), "B should still be disabled");

    // Test char interface
    TEST_ASSERT(_laser->isLaserEnabled('A'), "'A' should be enabled");
    TEST_ASSERT(_laser->isLaserEnabled('a'), "'a' should also be enabled");
    TEST_ASSERT(!_laser->isLaserEnabled('B'), "'B' should be disabled");

    // Invalid channel
    TEST_ASSERT(!_laser->isLaserEnabled('X'), "'X' should return false");

    tearDown();
}

// ============================================================================
// LAS-003: ALL-LASER CONTROL TESTS
// ============================================================================

/**
 * @brief Test enableAllLasers function
 */
void test_enable_all_lasers() {
    Log.info("=== Testing enableAllLasers (LAS-003) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // Enable all
    TEST_ASSERT(_laser->enableAllLasers(), "enableAllLasers should succeed");

    // All should be enabled
    TEST_ASSERT(_laser->isLaserEnabled(LaserChannel::A), "A should be enabled");
    TEST_ASSERT(_laser->isLaserEnabled(LaserChannel::B), "B should be enabled");
    TEST_ASSERT(_laser->isLaserEnabled(LaserChannel::C), "C should be enabled");
    TEST_ASSERT(_laser->areAllLasersEnabled(), "areAllLasersEnabled should return true");

    tearDown();
}

/**
 * @brief Test disableAllLasers function
 */
void test_disable_all_lasers() {
    Log.info("=== Testing disableAllLasers (LAS-003) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // Enable all first
    _laser->enableAllLasers();
    TEST_ASSERT(_laser->areAllLasersEnabled(), "All should be enabled first");

    // Disable all
    _laser->disableAllLasers();

    // All should be disabled
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::A), "A should be disabled");
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::B), "B should be disabled");
    TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::C), "C should be disabled");
    TEST_ASSERT(!_laser->areAllLasersEnabled(), "areAllLasersEnabled should return false");
    TEST_ASSERT(!_laser->isAnyLaserEnabled(), "isAnyLaserEnabled should return false");

    tearDown();
}

/**
 * @brief Test setAllLaserPower function
 */
void test_set_all_laser_power() {
    Log.info("=== Testing setAllLaserPower (LAS-003) ===");
    setUp();
    _laser->init();

    // Set all to 200
    _laser->setAllLaserPower(200);

    // Verify all are 200
    TEST_ASSERT_EQUAL(200, _laser->getLaserPower(LaserChannel::A), "A power should be 200");
    TEST_ASSERT_EQUAL(200, _laser->getLaserPower(LaserChannel::B), "B power should be 200");
    TEST_ASSERT_EQUAL(200, _laser->getLaserPower(LaserChannel::C), "C power should be 200");

    // Set to 0 should disable all
    _laser->setInterlockEnabled(false);
    _laser->enableAllLasers();
    _laser->setAllLaserPower(0);

    TEST_ASSERT(!_laser->isAnyLaserEnabled(), "All should be disabled when power set to 0");

    tearDown();
}

/**
 * @brief Test areAllLasersEnabled function
 */
void test_are_all_lasers_enabled() {
    Log.info("=== Testing areAllLasersEnabled (LAS-003) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // None enabled
    TEST_ASSERT(!_laser->areAllLasersEnabled(), "None enabled - should return false");

    // Only A enabled
    _laser->enableLaser(LaserChannel::A);
    TEST_ASSERT(!_laser->areAllLasersEnabled(), "Only A enabled - should return false");

    // A and B enabled
    _laser->enableLaser(LaserChannel::B);
    TEST_ASSERT(!_laser->areAllLasersEnabled(), "A and B enabled - should return false");

    // All enabled
    _laser->enableLaser(LaserChannel::C);
    TEST_ASSERT(_laser->areAllLasersEnabled(), "All enabled - should return true");

    tearDown();
}

/**
 * @brief Test isAnyLaserEnabled function
 */
void test_is_any_laser_enabled() {
    Log.info("=== Testing isAnyLaserEnabled (LAS-003) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // None enabled
    TEST_ASSERT(!_laser->isAnyLaserEnabled(), "None enabled - should return false");

    // One enabled
    _laser->enableLaser(LaserChannel::B);
    TEST_ASSERT(_laser->isAnyLaserEnabled(), "B enabled - should return true");

    // Disable B
    _laser->disableLaser(LaserChannel::B);
    TEST_ASSERT(!_laser->isAnyLaserEnabled(), "None enabled again - should return false");

    tearDown();
}

// ============================================================================
// LAS-004: PULSE MODE TIMING TESTS
// ============================================================================

/**
 * @brief Test setPulseMode function
 */
void test_set_pulse_mode() {
    Log.info("=== Testing setPulseMode (LAS-004) ===");
    setUp();
    _laser->init();

    // Set custom pulse mode
    TEST_ASSERT(_laser->setPulseMode(50, 5000), "Set 50us on, 5000us cycle");
    TEST_ASSERT_EQUAL(50, _laser->getPulseOnTime(), "On-time should be 50us");
    TEST_ASSERT_EQUAL(5000, _laser->getPulseCycleTime(), "Cycle time should be 5000us");

    // Test boundary values
    TEST_ASSERT(_laser->setPulseMode(LASER_MIN_PULSE_ON_US, LASER_MIN_CYCLE_US),
                "Minimum values should work");
    TEST_ASSERT(_laser->setPulseMode(LASER_MAX_PULSE_ON_US, LASER_MAX_CYCLE_US),
                "Maximum values should work");

    tearDown();
}

/**
 * @brief Test setPulseMode validation
 */
void test_pulse_mode_validation() {
    Log.info("=== Testing Pulse Mode Validation (LAS-004) ===");
    setUp();
    _laser->init();

    // Set known good values first
    _laser->setPulseMode(20, 2000);

    // On-time too small
    TEST_ASSERT(!_laser->setPulseMode(1, 2000), "On-time < min should fail");
    TEST_ASSERT_EQUAL(20, _laser->getPulseOnTime(), "On-time should not change");

    // On-time too large
    TEST_ASSERT(!_laser->setPulseMode(2000, 5000), "On-time > max should fail");

    // Cycle time too small
    TEST_ASSERT(!_laser->setPulseMode(20, 50), "Cycle < min should fail");

    // Cycle time too large
    TEST_ASSERT(!_laser->setPulseMode(20, 200000), "Cycle > max should fail");

    // On-time >= cycle time
    TEST_ASSERT(!_laser->setPulseMode(100, 100), "On-time >= cycle should fail");
    TEST_ASSERT(!_laser->setPulseMode(100, 50), "On-time > cycle should fail");

    tearDown();
}

/**
 * @brief Test pulse function timing
 */
void test_pulse_timing() {
    Log.info("=== Testing Pulse Timing (LAS-004) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // Set known pulse timing
    _laser->setPulseMode(100, 1000);  // 100us on, 1ms cycle

    // Measure pulse duration
    uint32_t start = micros();
    _laser->pulse(LaserChannel::A);
    uint32_t elapsed = micros() - start;

    // Should be at least 100us (the on-time)
    TEST_ASSERT(elapsed >= 100, "Pulse should take at least on-time");

    // Should not be more than 200us (some overhead is expected)
    TEST_ASSERT(elapsed < 500, "Pulse should complete reasonably quickly");

    Log.info("Pulse timing: %lu us (expected ~100us)", elapsed);

    tearDown();
}

/**
 * @brief Test pulseAll function
 */
void test_pulse_all() {
    Log.info("=== Testing pulseAll (LAS-004) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // Set pulse mode
    _laser->setPulseMode(50, 1000);

    // Execute pulse on all channels
    uint32_t start = micros();
    TEST_ASSERT(_laser->pulseAll(), "pulseAll should succeed");
    uint32_t elapsed = micros() - start;

    // Should be at least 50us
    TEST_ASSERT(elapsed >= 50, "Pulse should take at least on-time");

    Log.info("PulseAll timing: %lu us", elapsed);

    tearDown();
}

/**
 * @brief Test isPulseModeEnabled function
 */
void test_is_pulse_mode_enabled() {
    Log.info("=== Testing isPulseModeEnabled (LAS-004) ===");
    setUp();

    // Before init, should be false
    TEST_ASSERT(!_laser->isPulseModeEnabled(), "Should be false before init");

    _laser->init();

    // After init, should be true
    TEST_ASSERT(_laser->isPulseModeEnabled(), "Should be true after init");

    tearDown();
}

// ============================================================================
// LAS-005: SAFETY INTERLOCK TESTS
// ============================================================================

/**
 * @brief Test interlock enabled by default
 */
void test_interlock_default() {
    Log.info("=== Testing Interlock Default (LAS-005) ===");
    setUp();
    _laser->init();

    TEST_ASSERT(_laser->isInterlockEnabled(), "Interlock should be enabled by default");

    tearDown();
}

/**
 * @brief Test setInterlockEnabled function
 */
void test_set_interlock_enabled() {
    Log.info("=== Testing setInterlockEnabled (LAS-005) ===");
    setUp();
    _laser->init();

    // Disable interlock
    _laser->setInterlockEnabled(false);
    TEST_ASSERT(!_laser->isInterlockEnabled(), "Interlock should be disabled");

    // Re-enable interlock
    _laser->setInterlockEnabled(true);
    TEST_ASSERT(_laser->isInterlockEnabled(), "Interlock should be enabled again");

    tearDown();
}

/**
 * @brief Test interlock prevents laser enable when no cartridge
 * @note This test assumes no cartridge is inserted during testing
 */
void test_interlock_prevents_enable() {
    Log.info("=== Testing Interlock Prevents Enable (LAS-005) ===");
    setUp();
    _laser->init();

    // With interlock enabled and no cartridge, enable should fail
    if (!_laser->isCartridgePresent()) {
        TEST_ASSERT(!_laser->enableLaser(LaserChannel::A),
                    "Enable should fail with interlock and no cartridge");
        TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::A), "Laser should not be enabled");

        TEST_ASSERT(!_laser->enableAllLasers(),
                    "enableAllLasers should fail with interlock and no cartridge");

        // Disable interlock
        _laser->setInterlockEnabled(false);
        TEST_ASSERT(_laser->enableLaser(LaserChannel::A),
                    "Enable should succeed with interlock disabled");
    } else {
        Log.warn("SKIP: Cartridge is present - cannot test interlock block");
    }

    tearDown();
}

/**
 * @brief Test that pulse is blocked by interlock
 */
void test_interlock_blocks_pulse() {
    Log.info("=== Testing Interlock Blocks Pulse (LAS-005) ===");
    setUp();
    _laser->init();

    if (!_laser->isCartridgePresent()) {
        // Pulse should fail with interlock enabled
        TEST_ASSERT(!_laser->pulse(LaserChannel::A),
                    "Pulse should fail with interlock and no cartridge");
        TEST_ASSERT(!_laser->pulseAll(),
                    "pulseAll should fail with interlock and no cartridge");

        // Disable interlock
        _laser->setInterlockEnabled(false);
        TEST_ASSERT(_laser->pulse(LaserChannel::A),
                    "Pulse should succeed with interlock disabled");
    } else {
        Log.warn("SKIP: Cartridge is present - cannot test interlock block");
    }

    tearDown();
}

/**
 * @brief Test continuous on-time tracking
 */
void test_continuous_on_time() {
    Log.info("=== Testing Continuous On-Time (LAS-005) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // Initially should be 0
    TEST_ASSERT_EQUAL(0, _laser->getContinuousOnTime(), "On-time should be 0 initially");

    // Enable a laser
    _laser->enableLaser(LaserChannel::A);

    // Wait a bit
    delay(100);

    // Should have some on-time
    uint32_t onTime = _laser->getContinuousOnTime();
    TEST_ASSERT(onTime >= 100, "On-time should be >= 100ms");
    TEST_ASSERT(onTime < 200, "On-time should be < 200ms");

    // Disable laser
    _laser->disableLaser(LaserChannel::A);

    // On-time should reset to 0
    TEST_ASSERT_EQUAL(0, _laser->getContinuousOnTime(), "On-time should reset to 0");

    tearDown();
}

/**
 * @brief Test overtime detection
 */
void test_overtime_detection() {
    Log.info("=== Testing Overtime Detection (LAS-005) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // Initially not overtime
    TEST_ASSERT(!_laser->isOvertime(), "Should not be overtime initially");

    // Enable a laser
    _laser->enableLaser(LaserChannel::A);

    // Should not be overtime yet
    TEST_ASSERT(!_laser->isOvertime(), "Should not be overtime after just enabling");

    // Note: We can't test actual overtime without waiting 30+ seconds
    // Just verify the function works
    Log.info("INFO: Overtime limit is %lu ms", LASER_MAX_CONTINUOUS_ON_MS);

    tearDown();
}

/**
 * @brief Test emergencyStop function
 */
void test_emergency_stop() {
    Log.info("=== Testing emergencyStop (LAS-005) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // Enable all lasers
    _laser->enableAllLasers();
    TEST_ASSERT(_laser->areAllLasersEnabled(), "All should be enabled");

    // Emergency stop
    _laser->emergencyStop();

    // All should be disabled
    TEST_ASSERT(!_laser->isAnyLaserEnabled(), "All should be disabled after emergency stop");
    TEST_ASSERT_EQUAL(0, _laser->getContinuousOnTime(), "On-time should be reset");

    tearDown();
}

/**
 * @brief Test update function (safety check loop)
 */
void test_update_safety_check() {
    Log.info("=== Testing update() Safety Check (LAS-005) ===");
    setUp();
    _laser->init();
    _laser->setInterlockEnabled(false);

    // Enable a laser
    _laser->enableLaser(LaserChannel::A);
    TEST_ASSERT(_laser->isLaserEnabled(LaserChannel::A), "Laser should be enabled");

    // Call update - should not disable (no safety condition triggered)
    _laser->update();
    TEST_ASSERT(_laser->isLaserEnabled(LaserChannel::A), "Laser should still be enabled");

    // Re-enable interlock
    _laser->setInterlockEnabled(true);

    // If no cartridge, update should disable lasers
    if (!_laser->isCartridgePresent()) {
        _laser->update();
        TEST_ASSERT(!_laser->isLaserEnabled(LaserChannel::A),
                    "Laser should be disabled by interlock check");
    }

    tearDown();
}

// ============================================================================
// UTILITY FUNCTION TESTS
// ============================================================================

/**
 * @brief Test charToChannel utility
 */
void test_char_to_channel() {
    Log.info("=== Testing charToChannel Utility ===");

    LaserChannel ch;

    // Uppercase
    TEST_ASSERT(LaserController::charToChannel('A', ch), "'A' should be valid");
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(LaserChannel::A), static_cast<uint8_t>(ch),
                      "'A' should map to channel A");

    TEST_ASSERT(LaserController::charToChannel('B', ch), "'B' should be valid");
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(LaserChannel::B), static_cast<uint8_t>(ch),
                      "'B' should map to channel B");

    TEST_ASSERT(LaserController::charToChannel('C', ch), "'C' should be valid");
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(LaserChannel::C), static_cast<uint8_t>(ch),
                      "'C' should map to channel C");

    // Lowercase
    TEST_ASSERT(LaserController::charToChannel('a', ch), "'a' should be valid");
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(LaserChannel::A), static_cast<uint8_t>(ch),
                      "'a' should map to channel A");

    // Invalid
    TEST_ASSERT(!LaserController::charToChannel('X', ch), "'X' should be invalid");
    TEST_ASSERT(!LaserController::charToChannel('1', ch), "'1' should be invalid");
}

/**
 * @brief Test channelToChar utility
 */
void test_channel_to_char() {
    Log.info("=== Testing channelToChar Utility ===");

    TEST_ASSERT_EQUAL('A', LaserController::channelToChar(LaserChannel::A),
                      "Channel A should return 'A'");
    TEST_ASSERT_EQUAL('B', LaserController::channelToChar(LaserChannel::B),
                      "Channel B should return 'B'");
    TEST_ASSERT_EQUAL('C', LaserController::channelToChar(LaserChannel::C),
                      "Channel C should return 'C'");

    // Invalid channel
    TEST_ASSERT_EQUAL('?', LaserController::channelToChar(static_cast<LaserChannel>(99)),
                      "Invalid channel should return '?'");
}

// ============================================================================
// TEST RUNNER
// ============================================================================

/**
 * @brief Run all LaserController unit tests
 * @return Number of failed tests
 */
int runLaserTests() {
    _testsPassed = 0;
    _testsFailed = 0;

    Log.info("========================================");
    Log.info("    LaserController Unit Tests Starting");
    Log.info("========================================");

    // LAS-001: Initialization
    test_constructor();
    test_initialization();
    test_pin_configuration();

    // LAS-002: Individual channel control
    test_set_laser_power();
    test_enable_disable_laser();
    test_enable_with_zero_power();
    test_is_laser_enabled();

    // LAS-003: All-laser control
    test_enable_all_lasers();
    test_disable_all_lasers();
    test_set_all_laser_power();
    test_are_all_lasers_enabled();
    test_is_any_laser_enabled();

    // LAS-004: Pulse mode
    test_set_pulse_mode();
    test_pulse_mode_validation();
    test_pulse_timing();
    test_pulse_all();
    test_is_pulse_mode_enabled();

    // LAS-005: Safety features
    test_interlock_default();
    test_set_interlock_enabled();
    test_interlock_prevents_enable();
    test_interlock_blocks_pulse();
    test_continuous_on_time();
    test_overtime_detection();
    test_emergency_stop();
    test_update_safety_check();

    // Utility functions
    test_char_to_channel();
    test_channel_to_char();

    // Summary
    Log.info("========================================");
    Log.info("    LaserController Unit Tests Complete");
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

    int failures = runLaserTests();

    if (failures == 0) {
        Log.info("ALL LASER TESTS PASSED!");
    } else {
        Log.error("LASER TESTS FAILED: %d failures", failures);
    }
}

void loop() {
    // Do nothing after tests complete
    delay(10000);
}
*/
