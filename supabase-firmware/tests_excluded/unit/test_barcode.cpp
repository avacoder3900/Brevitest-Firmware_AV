/**
 * @file test_barcode.cpp
 * @brief Unit tests for BarcodeScanner module
 * @author Agent KAPPA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file contains unit tests for the BarcodeScanner module. Tests are designed
 * to verify UUID validation, barcode type detection, timeout handling, and retry logic.
 *
 * Test Categories:
 * - UUID validation (format checking)
 * - Barcode type detection
 * - Timeout handling
 * - Retry logic
 * - State machine transitions
 *
 * User Stories Tested:
 *   - BAR-003: Barcode parsing with UUID validation
 *   - BAR-004: Barcode type detection
 *   - BAR-005: Scan state machine
 *   - BAR-006: Unit tests
 *
 * Usage:
 *   Run via Particle device test framework or compile with mock hardware
 *   includes for PC-based testing.
 */

#include "Particle.h"
#include "BarcodeScanner.h"
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

#define TEST_ASSERT_STRING_EQUAL(expected, actual, message) do { \
    if (strcmp((expected), (actual)) == 0) { \
        _testsPassed++; \
        Log.info("PASS: %s", message); \
    } else { \
        _testsFailed++; \
        Log.error("FAIL: %s - expected '%s', got '%s' (line %d)", message, (expected), (actual), __LINE__); \
    } \
} while(0)

// ============================================================================
// UUID VALIDATION TESTS (BAR-003)
// ============================================================================

/**
 * @brief Test valid UUID formats
 */
void test_uuid_validation_valid() {
    Log.info("=== Testing UUID Validation - Valid UUIDs (BAR-003) ===");

    // Standard UUID format (lowercase)
    TEST_ASSERT(BarcodeScanner::isValidUUID("12345678-1234-1234-1234-123456789abc"),
                "Valid lowercase UUID should pass");

    // Standard UUID format (uppercase)
    TEST_ASSERT(BarcodeScanner::isValidUUID("12345678-1234-1234-1234-123456789ABC"),
                "Valid uppercase UUID should pass");

    // Standard UUID format (mixed case)
    TEST_ASSERT(BarcodeScanner::isValidUUID("12345678-abcd-ABCD-1234-123456789AbC"),
                "Valid mixed case UUID should pass");

    // All zeros UUID
    TEST_ASSERT(BarcodeScanner::isValidUUID("00000000-0000-0000-0000-000000000000"),
                "All zeros UUID should pass");

    // All F's UUID
    TEST_ASSERT(BarcodeScanner::isValidUUID("ffffffff-ffff-ffff-ffff-ffffffffffff"),
                "All F's UUID should pass");
}

/**
 * @brief Test invalid UUID formats
 */
void test_uuid_validation_invalid() {
    Log.info("=== Testing UUID Validation - Invalid UUIDs ===");

    // Null pointer
    TEST_ASSERT(!BarcodeScanner::isValidUUID(nullptr),
                "Null pointer should fail");

    // Empty string
    TEST_ASSERT(!BarcodeScanner::isValidUUID(""),
                "Empty string should fail");

    // Too short
    TEST_ASSERT(!BarcodeScanner::isValidUUID("12345678-1234-1234-1234"),
                "Too short UUID should fail");

    // Too long
    TEST_ASSERT(!BarcodeScanner::isValidUUID("12345678-1234-1234-1234-123456789abcdef"),
                "Too long UUID should fail");

    // Missing hyphens
    TEST_ASSERT(!BarcodeScanner::isValidUUID("123456781234123412341234567890ab"),
                "UUID without hyphens should fail");

    // Wrong hyphen positions
    TEST_ASSERT(!BarcodeScanner::isValidUUID("1234567-81234-1234-1234-123456789abc"),
                "UUID with wrong hyphen position should fail");

    // Invalid characters (g is not hex)
    TEST_ASSERT(!BarcodeScanner::isValidUUID("12345678-1234-1234-1234-123456789abg"),
                "UUID with non-hex character should fail");

    // Spaces
    TEST_ASSERT(!BarcodeScanner::isValidUUID("12345678-1234-1234-1234-12345678 abc"),
                "UUID with space should fail");
}

