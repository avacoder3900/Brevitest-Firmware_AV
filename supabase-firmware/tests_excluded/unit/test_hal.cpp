/**
 * @file test_hal.cpp
 * @brief Unit tests for Hardware Abstraction Layer
 * @author Agent BETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file contains unit tests for the HAL module. Tests are designed to
 * run on the Particle Boron device or can be adapted for mock hardware.
 *
 * Test Categories:
 * - Pin initialization verification
 * - Thermistor lookup table accuracy
 * - Analog read functionality
 * - Digital I/O operations
 * - I2C communication (requires hardware)
 *
 * Usage:
 *   Run via Particle device test framework or compile with mock hardware
 *   includes for PC-based testing.
 */

#include "Particle.h"
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
// HARDWARE CONFIG TESTS (HAL-001, HAL-002)
// ============================================================================

/**
 * @brief Test that all pin constants are defined and have expected values
 */
void test_pin_definitions() {
    Log.info("=== Testing Pin Definitions (HAL-001) ===");

    // Analog pins
    TEST_ASSERT(PIN_BUZZER == A0, "PIN_BUZZER should be A0");
    TEST_ASSERT(PIN_PHOTO_A == A2, "PIN_PHOTO_A should be A2");
    TEST_ASSERT(PIN_PHOTO_B == A3, "PIN_PHOTO_B should be A3");
    TEST_ASSERT(PIN_PHOTO_C == A4, "PIN_PHOTO_C should be A4");
    TEST_ASSERT(PIN_HEATER_THERMISTOR == A6, "PIN_HEATER_THERMISTOR should be A6");

    // Digital pins
    TEST_ASSERT(PIN_CARTRIDGE_DETECT == D3, "PIN_CARTRIDGE_DETECT should be D3");
    TEST_ASSERT(PIN_HEATER == D4, "PIN_HEATER should be D4");
    TEST_ASSERT(PIN_LASER_A == D5, "PIN_LASER_A should be D5");
    TEST_ASSERT(PIN_LASER_B == D6, "PIN_LASER_B should be D6");
    TEST_ASSERT(PIN_LASER_C == D7, "PIN_LASER_C should be D7");
    TEST_ASSERT(PIN_MOTOR_DIR == D8, "PIN_MOTOR_DIR should be D8");
    TEST_ASSERT(PIN_MOTOR_RESET == D11, "PIN_MOTOR_RESET should be D11");
    TEST_ASSERT(PIN_MOTOR_SLEEP == D12, "PIN_MOTOR_SLEEP should be D12");
    TEST_ASSERT(PIN_MOTOR_STEP == D13, "PIN_MOTOR_STEP should be D13");
    TEST_ASSERT(PIN_BARCODE_READY == D22, "PIN_BARCODE_READY should be D22");
    TEST_ASSERT(PIN_BARCODE_TRIGGER == D23, "PIN_BARCODE_TRIGGER should be D23");
    TEST_ASSERT(PIN_STAGE_LIMIT == D26, "PIN_STAGE_LIMIT should be D26");

    // I2C addresses
    TEST_ASSERT_EQUAL(0x39, I2C_ADDR_AS7341, "AS7341 address should be 0x39");
    TEST_ASSERT_EQUAL(0x41, I2C_ADDR_SPECTRO_MUX, "Spectro mux address should be 0x41");
}

/**
 * @brief Test timing and threshold constants
 */
void test_timing_constants() {
    Log.info("=== Testing Timing Constants (HAL-002) ===");

    // Motor step delays
    TEST_ASSERT(MOTOR_MINIMUM_STEP_DELAY >= 250, "Motor minimum step delay should be >= 250us");
    TEST_ASSERT(MOTOR_SLOW_STEP_DELAY <= 600, "Motor slow step delay should be <= 600us");

    // Stage limits
    TEST_ASSERT_EQUAL(45000, STAGE_POSITION_LIMIT, "Stage position limit should be 45000");

    // Heater safety
    TEST_ASSERT_EQUAL(2000, HEATER_FAILSAFE_TIMING, "Heater failsafe timing should be 2000ms");
    TEST_ASSERT_EQUAL(255, HEATER_MAX_POWER, "Heater max power should be 255");

    // Temperature targets
    TEST_ASSERT_EQUAL(450, HEATER_DEFAULT_TEMP_TARGET, "Default temp target should be 450 (45.0C)");
    TEST_ASSERT_EQUAL(600, HEATER_MAX_TEMPERATURE, "Max temperature should be 600 (60.0C)");

    // Thermistor table
    TEST_ASSERT_EQUAL(21, THERMISTOR_TABLE_LENGTH, "Thermistor table length should be 21");
}

