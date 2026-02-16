/**
 * @file StorageManager.cpp
 * @brief Implementation of EEPROM and filesystem management
 * @author Agent EPSILON - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * Implementation of the StorageManager class providing:
 * - EEPROM read/write with version checking
 * - Filesystem directory management
 * - Test data caching with FIFO eviction
 * - Assay caching with checksum verification
 * - Crash recovery data management
 *
 * User Stories Implemented:
 *   - STOR-001: StorageManager class
 *   - STOR-002: EEPROM operations
 *   - STOR-003: Crash recovery
 *   - STOR-004: Filesystem cache
 *   - STOR-005: Cache cleanup
 */

#include "StorageManager.h"

// ============================================================================
// CRC32 LOOKUP TABLE (matches legacy firmware)
// ============================================================================

static const uint32_t crc32_tab[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
    0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
    0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
    0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
    0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
    0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
    0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
    0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
    0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
    0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
    0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
    0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
    0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
    0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
    0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
    0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
    0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

StorageManager::StorageManager() : _initialized(false) {
    memset(&_eeprom, 0, sizeof(_eeprom));
    memset(_assayBuffer, 0, sizeof(_assayBuffer));
}

StorageManager::~StorageManager() {
    // Nothing to clean up
}

// ============================================================================
// INITIALIZATION (STOR-001)
// ============================================================================

bool StorageManager::init() {
    Log.info("StorageManager::init() - Starting initialization");

    // Step 1: Load EEPROM configuration
    if (!loadConfig()) {
        Log.error("StorageManager::init() - Failed to load EEPROM config");
        return false;
    }

    // Step 2: Create filesystem directories
    if (!createDirIfNotExists(CACHE_DIR)) {
        Log.error("StorageManager::init() - Failed to create cache directory");
        return false;
    }

    if (!createDirIfNotExists(ASSAY_DIR)) {
        Log.error("StorageManager::init() - Failed to create assay directory");
        return false;
    }

    if (!createDirIfNotExists(VALIDATION_DIR)) {
        Log.error("StorageManager::init() - Failed to create validation directory");
        return false;
    }

    if (!createDirIfNotExists(BUFFER_DIR)) {
        Log.warn("StorageManager::init() - Failed to create buffer directory");
        // Not critical, continue
    }

    _initialized = true;

    // Log storage information
    Log.info("StorageManager::init() - EEPROM structure size: %d bytes", sizeof(Particle_EEPROM));
    Log.info("StorageManager::init() - Firmware version: %d", _eeprom.firmware_version);
    Log.info("StorageManager::init() - Data format version: %d", _eeprom.data_format_version);
    Log.info("StorageManager::init() - Lifetime stress test cycles: %d", _eeprom.lifetime_stress_test_cycles);

    // Check for crash recovery data
    if (hasRecoveryData()) {
        RecoveryInfo info = getRecoveryInfo();
        Log.warn("StorageManager::init() - Recovery data found: %s (Assay: %s)",
                 info.cartridge_uuid, info.assay_id);
    }

    Log.info("StorageManager::init() - Initialization complete");
    return true;
}

bool StorageManager::isInitialized() const {
    return _initialized;
}

StorageInfo StorageManager::getStorageInfo() {
    StorageInfo info;

    // EEPROM info
    info.eepromSize = 4096;  // Particle devices typically have 4KB EEPROM
    info.eepromUsed = sizeof(Particle_EEPROM);
    info.eepromValid = (_eeprom.firmware_version != 0xFF);

    // Filesystem info
    info.filesystemReady = isDirectory(CACHE_DIR);
    info.cacheFileCount = countFilesInDir(CACHE_DIR, BARCODE_UUID_LENGTH);
    info.assayFileCount = countFilesInDir(ASSAY_DIR, ASSAY_UUID_LENGTH);
    info.validationFileCount = countFilesInDir(VALIDATION_DIR, 0);

    return info;
}

// ============================================================================
// EEPROM OPERATIONS (STOR-002)
// ============================================================================

bool StorageManager::loadConfig() {
    uint8_t firstByte;

    // Read first byte to check if EEPROM is initialized
    EEPROM.get(0, firstByte);

    if (firstByte == 0xFF) {
        // EEPROM is uninitialized, reset to defaults
        Log.info("StorageManager::loadConfig() - EEPROM uninitialized, resetting");
        return resetConfig();
    }

    // Load full structure
    EEPROM.get(0, _eeprom);

    // Check version compatibility
    if (_eeprom.firmware_version != FIRMWARE_VERSION ||
        _eeprom.data_format_version != DATA_FORMAT_VERSION) {
        Log.info("StorageManager::loadConfig() - Version mismatch (FW: %d/%d, Format: %d/%d), resetting",
                 _eeprom.firmware_version, FIRMWARE_VERSION,
                 _eeprom.data_format_version, DATA_FORMAT_VERSION);
        return resetConfig();
    }

    Log.info("StorageManager::loadConfig() - Config loaded successfully");
    return true;
}

bool StorageManager::saveConfig() {
    EEPROM.put(0, _eeprom);
    Log.info("StorageManager::saveConfig() - Config saved");
    return true;
}

bool StorageManager::resetConfig() {
    // Preserve lifetime counter
    int32_t lifetime = _eeprom.lifetime_stress_test_cycles;
    if (lifetime == -1) {
        lifetime = 0;
    }

    // Clear EEPROM
    EEPROM.clear();

    // Initialize with defaults
    Particle_EEPROM defaults;
    memcpy(&_eeprom, &defaults, sizeof(Particle_EEPROM));

    // Restore lifetime counter
    _eeprom.lifetime_stress_test_cycles = lifetime;

    // Save to EEPROM
    EEPROM.put(0, _eeprom);

    Log.info("StorageManager::resetConfig() - EEPROM reset complete");
    return true;
}

const Particle_EEPROM& StorageManager::getConfig() const {
    return _eeprom;
}

Particle_EEPROM& StorageManager::getConfigMutable() {
    return _eeprom;
}

uint8_t StorageManager::getFirmwareVersion() const {
    return _eeprom.firmware_version;
}

uint8_t StorageManager::getDataFormatVersion() const {
    return _eeprom.data_format_version;
}

// ============================================================================
// STRESS TEST COUNTERS (STOR-002)
// ============================================================================

int32_t StorageManager::getLifetimeStressTestCycles() const {
    return _eeprom.lifetime_stress_test_cycles;
}

int32_t StorageManager::getStressTestCyclesSinceReset() const {
    return _eeprom.stress_test_cycles_since_reset;
}

int32_t StorageManager::getStressTestCycles() const {
    return _eeprom.stress_test_cycles;
}

bool StorageManager::incrementStressTestCounters() {
    _eeprom.stress_test_cycles++;
    _eeprom.stress_test_cycles_since_reset++;
    _eeprom.lifetime_stress_test_cycles++;

    Log.info("StorageManager: Stress test cycles - current=%d, sinceReset=%d, lifetime=%d",
             _eeprom.stress_test_cycles,
             _eeprom.stress_test_cycles_since_reset,
             _eeprom.lifetime_stress_test_cycles);

    return saveConfig();
}

bool StorageManager::resetStressTestCyclesSinceReset() {
    _eeprom.stress_test_cycles_since_reset = 0;
    return saveConfig();
}

bool StorageManager::resetStressTestCycles() {
    _eeprom.stress_test_cycles = 0;
    _eeprom.stress_test_reading_count = 0;
    return saveConfig();
}

// ============================================================================
// CRASH RECOVERY (STOR-003)
// ============================================================================

bool StorageManager::hasRecoveryData() const {
    return _eeprom.running_test_uuid[0] != '\0';
}

RecoveryInfo StorageManager::getRecoveryInfo() const {
    RecoveryInfo info;

    if (hasRecoveryData()) {
        strncpy(info.cartridge_uuid, _eeprom.running_test_uuid, BARCODE_UUID_LENGTH);
        info.cartridge_uuid[BARCODE_UUID_LENGTH] = '\0';

        strncpy(info.assay_id, _eeprom.running_assay_id, ASSAY_UUID_LENGTH);
        info.assay_id[ASSAY_UUID_LENGTH] = '\0';

        info.valid = true;
    }

    return info;
}

bool StorageManager::setRecoveryData(const char* cartridgeUuid, const char* assayId) {
    if (cartridgeUuid == nullptr || assayId == nullptr) {
        Log.error("StorageManager::setRecoveryData() - Null parameters");
        return false;
    }

    // Copy cartridge UUID
    strncpy(_eeprom.running_test_uuid, cartridgeUuid, BARCODE_UUID_LENGTH);
    _eeprom.running_test_uuid[BARCODE_UUID_LENGTH] = '\0';

    // Copy assay ID
    strncpy(_eeprom.running_assay_id, assayId, ASSAY_UUID_LENGTH);
    _eeprom.running_assay_id[ASSAY_UUID_LENGTH] = '\0';

    Log.info("StorageManager::setRecoveryData() - Set recovery for %s (Assay: %s)",
             _eeprom.running_test_uuid, _eeprom.running_assay_id);

    return saveConfig();
}

bool StorageManager::clearRecoveryData() {
    memset(_eeprom.running_test_uuid, 0, BARCODE_UUID_LENGTH + 1);
    memset(_eeprom.running_assay_id, 0, ASSAY_UUID_LENGTH + 1);

    Log.info("StorageManager::clearRecoveryData() - Recovery data cleared");

    return saveConfig();
}

// ============================================================================
// TEST CACHE OPERATIONS (STOR-004)
// ============================================================================

bool StorageManager::cacheTestData(const BrevitestTestRecord* record) {
    if (record == nullptr) {
        Log.error("StorageManager::cacheTestData() - Null record");
        return false;
    }

    if (record->cartridge_id[0] == '\0') {
        Log.error("StorageManager::cacheTestData() - Empty cartridge ID");
        return false;
    }

    // Enforce cache limit before adding new file
    enforceCacheLimit();

    // Build filename
    char filename[MAX_PATH_LENGTH];
    snprintf(filename, sizeof(filename), "%s/%s", CACHE_DIR, record->cartridge_id);

    // Write file using binary format
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        Log.error("StorageManager::cacheTestData() - Failed to open %s, errno=%d", filename, errno);
        return false;
    }

    ssize_t written = write(fd, record, sizeof(BrevitestTestRecord));
    close(fd);

    if (written != sizeof(BrevitestTestRecord)) {
        Log.error("StorageManager::cacheTestData() - Write failed, wrote %d of %d bytes",
                  (int)written, sizeof(BrevitestTestRecord));
        unlink(filename);
        return false;
    }

    Log.info("StorageManager::cacheTestData() - Cached test %s (%d bytes)",
             record->cartridge_id, (int)written);

    return true;
}

