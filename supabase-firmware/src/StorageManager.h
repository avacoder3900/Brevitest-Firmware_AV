/**
 * @file StorageManager.h
 * @brief EEPROM and filesystem management for Brevitest firmware
 * @author Agent EPSILON - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module provides persistent storage management including:
 * - EEPROM operations for configuration and crash recovery
 * - Filesystem cache for failed uploads and assay data
 * - Crash recovery detection and handling
 * - Cache management with FIFO eviction
 *
 * Storage Layout:
 *   EEPROM (Address 0): Particle_EEPROM structure
 *     - Firmware version, data format version
 *     - Stress test counters (lifetime, since reset, current)
 *     - Running test UUID/assay ID for crash recovery
 *
 *   Filesystem Directories:
 *     /cache/      - Failed upload cache (max 50 files, FIFO eviction)
 *     /assay/      - Downloaded assay cache (max 50 files)
 *     /validation/ - Magnetometer validation data
 *
 * Usage:
 *   StorageManager storage;
 *   storage.init();  // Call once in setup()
 *
 *   // Check for crash recovery
 *   if (storage.hasRecoveryData()) {
 *       RecoveryInfo info = storage.getRecoveryInfo();
 *       // Handle recovery...
 *   }
 *
 * User Stories Implemented:
 *   - STOR-001: StorageManager class
 *   - STOR-002: EEPROM operations
 *   - STOR-003: Crash recovery
 *   - STOR-004: Filesystem cache
 *   - STOR-005: Cache cleanup
 */

#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "Particle.h"
#include "DataTypes.h"
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>

// ============================================================================
// STORAGE CONSTANTS
// ============================================================================

/** @brief Maximum number of cached test files */
constexpr uint8_t CACHE_MAX_FILES = 50;

/** @brief Maximum number of cached assay files */
constexpr uint8_t ASSAY_MAX_FILES = 50;

/** @brief Maximum number of magnetometer validation files */
constexpr uint8_t VALIDATION_MAX_FILES = 50;

/** @brief Maximum path length for file operations */
constexpr size_t MAX_PATH_LENGTH = 128;

/** @brief Cache directory path */
constexpr const char* CACHE_DIR = "/cache";

/** @brief Assay directory path */
constexpr const char* ASSAY_DIR = "/assay";

/** @brief Validation directory path */
constexpr const char* VALIDATION_DIR = "/validation";

/** @brief Buffer directory path (for temporary files) */
constexpr const char* BUFFER_DIR = "/buffer";

/** @brief Assay data buffer size */
constexpr size_t ASSAY_BUFFER_SIZE = 6000;

// ============================================================================
// RECOVERY DATA STRUCTURE
// ============================================================================

/**
 * @brief Information about an interrupted test for crash recovery
 */
struct RecoveryInfo {
    char cartridge_uuid[BARCODE_UUID_LENGTH + 1];  ///< UUID of interrupted test
    char assay_id[ASSAY_UUID_LENGTH + 1];          ///< Assay ID of interrupted test
    bool valid;                                      ///< True if recovery data is valid

    /** @brief Default constructor */
    RecoveryInfo() : valid(false) {
        cartridge_uuid[0] = '\0';
        assay_id[0] = '\0';
    }
};

/**
 * @brief Storage capacity and usage information
 */
struct StorageInfo {
    uint32_t eepromSize;         ///< Total EEPROM size in bytes
    uint32_t eepromUsed;         ///< EEPROM bytes used by structure
    uint32_t cacheFileCount;     ///< Number of files in cache directory
    uint32_t assayFileCount;     ///< Number of files in assay directory
    uint32_t validationFileCount;///< Number of files in validation directory
    bool eepromValid;            ///< True if EEPROM data is valid
    bool filesystemReady;        ///< True if filesystem is mounted

    /** @brief Default constructor */
    StorageInfo() :
        eepromSize(0), eepromUsed(0),
        cacheFileCount(0), assayFileCount(0), validationFileCount(0),
        eepromValid(false), filesystemReady(false) {}
};

/**
 * @brief Cache file entry with metadata
 */
struct CacheFileEntry {
    char filename[MAX_PATH_LENGTH];  ///< Full path to file
    uint32_t size;                   ///< File size in bytes
    uint32_t timestamp;              ///< File modification time (Unix timestamp)

