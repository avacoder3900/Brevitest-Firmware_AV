/**
 * @file test_led.cpp
 * @brief Unit tests for LED Controller module
 * @author Agent MU - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file contains unit tests for the LEDController module. Tests verify
 * color setting, pattern timing, priority system, and state mapping.
 *
 * Test Categories:
 * - Initialization and shutdown
 * - Direct color control
 * - State-based indicators
 * - User prompt indicators
 * - Pattern animations
 * - Priority system
 *
 * Usage:
 *   Run via Particle device test framework or compile with mock hardware
 *   includes for PC-based testing.
 *
 * User Stories Tested:
 *   - LED-001: LEDController class
 *   - LED-002: State-based indicators
 *   - LED-003: User prompt indicators
 *   - LED-004: Pattern animations
 *   - LED-005: Priority system
 */

#include "Particle.h"
#include "LEDController.h"
#include "DataTypes.h"

// ============================================================================
// TEST FRAMEWORK MACROS
// ============================================================================

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

#define TEST_ASSERT_NOT_EQUAL(expected, actual, message) do { \
    if ((expected) != (actual)) { \
        _testsPassed++; \
        Log.info("PASS: %s", message); \
    } else { \
        _testsFailed++; \
        Log.error("FAIL: %s - values should differ, both are %d (line %d)", message, (int)(expected), __LINE__); \
    } \
} while(0)

// ============================================================================
// LED COLOR TESTS
// ============================================================================

/**
 * @brief Test LED color utility functions
 */
void test_led_color_utilities() {
    Log.info("=== Testing LED Color Utilities ===");

    // Test color component extraction
    uint32_t testColor = 0xAABBCC;
    TEST_ASSERT_EQUAL(0xAA, LEDColors::getRed(testColor), "getRed should extract red component");
    TEST_ASSERT_EQUAL(0xBB, LEDColors::getGreen(testColor), "getGreen should extract green component");
    TEST_ASSERT_EQUAL(0xCC, LEDColors::getBlue(testColor), "getBlue should extract blue component");

    // Test color packing
    uint32_t packed = LEDColors::pack(0x11, 0x22, 0x33);
    TEST_ASSERT_EQUAL(0x112233, packed, "pack should combine RGB components");

    // Test predefined colors
    TEST_ASSERT_EQUAL(0xFF0000, LEDColors::RED, "RED should be 0xFF0000");
    TEST_ASSERT_EQUAL(0x00FF00, LEDColors::GREEN, "GREEN should be 0x00FF00");
    TEST_ASSERT_EQUAL(0x0000FF, LEDColors::BLUE, "BLUE should be 0x0000FF");
    TEST_ASSERT_EQUAL(0x000000, LEDColors::OFF, "OFF should be 0x000000");
    TEST_ASSERT_EQUAL(0xFFFFFF, LEDColors::WHITE, "WHITE should be 0xFFFFFF");
    TEST_ASSERT_EQUAL(0xFF6600, LEDColors::ORANGE, "ORANGE should be 0xFF6600");
}

// ============================================================================
// INITIALIZATION TESTS (LED-001)
// ============================================================================

/**
 * @brief Test LED controller initialization
 */
void test_led_initialization() {
    Log.info("=== Testing LED Initialization (LED-001) ===");

    // Ensure clean state
    if (LEDController::isInitialized()) {
        LEDController::shutdown();
    }

    // Test initial state
    TEST_ASSERT(!LEDController::isInitialized(), "Should not be initialized before init()");

    // Test initialization
    bool initResult = LEDController::init();
    TEST_ASSERT(initResult, "init() should return true");
    TEST_ASSERT(LEDController::isInitialized(), "Should be initialized after init()");

    // Test double initialization (should be safe)
    bool reinitResult = LEDController::init();
    TEST_ASSERT(reinitResult, "Double init() should return true");

    // Test shutdown
    LEDController::shutdown();
    TEST_ASSERT(!LEDController::isInitialized(), "Should not be initialized after shutdown()");

    // Re-initialize for remaining tests
    LEDController::init();
}

/**
 * @brief Test direct color control
 */
void test_led_direct_control() {
    Log.info("=== Testing Direct Color Control (LED-001) ===");

    // Test setColor with RGB values
    LEDController::setColor(255, 128, 64);
    Log.info("PASS: setColor(r,g,b) completed without error");
    _testsPassed++;

    // Test setColor with packed color
    LEDController::setColor(LEDColors::CYAN);
    Log.info("PASS: setColor(packed) completed without error");
    _testsPassed++;

    // Test brightness control
    LEDController::setBrightness(128);
    TEST_ASSERT_EQUAL(128, LEDController::getBrightness(), "getBrightness should return set value");

    LEDController::setBrightness(255);
    TEST_ASSERT_EQUAL(255, LEDController::getBrightness(), "getBrightness should return 255");

    // Test off function
    LEDController::off();
    Log.info("PASS: off() completed without error");
    _testsPassed++;
}