bool StorageManager::hasCachedTests() {
    return getCachedTestCount() > 0;
}

bool StorageManager::getNextCachedTest(char* filename) {
    if (filename == nullptr) {
        return false;
    }

    DIR* dir = opendir(CACHE_DIR);
    if (dir == nullptr) {
        Log.error("StorageManager::getNextCachedTest() - Failed to open cache directory");
        return false;
    }

    struct dirent* entry;
    bool found = false;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        // Check if filename matches expected UUID length
        if (strlen(entry->d_name) == BARCODE_UUID_LENGTH) {
            snprintf(filename, MAX_PATH_LENGTH, "%s/%s", CACHE_DIR, entry->d_name);
            found = true;
            Log.info("StorageManager::getNextCachedTest() - Found: %s", filename);
            break;
        }
    }

    closedir(dir);
    return found;
}

bool StorageManager::loadCachedTest(const char* filename, BrevitestTestRecord* record) {
    if (filename == nullptr || record == nullptr) {
        Log.error("StorageManager::loadCachedTest() - Null parameters");
        return false;
    }

    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        Log.error("StorageManager::loadCachedTest() - Failed to open %s, errno=%d", filename, errno);
        return false;
    }

    ssize_t bytesRead = read(fd, record, sizeof(BrevitestTestRecord));
    close(fd);

    if (bytesRead != sizeof(BrevitestTestRecord)) {
        Log.error("StorageManager::loadCachedTest() - Read failed, got %d of %d bytes",
                  (int)bytesRead, sizeof(BrevitestTestRecord));
        return false;
    }

    Log.info("StorageManager::loadCachedTest() - Loaded %s (cartridge: %s, readings: %d)",
             filename, record->cartridge_id, record->number_of_readings);

    return true;
}