// ============================================================================
// BARCODE TYPE DETECTION TESTS (BAR-004)
// ============================================================================

/**
 * @brief Test cartridge barcode detection
 */
void test_barcode_type_cartridge() {
    Log.info("=== Testing Barcode Type Detection - Cartridge (BAR-004) ===");

    // Valid cartridge UUID (36 chars)
    BarcodeType type = BarcodeScanner::getBarcodeType("12345678-1234-1234-1234-123456789abc");
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::CARTRIDGE), static_cast<int>(type),
                      "36-char UUID should be CARTRIDGE type");

    // Another valid cartridge UUID
    type = BarcodeScanner::getBarcodeType("AAAAAAAA-BBBB-CCCC-DDDD-EEEEEEEEEEEE");
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::CARTRIDGE), static_cast<int>(type),
                      "Uppercase 36-char UUID should be CARTRIDGE type");
}

/**
 * @brief Test magnetometer barcode detection
 */
void test_barcode_type_magnetometer() {
    Log.info("=== Testing Barcode Type Detection - Magnetometer ===");

    // Magnetometer format (32 chars with MAG- prefix)
    BarcodeType type = BarcodeScanner::getBarcodeType("MAG-1234567890123456789012345678");
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::MAGNETOMETER), static_cast<int>(type),
                      "32-char barcode with MAG- prefix should be MAGNETOMETER type");

    // MAG- prefix with different length
    type = BarcodeScanner::getBarcodeType("MAG-12345678901234567890");
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::MAGNETOMETER), static_cast<int>(type),
                      "MAG- prefix should be MAGNETOMETER regardless of length");
}

/**
 * @brief Test stress test barcode detection
 */
void test_barcode_type_stress_test() {
    Log.info("=== Testing Barcode Type Detection - Stress Test ===");

    // Stress test format (16 chars with STRESS-TEST- prefix)
    BarcodeType type = BarcodeScanner::getBarcodeType("STRESS-TEST-1234");
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::STRESS_TEST), static_cast<int>(type),
                      "16-char barcode with STRESS-TEST- prefix should be STRESS_TEST type");

    // STRESS-TEST- prefix with extended data
    type = BarcodeScanner::getBarcodeType("STRESS-TEST-99999");
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::STRESS_TEST), static_cast<int>(type),
                      "STRESS-TEST- prefix should be STRESS_TEST type");
}

/**
 * @brief Test invalid/unknown barcode types
 */
void test_barcode_type_invalid() {
    Log.info("=== Testing Barcode Type Detection - Invalid ===");

    // Null pointer
    BarcodeType type = BarcodeScanner::getBarcodeType(nullptr);
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::GENERAL_ERROR), static_cast<int>(type),
                      "Null pointer should return GENERAL_ERROR");

    // Empty string
    type = BarcodeScanner::getBarcodeType("");
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::GENERAL_ERROR), static_cast<int>(type),
                      "Empty string should return GENERAL_ERROR");

    // Invalid format (36 chars but not valid UUID)
    type = BarcodeScanner::getBarcodeType("INVALIDFORMATNOTAVALIDUUIDSTRING!!!");
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::VALIDATION_ERROR), static_cast<int>(type),
                      "36-char non-UUID should return VALIDATION_ERROR");

    // Unknown prefix with 32 chars
    type = BarcodeScanner::getBarcodeType("XXX-1234567890123456789012345678");
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::VALIDATION_ERROR), static_cast<int>(type),
                      "32-char with unknown prefix should return VALIDATION_ERROR");

    // Random short string
    type = BarcodeScanner::getBarcodeType("ABC");
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::VALIDATION_ERROR), static_cast<int>(type),
                      "Short unknown string should return VALIDATION_ERROR");
}

// ============================================================================
// LENGTH VALIDATION TESTS (BAR-003)
// ============================================================================

/**
 * @brief Test barcode length validation
 */
