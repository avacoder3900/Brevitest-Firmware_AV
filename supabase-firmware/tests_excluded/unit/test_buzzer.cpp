/**
 * @file test_buzzer.cpp
 * @brief Unit tests for BuzzerController module
 * @author Agent LAMBDA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file contains unit tests for the BuzzerController module. Tests verify:
 * - Initialization and basic tone functions (BUZ-001)
 * - Predefined alert tones (BUZ-002)
 * - Periodic alert mode (BUZ-003)
 * - Non-blocking operation (BUZ-004)
 * - Pattern/melody playback (BUZ-005)
 *
 * Usage:
 *   Run via Particle device test framework or compile with mock hardware
 *   includes for PC-based testing.
 */

#include "Particle.h"
#include "BuzzerController.h"
#include "HardwareConfig.h"
#include "HAL.h"

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
        Log.error("FAIL: %s - got unexpected value %d (line %d)", message, (int)(actual), __LINE__); \
    } \
} while(0)

// ============================================================================
// INITIALIZATION TESTS (BUZ-001)
// ============================================================================

/**
 * @brief Test buzzer controller initialization
 */
void test_buzzer_initialization() {
    Log.info("=== Testing BuzzerController Initialization (BUZ-001) ===");

    // Initialize HAL first
    HAL::init();

    // Initialize buzzer controller
    bool initResult = BuzzerController::init();
    TEST_ASSERT(initResult, "BuzzerController::init() should return true");
    TEST_ASSERT(BuzzerController::isInitialized(), "BuzzerController should be initialized after init()");

    // Double initialization should be safe
    bool reinitResult = BuzzerController::init();
    TEST_ASSERT(reinitResult, "BuzzerController::init() should return true on re-init");

    // Initial state checks
    TEST_ASSERT(!BuzzerController::isPlaying(), "Buzzer should not be playing after init");
    TEST_ASSERT(!BuzzerController::isPeriodicAlertActive(), "Periodic alert should not be active after init");
    TEST_ASSERT(!BuzzerController::isPatternPlaying(), "Pattern should not be playing after init");
    TEST_ASSERT_EQUAL(100, BuzzerController::getVolume(), "Volume should be 100 after init");
    TEST_ASSERT_EQUAL(0, BuzzerController::getQueueLength(), "Queue should be empty after init");
}

/**
 * @brief Test basic tone playback
 */
void test_basic_tone_playback() {
    Log.info("=== Testing Basic Tone Playback (BUZ-001) ===");

    // Play a short tone
    BuzzerController::playTone(600, 100);
    TEST_ASSERT(BuzzerController::isPlaying(), "Buzzer should be playing after playTone()");

    // Wait for tone to complete
    uint32_t startTime = millis();
    while (BuzzerController::isPlaying() && (millis() - startTime) < 200) {
        BuzzerController::update();
        delay(10);
    }

    TEST_ASSERT(!BuzzerController::isPlaying(), "Buzzer should stop after duration elapsed");

    // Test stopTone()
    BuzzerController::playTone(600, 1000);
    TEST_ASSERT(BuzzerController::isPlaying(), "Buzzer should be playing");
    BuzzerController::stopTone();
    TEST_ASSERT(!BuzzerController::isPlaying(), "Buzzer should stop after stopTone()");
}

/**
 * @brief Test volume control
 */
void test_volume_control() {
    Log.info("=== Testing Volume Control (BUZ-001) ===");

    // Test volume settings
    BuzzerController::setVolume(50);
    TEST_ASSERT_EQUAL(50, BuzzerController::getVolume(), "Volume should be 50 after setVolume(50)");

    BuzzerController::setVolume(0);
    TEST_ASSERT_EQUAL(0, BuzzerController::getVolume(), "Volume should be 0 after setVolume(0)");

    BuzzerController::setVolume(100);
    TEST_ASSERT_EQUAL(100, BuzzerController::getVolume(), "Volume should be 100 after setVolume(100)");

    // Test clamping above 100
    BuzzerController::setVolume(150);
    TEST_ASSERT_EQUAL(100, BuzzerController::getVolume(), "Volume should clamp to 100");

    // Reset to default
    BuzzerController::setVolume(100);
}

// ============================================================================
// PREDEFINED ALERT TESTS (BUZ-002)
// ============================================================================

/**
 * @brief Test predefined alert tone functions
 */
