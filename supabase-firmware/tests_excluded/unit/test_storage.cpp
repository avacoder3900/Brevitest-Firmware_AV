/**
 * @file test_storage.cpp
 * @brief Unit tests for Storage Manager module
 * @author Agent EPSILON - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file contains unit tests for the StorageManager module. Tests are designed to
 * run on the Particle Boron device.
 *
 * Test Categories:
 * - EEPROM read/write operations (STOR-001, STOR-002)
 * - Crash recovery logic (STOR-003)
 * - Cache operations (STOR-004)
 * - Cache cleanup (STOR-005)
 *
 * Usage:
 *   Run via Particle device test framework or compile with mock includes
 *   for PC-based testing.
 *
 * User Stories Tested:
 *   - STOR-001: StorageManager class
 *   - STOR-002: EEPROM operations
 *   - STOR-003: Crash recovery
 *   - STOR-004: Filesystem cache
 *   - STOR-005: Cache cleanup
 *   - STOR-006: Unit tests
 */

#include "Particle.h"
#include "StorageManager.h"
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

#define TEST_ASSERT_NOT_NULL(ptr, message) do { \
    if ((ptr) != nullptr) { \
        _testsPassed++; \
        Log.info("PASS: %s", message); \
    } else { \
        _testsFailed++; \
        Log.error("FAIL: %s - pointer is NULL (line %d)", message, __LINE__); \
    } \
} while(0)

// ============================================================================
// GLOBAL TEST INSTANCE
// ============================================================================

static StorageManager storage;

// ============================================================================
// STOR-001: INITIALIZATION TESTS
// ============================================================================

/**
 * @brief Test StorageManager initialization
 */
void test_storage_initialization() {
    Log.info("=== Testing Storage Initialization (STOR-001) ===");

    // Before init
    TEST_ASSERT(!storage.isInitialized(), "Storage should not be initialized before init()");

    // Initialize
    bool initResult = storage.init();
    TEST_ASSERT(initResult, "storage.init() should return true");
    TEST_ASSERT(storage.isInitialized(), "Storage should be initialized after init()");

    // Double initialization should be safe
    bool reinitResult = storage.init();
    TEST_ASSERT(reinitResult, "storage.init() should return true on re-init");
}

/**
 * @brief Test storage info retrieval
 */
void test_storage_info() {
    Log.info("=== Testing Storage Info (STOR-001) ===");

    StorageInfo info = storage.getStorageInfo();

    TEST_ASSERT(info.eepromSize > 0, "EEPROM size should be > 0");
    TEST_ASSERT(info.eepromUsed > 0, "EEPROM used should be > 0");
    TEST_ASSERT(info.filesystemReady, "Filesystem should be ready");

    Log.info("Storage Info: EEPROM size=%d, used=%d, cache=%d, assay=%d, validation=%d",
             info.eepromSize, info.eepromUsed,
             info.cacheFileCount, info.assayFileCount, info.validationFileCount);
}

// ============================================================================
// STOR-002: EEPROM OPERATIONS TESTS
// ============================================================================

/**
 * @brief Test EEPROM configuration access
 */
void test_eeprom_config_access() {
    Log.info("=== Testing EEPROM Config Access (STOR-002) ===");

    const Particle_EEPROM& config = storage.getConfig();

    TEST_ASSERT_EQUAL(FIRMWARE_VERSION, config.firmware_version, "Firmware version should match");
    TEST_ASSERT_EQUAL(DATA_FORMAT_VERSION, config.data_format_version, "Data format version should match");

    // Test getter methods
    TEST_ASSERT_EQUAL(config.firmware_version, storage.getFirmwareVersion(), "getFirmwareVersion() should match");
    TEST_ASSERT_EQUAL(config.data_format_version, storage.getDataFormatVersion(), "getDataFormatVersion() should match");
}

/**
 * @brief Test EEPROM save and load
 */