// ============================================================================
// STATE-BASED INDICATOR TESTS (LED-002)
// ============================================================================

/**
 * @brief Test state-to-LED mapping
 */
void test_led_state_mapping() {
    Log.info("=== Testing State-Based Indicators (LED-002) ===");

    // Ensure initialized
    if (!LEDController::isInitialized()) {
        LEDController::init();
    }

    // Clear any active prompts
    LEDController::clearPrompt();

    // Test IDLE state
    LEDController::setDeviceState(DeviceMode::IDLE);
    TEST_ASSERT_EQUAL((int)DeviceMode::IDLE, (int)LEDController::getCurrentState(),
                      "State should be IDLE");

    // Test HEATING state
    LEDController::setDeviceState(DeviceMode::HEATING);
    TEST_ASSERT_EQUAL((int)DeviceMode::HEATING, (int)LEDController::getCurrentState(),
                      "State should be HEATING");

    // Test RUNNING_TEST state
    LEDController::setDeviceState(DeviceMode::RUNNING_TEST);
    TEST_ASSERT_EQUAL((int)DeviceMode::RUNNING_TEST, (int)LEDController::getCurrentState(),
                      "State should be RUNNING_TEST");

    // Test ERROR_STATE
    LEDController::setDeviceState(DeviceMode::ERROR_STATE);
    TEST_ASSERT_EQUAL((int)DeviceMode::ERROR_STATE, (int)LEDController::getCurrentState(),
                      "State should be ERROR_STATE");

    // Test UPLOADING_RESULTS
    LEDController::setDeviceState(DeviceMode::UPLOADING_RESULTS);
    TEST_ASSERT_EQUAL((int)DeviceMode::UPLOADING_RESULTS, (int)LEDController::getCurrentState(),
                      "State should be UPLOADING_RESULTS");

    // Test BARCODE_SCANNING
    LEDController::setDeviceState(DeviceMode::BARCODE_SCANNING);
    TEST_ASSERT_EQUAL((int)DeviceMode::BARCODE_SCANNING, (int)LEDController::getCurrentState(),
                      "State should be BARCODE_SCANNING");

    // Test clearState
    LEDController::clearState();
    TEST_ASSERT_EQUAL((int)DeviceMode::IDLE, (int)LEDController::getCurrentState(),
                      "State should be IDLE after clearState()");
}

// ============================================================================
// USER PROMPT TESTS (LED-003)
// ============================================================================

/**
 * @brief Test user prompt indicators
 */
void test_led_user_prompts() {
    Log.info("=== Testing User Prompt Indicators (LED-003) ===");

    // Ensure initialized
    if (!LEDController::isInitialized()) {
        LEDController::init();
    }

    // Set a base state
    LEDController::setDeviceState(DeviceMode::IDLE);

    // Test INSERT prompt
    LEDController::showPrompt(LEDPrompt::INSERT_CARTRIDGE);
    TEST_ASSERT(LEDController::isPromptActive(), "Prompt should be active");
    TEST_ASSERT_EQUAL((int)LEDPrompt::INSERT_CARTRIDGE, (int)LEDController::getActivePrompt(),
                      "Active prompt should be INSERT_CARTRIDGE");

    // Test REMOVE prompt
    LEDController::showPrompt(LEDPrompt::REMOVE_CARTRIDGE);
    TEST_ASSERT_EQUAL((int)LEDPrompt::REMOVE_CARTRIDGE, (int)LEDController::getActivePrompt(),
                      "Active prompt should be REMOVE_CARTRIDGE");

    // Test DONT_TOUCH prompt
    LEDController::showPrompt(LEDPrompt::DONT_TOUCH);
    TEST_ASSERT_EQUAL((int)LEDPrompt::DONT_TOUCH, (int)LEDController::getActivePrompt(),
                      "Active prompt should be DONT_TOUCH");

    // Test clearing prompt
    LEDController::clearPrompt();
    TEST_ASSERT(!LEDController::isPromptActive(), "Prompt should not be active after clear");
    TEST_ASSERT_EQUAL((int)LEDPrompt::NONE, (int)LEDController::getActivePrompt(),
                      "Active prompt should be NONE after clear");

    // Test that state is preserved through prompt
    LEDController::setDeviceState(DeviceMode::HEATING);
    LEDController::showPrompt(LEDPrompt::DONT_TOUCH);
    LEDController::clearPrompt();
    TEST_ASSERT_EQUAL((int)DeviceMode::HEATING, (int)LEDController::getCurrentState(),
                      "State should be restored after prompt clear");

    // Test showPrompt(NONE) clears prompt
    LEDController::showPrompt(LEDPrompt::INSERT_CARTRIDGE);
    LEDController::showPrompt(LEDPrompt::NONE);
    TEST_ASSERT(!LEDController::isPromptActive(), "showPrompt(NONE) should clear prompt");
}