bool StorageManager::deleteCachedTest(const char* filename) {
    if (filename == nullptr) {
        return false;
    }

    if (unlink(filename) == 0) {
        Log.info("StorageManager::deleteCachedTest() - Deleted %s", filename);
        return true;
    } else {
        Log.error("StorageManager::deleteCachedTest() - Failed to delete %s, errno=%d", filename, errno);
        return false;
    }
}

uint32_t StorageManager::getCachedTestCount() {
    return countFilesInDir(CACHE_DIR, BARCODE_UUID_LENGTH);
}

// ============================================================================
// ASSAY CACHE OPERATIONS (STOR-004)
// ============================================================================

bool StorageManager::cacheAssay(const BrevitestAssay* assay) {
    if (assay == nullptr) {
        Log.error("StorageManager::cacheAssay() - Null assay");
        return false;
    }

    if (assay->id[0] == '\0') {
        Log.error("StorageManager::cacheAssay() - Empty assay ID");
        return false;
    }

    // Build filename
    char filename[MAX_PATH_LENGTH];
    snprintf(filename, sizeof(filename), "%s/%s", ASSAY_DIR, assay->id);

    // Open file for writing
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        Log.error("StorageManager::cacheAssay() - Failed to open %s, errno=%d", filename, errno);
        return false;
    }

    // Format: id\nduration\nBCODE\n
    char buffer[ASSAY_BUFFER_SIZE];
    int len = snprintf(buffer, sizeof(buffer), "%s\n%d\n%s\n",
                       assay->id, (int)assay->duration, assay->BCODE);

    ssize_t written = write(fd, buffer, len);
    close(fd);

    if (written != len) {
        Log.error("StorageManager::cacheAssay() - Write failed");
        unlink(filename);
        return false;
    }

    Log.info("StorageManager::cacheAssay() - Cached assay %s (duration: %d, BCODE len: %d)",
             assay->id, (int)assay->duration, (int)strlen(assay->BCODE));

    return true;
}