    /** @brief Default constructor */
    CacheFileEntry() : size(0), timestamp(0) {
        filename[0] = '\0';
    }
};

// ============================================================================
// STORAGE MANAGER CLASS
// ============================================================================

/**
 * @class StorageManager
 * @brief Manages EEPROM and filesystem storage operations
 *
 * Provides a unified interface for all persistent storage operations including
 * EEPROM configuration, filesystem caching, and crash recovery handling.
 *
 * Thread Safety: Not thread-safe. Should be called from main loop only.
 */
class StorageManager {
public:
    // ========================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ========================================================================

    /**
     * @brief Constructor
     */
    StorageManager();

    /**
     * @brief Destructor
     */
    ~StorageManager();

    // ========================================================================
    // INITIALIZATION (STOR-001)
    // ========================================================================

    /**
     * @brief Initialize storage manager
     *
     * Performs the following initialization steps:
     * 1. Load EEPROM configuration (reset if invalid)
     * 2. Create filesystem directories if needed
     * 3. Check for crash recovery data
     *
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Check if storage manager is initialized
     * @return true if init() has been called successfully
     */
    bool isInitialized() const;

    /**
     * @brief Get storage capacity and usage information
     * @return StorageInfo structure with capacity details
     */
    StorageInfo getStorageInfo();

    // ========================================================================
    // EEPROM OPERATIONS (STOR-002)
    // ========================================================================

    /**
     * @brief Load configuration from EEPROM
     *
     * Reads EEPROM data into internal structure. If EEPROM is empty (0xFF)
     * or version mismatch detected, resets to defaults.
     *
     * @return true if load successful (or reset performed)
     */
    bool loadConfig();

    /**
     * @brief Save configuration to EEPROM
     * @return true if save successful
     */
    bool saveConfig();

    /**
     * @brief Reset EEPROM to default values
     *
     * Preserves lifetime_stress_test_cycles counter.
     *
     * @return true if reset successful
     */
    bool resetConfig();

    /**
     * @brief Get current EEPROM configuration
     * @return Reference to internal EEPROM structure
     */
    const Particle_EEPROM& getConfig() const;

    /**
     * @brief Get mutable reference to EEPROM configuration
     *
     * Use this to modify configuration, then call saveConfig().
     *
     * @return Mutable reference to internal EEPROM structure
     */
    Particle_EEPROM& getConfigMutable();

    /**
     * @brief Get firmware version from EEPROM
     * @return Firmware version number
     */
    uint8_t getFirmwareVersion() const;

    /**
     * @brief Get data format version from EEPROM
     * @return Data format version number
     */
    uint8_t getDataFormatVersion() const;

    // ========================================================================
    // STRESS TEST COUNTERS (STOR-002)
    // ========================================================================

    /**
     * @brief Get lifetime stress test cycles
     * @return Total stress test cycles ever run
     */
    int32_t getLifetimeStressTestCycles() const;

    /**
     * @brief Get stress test cycles since reset
     * @return Cycles since last counter reset
     */
    int32_t getStressTestCyclesSinceReset() const;

    /**
     * @brief Get current stress test cycles
     * @return Current stress test cycle count
     */
    int32_t getStressTestCycles() const;

    /**
     * @brief Increment stress test counters
     *
     * Increments all three counters and saves to EEPROM.
     *
     * @return true if save successful
     */
    bool incrementStressTestCounters();

    /**
     * @brief Reset stress test cycles (since reset counter)
     *
     * Resets stress_test_cycles_since_reset to 0.
     *
     * @return true if save successful
     */
    bool resetStressTestCyclesSinceReset();

    /**
     * @brief Reset current stress test cycles
     *
     * Resets stress_test_cycles to 0.
     *
     * @return true if save successful
     */
    bool resetStressTestCycles();

    // ========================================================================
    // CRASH RECOVERY (STOR-003)
    // ========================================================================

    /**
     * @brief Check if recovery data exists
     * @return true if there is an incomplete test to recover
     */
    bool hasRecoveryData() const;

    /**
     * @brief Get recovery information
     * @return RecoveryInfo structure with test details
     */
    RecoveryInfo getRecoveryInfo() const;