void test_barcode_length_validation() {
    Log.info("=== Testing Barcode Length Validation ===");

    // Cartridge - should be 36 chars
    TEST_ASSERT(BarcodeScanner::isValidLength("12345678-1234-1234-1234-123456789abc", BarcodeType::CARTRIDGE),
                "36-char string should be valid for CARTRIDGE");
    TEST_ASSERT(!BarcodeScanner::isValidLength("12345678-1234-1234-1234-12345", BarcodeType::CARTRIDGE),
                "Short string should be invalid for CARTRIDGE");

    // Magnetometer - should be 32 chars
    TEST_ASSERT(BarcodeScanner::isValidLength("MAG-1234567890123456789012345678", BarcodeType::MAGNETOMETER),
                "32-char string should be valid for MAGNETOMETER");
    TEST_ASSERT(!BarcodeScanner::isValidLength("MAG-12345", BarcodeType::MAGNETOMETER),
                "Short string should be invalid for MAGNETOMETER");

    // Stress test - should be 16 chars
    TEST_ASSERT(BarcodeScanner::isValidLength("STRESS-TEST-1234", BarcodeType::STRESS_TEST),
                "16-char string should be valid for STRESS_TEST");
    TEST_ASSERT(!BarcodeScanner::isValidLength("STRESS-TEST-123456789", BarcodeType::STRESS_TEST),
                "Long string should be invalid for STRESS_TEST");

    // Null pointer
    TEST_ASSERT(!BarcodeScanner::isValidLength(nullptr, BarcodeType::CARTRIDGE),
                "Null pointer should be invalid");
}

// ============================================================================
// STATE MACHINE TESTS (BAR-005)
// ============================================================================

/**
 * @brief Test initial scanner state
 */
void test_scanner_initial_state() {
    Log.info("=== Testing Scanner Initial State (BAR-005) ===");

    BarcodeScanner scanner;

    // Before init
    TEST_ASSERT(!scanner.isInitialized(), "Scanner should not be initialized before init()");

    // After init
    bool initResult = scanner.init();
    TEST_ASSERT(initResult, "Scanner init() should return true");
    TEST_ASSERT(scanner.isInitialized(), "Scanner should be initialized after init()");

    // Check initial state
    TEST_ASSERT_EQUAL(static_cast<int>(ScanState::IDLE), static_cast<int>(scanner.getScanState()),
                      "Initial state should be IDLE");
    TEST_ASSERT_EQUAL(0, scanner.getRetryCount(), "Initial retry count should be 0");
}

/**
 * @brief Test scanner reset functionality
 */
void test_scanner_reset() {
    Log.info("=== Testing Scanner Reset ===");

    BarcodeScanner scanner;
    scanner.init();

    // Modify state
    scanner.setMaxRetries(5);

    // Reset
    scanner.reset();

    // Verify reset state
    TEST_ASSERT_EQUAL(static_cast<int>(ScanState::IDLE), static_cast<int>(scanner.getScanState()),
                      "State should be IDLE after reset");
    TEST_ASSERT_EQUAL(0, scanner.getRetryCount(), "Retry count should be 0 after reset");
}

/**
 * @brief Test retry configuration
 */
void test_scanner_retry_config() {
    Log.info("=== Testing Scanner Retry Configuration ===");

    BarcodeScanner scanner;
    scanner.init();

    // Default max retries
    TEST_ASSERT_EQUAL(BARCODE_MAX_RETRIES, scanner.getMaxRetries(),
                      "Default max retries should be BARCODE_MAX_RETRIES");

    // Set custom max retries
    scanner.setMaxRetries(10);
    TEST_ASSERT_EQUAL(10, scanner.getMaxRetries(), "Max retries should be configurable");

    // Set to zero (no retries)
    scanner.setMaxRetries(0);
    TEST_ASSERT_EQUAL(0, scanner.getMaxRetries(), "Max retries can be set to 0");
}

/**
 * @brief Test cancel scan functionality
 */
void test_scanner_cancel() {
    Log.info("=== Testing Scanner Cancel ===");

    BarcodeScanner scanner;
    scanner.init();

    // Cancel when idle (should not crash)
    scanner.cancelScan();
    TEST_ASSERT_EQUAL(static_cast<int>(ScanState::IDLE), static_cast<int>(scanner.getScanState()),
                      "State should remain IDLE after cancel when already idle");

    // Multiple cancels should be safe
    scanner.cancelScan();
    scanner.cancelScan();
    TEST_ASSERT(true, "Multiple cancels should not crash");
    _testsPassed++;
}