bool StorageManager::loadCachedAssay(const char* assayId, BrevitestAssay* assay, uint32_t expectedChecksum) {
    if (assayId == nullptr || assay == nullptr) {
        Log.error("StorageManager::loadCachedAssay() - Null parameters");
        return false;
    }

    if (strlen(assayId) == 0) {
        Log.error("StorageManager::loadCachedAssay() - Empty assay ID");
        return false;
    }

    // Build filename
    char filename[MAX_PATH_LENGTH];
    snprintf(filename, sizeof(filename), "%s/%s", ASSAY_DIR, assayId);

    // Open file for reading
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        Log.error("StorageManager::loadCachedAssay() - Failed to open %s, errno=%d", filename, errno);
        return false;
    }

    // Read file content
    ssize_t bytesRead = read(fd, _assayBuffer, sizeof(_assayBuffer) - 1);
    close(fd);

    if (bytesRead <= 0) {
        Log.error("StorageManager::loadCachedAssay() - Read failed");
        return false;
    }

    _assayBuffer[bytesRead] = '\0';

    // Parse format: id\nduration\nBCODE\n
    // Verify assay ID matches
    if (strncmp(assayId, _assayBuffer, ASSAY_UUID_LENGTH) != 0) {
        Log.error("StorageManager::loadCachedAssay() - ID mismatch");
        return false;
    }

    // Copy assay ID
    strncpy(assay->id, assayId, ASSAY_UUID_LENGTH);
    assay->id[ASSAY_UUID_LENGTH] = '\0';

    // Parse duration
    char* mark = &_assayBuffer[ASSAY_UUID_LENGTH + 1];  // Skip ID and newline
    int index = strcspn(mark, "\n");
    char durationStr[16];
    strncpy(durationStr, mark, index);
    durationStr[index] = '\0';
    assay->duration = atoi(durationStr);

    // Parse BCODE
    mark += index + 1;  // Skip duration and newline
    index = strcspn(mark, "\n");
    if (index >= BCODE_CAPACITY) {
        Log.error("StorageManager::loadCachedAssay() - BCODE too long");
        return false;
    }
    strncpy(assay->BCODE, mark, index);
    assay->BCODE[index] = '\0';
    assay->BCODE_length = index;

    // Verify checksum if provided
    if (expectedChecksum != 0) {
        uint32_t actualChecksum = calculateChecksum(assay->BCODE, strlen(assay->BCODE));
        if (actualChecksum != expectedChecksum) {
            Log.error("StorageManager::loadCachedAssay() - Checksum mismatch: expected 0x%08lX, got 0x%08lX",
                      (unsigned long)expectedChecksum, (unsigned long)actualChecksum);
            assay->id[0] = '\0';
            return false;
        }
    }

    Log.info("StorageManager::loadCachedAssay() - Loaded assay %s (duration: %d)",
             assay->id, (int)assay->duration);

    return true;
}