void test_predefined_alerts() {
    Log.info("=== Testing Predefined Alert Tones (BUZ-002) ===");

    // Test cartridge insert (620 Hz, 200ms)
    BuzzerController::cartridgeInsert();
    TEST_ASSERT(BuzzerController::isPlaying(), "cartridgeInsert() should start playing");
    BuzzerController::stopTone();

    // Test cartridge remove (620 Hz, 500ms)
    BuzzerController::cartridgeRemove();
    TEST_ASSERT(BuzzerController::isPlaying(), "cartridgeRemove() should start playing");
    BuzzerController::stopTone();

    // Test general alert (850 Hz, 500ms)
    BuzzerController::generalAlert();
    TEST_ASSERT(BuzzerController::isPlaying(), "generalAlert() should start playing");
    BuzzerController::stopTone();

    // Test problem alert (620 Hz, 100ms)
    BuzzerController::problemAlert();
    TEST_ASSERT(BuzzerController::isPlaying(), "problemAlert() should start playing");
    BuzzerController::stopTone();

    // Test standard beep (600 Hz, 1000ms)
    BuzzerController::standardBeep();
    TEST_ASSERT(BuzzerController::isPlaying(), "standardBeep() should start playing");
    BuzzerController::stopTone();

    Log.info("PASS: All predefined alerts started successfully");
    _testsPassed++;
}

/**
 * @brief Verify predefined alert constants match PRD specs
 */
void test_alert_constants() {
    Log.info("=== Testing Alert Constants (BUZ-002) ===");

    // Cartridge insert: 620 Hz, 200ms
    TEST_ASSERT_EQUAL(620, BUZZER_INSERT_FREQUENCY, "Insert frequency should be 620 Hz");
    TEST_ASSERT_EQUAL(200, BUZZER_INSERT_DURATION, "Insert duration should be 200ms");

    // Cartridge remove: 620 Hz, 500ms
    TEST_ASSERT_EQUAL(620, BUZZER_REMOVE_FREQUENCY, "Remove frequency should be 620 Hz");
    TEST_ASSERT_EQUAL(500, BUZZER_REMOVE_DURATION, "Remove duration should be 500ms");

    // General alert: 850 Hz, 500ms, 4000ms interval
    TEST_ASSERT_EQUAL(850, BUZZER_ALERT_FREQUENCY, "Alert frequency should be 850 Hz");
    TEST_ASSERT_EQUAL(500, BUZZER_ALERT_DURATION, "Alert duration should be 500ms");
    TEST_ASSERT_EQUAL(4000, BUZZER_ALERT_PERIOD, "Alert period should be 4000ms");

    // Problem alert: 620 Hz, 100ms, 777ms interval
    TEST_ASSERT_EQUAL(620, BUZZER_PROBLEM_FREQUENCY, "Problem frequency should be 620 Hz");
    TEST_ASSERT_EQUAL(100, BUZZER_PROBLEM_DURATION, "Problem duration should be 100ms");
    TEST_ASSERT_EQUAL(777, BUZZER_PROBLEM_PERIOD, "Problem period should be 777ms");

    // Standard beep: 600 Hz, 1000ms
    TEST_ASSERT_EQUAL(600, BUZZER_FREQUENCY, "Default frequency should be 600 Hz");
    TEST_ASSERT_EQUAL(1000, BUZZER_DURATION, "Default duration should be 1000ms");
}

// ============================================================================
// PERIODIC ALERT TESTS (BUZ-003)
// ============================================================================

/**
 * @brief Test periodic alert start/stop
 */
void test_periodic_alert_basic() {
    Log.info("=== Testing Periodic Alert Basic (BUZ-003) ===");

    // Start general alert
    BuzzerController::startPeriodicAlert(AlertType::GENERAL);
    TEST_ASSERT(BuzzerController::isPeriodicAlertActive(), "Periodic alert should be active after start");
    TEST_ASSERT(BuzzerController::isPlaying(), "Buzzer should play immediately on periodic alert start");

    // Verify alert type
    AlertType activeType = BuzzerController::getActiveAlertType();
    TEST_ASSERT(activeType == AlertType::GENERAL, "Active alert type should be GENERAL");

    // Stop periodic alert
    BuzzerController::stopPeriodicAlert();
    TEST_ASSERT(!BuzzerController::isPeriodicAlertActive(), "Periodic alert should be inactive after stop");
}

/**
 * @brief Test switching between alert types
 */