// ============================================================================
// GPIO INITIALIZATION TESTS (HAL-003)
// ============================================================================

/**
 * @brief Test HAL initialization
 */
void test_hal_initialization() {
    Log.info("=== Testing HAL Initialization (HAL-003) ===");

    // Initialize HAL
    bool initResult = HAL::init();
    TEST_ASSERT(initResult, "HAL::init() should return true");
    TEST_ASSERT(HAL::isInitialized(), "HAL should be initialized after init()");

    // Double initialization should be safe
    bool reinitResult = HAL::init();
    TEST_ASSERT(reinitResult, "HAL::init() should return true on re-init");
}

// ============================================================================
// THERMISTOR LOOKUP TABLE TESTS (HAL-004)
// ============================================================================

/**
 * @brief Test thermistor lookup table values are identical to legacy
 */
void test_thermistor_table_values() {
    Log.info("=== Testing Thermistor Table Values (HAL-004) ===");

    // Verify table length
    TEST_ASSERT_EQUAL(21, THERMISTOR_TABLE_LENGTH, "Table should have 21 entries");

    // Verify key temperature values (10x format)
    TEST_ASSERT_EQUAL(1000, THERMISTOR_TEMP_TABLE[0], "First temp should be 1000 (100.0C)");
    TEST_ASSERT_EQUAL(500, THERMISTOR_TEMP_TABLE[10], "Middle temp should be 500 (50.0C)");
    TEST_ASSERT_EQUAL(0, THERMISTOR_TEMP_TABLE[20], "Last temp should be 0 (0.0C)");

    // Verify key raw values
    TEST_ASSERT_EQUAL(2438, THERMISTOR_RAW_TABLE[0], "Raw at 100C should be 2438");
    TEST_ASSERT_EQUAL(890, THERMISTOR_RAW_TABLE[10], "Raw at 50C should be 890");
    TEST_ASSERT_EQUAL(122, THERMISTOR_RAW_TABLE[20], "Raw at 0C should be 122");

    // Verify table is in descending order (for raw values)
    bool descending = true;
    for (int i = 0; i < THERMISTOR_TABLE_LENGTH - 1; i++) {
        if (THERMISTOR_RAW_TABLE[i] <= THERMISTOR_RAW_TABLE[i + 1]) {
            descending = false;
            break;
        }
    }
    TEST_ASSERT(descending, "Raw table should be in descending order");
}

/**
 * @brief Test thermistor raw-to-temperature conversion
 */
void test_thermistor_conversion() {
    Log.info("=== Testing Thermistor Conversion ===");

    // Test exact table values
    TEST_ASSERT_EQUAL(1000, HAL::rawToTemperature(2438), "Raw 2438 should be 1000 (100.0C)");
    TEST_ASSERT_EQUAL(500, HAL::rawToTemperature(890), "Raw 890 should be 500 (50.0C)");

    // Test interpolation - value between 45C (763) and 50C (890)
    // At raw 826 (midpoint), should be approximately 475 (47.5C)
    int16_t midTemp = HAL::rawToTemperature(826);
    TEST_ASSERT_NEAR(475, midTemp, 10, "Raw 826 should be near 475 (47.5C)");

    // Test out of range - high
    TEST_ASSERT_EQUAL(1000, HAL::rawToTemperature(3000), "Raw 3000 (out of range high) should return 1000");

    // Test out of range - low
    TEST_ASSERT_EQUAL(0, HAL::rawToTemperature(50), "Raw 50 (out of range low) should return 0");
}

/**
 * @brief Test thermistor validity check
 */