bool StorageManager::hasAssay(const char* assayId) {
    if (assayId == nullptr || strlen(assayId) == 0) {
        return false;
    }

    char filename[MAX_PATH_LENGTH];
    snprintf(filename, sizeof(filename), "%s/%s", ASSAY_DIR, assayId);

    return fileExists(filename);
}

bool StorageManager::deleteCachedAssay(const char* assayId) {
    if (assayId == nullptr || strlen(assayId) == 0) {
        return false;
    }

    char filename[MAX_PATH_LENGTH];
    snprintf(filename, sizeof(filename), "%s/%s", ASSAY_DIR, assayId);

    return deleteFile(filename);
}

uint32_t StorageManager::listCachedAssays() {
    DIR* dir = opendir(ASSAY_DIR);
    if (dir == nullptr) {
        Log.error("StorageManager::listCachedAssays() - Failed to open assay directory");
        return 0;
    }

    uint32_t count = 0;
    struct dirent* entry;

    Log.info("Assay file directory:");

    while ((entry = readdir(dir)) != nullptr && count < ASSAY_MAX_FILES) {
        if (entry->d_type != DT_REG) {
            continue;
        }
        count++;
        Log.info("  %s", entry->d_name);
    }

    closedir(dir);
    return count;
}

// ============================================================================
// VALIDATION DATA OPERATIONS (STOR-004)
// ============================================================================

bool StorageManager::saveValidationData(const char* data, uint32_t timestamp) {
    if (data == nullptr) {
        return false;
    }

    char filename[MAX_PATH_LENGTH];
    snprintf(filename, sizeof(filename), "%s/magnet_%u.txt", VALIDATION_DIR, timestamp);

    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) {
        Log.error("StorageManager::saveValidationData() - Failed to open %s", filename);
        return false;
    }

    size_t len = strlen(data);
    ssize_t written = write(fd, data, len);
    close(fd);

    if (written != (ssize_t)len) {
        Log.error("StorageManager::saveValidationData() - Write failed");
        unlink(filename);
        return false;
    }

    Log.info("StorageManager::saveValidationData() - Saved %s", filename);
    return true;
}

bool StorageManager::loadLatestValidationData(char* buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) {
        return false;
    }

    DIR* dir = opendir(VALIDATION_DIR);
    if (dir == nullptr) {
        Log.error("StorageManager::loadLatestValidationData() - Failed to open validation directory");
        return false;
    }

    // Find the file with the highest timestamp
    uint32_t latestTimestamp = 0;
    char latestFilename[MAX_PATH_LENGTH] = {0};

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        // Parse timestamp from filename: magnet_TIMESTAMP.txt
        if (strncmp(entry->d_name, "magnet_", 7) == 0) {
            char* timestampStr = &entry->d_name[7];
            uint32_t timestamp = atoi(timestampStr);
            if (timestamp > latestTimestamp) {
                latestTimestamp = timestamp;
                snprintf(latestFilename, sizeof(latestFilename), "%s/%s",
                         VALIDATION_DIR, entry->d_name);
            }
        }
    }

    closedir(dir);

    if (latestTimestamp == 0) {
        Log.info("StorageManager::loadLatestValidationData() - No validation files found");
        return false;
    }

    // Load the latest file
    int fd = open(latestFilename, O_RDONLY);
    if (fd < 0) {
        Log.error("StorageManager::loadLatestValidationData() - Failed to open %s", latestFilename);
        return false;
    }

    ssize_t bytesRead = read(fd, buffer, bufferSize - 1);
    close(fd);

    if (bytesRead <= 0) {
        Log.error("StorageManager::loadLatestValidationData() - Read failed");
        return false;
    }

    buffer[bytesRead] = '\0';

    Log.info("StorageManager::loadLatestValidationData() - Loaded %s (%d bytes)",
             latestFilename, (int)bytesRead);

    return true;
}