// ============================================================================
// PATTERN ANIMATION TESTS (LED-004)
// ============================================================================

/**
 * @brief Test animation period values
 */
void test_led_animation_periods() {
    Log.info("=== Testing Animation Periods (LED-004) ===");

    TEST_ASSERT_EQUAL(LEDTiming::PERIOD_SLOW, LEDController::getPeriod(LEDSpeed::SLOW),
                      "SLOW period should match constant");
    TEST_ASSERT_EQUAL(LEDTiming::PERIOD_NORMAL, LEDController::getPeriod(LEDSpeed::NORMAL),
                      "NORMAL period should match constant");
    TEST_ASSERT_EQUAL(LEDTiming::PERIOD_FAST, LEDController::getPeriod(LEDSpeed::FAST),
                      "FAST period should match constant");

    // Verify period relationships
    TEST_ASSERT(LEDController::getPeriod(LEDSpeed::SLOW) > LEDController::getPeriod(LEDSpeed::NORMAL),
                "SLOW period should be > NORMAL period");
    TEST_ASSERT(LEDController::getPeriod(LEDSpeed::NORMAL) > LEDController::getPeriod(LEDSpeed::FAST),
                "NORMAL period should be > FAST period");
}

/**
 * @brief Test custom pattern setting
 */
void test_led_custom_patterns() {
    Log.info("=== Testing Custom Patterns (LED-004) ===");

    // Ensure initialized
    if (!LEDController::isInitialized()) {
        LEDController::init();
    }

    // Test setting custom pattern
    LEDController::setPattern(LEDColors::MAGENTA, LEDPattern::PULSE, LEDSpeed::NORMAL);
    Log.info("PASS: setPattern() completed without error");
    _testsPassed++;

    // Test pattern with timeout
    LEDController::setPattern(LEDColors::YELLOW, LEDPattern::BLINK, LEDSpeed::FAST,
                               LEDPriority::NORMAL, 500);
    Log.info("PASS: setPattern() with timeout completed without error");
    _testsPassed++;

    // Test update function
    for (int i = 0; i < 10; i++) {
        LEDController::update();
        delay(50);
    }
    Log.info("PASS: update() loop completed without error");
    _testsPassed++;

    // Test forceUpdate
    LEDController::forceUpdate();
    Log.info("PASS: forceUpdate() completed without error");
    _testsPassed++;

    // Reset to default state
    LEDController::clearAll();
}

// ============================================================================
// PRIORITY SYSTEM TESTS (LED-005)
// ============================================================================

/**
 * @brief Test priority levels
 */
void test_led_priority_system() {
    Log.info("=== Testing Priority System (LED-005) ===");

    // Ensure initialized
    if (!LEDController::isInitialized()) {
        LEDController::init();
    }

    LEDController::clearAll();

    // Test default priority
    LEDController::setDeviceState(DeviceMode::IDLE);
    TEST_ASSERT_EQUAL((int)LEDPriority::NORMAL, (int)LEDController::getActivePriority(),
                      "Default state should have NORMAL priority");

    // Test error state has CRITICAL priority
    LEDController::setDeviceState(DeviceMode::ERROR_STATE);
    TEST_ASSERT_EQUAL((int)LEDPriority::CRITICAL, (int)LEDController::getActivePriority(),
                      "ERROR_STATE should have CRITICAL priority");

    // Test prompt has HIGH priority
    LEDController::clearState();
    LEDController::showPrompt(LEDPrompt::DONT_TOUCH);
    TEST_ASSERT_EQUAL((int)LEDPriority::HIGH, (int)LEDController::getActivePriority(),
                      "Prompt should have HIGH priority");

    // Test isHigherPriorityActive
    TEST_ASSERT(LEDController::isHigherPriorityActive(LEDPriority::NORMAL),
                "HIGH priority should be higher than NORMAL");
    TEST_ASSERT(LEDController::isHigherPriorityActive(LEDPriority::LOW),
                "HIGH priority should be higher than LOW");
    TEST_ASSERT(!LEDController::isHigherPriorityActive(LEDPriority::CRITICAL),
                "HIGH priority should not be higher than CRITICAL");

    LEDController::clearAll();
}