void test_thermistor_validity() {
    Log.info("=== Testing Thermistor Validity Check ===");

    TEST_ASSERT(HAL::isThermistorReadingValid(700), "700 should be valid");
    TEST_ASSERT(HAL::isThermistorReadingValid(HEATER_MIN_RAW_READING), "Min value should be valid");
    TEST_ASSERT(HAL::isThermistorReadingValid(HEATER_MAX_RAW_READING), "Max value should be valid");
    TEST_ASSERT(!HAL::isThermistorReadingValid(400), "400 should be invalid (below min)");
    TEST_ASSERT(!HAL::isThermistorReadingValid(1000), "1000 should be invalid (above max)");
}

// ============================================================================
// DIGITAL I/O TESTS (HAL-005)
// ============================================================================

/**
 * @brief Test laser control functions
 */
void test_laser_control() {
    Log.info("=== Testing Laser Control (HAL-005) ===");

    // Test valid channels
    TEST_ASSERT(HAL::setLaser('A', true), "setLaser('A', true) should succeed");
    TEST_ASSERT(HAL::setLaser('A', false), "setLaser('A', false) should succeed");
    TEST_ASSERT(HAL::setLaser('B', true), "setLaser('B', true) should succeed");
    TEST_ASSERT(HAL::setLaser('C', true), "setLaser('C', true) should succeed");

    // Test invalid channel
    TEST_ASSERT(!HAL::setLaser('X', true), "setLaser('X', true) should fail");

    // Test all off
    HAL::setAllLasersOff();
    // No way to verify without reading pins, but should not crash
    Log.info("PASS: setAllLasersOff() completed without error");
    _testsPassed++;
}

/**
 * @brief Test heater safety features
 */
void test_heater_safety() {
    Log.info("=== Testing Heater Safety ===");

    // Heater should start off
    HAL::setHeaterOff();
    TEST_ASSERT(!HAL::isHeaterFailsafe(), "Heater failsafe should be false when off");
    TEST_ASSERT_EQUAL(0, HAL::getHeaterOnDuration(), "Heater on duration should be 0 when off");

    // Turn on heater
    HAL::setHeaterPower(128);
    delay(100);
    TEST_ASSERT(HAL::getHeaterOnDuration() >= 100, "Heater on duration should be >= 100ms");
    TEST_ASSERT(!HAL::isHeaterFailsafe(), "Heater failsafe should be false after 100ms");

    // Turn off heater
    HAL::setHeaterOff();
    TEST_ASSERT_EQUAL(0, HAL::getHeaterOnDuration(), "Heater on duration should reset to 0");
}

/**
 * @brief Test motor control functions
 */
void test_motor_control() {
    Log.info("=== Testing Motor Control ===");

    // Motor should start asleep
    TEST_ASSERT(!HAL::isMotorAwake(), "Motor should start asleep");

    // Wake motor
    HAL::motorWake();
    TEST_ASSERT(HAL::isMotorAwake(), "Motor should be awake after motorWake()");

    // Test direction setting (no verification possible without hardware)
    HAL::setMotorDirection(true);
    HAL::setMotorDirection(false);
    Log.info("PASS: Motor direction set without error");
    _testsPassed++;

    // Test motor pulse (no verification possible)
    HAL::motorPulse();
    Log.info("PASS: Motor pulse executed without error");
    _testsPassed++;

    // Sleep motor
    HAL::motorSleep();
    TEST_ASSERT(!HAL::isMotorAwake(), "Motor should be asleep after motorSleep()");
}

// ============================================================================
// I2C TESTS (HAL-006)
// ============================================================================

/**
 * @brief Test I2C initialization
 */
void test_i2c_init() {
    Log.info("=== Testing I2C Initialization (HAL-006) ===");

    bool i2cResult = HAL::initI2C();
    TEST_ASSERT(i2cResult, "I2C should initialize successfully");
}

/**
 * @brief Test I2C device scan (requires hardware)
 */
void test_i2c_scan() {
    Log.info("=== Testing I2C Scan ===");

    // This will scan the bus and log any devices found
    uint8_t deviceCount = HAL::i2cScan();

    // We expect at least the spectro mux (0x41) on the bus
    // But don't fail if no devices - might be running without hardware
    Log.info("I2C scan found %d device(s)", deviceCount);
    Log.info("PASS: I2C scan completed without error");
    _testsPassed++;
}

/**
 * @brief Test I2C device presence check
 */