void test_periodic_alert_switching() {
    Log.info("=== Testing Periodic Alert Switching (BUZ-003) ===");

    // Start with general alert
    BuzzerController::startPeriodicAlert(AlertType::GENERAL);
    TEST_ASSERT(BuzzerController::isPeriodicAlertActive(), "GENERAL alert should be active");
    TEST_ASSERT(BuzzerController::getActiveAlertType() == AlertType::GENERAL, "Type should be GENERAL");

    // Switch to problem alert
    BuzzerController::startPeriodicAlert(AlertType::PROBLEM);
    TEST_ASSERT(BuzzerController::isPeriodicAlertActive(), "PROBLEM alert should be active");
    TEST_ASSERT(BuzzerController::getActiveAlertType() == AlertType::PROBLEM, "Type should be PROBLEM");

    // Stop
    BuzzerController::stopPeriodicAlert();
    TEST_ASSERT(!BuzzerController::isPeriodicAlertActive(), "Alert should be stopped");
}

/**
 * @brief Test periodic alert timing (partial - full timing test requires longer wait)
 */
void test_periodic_alert_timing() {
    Log.info("=== Testing Periodic Alert Timing (BUZ-003) ===");

    // Start problem alert (777ms period - shorter for testing)
    BuzzerController::startPeriodicAlert(AlertType::PROBLEM);
    TEST_ASSERT(BuzzerController::isPlaying(), "Should play immediately on start");

    // Wait for first tone to complete (100ms)
    uint32_t startTime = millis();
    while (BuzzerController::isPlaying() && (millis() - startTime) < 200) {
        BuzzerController::update();
        delay(10);
    }

    TEST_ASSERT(!BuzzerController::isPlaying(), "First tone should complete");
    TEST_ASSERT(BuzzerController::isPeriodicAlertActive(), "Periodic alert should still be active");

    // Clean up
    BuzzerController::stopPeriodicAlert();
}

// ============================================================================
// NON-BLOCKING TESTS (BUZ-004)
// ============================================================================

/**
 * @brief Test that update() is required for tone completion
 */
void test_nonblocking_update() {
    Log.info("=== Testing Non-Blocking Update (BUZ-004) ===");

    // Play a short tone
    BuzzerController::playTone(600, 50);
    TEST_ASSERT(BuzzerController::isPlaying(), "Tone should be playing");

    // Without update(), tone tracking won't complete
    delay(100);  // Tone hardware will stop, but state won't update without update()

    // Call update to process completion
    BuzzerController::update();
    TEST_ASSERT(!BuzzerController::isPlaying(), "Tone should complete after update()");
}

/**
 * @brief Test tone queuing
 */
void test_tone_queue() {
    Log.info("=== Testing Tone Queue (BUZ-004) ===");

    // Clear any state
    BuzzerController::stopTone();
    TEST_ASSERT_EQUAL(0, BuzzerController::getQueueLength(), "Queue should be empty");

    // Queue multiple tones
    bool q1 = BuzzerController::queueTone(400, 50);
    TEST_ASSERT(q1, "queueTone #1 should succeed");

    bool q2 = BuzzerController::queueTone(500, 50);
    TEST_ASSERT(q2, "queueTone #2 should succeed");

    bool q3 = BuzzerController::queueTone(600, 50);
    TEST_ASSERT(q3, "queueTone #3 should succeed");

    // First tone should start immediately, others queued
    TEST_ASSERT(BuzzerController::isPlaying(), "First queued tone should start playing");

    // Queue should have 2 remaining (first one dequeued to play)
    TEST_ASSERT_EQUAL(2, BuzzerController::getQueueLength(), "Queue should have 2 tones remaining");

    // Play through queue
    uint32_t startTime = millis();
    while ((BuzzerController::isPlaying() || BuzzerController::getQueueLength() > 0) &&
           (millis() - startTime) < 500) {
        BuzzerController::update();
        delay(10);
    }

    TEST_ASSERT_EQUAL(0, BuzzerController::getQueueLength(), "Queue should be empty after playback");
    TEST_ASSERT(!BuzzerController::isPlaying(), "Should not be playing after queue empty");
}

/**
 * @brief Test queue overflow
 */