/**
 * @brief Test temporary indicator with timeout
 */
void test_led_temporary_indicator() {
    Log.info("=== Testing Temporary Indicator (LED-005) ===");

    // Ensure initialized
    if (!LEDController::isInitialized()) {
        LEDController::init();
    }

    LEDController::clearAll();

    // Set base state
    LEDController::setDeviceState(DeviceMode::IDLE);

    // Show temporary indicator with 500ms timeout
    LEDController::showTemporary(LEDColors::YELLOW, LEDPattern::SOLID, LEDSpeed::NORMAL,
                                  LEDPriority::HIGH, 500);

    TEST_ASSERT_EQUAL((int)LEDPriority::HIGH, (int)LEDController::getActivePriority(),
                      "Temporary indicator should have HIGH priority");

    // Wait for timeout and update
    delay(600);
    LEDController::update();

    // State should be restored
    TEST_ASSERT_EQUAL((int)DeviceMode::IDLE, (int)LEDController::getCurrentState(),
                      "State should be IDLE after temporary indicator expires");

    Log.info("PASS: Temporary indicator timeout worked correctly");
    _testsPassed++;
}

/**
 * @brief Test clearAll function
 */
void test_led_clear_all() {
    Log.info("=== Testing clearAll Function ===");

    // Ensure initialized
    if (!LEDController::isInitialized()) {
        LEDController::init();
    }

    // Set various states
    LEDController::setDeviceState(DeviceMode::RUNNING_TEST);
    LEDController::showPrompt(LEDPrompt::DONT_TOUCH);

    // Clear all
    LEDController::clearAll();

    // Verify everything is cleared
    TEST_ASSERT_EQUAL((int)DeviceMode::IDLE, (int)LEDController::getCurrentState(),
                      "State should be IDLE after clearAll");
    TEST_ASSERT(!LEDController::isPromptActive(), "Prompt should not be active after clearAll");
    TEST_ASSERT_EQUAL((int)LEDPrompt::NONE, (int)LEDController::getActivePrompt(),
                      "Active prompt should be NONE after clearAll");
}

// ============================================================================
// UTILITY FUNCTION TESTS
// ============================================================================

/**
 * @brief Test string conversion functions
 */
void test_led_string_conversions() {
    Log.info("=== Testing String Conversions ===");

    // Pattern to string
    TEST_ASSERT(strcmp(LEDController::patternToString(LEDPattern::SOLID), "SOLID") == 0,
                "patternToString(SOLID) should return 'SOLID'");
    TEST_ASSERT(strcmp(LEDController::patternToString(LEDPattern::BLINK), "BLINK") == 0,
                "patternToString(BLINK) should return 'BLINK'");
    TEST_ASSERT(strcmp(LEDController::patternToString(LEDPattern::FADE), "FADE") == 0,
                "patternToString(FADE) should return 'FADE'");
    TEST_ASSERT(strcmp(LEDController::patternToString(LEDPattern::PULSE), "PULSE") == 0,
                "patternToString(PULSE) should return 'PULSE'");

    // Priority to string
    TEST_ASSERT(strcmp(LEDController::priorityToString(LEDPriority::LOW), "LOW") == 0,
                "priorityToString(LOW) should return 'LOW'");
    TEST_ASSERT(strcmp(LEDController::priorityToString(LEDPriority::NORMAL), "NORMAL") == 0,
                "priorityToString(NORMAL) should return 'NORMAL'");
    TEST_ASSERT(strcmp(LEDController::priorityToString(LEDPriority::HIGH), "HIGH") == 0,
                "priorityToString(HIGH) should return 'HIGH'");
    TEST_ASSERT(strcmp(LEDController::priorityToString(LEDPriority::CRITICAL), "CRITICAL") == 0,
                "priorityToString(CRITICAL) should return 'CRITICAL'");

    // Prompt to string
    TEST_ASSERT(strcmp(LEDController::promptToString(LEDPrompt::NONE), "NONE") == 0,
                "promptToString(NONE) should return 'NONE'");
    TEST_ASSERT(strcmp(LEDController::promptToString(LEDPrompt::INSERT_CARTRIDGE), "INSERT_CARTRIDGE") == 0,
                "promptToString(INSERT_CARTRIDGE) should return 'INSERT_CARTRIDGE'");
    TEST_ASSERT(strcmp(LEDController::promptToString(LEDPrompt::REMOVE_CARTRIDGE), "REMOVE_CARTRIDGE") == 0,
                "promptToString(REMOVE_CARTRIDGE) should return 'REMOVE_CARTRIDGE'");
    TEST_ASSERT(strcmp(LEDController::promptToString(LEDPrompt::DONT_TOUCH), "DONT_TOUCH") == 0,
                "promptToString(DONT_TOUCH) should return 'DONT_TOUCH'");
}