// ============================================================================
// MAGNETOMETER VALIDATION FILE MANAGEMENT (GAMMA-014)
// ============================================================================

uint32_t StorageManager::listValidationFiles() {
    DIR* dir = opendir(VALIDATION_DIR);
    if (dir == nullptr) {
        Log.error("StorageManager::listValidationFiles() - Failed to open validation directory");
        return 0;
    }

    uint32_t count = 0;
    struct dirent* entry;

    Log.info("Validation file directory:");

    while ((entry = readdir(dir)) != nullptr && count < VALIDATION_MAX_FILES) {
        if (entry->d_type != DT_REG) {
            continue;
        }
        count++;
        Log.info("  %s", entry->d_name);
    }

    closedir(dir);

    Log.info("Total validation files: %u", count);
    return count;
}

uint32_t StorageManager::clearValidationFiles() {
    DIR* dir = opendir(VALIDATION_DIR);
    if (dir == nullptr) {
        Log.error("StorageManager::clearValidationFiles() - Failed to open validation directory");
        return 0;
    }

    uint32_t deletedCount = 0;
    struct dirent* entry;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        char filename[MAX_PATH_LENGTH];
        snprintf(filename, sizeof(filename), "%s/%s", VALIDATION_DIR, entry->d_name);

        if (unlink(filename) == 0) {
            deletedCount++;
            Log.info("StorageManager::clearValidationFiles() - Deleted %s", filename);
        } else {
            Log.error("StorageManager::clearValidationFiles() - Failed to delete %s, errno=%d",
                      filename, errno);
        }
    }

    closedir(dir);

    Log.info("StorageManager::clearValidationFiles() - Deleted %u files", deletedCount);
    return deletedCount;
}

uint32_t StorageManager::getValidationFileCount() {
    return countFilesInDir(VALIDATION_DIR, 0);
}

// ============================================================================
// INTERRUPTED TEST RECOVERY (GAMMA-013)
// ============================================================================

bool StorageManager::handleInterruptedTestRecovery(BrevitestTestRecord* record) {
    /**
     * LEGACY BEHAVIOR (brevitest-firmware.ino:4834-4844):
     *
     * bool test_interrupted = eeprom.running_test_uuid[0] != '\0';
     * if (test_interrupted) {
     *     Log.info("Test interrupted: %s (Assay %s) - saving cancelled test to file",
     *              eeprom.running_test_uuid, eeprom.running_assay_id);
     *     memcpy(test.cartridge_id, eeprom.running_test_uuid, BARCODE_UUID_LENGTH + 1);
     *     memcpy(test.assay_id, eeprom.running_assay_id, ASSAY_UUID_LENGTH + 1);
     *     write_test_to_file();
     *     memset(eeprom.running_test_uuid, 0, BARCODE_UUID_LENGTH + 1);
     *     memset(eeprom.running_assay_id, 0, ASSAY_UUID_LENGTH + 1);
     *     EEPROM.put(0, eeprom);
     * }
     */

    // Check if there's recovery data
    if (!hasRecoveryData()) {
        return false;
    }

    RecoveryInfo info = getRecoveryInfo();

    Log.info("Test interrupted: %s (Assay %s) - saving cancelled test to file",
             info.cartridge_uuid, info.assay_id);

    // Populate test record with recovery info if provided
    if (record != nullptr) {
        memcpy(record->cartridge_id, info.cartridge_uuid, BARCODE_UUID_LENGTH + 1);
        memcpy(record->assay_id, info.assay_id, ASSAY_UUID_LENGTH + 1);

        // Cache the interrupted test record
        if (!cacheTestData(record)) {
            Log.error("StorageManager::handleInterruptedTestRecovery() - Failed to cache test");
        }
    } else {
        // Create a minimal test record for caching
        BrevitestTestRecord minimalRecord;
        memcpy(minimalRecord.cartridge_id, info.cartridge_uuid, BARCODE_UUID_LENGTH + 1);
        memcpy(minimalRecord.assay_id, info.assay_id, ASSAY_UUID_LENGTH + 1);
        minimalRecord.start_time = Time.now();
        minimalRecord.duration = 0;  // Unknown - test was interrupted
        minimalRecord.number_of_readings = 0;

        if (!cacheTestData(&minimalRecord)) {
            Log.error("StorageManager::handleInterruptedTestRecovery() - Failed to cache minimal test");
        }
    }

    // Clear recovery data from EEPROM
    if (!clearRecoveryData()) {
        Log.error("StorageManager::handleInterruptedTestRecovery() - Failed to clear recovery data");
        return false;
    }

    Log.info("StorageManager::handleInterruptedTestRecovery() - Recovery complete");
    return true;
}