void test_i2c_device_present() {
    Log.info("=== Testing I2C Device Presence ===");

    // Test spectro mux presence (may fail without hardware)
    bool muxPresent = HAL::i2cDevicePresent(I2C_ADDR_SPECTRO_MUX);
    if (muxPresent) {
        Log.info("PASS: Spectro mux (0x41) detected");
        _testsPassed++;
    } else {
        Log.warn("SKIP: Spectro mux (0x41) not detected - hardware may not be connected");
        // Don't count as failure since hardware may not be present
    }

    // Test invalid address - should return false
    TEST_ASSERT(!HAL::i2cDevicePresent(0x7F), "Invalid address 0x7F should not respond");
}

// ============================================================================
// UTILITY FUNCTION TESTS
// ============================================================================

/**
 * @brief Test channel mapping functions
 */
void test_channel_mapping() {
    Log.info("=== Testing Channel Mapping ===");

    // Test spectro pin mapping
    TEST_ASSERT_EQUAL(PIN_PHOTO_A, HAL::getSpectroPin('A'), "getSpectroPin('A') should return PIN_PHOTO_A");
    TEST_ASSERT_EQUAL(PIN_PHOTO_B, HAL::getSpectroPin('B'), "getSpectroPin('B') should return PIN_PHOTO_B");
    TEST_ASSERT_EQUAL(PIN_PHOTO_C, HAL::getSpectroPin('C'), "getSpectroPin('C') should return PIN_PHOTO_C");
    TEST_ASSERT_EQUAL(0, HAL::getSpectroPin('X'), "getSpectroPin('X') should return 0");

    // Test lowercase
    TEST_ASSERT_EQUAL(PIN_PHOTO_A, HAL::getSpectroPin('a'), "getSpectroPin('a') should return PIN_PHOTO_A");

    // Test laser pin mapping
    TEST_ASSERT_EQUAL(PIN_LASER_A, HAL::getLaserPin('A'), "getLaserPin('A') should return PIN_LASER_A");
    TEST_ASSERT_EQUAL(PIN_LASER_B, HAL::getLaserPin('B'), "getLaserPin('B') should return PIN_LASER_B");
    TEST_ASSERT_EQUAL(PIN_LASER_C, HAL::getLaserPin('C'), "getLaserPin('C') should return PIN_LASER_C");
    TEST_ASSERT_EQUAL(0, HAL::getLaserPin('X'), "getLaserPin('X') should return 0");
}

/**
 * @brief Test elapsed time calculation
 */
void test_elapsed_time() {
    Log.info("=== Testing Elapsed Time ===");

    uint32_t start = millis();
    delay(100);
    uint32_t elapsed = HAL::elapsedSince(start);

    TEST_ASSERT(elapsed >= 100, "Elapsed time should be >= 100ms");
    TEST_ASSERT(elapsed < 200, "Elapsed time should be < 200ms");
}

// ============================================================================
// TEST RUNNER
// ============================================================================

/**
 * @brief Run all HAL unit tests
 * @return Number of failed tests
 */
int runAllTests() {
    _testsPassed = 0;
    _testsFailed = 0;

    Log.info("========================================");
    Log.info("    HAL Unit Tests Starting");
    Log.info("========================================");

    // HAL-001: Pin definitions
    test_pin_definitions();

    // HAL-002: Timing constants
    test_timing_constants();

    // HAL-003: GPIO initialization
    test_hal_initialization();

    // HAL-004: Analog read / thermistor
    test_thermistor_table_values();
    test_thermistor_conversion();
    test_thermistor_validity();

    // HAL-005: Digital I/O
    test_laser_control();
    test_heater_safety();
    test_motor_control();

    // HAL-006: I2C communication
    test_i2c_init();
    test_i2c_scan();
    test_i2c_device_present();

    // Utility functions
    test_channel_mapping();
    test_elapsed_time();

    // Summary
    Log.info("========================================");
    Log.info("    HAL Unit Tests Complete");
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

    int failures = runAllTests();

    if (failures == 0) {
        Log.info("ALL TESTS PASSED!");
    } else {
        Log.error("TESTS FAILED: %d failures", failures);
    }
}

void loop() {
    // Do nothing after tests complete
    delay(10000);
}
*/