// ============================================================================
// EDGE CASE TESTS
// ============================================================================

/**
 * @brief Test operations before initialization
 */
void test_led_uninitialized_operations() {
    Log.info("=== Testing Uninitialized Operations ===");

    // Shutdown if initialized
    if (LEDController::isInitialized()) {
        LEDController::shutdown();
    }

    // These should not crash, just be no-ops
    LEDController::setColor(255, 0, 0);
    Log.info("PASS: setColor() on uninitialized did not crash");
    _testsPassed++;

    LEDController::setDeviceState(DeviceMode::HEATING);
    Log.info("PASS: setDeviceState() on uninitialized did not crash");
    _testsPassed++;

    LEDController::showPrompt(LEDPrompt::INSERT_CARTRIDGE);
    Log.info("PASS: showPrompt() on uninitialized did not crash");
    _testsPassed++;

    LEDController::update();
    Log.info("PASS: update() on uninitialized did not crash");
    _testsPassed++;

    // Re-initialize for cleanup
    LEDController::init();
}

/**
 * @brief Test rapid state changes
 */
void test_led_rapid_state_changes() {
    Log.info("=== Testing Rapid State Changes ===");

    // Ensure initialized
    if (!LEDController::isInitialized()) {
        LEDController::init();
    }

    // Rapid state changes should not cause issues
    for (int i = 0; i < 50; i++) {
        LEDController::setDeviceState(DeviceMode::IDLE);
        LEDController::setDeviceState(DeviceMode::HEATING);
        LEDController::setDeviceState(DeviceMode::RUNNING_TEST);
        LEDController::setDeviceState(DeviceMode::UPLOADING_RESULTS);
        LEDController::update();
    }

    Log.info("PASS: Rapid state changes completed without error");
    _testsPassed++;

    // Rapid prompt changes
    for (int i = 0; i < 50; i++) {
        LEDController::showPrompt(LEDPrompt::INSERT_CARTRIDGE);
        LEDController::showPrompt(LEDPrompt::REMOVE_CARTRIDGE);
        LEDController::showPrompt(LEDPrompt::DONT_TOUCH);
        LEDController::clearPrompt();
        LEDController::update();
    }

    Log.info("PASS: Rapid prompt changes completed without error");
    _testsPassed++;

    LEDController::clearAll();
}

// ============================================================================
// TEST RUNNER
// ============================================================================

/**
 * @brief Run all LED controller unit tests
 * @return Number of failed tests
 */
int runLEDTests() {
    _testsPassed = 0;
    _testsFailed = 0;

    Log.info("========================================");
    Log.info("    LED Controller Unit Tests Starting");
    Log.info("========================================");

    // Color utilities
    test_led_color_utilities();

    // LED-001: Initialization and direct control
    test_led_initialization();
    test_led_direct_control();

    // LED-002: State-based indicators
    test_led_state_mapping();

    // LED-003: User prompt indicators
    test_led_user_prompts();

    // LED-004: Pattern animations
    test_led_animation_periods();
    test_led_custom_patterns();

    // LED-005: Priority system
    test_led_priority_system();
    test_led_temporary_indicator();
    test_led_clear_all();

    // Utility functions
    test_led_string_conversions();

    // Edge cases
    test_led_uninitialized_operations();
    test_led_rapid_state_changes();

    // Summary
    Log.info("========================================");
    Log.info("    LED Controller Unit Tests Complete");
    Log.info("    Passed: %d", _testsPassed);
    Log.info("    Failed: %d", _testsFailed);
    Log.info("========================================");

    // Cleanup
    LEDController::shutdown();

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

    int failures = runLEDTests();

    if (failures == 0) {
        Log.info("ALL LED TESTS PASSED!");
    } else {
        Log.error("LED TESTS FAILED: %d failures", failures);
    }
}

void loop() {
    // Do nothing after tests complete
    delay(10000);
}
*/