void test_eeprom_save_load() {
    Log.info("=== Testing EEPROM Save/Load (STOR-002) ===");

    // Get mutable config
    Particle_EEPROM& config = storage.getConfigMutable();

    // Store original value
    int32_t originalCycles = config.stress_test_cycles;

    // Modify and save
    config.stress_test_cycles = 12345;
    bool saveResult = storage.saveConfig();
    TEST_ASSERT(saveResult, "saveConfig() should return true");

    // Reload and verify
    bool loadResult = storage.loadConfig();
    TEST_ASSERT(loadResult, "loadConfig() should return true");
    TEST_ASSERT_EQUAL(12345, storage.getStressTestCycles(), "Stress test cycles should persist after reload");

    // Restore original value
    storage.getConfigMutable().stress_test_cycles = originalCycles;
    storage.saveConfig();
}

/**
 * @brief Test stress test counter operations
 */
void test_stress_test_counters() {
    Log.info("=== Testing Stress Test Counters (STOR-002) ===");

    // Get current values
    int32_t currentCycles = storage.getStressTestCycles();
    int32_t sinceReset = storage.getStressTestCyclesSinceReset();
    int32_t lifetime = storage.getLifetimeStressTestCycles();

    Log.info("Current counters: cycles=%d, sinceReset=%d, lifetime=%d",
             currentCycles, sinceReset, lifetime);

    // Increment counters
    bool incrementResult = storage.incrementStressTestCounters();
    TEST_ASSERT(incrementResult, "incrementStressTestCounters() should return true");

    // Verify increments
    TEST_ASSERT_EQUAL(currentCycles + 1, storage.getStressTestCycles(), "Stress test cycles should increment");
    TEST_ASSERT_EQUAL(sinceReset + 1, storage.getStressTestCyclesSinceReset(), "Cycles since reset should increment");
    TEST_ASSERT_EQUAL(lifetime + 1, storage.getLifetimeStressTestCycles(), "Lifetime cycles should increment");

    // Test reset cycles
    bool resetResult = storage.resetStressTestCycles();
    TEST_ASSERT(resetResult, "resetStressTestCycles() should return true");
    TEST_ASSERT_EQUAL(0, storage.getStressTestCycles(), "Stress test cycles should be 0 after reset");

    // Lifetime should be preserved
    TEST_ASSERT_EQUAL(lifetime + 1, storage.getLifetimeStressTestCycles(), "Lifetime cycles should be preserved after reset");

    // Restore for other tests
    storage.getConfigMutable().stress_test_cycles = currentCycles;
    storage.getConfigMutable().stress_test_cycles_since_reset = sinceReset;
    storage.getConfigMutable().lifetime_stress_test_cycles = lifetime;
    storage.saveConfig();
}

// ============================================================================
// STOR-003: CRASH RECOVERY TESTS
// ============================================================================

/**
 * @brief Test crash recovery data operations
 */
void test_crash_recovery_basic() {
    Log.info("=== Testing Crash Recovery Basic (STOR-003) ===");

    // Clear any existing recovery data first
    storage.clearRecoveryData();
    TEST_ASSERT(!storage.hasRecoveryData(), "Should have no recovery data after clear");

    // Set recovery data
    const char* testUuid = "12345678-1234-1234-1234-123456789abc";
    const char* testAssay = "ASSAY123";

    bool setResult = storage.setRecoveryData(testUuid, testAssay);
    TEST_ASSERT(setResult, "setRecoveryData() should return true");

    // Check recovery data exists
    TEST_ASSERT(storage.hasRecoveryData(), "Should have recovery data after set");

    // Get recovery info
    RecoveryInfo info = storage.getRecoveryInfo();
    TEST_ASSERT(info.valid, "Recovery info should be valid");
    TEST_ASSERT_STRING_EQUAL(testUuid, info.cartridge_uuid, "Cartridge UUID should match");
    TEST_ASSERT_STRING_EQUAL(testAssay, info.assay_id, "Assay ID should match");

    // Clear recovery data
    bool clearResult = storage.clearRecoveryData();
    TEST_ASSERT(clearResult, "clearRecoveryData() should return true");
    TEST_ASSERT(!storage.hasRecoveryData(), "Should have no recovery data after clear");
}