    /**
     * @brief Set recovery data for running test
     *
     * Call this when starting a test to enable crash recovery.
     *
     * @param cartridgeUuid Cartridge UUID (must be BARCODE_UUID_LENGTH chars)
     * @param assayId Assay ID (must be ASSAY_UUID_LENGTH chars)
     * @return true if save successful
     */
    bool setRecoveryData(const char* cartridgeUuid, const char* assayId);

    /**
     * @brief Clear recovery data
     *
     * Call this when test completes successfully.
     *
     * @return true if clear successful
     */
    bool clearRecoveryData();

    // ========================================================================
    // TEST CACHE OPERATIONS (STOR-004)
    // ========================================================================

    /**
     * @brief Cache test record to filesystem
     *
     * Saves test record to /cache/ directory using cartridge_id as filename.
     * If cache is full, removes oldest file first (FIFO).
     *
     * @param record Test record to cache
     * @return true if cache successful
     */
    bool cacheTestData(const BrevitestTestRecord* record);

    /**
     * @brief Check if there are cached tests
     * @return true if at least one test is cached
     */
    bool hasCachedTests();

    /**
     * @brief Get next cached test filename
     *
     * Returns the filename of the first cached test found.
     *
     * @param filename Buffer to store filename (must be at least MAX_PATH_LENGTH)
     * @return true if a cached test was found
     */
    bool getNextCachedTest(char* filename);

    /**
     * @brief Load cached test from file
     *
     * Loads test record from specified cache file.
     *
     * @param filename Full path to cache file
     * @param record Pointer to test record to populate
     * @return true if load successful
     */
    bool loadCachedTest(const char* filename, BrevitestTestRecord* record);

    /**
     * @brief Delete cached test file
     * @param filename Full path to cache file
     * @return true if delete successful
     */
    bool deleteCachedTest(const char* filename);

    /**
     * @brief Get number of cached tests
     * @return Number of files in cache directory
     */
    uint32_t getCachedTestCount();

    // ========================================================================
    // ASSAY CACHE OPERATIONS (STOR-004)
    // ========================================================================

    /**
     * @brief Cache assay to filesystem
     *
     * Saves assay data to /assay/ directory using assay_id as filename.
     *
     * @param assay Assay structure to cache
     * @return true if cache successful
     */
    bool cacheAssay(const BrevitestAssay* assay);

    /**
     * @brief Load cached assay from file
     *
     * @param assayId Assay ID to load
     * @param assay Pointer to assay structure to populate
     * @param expectedChecksum Optional expected BCODE checksum (0 to skip verification)
     * @return true if load successful (and checksum valid if provided)
     */
    bool loadCachedAssay(const char* assayId, BrevitestAssay* assay, uint32_t expectedChecksum = 0);

    /**
     * @brief Check if assay is cached
     * @param assayId Assay ID to check
     * @return true if assay exists in cache
     */
    bool hasAssay(const char* assayId);

    /**
     * @brief Delete cached assay
     * @param assayId Assay ID to delete
     * @return true if delete successful
     */
    bool deleteCachedAssay(const char* assayId);

    /**
     * @brief List all cached assays
     *
     * Logs all assay files in cache directory.
     *
     * @return Number of assay files found
     */
    uint32_t listCachedAssays();

    // ========================================================================
    // VALIDATION DATA OPERATIONS (STOR-004)
    // ========================================================================

    /**
     * @brief Save magnetometer validation data
     *
     * @param data Validation data string
     * @param timestamp Unix timestamp for filename
     * @return true if save successful
     */
    bool saveValidationData(const char* data, uint32_t timestamp);

    /**
     * @brief Load latest magnetometer validation data
     *
     * @param buffer Buffer to store data (must be at least 1024 bytes)
     * @param bufferSize Size of buffer
     * @return true if load successful
     */
    bool loadLatestValidationData(char* buffer, size_t bufferSize);

    /**
     * @brief List all magnetometer validation files
     *
     * LEGACY BEHAVIOR (brevitest-firmware.cpp):
     * - Opens /validation directory
     * - Iterates through all regular files
     * - Logs each filename via Log.info()
     *
     * @return Number of validation files found
     */
    uint32_t listValidationFiles();