// ============================================================================
// CACHE CLEANUP (STOR-005)
// ============================================================================

uint32_t StorageManager::clearOldCache(uint32_t maxAgeDays) {
    // Get current time
    uint32_t now = Time.now();
    uint32_t maxAgeSeconds = maxAgeDays * 24 * 60 * 60;

    DIR* dir = opendir(CACHE_DIR);
    if (dir == nullptr) {
        return 0;
    }

    uint32_t deletedCount = 0;
    struct dirent* entry;
    struct stat statbuf;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        char filename[MAX_PATH_LENGTH];
        snprintf(filename, sizeof(filename), "%s/%s", CACHE_DIR, entry->d_name);

        if (stat(filename, &statbuf) == 0) {
            uint32_t fileAge = now - statbuf.st_mtime;
            if (fileAge > maxAgeSeconds) {
                if (unlink(filename) == 0) {
                    deletedCount++;
                    Log.info("StorageManager::clearOldCache() - Deleted old file: %s", filename);
                }
            }
        }
    }

    closedir(dir);

    if (deletedCount > 0) {
        Log.info("StorageManager::clearOldCache() - Deleted %d old files", deletedCount);
    }

    return deletedCount;
}

uint32_t StorageManager::getCacheSize() {
    DIR* dir = opendir(CACHE_DIR);
    if (dir == nullptr) {
        return 0;
    }

    uint32_t totalSize = 0;
    struct dirent* entry;
    struct stat statbuf;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        char filename[MAX_PATH_LENGTH];
        snprintf(filename, sizeof(filename), "%s/%s", CACHE_DIR, entry->d_name);

        if (stat(filename, &statbuf) == 0) {
            totalSize += statbuf.st_size;
        }
    }

    closedir(dir);
    return totalSize;
}

uint32_t StorageManager::getCacheCount() {
    return countFilesInDir(CACHE_DIR, 0);
}

bool StorageManager::clearCache() {
    DIR* dir = opendir(CACHE_DIR);
    if (dir == nullptr) {
        return false;
    }

    struct dirent* entry;
    int failCount = 0;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        char filename[MAX_PATH_LENGTH];
        snprintf(filename, sizeof(filename), "%s/%s", CACHE_DIR, entry->d_name);

        if (unlink(filename) != 0) {
            failCount++;
            Log.error("StorageManager::clearCache() - Failed to delete %s", filename);
        }
    }

    closedir(dir);

    Log.info("StorageManager::clearCache() - Cache cleared");
    return failCount == 0;
}

bool StorageManager::clearAssayCache() {
    DIR* dir = opendir(ASSAY_DIR);
    if (dir == nullptr) {
        return false;
    }

    struct dirent* entry;
    int failCount = 0;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        char filename[MAX_PATH_LENGTH];
        snprintf(filename, sizeof(filename), "%s/%s", ASSAY_DIR, entry->d_name);

        if (unlink(filename) != 0) {
            failCount++;
            Log.error("StorageManager::clearAssayCache() - Failed to delete %s", filename);
        }
    }

    closedir(dir);

    Log.info("StorageManager::clearAssayCache() - Assay cache cleared");
    return failCount == 0;
}