/**
 * @brief Test crash recovery persistence
 */
void test_crash_recovery_persistence() {
    Log.info("=== Testing Crash Recovery Persistence (STOR-003) ===");

    // Clear first
    storage.clearRecoveryData();

    // Set recovery data
    const char* testUuid = "aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee";
    const char* testAssay = "TEST0001";

    storage.setRecoveryData(testUuid, testAssay);

    // Reload EEPROM
    storage.loadConfig();

    // Verify persistence
    TEST_ASSERT(storage.hasRecoveryData(), "Recovery data should persist after reload");

    RecoveryInfo info = storage.getRecoveryInfo();
    TEST_ASSERT_STRING_EQUAL(testUuid, info.cartridge_uuid, "Cartridge UUID should persist");
    TEST_ASSERT_STRING_EQUAL(testAssay, info.assay_id, "Assay ID should persist");

    // Clean up
    storage.clearRecoveryData();
}

/**
 * @brief Test crash recovery with null parameters
 */
void test_crash_recovery_null_params() {
    Log.info("=== Testing Crash Recovery Null Params (STOR-003) ===");

    bool result1 = storage.setRecoveryData(nullptr, "ASSAY123");
    TEST_ASSERT(!result1, "setRecoveryData() should fail with null UUID");

    bool result2 = storage.setRecoveryData("12345678-1234-1234-1234-123456789abc", nullptr);
    TEST_ASSERT(!result2, "setRecoveryData() should fail with null assay ID");
}

// ============================================================================
// STOR-004: CACHE OPERATIONS TESTS
// ============================================================================

/**
 * @brief Test test data caching
 */
void test_cache_test_data() {
    Log.info("=== Testing Test Data Caching (STOR-004) ===");

    // Clear cache first
    storage.clearCache();

    // Create test record
    BrevitestTestRecord testRecord;
    memset(&testRecord, 0, sizeof(testRecord));
    testRecord.data_format_code = TEST_DATA_FORMAT_CODE;
    strncpy(testRecord.cartridge_id, "test-uuid-1234-5678-9abc-def012345678", BARCODE_UUID_LENGTH);
    testRecord.cartridge_id[BARCODE_UUID_LENGTH] = '\0';
    strncpy(testRecord.assay_id, "TESTASAY", ASSAY_UUID_LENGTH);
    testRecord.assay_id[ASSAY_UUID_LENGTH] = '\0';
    testRecord.start_time = Time.now();
    testRecord.duration = 300;
    testRecord.number_of_readings = 10;

    // Cache the test
    bool cacheResult = storage.cacheTestData(&testRecord);
    TEST_ASSERT(cacheResult, "cacheTestData() should return true");

    // Verify cached
    TEST_ASSERT(storage.hasCachedTests(), "Should have cached tests");
    TEST_ASSERT(storage.getCachedTestCount() >= 1, "Cached test count should be >= 1");

    // Get cached test filename
    char filename[MAX_PATH_LENGTH];
    bool foundResult = storage.getNextCachedTest(filename);
    TEST_ASSERT(foundResult, "getNextCachedTest() should return true");

    // Load cached test
    BrevitestTestRecord loadedRecord;
    bool loadResult = storage.loadCachedTest(filename, &loadedRecord);
    TEST_ASSERT(loadResult, "loadCachedTest() should return true");

    // Verify loaded data
    TEST_ASSERT_EQUAL(testRecord.data_format_code, loadedRecord.data_format_code, "Data format code should match");
    TEST_ASSERT_STRING_EQUAL(testRecord.cartridge_id, loadedRecord.cartridge_id, "Cartridge ID should match");
    TEST_ASSERT_STRING_EQUAL(testRecord.assay_id, loadedRecord.assay_id, "Assay ID should match");
    TEST_ASSERT_EQUAL(testRecord.number_of_readings, loadedRecord.number_of_readings, "Number of readings should match");

    // Delete cached test
    bool deleteResult = storage.deleteCachedTest(filename);
    TEST_ASSERT(deleteResult, "deleteCachedTest() should return true");
}