    /**
     * @brief Clear all magnetometer validation files
     *
     * LEGACY BEHAVIOR (brevitest-firmware.cpp):
     * - Opens /validation directory
     * - Deletes all regular files
     * - Clears internal validation data buffer
     *
     * @return Number of files deleted
     */
    uint32_t clearValidationFiles();

    /**
     * @brief Get validation file count
     * @return Number of files in validation directory
     */
    uint32_t getValidationFileCount();

    // ========================================================================
    // INTERRUPTED TEST RECOVERY (GAMMA-013)
    // ========================================================================

    /**
     * @brief Handle interrupted test recovery on startup
     *
     * LEGACY BEHAVIOR (brevitest-firmware.ino:4834-4844):
     * - Checks if running_test_uuid is non-empty
     * - If so, saves incomplete test data to cache file
     * - Clears running_test_uuid and running_assay_id
     * - Saves updated EEPROM
     *
     * @param record Test record to populate with recovery data (for caching)
     * @return true if an interrupted test was recovered
     */
    bool handleInterruptedTestRecovery(BrevitestTestRecord* record);

    // ========================================================================
    // CACHE CLEANUP (STOR-005)
    // ========================================================================

    /**
     * @brief Clear old cache files by age
     *
     * Removes files older than specified age.
     *
     * @param maxAgeDays Maximum age in days (files older than this are deleted)
     * @return Number of files deleted
     */
    uint32_t clearOldCache(uint32_t maxAgeDays);

    /**
     * @brief Get total cache size
     * @return Total size of all cache files in bytes
     */
    uint32_t getCacheSize();

    /**
     * @brief Get cache file count
     * @return Number of files in cache directory
     */
    uint32_t getCacheCount();

    /**
     * @brief Clear all cache files
     *
     * Removes all files from /cache/ directory.
     *
     * @return true if clear successful
     */
    bool clearCache();

    /**
     * @brief Clear all assay cache files
     *
     * Removes all files from /assay/ directory.
     *
     * @return true if clear successful
     */
    bool clearAssayCache();

    /**
     * @brief Enforce cache size limit
     *
     * If cache exceeds CACHE_MAX_FILES, removes oldest files (FIFO).
     *
     * @return Number of files removed
     */
    uint32_t enforceCacheLimit();

private:
    // ========================================================================
    // PRIVATE MEMBERS
    // ========================================================================

    /** @brief Current EEPROM configuration */
    Particle_EEPROM _eeprom;

    /** @brief Initialization flag */
    bool _initialized;

    /** @brief Buffer for assay file operations */
    char _assayBuffer[ASSAY_BUFFER_SIZE];

    // ========================================================================
    // PRIVATE METHODS
    // ========================================================================

    /**
     * @brief Create directory if it doesn't exist
     * @param path Directory path
     * @return true if directory exists or was created
     */
    bool createDirIfNotExists(const char* path);

    /**
     * @brief Check if path is a directory
     * @param path Path to check
     * @return true if path is a directory
     */
    bool isDirectory(const char* path);

    /**
     * @brief Check if file exists
     * @param path File path to check
     * @return true if file exists
     */
    bool fileExists(const char* path);

    /**
     * @brief Get file size
     * @param path File path
     * @return File size in bytes, or 0 if file doesn't exist
     */
    uint32_t getFileSize(const char* path);

    /**
     * @brief Count files in directory
     * @param path Directory path
     * @param expectedLength Expected filename length (0 for any)
     * @return Number of regular files found
     */
    uint32_t countFilesInDir(const char* path, size_t expectedLength = 0);

    /**
     * @brief Get oldest file in directory
     *
     * @param path Directory path
     * @param filename Buffer to store filename
     * @param bufferSize Size of filename buffer
     * @return true if a file was found
     */
    bool getOldestFile(const char* path, char* filename, size_t bufferSize);

    /**
     * @brief Delete file
     * @param path File path
     * @return true if delete successful
     */
    bool deleteFile(const char* path);

    /**
     * @brief Calculate CRC32 checksum
     * @param data Data buffer
     * @param length Data length
     * @return CRC32 checksum
     */
    uint32_t calculateChecksum(const char* data, size_t length);
};

#endif // STORAGE_MANAGER_H