uint32_t StorageManager::enforceCacheLimit() {
    uint32_t currentCount = getCachedTestCount();
    uint32_t removedCount = 0;

    while (currentCount >= CACHE_MAX_FILES) {
        char oldestFile[MAX_PATH_LENGTH];

        if (!getOldestFile(CACHE_DIR, oldestFile, sizeof(oldestFile))) {
            break;
        }

        if (deleteFile(oldestFile)) {
            removedCount++;
            currentCount--;
            Log.info("StorageManager::enforceCacheLimit() - Removed old file: %s", oldestFile);
        } else {
            break;
        }
    }

    if (removedCount > 0) {
        Log.info("StorageManager::enforceCacheLimit() - Removed %d files (FIFO)", removedCount);
    }

    return removedCount;
}

// ============================================================================
// PRIVATE HELPER METHODS
// ============================================================================

bool StorageManager::createDirIfNotExists(const char* path) {
    struct stat statbuf;

    int result = stat(path, &statbuf);
    if (result == 0) {
        // Path exists
        if ((statbuf.st_mode & S_IFDIR) != 0) {
            Log.info("StorageManager: Directory exists: %s", path);
            return true;
        }

        // Path exists but is not a directory - delete and recreate
        Log.warn("StorageManager: File in the way, deleting: %s", path);
        unlink(path);
    } else if (errno != ENOENT) {
        // Error other than "not found"
        Log.error("StorageManager: stat() failed for %s, errno=%d", path, errno);
        return false;
    }

    // Create directory
    result = mkdir(path, 0777);
    if (result == 0) {
        Log.info("StorageManager: Created directory: %s", path);
        return true;
    } else {
        Log.error("StorageManager: mkdir() failed for %s, errno=%d", path, errno);
        return false;
    }
}

bool StorageManager::isDirectory(const char* path) {
    struct stat statbuf;

    if (stat(path, &statbuf) != 0) {
        return false;
    }

    return (statbuf.st_mode & S_IFDIR) != 0;
}

bool StorageManager::fileExists(const char* path) {
    struct stat statbuf;
    return stat(path, &statbuf) == 0;
}

uint32_t StorageManager::getFileSize(const char* path) {
    struct stat statbuf;

    if (stat(path, &statbuf) != 0) {
        return 0;
    }

    return statbuf.st_size;
}

uint32_t StorageManager::countFilesInDir(const char* path, size_t expectedLength) {
    DIR* dir = opendir(path);
    if (dir == nullptr) {
        return 0;
    }

    uint32_t count = 0;
    struct dirent* entry;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        // If expectedLength is 0, count all regular files
        // Otherwise, only count files with matching name length
        if (expectedLength == 0 || strlen(entry->d_name) == expectedLength) {
            count++;
        }
    }

    closedir(dir);
    return count;
}

bool StorageManager::getOldestFile(const char* path, char* filename, size_t bufferSize) {
    DIR* dir = opendir(path);
    if (dir == nullptr) {
        return false;
    }

    uint32_t oldestTime = UINT32_MAX;
    char oldestName[MAX_PATH_LENGTH] = {0};

    struct dirent* entry;
    struct stat statbuf;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_REG) {
            continue;
        }

        char fullPath[MAX_PATH_LENGTH];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        if (stat(fullPath, &statbuf) == 0) {
            if ((uint32_t)statbuf.st_mtime < oldestTime) {
                oldestTime = statbuf.st_mtime;
                strncpy(oldestName, fullPath, sizeof(oldestName) - 1);
                oldestName[sizeof(oldestName) - 1] = '\0';
            }
        }
    }

    closedir(dir);

    if (oldestName[0] != '\0') {
        strncpy(filename, oldestName, bufferSize - 1);
        filename[bufferSize - 1] = '\0';
        return true;
    }

    return false;
}

bool StorageManager::deleteFile(const char* path) {
    if (unlink(path) == 0) {
        Log.info("StorageManager: Deleted file: %s", path);
        return true;
    } else {
        Log.error("StorageManager: Failed to delete %s, errno=%d", path, errno);
        return false;
    }
}

uint32_t StorageManager::calculateChecksum(const char* data, size_t length) {
    uint8_t* p = (uint8_t*)data;
    uint32_t crc = ~0U;

    while (length--) {
        crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ ~0U;
}