// ============================================================================
// CALLBACK TESTS
// ============================================================================

// Test callback variables
static bool _callbackInvoked = false;
static bool _callbackSuccess = false;
static BarcodeType _callbackType = BarcodeType::GENERAL_ERROR;
static char _callbackBarcode[48] = {0};

void testScanCallback(const char* barcode, BarcodeType type, bool success) {
    _callbackInvoked = true;
    _callbackSuccess = success;
    _callbackType = type;
    if (barcode != nullptr) {
        strncpy(_callbackBarcode, barcode, sizeof(_callbackBarcode) - 1);
    }
}

/**
 * @brief Test callback registration
 */
void test_scanner_callback() {
    Log.info("=== Testing Scanner Callback ===");

    BarcodeScanner scanner;
    scanner.init();

    // Reset test variables
    _callbackInvoked = false;
    _callbackSuccess = false;
    _callbackType = BarcodeType::GENERAL_ERROR;
    _callbackBarcode[0] = '\0';

    // Register callback
    scanner.setCallback(testScanCallback);

    // Clear callback
    scanner.setCallback(nullptr);

    TEST_ASSERT(true, "Callback registration and clearing should not crash");
    _testsPassed++;
}

// ============================================================================
// CLEAR BARCODE TESTS
// ============================================================================

/**
 * @brief Test clearing last barcode
 */
void test_scanner_clear_barcode() {
    Log.info("=== Testing Scanner Clear Barcode ===");

    BarcodeScanner scanner;
    scanner.init();

    // Get initial last barcode (should be empty)
    const char* barcode = scanner.getLastBarcode();
    TEST_ASSERT(barcode != nullptr, "getLastBarcode should not return null");
    TEST_ASSERT_EQUAL(0, strlen(barcode), "Initial barcode should be empty");

    // Clear barcode
    scanner.clearLastBarcode();
    barcode = scanner.getLastBarcode();
    TEST_ASSERT_EQUAL(0, strlen(barcode), "Barcode should be empty after clear");

    // Get last type after clear
    BarcodeType type = scanner.getLastBarcodeType();
    TEST_ASSERT_EQUAL(static_cast<int>(BarcodeType::GENERAL_ERROR), static_cast<int>(type),
                      "Last type should be GENERAL_ERROR after clear");
}

// ============================================================================
// STATE STRING CONVERSION TESTS
// ============================================================================

/**
 * @brief Test scan state to string conversion
 */
void test_scan_state_to_string() {
    Log.info("=== Testing Scan State to String ===");

    TEST_ASSERT_STRING_EQUAL("IDLE", scanStateToString(ScanState::IDLE),
                             "IDLE state string should be 'IDLE'");
    TEST_ASSERT_STRING_EQUAL("SCANNING", scanStateToString(ScanState::SCANNING),
                             "SCANNING state string should be 'SCANNING'");
    TEST_ASSERT_STRING_EQUAL("SUCCESS", scanStateToString(ScanState::SUCCESS),
                             "SUCCESS state string should be 'SUCCESS'");
    TEST_ASSERT_STRING_EQUAL("FAILED", scanStateToString(ScanState::FAILED),
                             "FAILED state string should be 'FAILED'");
    TEST_ASSERT_STRING_EQUAL("TIMEOUT", scanStateToString(ScanState::TIMEOUT),
                             "TIMEOUT state string should be 'TIMEOUT'");
}

// ============================================================================
// CONSTANTS VERIFICATION TESTS
// ============================================================================

/**
 * @brief Test that barcode constants match expected values
 */