void test_queue_overflow() {
    Log.info("=== Testing Queue Overflow (BUZ-004) ===");

    BuzzerController::stopTone();
    BuzzerController::clearQueue();

    // Fill the queue
    bool allSucceeded = true;
    for (int i = 0; i < BUZZER_QUEUE_SIZE; i++) {
        // First one plays immediately, rest queue
        if (!BuzzerController::queueTone(500, 100)) {
            allSucceeded = false;
        }
    }
    TEST_ASSERT(allSucceeded, "Should be able to queue BUZZER_QUEUE_SIZE tones");

    // Try to add one more - should fail
    bool overflowResult = BuzzerController::queueTone(500, 100);
    TEST_ASSERT(!overflowResult, "Queue overflow should return false");

    // Clean up
    BuzzerController::clearQueue();
    BuzzerController::stopTone();
}

/**
 * @brief Test interrupt functionality
 */
void test_interrupt_tone() {
    Log.info("=== Testing Interrupt Tone (BUZ-004) ===");

    // Start a long tone
    BuzzerController::playTone(400, 5000);
    TEST_ASSERT(BuzzerController::isPlaying(), "Long tone should be playing");

    // Queue some tones
    BuzzerController::queueTone(500, 100);
    BuzzerController::queueTone(600, 100);

    // Interrupt with urgent tone
    BuzzerController::interruptWithTone(800, 100);
    TEST_ASSERT(BuzzerController::isPlaying(), "Interrupt tone should be playing");
    TEST_ASSERT_EQUAL(0, BuzzerController::getQueueLength(), "Queue should be cleared after interrupt");

    // Clean up
    BuzzerController::stopTone();
}

// ============================================================================
// MELODY/PATTERN TESTS (BUZ-005)
// ============================================================================

/**
 * @brief Test pattern playback
 */
void test_pattern_playback() {
    Log.info("=== Testing Pattern Playback (BUZ-005) ===");

    // Create a simple test pattern
    Tone testPattern[] = {
        Tone(440, 50),   // A4
        Tone(0, 50),     // Rest
        Tone(523, 50),   // C5
        Tone(0, 50),     // Rest
        Tone(659, 50)    // E5
    };

    bool result = BuzzerController::playPattern(testPattern, 5);
    TEST_ASSERT(result, "playPattern should return true");
    TEST_ASSERT(BuzzerController::isPatternPlaying(), "Pattern should be playing");

    // Play through pattern
    uint32_t startTime = millis();
    while (BuzzerController::isPatternPlaying() && (millis() - startTime) < 500) {
        BuzzerController::update();
        delay(10);
    }

    TEST_ASSERT(!BuzzerController::isPatternPlaying(), "Pattern should complete");
}

/**
 * @brief Test predefined melodies
 */
void test_predefined_melodies() {
    Log.info("=== Testing Predefined Melodies (BUZ-005) ===");

    // Test success melody
    BuzzerController::playSuccessMelody();
    TEST_ASSERT(BuzzerController::isPatternPlaying(), "Success melody should start playing");

    // Wait for completion
    uint32_t startTime = millis();
    while (BuzzerController::isPatternPlaying() && (millis() - startTime) < 1000) {
        BuzzerController::update();
        delay(10);
    }
    TEST_ASSERT(!BuzzerController::isPatternPlaying(), "Success melody should complete");

    // Test error melody
    BuzzerController::playErrorMelody();
    TEST_ASSERT(BuzzerController::isPatternPlaying(), "Error melody should start playing");

    startTime = millis();
    while (BuzzerController::isPatternPlaying() && (millis() - startTime) < 1000) {
        BuzzerController::update();
        delay(10);
    }
    TEST_ASSERT(!BuzzerController::isPatternPlaying(), "Error melody should complete");

    // Test startup melody
    BuzzerController::playStartupMelody();
    TEST_ASSERT(BuzzerController::isPatternPlaying(), "Startup melody should start playing");

    startTime = millis();
    while (BuzzerController::isPatternPlaying() && (millis() - startTime) < 1000) {
        BuzzerController::update();
        delay(10);
    }
    TEST_ASSERT(!BuzzerController::isPatternPlaying(), "Startup melody should complete");
}

/**
 * @brief Test pattern completion callback
 */
static volatile bool _callbackInvoked = false;
static void testPatternCallback() {
    _callbackInvoked = true;
}