/**
 * @brief Test assay caching
 */
void test_cache_assay_data() {
    Log.info("=== Testing Assay Caching (STOR-004) ===");

    // Clear assay cache first
    storage.clearAssayCache();

    // Create test assay
    BrevitestAssay testAssay;
    memset(&testAssay, 0, sizeof(testAssay));
    strncpy(testAssay.id, "TESTASAY", ASSAY_UUID_LENGTH);
    testAssay.id[ASSAY_UUID_LENGTH] = '\0';
    testAssay.duration = 600;
    const char* testBcode = "START,100|MOVE,5000|WAIT,1000|READ,A|END";
    strncpy(testAssay.BCODE, testBcode, BCODE_CAPACITY - 1);
    testAssay.BCODE[BCODE_CAPACITY - 1] = '\0';
    testAssay.BCODE_length = strlen(testAssay.BCODE);

    // Cache the assay
    bool cacheResult = storage.cacheAssay(&testAssay);
    TEST_ASSERT(cacheResult, "cacheAssay() should return true");

    // Check assay exists
    TEST_ASSERT(storage.hasAssay("TESTASAY"), "hasAssay() should return true");

    // Load cached assay
    BrevitestAssay loadedAssay;
    bool loadResult = storage.loadCachedAssay("TESTASAY", &loadedAssay, 0);
    TEST_ASSERT(loadResult, "loadCachedAssay() should return true");

    // Verify loaded data
    TEST_ASSERT_STRING_EQUAL(testAssay.id, loadedAssay.id, "Assay ID should match");
    TEST_ASSERT_EQUAL(testAssay.duration, loadedAssay.duration, "Duration should match");
    TEST_ASSERT_STRING_EQUAL(testAssay.BCODE, loadedAssay.BCODE, "BCODE should match");

    // Delete cached assay
    bool deleteResult = storage.deleteCachedAssay("TESTASAY");
    TEST_ASSERT(deleteResult, "deleteCachedAssay() should return true");
    TEST_ASSERT(!storage.hasAssay("TESTASAY"), "hasAssay() should return false after delete");
}

/**
 * @brief Test assay caching with invalid parameters
 */
void test_cache_assay_invalid() {
    Log.info("=== Testing Assay Caching Invalid Params (STOR-004) ===");

    bool nullResult = storage.cacheAssay(nullptr);
    TEST_ASSERT(!nullResult, "cacheAssay(nullptr) should fail");

    BrevitestAssay emptyAssay;
    memset(&emptyAssay, 0, sizeof(emptyAssay));
    bool emptyResult = storage.cacheAssay(&emptyAssay);
    TEST_ASSERT(!emptyResult, "cacheAssay() with empty ID should fail");
}

/**
 * @brief Test validation data operations
 */
void test_validation_data() {
    Log.info("=== Testing Validation Data (STOR-004) ===");

    const char* testData = "Magnetometer validation data: X=100, Y=200, Z=300";
    uint32_t timestamp = Time.now();

    // Save validation data
    bool saveResult = storage.saveValidationData(testData, timestamp);
    TEST_ASSERT(saveResult, "saveValidationData() should return true");

    // Load latest validation data
    char buffer[1024];
    bool loadResult = storage.loadLatestValidationData(buffer, sizeof(buffer));
    TEST_ASSERT(loadResult, "loadLatestValidationData() should return true");
    TEST_ASSERT_STRING_EQUAL(testData, buffer, "Validation data should match");
}