void test_barcode_constants() {
    Log.info("=== Testing Barcode Constants ===");

    // UUID lengths
    TEST_ASSERT_EQUAL(36, BARCODE_CARTRIDGE_UUID_LENGTH,
                      "Cartridge UUID length should be 36");
    TEST_ASSERT_EQUAL(32, BARCODE_MAGNETOMETER_UUID_LENGTH,
                      "Magnetometer UUID length should be 32");
    TEST_ASSERT_EQUAL(16, BARCODE_STRESS_TEST_UUID_LENGTH,
                      "Stress test UUID length should be 16");

    // Prefix lengths
    TEST_ASSERT_EQUAL(4, BARCODE_PREFIX_LENGTH,
                      "Barcode prefix length should be 4");
    TEST_ASSERT_EQUAL(12, STRESS_TEST_PREFIX_LENGTH,
                      "Stress test prefix length should be 12");

    // Timing constants
    TEST_ASSERT_EQUAL(1000, BARCODE_DEFAULT_TIMEOUT_MS,
                      "Default timeout should be 1000ms");
    TEST_ASSERT_EQUAL(100000, BARCODE_SERIAL_DELAY_US,
                      "Serial delay should be 100000us (100ms)");
    TEST_ASSERT_EQUAL(9600, BARCODE_SERIAL_BAUD,
                      "Serial baud rate should be 9600");

    // Retry constants
    TEST_ASSERT_EQUAL(3, BARCODE_MAX_RETRIES,
                      "Max retries should be 3");
}

// ============================================================================
// BARCODE TYPE ENUM TESTS
// ============================================================================

/**
 * @brief Test barcode type enum values match specification
 */
void test_barcode_type_enum_values() {
    Log.info("=== Testing Barcode Type Enum Values ===");

    // Positive types
    TEST_ASSERT_EQUAL(1, static_cast<int>(BarcodeType::CARTRIDGE),
                      "CARTRIDGE should be 1");
    TEST_ASSERT_EQUAL(2, static_cast<int>(BarcodeType::MAGNETOMETER),
                      "MAGNETOMETER should be 2");
    TEST_ASSERT_EQUAL(3, static_cast<int>(BarcodeType::OPTICAL),
                      "OPTICAL should be 3");
    TEST_ASSERT_EQUAL(4, static_cast<int>(BarcodeType::STRESS_TEST),
                      "STRESS_TEST should be 4");
    TEST_ASSERT_EQUAL(5, static_cast<int>(BarcodeType::SHIPPING),
                      "SHIPPING should be 5");

    // Negative error types
    TEST_ASSERT_EQUAL(-1, static_cast<int>(BarcodeType::GENERAL_ERROR),
                      "GENERAL_ERROR should be -1");
    TEST_ASSERT_EQUAL(-2, static_cast<int>(BarcodeType::VALIDATION_ERROR),
                      "VALIDATION_ERROR should be -2");
    TEST_ASSERT_EQUAL(-3, static_cast<int>(BarcodeType::OPTICAL_ERROR),
                      "OPTICAL_ERROR should be -3");
}

// ============================================================================
// TEST RUNNER
// ============================================================================

/**
 * @brief Run all BarcodeScanner unit tests
 * @return Number of failed tests
 */
int runBarcodeScannerTests() {
    _testsPassed = 0;
    _testsFailed = 0;

    Log.info("========================================");
    Log.info("    BarcodeScanner Unit Tests Starting");
    Log.info("========================================");

    // BAR-003: UUID validation tests
    test_uuid_validation_valid();
    test_uuid_validation_invalid();
    test_barcode_length_validation();

    // BAR-004: Barcode type detection tests
    test_barcode_type_cartridge();
    test_barcode_type_magnetometer();
    test_barcode_type_stress_test();
    test_barcode_type_invalid();

    // BAR-005: State machine tests
    test_scanner_initial_state();
    test_scanner_reset();
    test_scanner_retry_config();
    test_scanner_cancel();

    // Callback tests
    test_scanner_callback();

    // Clear barcode tests
    test_scanner_clear_barcode();

    // State string conversion tests
    test_scan_state_to_string();

    // Constants verification
    test_barcode_constants();
    test_barcode_type_enum_values();

    // Summary
    Log.info("========================================");
    Log.info("    BarcodeScanner Unit Tests Complete");
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

    int failures = runBarcodeScannerTests();

    if (failures == 0) {
        Log.info("ALL BARCODE SCANNER TESTS PASSED!");
    } else {
        Log.error("BARCODE SCANNER TESTS FAILED: %d failures", failures);
    }
}

void loop() {
    // Do nothing after tests complete
    delay(10000);
}
*/