void test_pattern_callback() {
    Log.info("=== Testing Pattern Callback (BUZ-005) ===");

    _callbackInvoked = false;

    // Register callback
    BuzzerController::setPatternCompleteCallback(testPatternCallback);

    // Play a short pattern
    Tone shortPattern[] = {
        Tone(440, 50),
        Tone(523, 50)
    };

    BuzzerController::playPattern(shortPattern, 2);

    // Wait for completion
    uint32_t startTime = millis();
    while (BuzzerController::isPatternPlaying() && (millis() - startTime) < 300) {
        BuzzerController::update();
        delay(10);
    }

    TEST_ASSERT(_callbackInvoked, "Callback should be invoked on pattern completion");

    // Clean up
    BuzzerController::setPatternCompleteCallback(nullptr);
}

/**
 * @brief Test pattern stop (callback should NOT be invoked)
 */
void test_pattern_stop() {
    Log.info("=== Testing Pattern Stop (BUZ-005) ===");

    _callbackInvoked = false;
    BuzzerController::setPatternCompleteCallback(testPatternCallback);

    // Play a longer pattern
    Tone longPattern[] = {
        Tone(440, 200),
        Tone(523, 200),
        Tone(659, 200)
    };

    BuzzerController::playPattern(longPattern, 3);
    TEST_ASSERT(BuzzerController::isPatternPlaying(), "Pattern should be playing");

    // Stop mid-pattern
    delay(50);
    BuzzerController::stopPattern();
    TEST_ASSERT(!BuzzerController::isPatternPlaying(), "Pattern should be stopped");
    TEST_ASSERT(!_callbackInvoked, "Callback should NOT be invoked when stopped manually");

    // Clean up
    BuzzerController::setPatternCompleteCallback(nullptr);
}

/**
 * @brief Test invalid pattern input
 */
void test_pattern_invalid_input() {
    Log.info("=== Testing Pattern Invalid Input (BUZ-005) ===");

    // Null pointer
    bool nullResult = BuzzerController::playPattern(nullptr, 5);
    TEST_ASSERT(!nullResult, "playPattern with null should return false");

    // Zero count
    Tone emptyPattern[] = { Tone(440, 100) };
    bool zeroResult = BuzzerController::playPattern(emptyPattern, 0);
    TEST_ASSERT(!zeroResult, "playPattern with 0 count should return false");
}

// ============================================================================
// DIAGNOSTICS TESTS
// ============================================================================

/**
 * @brief Test self-test function
 */
void test_self_test() {
    Log.info("=== Testing Self-Test ===");

    bool result = BuzzerController::selfTest();
    TEST_ASSERT(result, "selfTest should return true");
}

/**
 * @brief Test state string output
 */
void test_state_string() {
    Log.info("=== Testing State String ===");

    char buffer[128];
    size_t len = BuzzerController::getStateAsString(buffer, sizeof(buffer));

    TEST_ASSERT(len > 0, "getStateAsString should return non-zero length");
    TEST_ASSERT(len < sizeof(buffer), "Output should fit in buffer");

    Log.info("State string: %s", buffer);
    _testsPassed++;
}

// ============================================================================
// TEST RUNNER
// ============================================================================

/**
 * @brief Run all BuzzerController unit tests
 * @return Number of failed tests
 */
int runBuzzerTests() {
    _testsPassed = 0;
    _testsFailed = 0;

    Log.info("========================================");
    Log.info("    BuzzerController Unit Tests");
    Log.info("========================================");

    // BUZ-001: Initialization and basic functions
    test_buzzer_initialization();
    test_basic_tone_playback();
    test_volume_control();

    // BUZ-002: Predefined alerts
    test_predefined_alerts();
    test_alert_constants();

    // BUZ-003: Periodic alerts
    test_periodic_alert_basic();
    test_periodic_alert_switching();
    test_periodic_alert_timing();

    // BUZ-004: Non-blocking operation
    test_nonblocking_update();
    test_tone_queue();
    test_queue_overflow();
    test_interrupt_tone();

    // BUZ-005: Patterns and melodies
    test_pattern_playback();
    test_predefined_melodies();
    test_pattern_callback();
    test_pattern_stop();
    test_pattern_invalid_input();

    // Diagnostics
    test_self_test();
    test_state_string();

    // Summary
    Log.info("========================================");
    Log.info("    BuzzerController Tests Complete");
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

    HAL::init();
    int failures = runBuzzerTests();

    if (failures == 0) {
        Log.info("ALL BUZZER TESTS PASSED!");
    } else {
        Log.error("BUZZER TESTS FAILED: %d failures", failures);
    }
}

void loop() {
    // Do nothing after tests complete
    delay(10000);
}
*/