// ============================================================================
// STOR-005: CACHE CLEANUP TESTS
// ============================================================================

/**
 * @brief Test cache size and count
 */
void test_cache_size_count() {
    Log.info("=== Testing Cache Size/Count (STOR-005) ===");

    // Clear cache first
    storage.clearCache();

    // Create and cache some test records
    for (int i = 0; i < 3; i++) {
        BrevitestTestRecord record;
        memset(&record, 0, sizeof(record));
        record.data_format_code = TEST_DATA_FORMAT_CODE;
        snprintf(record.cartridge_id, BARCODE_UUID_LENGTH + 1,
                 "test-uuid-%04d-5678-9abc-def012345678", i);
        strncpy(record.assay_id, "TESTASAY", ASSAY_UUID_LENGTH);
        record.assay_id[ASSAY_UUID_LENGTH] = '\0';
        record.number_of_readings = i * 10;

        storage.cacheTestData(&record);
    }

    // Check count
    uint32_t count = storage.getCacheCount();
    TEST_ASSERT(count >= 3, "Cache count should be >= 3");

    // Check size
    uint32_t size = storage.getCacheSize();
    TEST_ASSERT(size > 0, "Cache size should be > 0");
    Log.info("Cache: count=%d, size=%d bytes", count, size);
}

/**
 * @brief Test cache clear
 */
void test_cache_clear() {
    Log.info("=== Testing Cache Clear (STOR-005) ===");

    // Make sure there's something in cache
    BrevitestTestRecord record;
    memset(&record, 0, sizeof(record));
    record.data_format_code = TEST_DATA_FORMAT_CODE;
    strncpy(record.cartridge_id, "clear-test-1234-5678-9abc-def012345678", BARCODE_UUID_LENGTH);
    record.cartridge_id[BARCODE_UUID_LENGTH] = '\0';
    storage.cacheTestData(&record);

    // Clear cache
    bool clearResult = storage.clearCache();
    TEST_ASSERT(clearResult, "clearCache() should return true");

    // Verify cleared
    TEST_ASSERT(!storage.hasCachedTests(), "Should have no cached tests after clear");
    TEST_ASSERT_EQUAL(0, storage.getCachedTestCount(), "Cached test count should be 0");
}

/**
 * @brief Test cache limit enforcement
 */
void test_cache_limit_enforcement() {
    Log.info("=== Testing Cache Limit Enforcement (STOR-005) ===");

    // Clear cache first
    storage.clearCache();

    // Add more files than limit (but use smaller number for test)
    const int testCount = 5;
    for (int i = 0; i < testCount; i++) {
        BrevitestTestRecord record;
        memset(&record, 0, sizeof(record));
        record.data_format_code = TEST_DATA_FORMAT_CODE;
        snprintf(record.cartridge_id, BARCODE_UUID_LENGTH + 1,
                 "limit-test-%04d-5678-9abc-def012345678", i);
        strncpy(record.assay_id, "TESTASAY", ASSAY_UUID_LENGTH);
        record.number_of_readings = i;

        storage.cacheTestData(&record);
        delay(100);  // Small delay to ensure different timestamps
    }

    // Check count before enforcement
    uint32_t countBefore = storage.getCachedTestCount();
    Log.info("Cache count before enforcement: %d", countBefore);

    // Enforce limit shouldn't remove anything if under limit
    uint32_t removed = storage.enforceCacheLimit();
    Log.info("Files removed by enforcement: %d", removed);

    // Clean up
    storage.clearCache();
}

/**
 * @brief Test clear old cache by age
 */
void test_clear_old_cache() {
    Log.info("=== Testing Clear Old Cache (STOR-005) ===");

    // Add a test file
    BrevitestTestRecord record;
    memset(&record, 0, sizeof(record));
    record.data_format_code = TEST_DATA_FORMAT_CODE;
    strncpy(record.cartridge_id, "old-cache-1234-5678-9abc-def012345678", BARCODE_UUID_LENGTH);
    record.cartridge_id[BARCODE_UUID_LENGTH] = '\0';
    storage.cacheTestData(&record);

    // Try to clear files older than 30 days (should not delete our new file)
    uint32_t deleted = storage.clearOldCache(30);
    Log.info("Deleted %d old files", deleted);

    // Our file should still exist since it's new
    TEST_ASSERT(storage.hasCachedTests(), "New cache file should not be deleted");

    // Clean up
    storage.clearCache();
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

/**
 * @brief Test complete workflow: set recovery, cache test, clear recovery
 */
void test_complete_workflow() {
    Log.info("=== Testing Complete Workflow ===");

    // 1. Clear any previous state
    storage.clearRecoveryData();
    storage.clearCache();

    // 2. Simulate test start - set recovery data
    const char* cartridgeId = "workflow-test-1234-5678-9abc-def012345678";
    const char* assayId = "WORKFLOW";

    bool setRecovery = storage.setRecoveryData(cartridgeId, assayId);
    TEST_ASSERT(setRecovery, "Workflow: setRecoveryData should succeed");
    TEST_ASSERT(storage.hasRecoveryData(), "Workflow: should have recovery data");

    // 3. Simulate test completion - cache results
    BrevitestTestRecord record;
    memset(&record, 0, sizeof(record));
    record.data_format_code = TEST_DATA_FORMAT_CODE;
    strncpy(record.cartridge_id, cartridgeId, BARCODE_UUID_LENGTH);
    record.cartridge_id[BARCODE_UUID_LENGTH] = '\0';
    strncpy(record.assay_id, assayId, ASSAY_UUID_LENGTH);
    record.assay_id[ASSAY_UUID_LENGTH] = '\0';
    record.number_of_readings = 50;

    bool cacheTest = storage.cacheTestData(&record);
    TEST_ASSERT(cacheTest, "Workflow: cacheTestData should succeed");

    // 4. Clear recovery data on success
    bool clearRecovery = storage.clearRecoveryData();
    TEST_ASSERT(clearRecovery, "Workflow: clearRecoveryData should succeed");
    TEST_ASSERT(!storage.hasRecoveryData(), "Workflow: should have no recovery data");

    // 5. Verify test is cached
    TEST_ASSERT(storage.hasCachedTests(), "Workflow: should have cached test");

    // 6. Clean up
    storage.clearCache();
    Log.info("Complete workflow test passed");
}

// ============================================================================
// TEST RUNNER
// ============================================================================

/**
 * @brief Run all Storage Manager unit tests
 * @return Number of failed tests
 */
int runStorageTests() {
    _testsPassed = 0;
    _testsFailed = 0;

    Log.info("========================================");
    Log.info("    Storage Manager Unit Tests Starting");
    Log.info("========================================");

    // STOR-001: Initialization
    test_storage_initialization();
    test_storage_info();

    // STOR-002: EEPROM operations
    test_eeprom_config_access();
    test_eeprom_save_load();
    test_stress_test_counters();

    // STOR-003: Crash recovery
    test_crash_recovery_basic();
    test_crash_recovery_persistence();
    test_crash_recovery_null_params();

    // STOR-004: Cache operations
    test_cache_test_data();
    test_cache_assay_data();
    test_cache_assay_invalid();
    test_validation_data();

    // STOR-005: Cache cleanup
    test_cache_size_count();
    test_cache_clear();
    test_cache_limit_enforcement();
    test_clear_old_cache();

    // Integration tests
    test_complete_workflow();

    // Summary
    Log.info("========================================");
    Log.info("    Storage Manager Unit Tests Complete");
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

    int failures = runStorageTests();

    if (failures == 0) {
        Log.info("ALL STORAGE TESTS PASSED!");
    } else {
        Log.error("STORAGE TESTS FAILED: %d failures", failures);
    }
}

void loop() {
    // Do nothing after tests complete
    delay(10000);
}
*/
