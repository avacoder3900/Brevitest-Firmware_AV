/**
 * @file DataTypes.h
 * @brief Core data type definitions for Brevitest firmware
 * @details This module defines all data structures used throughout the firmware
 *          for test records, spectrophotometer readings, assay storage, and
 *          persistent EEPROM data. All structures are byte-packed for binary
 *          compatibility with legacy firmware.
 *
 * Platform: Particle B-Series SoM (NRF52840) on Acuity GEN2 Main Board R5
 *
 * @note Structure sizes are critical for binary compatibility:
 *       - BrevitestSpectrophotometerReading: 32 bytes
 *       - BrevitestTestRecord: 9668 bytes
 *
 * @note FIRMWARE_VERSION is defined ONLY here. Do not define it elsewhere.
 *
 * @version 2.0
 * @date 2026-02-11
 *
 * User Stories Implemented:
 *   - ALPHA-001: Single firmware version source of truth
 *   - DATA-001: BrevitestTestRecord structure (9668 bytes)
 *   - DATA-002: BrevitestSpectrophotometerReading structure (32 bytes)
 *   - DATA-003: BrevitestAssay structure with BCODE buffer
 *   - DATA-004: EEPROM storage structure
 *   - DATA-005: All enumeration types
 *   - DATA-006: Serialization utilities (declarations)
 */

#ifndef DATATYPES_H
#define DATATYPES_H

#include <stdint.h>
#include <stddef.h>

//==============================================================================
// FIRMWARE VERSION AND FORMAT CONSTANTS
//==============================================================================

/** @brief Current firmware version number (SINGLE SOURCE OF TRUTH - do not redefine) */
#define FIRMWARE_VERSION 200

/** @brief Data format version for binary compatibility */
#define DATA_FORMAT_VERSION 40

/** @brief Single character code identifying test data format */
#define TEST_DATA_FORMAT_CODE 'J'

//==============================================================================
// SIZE CONSTANTS
//==============================================================================

/** @brief Length of assay UUID (8 characters) */
#define ASSAY_UUID_LENGTH 8

/** @brief Length of barcode/cartridge UUID (36 characters) */
#define BARCODE_UUID_LENGTH 36

/** @brief Length of magnetometer UUID (32 characters) */
#define MAGNETOMETER_UUID_LENGTH 32

/** @brief Length of stress test UUID (16 characters) */
#define STRESS_TEST_UUID_LENGTH 16

/** @brief Length of device UUID (24 characters) */
#define DEVICE_UUID_LENGTH 24

//==============================================================================
// SPECTROPHOTOMETER CONSTANTS
//==============================================================================

/** @brief Maximum number of spectrophotometer readings per test */
#define SPECTRO_MAX_READINGS 300

/** @brief Default ASTEP value for spectrophotometer */
#define SPECTRO_ASTEP_DEFAULT 999

/** @brief Default ATIME value for spectrophotometer */
#define SPECTRO_ATIME_DEFAULT 49

/** @brief Default AGAIN value for spectrophotometer */
#define SPECTRO_AGAIN_DEFAULT 7

//==============================================================================
// BCODE CONSTANTS
//==============================================================================

/** @brief Maximum size of BCODE instruction buffer (5000 bytes) */
#define BCODE_CAPACITY 5000

//==============================================================================
// DELIMITER CONSTANTS (for parsing)
//==============================================================================

/** @brief Argument delimiter character */
#define ARG_DELIM ','

/** @brief Attribute delimiter character */
#define ATTR_DELIM ':'

/** @brief Item delimiter character */
#define ITEM_DELIM '|'

/** @brief End delimiter character */
#define END_DELIM '#'

//==============================================================================
// ENUMERATION TYPES (DATA-005)
//==============================================================================

/**
 * @brief Primary device operational states
 * @details Represents the main operational modes of the device. Only one
 *          DeviceMode can be active at a time. Values match legacy firmware
 *          for state compatibility.
 *
 * State Transitions:
 *   INITIALIZING -> IDLE (after setup)
 *   IDLE -> HEATING (cartridge inserted)
 *   HEATING -> BARCODE_SCANNING (heater ready)
 *   BARCODE_SCANNING -> VALIDATING_CARTRIDGE (barcode read)
 *   VALIDATING_CARTRIDGE -> RUNNING_TEST (validated)
 *   RUNNING_TEST -> UPLOADING_RESULTS (test complete)
 *   Any state -> ERROR_STATE (on error)
 *   Any state -> IDLE (cartridge removed)
 */
enum class DeviceMode : uint8_t {
    IDLE = 0,                    ///< Ready for cartridge insertion - normal standby state
    INITIALIZING = 1,            ///< Device startup - occurs during setup()
    HEATING = 2,                 ///< Waiting for heater to reach target temperature
    BARCODE_SCANNING = 3,        ///< Reading barcode from inserted cartridge
    VALIDATING_CARTRIDGE = 4,    ///< Cloud validation in progress
    VALIDATING_MAGNETOMETER = 5, ///< BLE magnetometer validation in progress
    RUNNING_TEST = 6,            ///< Test execution (BCODE running)
    UPLOADING_RESULTS = 7,       ///< Cloud upload in progress
    RESETTING_CARTRIDGE = 8,     ///< Cloud reset in progress
    STRESS_TESTING = 9,          ///< Stress test mode - automated testing cycles
    ERROR_STATE = 10             ///< Error condition - needs attention
};

/**
 * @brief Test execution sub-states
 * @details Tracks the progress of individual test runs through execution
 *          and upload phases.
 */
enum class TestState : uint8_t {
    NOT_STARTED = 0,        ///< No test has been initiated
    RUNNING = 1,            ///< Test is currently executing (BCODE running)
    COMPLETED = 2,          ///< Test finished successfully
    CANCELLED = 3,          ///< Test was cancelled (e.g., cartridge removed)
    UPLOAD_PENDING = 4,     ///< Test completed, waiting to upload to cloud
    UPLOAD_IN_PROGRESS = 5, ///< Currently uploading test results
    UPLOADED = 6            ///< Test results successfully uploaded
};

/**
 * @brief Cartridge-related states
 * @details Tracks the status of cartridge insertion and validation.
 *          Cartridges progress: detected -> barcode read -> validated.
 */
enum class CartridgeState : uint8_t {
    NOT_INSERTED = 0,   ///< No cartridge detected
    DETECTED = 1,       ///< Cartridge physically detected (hardware interrupt)
    BARCODE_READ = 2,   ///< Barcode successfully scanned
    VALIDATED = 3,      ///< Cartridge validated by cloud server
    INVALID = 4,        ///< Cartridge validation failed
    TEST_COMPLETE = 5   ///< Test completed for this cartridge
};

/**
 * @brief Barcode type enumeration
 * @details Identifies the type of barcode scanned. Positive values indicate
 *          valid barcode types, negative values indicate errors.
 */
enum class BarcodeType : int8_t {
    CARTRIDGE = 1,          ///< Standard test cartridge barcode
    MAGNETOMETER = 2,       ///< Magnetometer calibration barcode
    OPTICAL = 3,            ///< Optical calibration barcode
    STRESS_TEST = 4,        ///< Stress test barcode
    SHIPPING = 5,           ///< Shipping/logistics barcode
    GENERAL_ERROR = -1,     ///< General barcode read error
    VALIDATION_ERROR = -2,  ///< Barcode validation failed
    OPTICAL_ERROR = -3      ///< Optical calibration error
};

/**
 * @brief Error codes for firmware operations
 * @details Comprehensive error codes for all error conditions that can occur
 *          during device operation.
 */
enum class ErrorCode : int16_t {
    SUCCESS = 0,                    ///< Operation completed successfully

    // General errors (1-99)
    ERR_UNKNOWN = 1,                ///< Unknown error occurred
    ERR_TIMEOUT = 2,                ///< Operation timed out
    ERR_INVALID_STATE = 3,          ///< Invalid state for operation
    ERR_INVALID_PARAMETER = 4,      ///< Invalid parameter provided
    ERR_MEMORY_ALLOCATION = 5,      ///< Memory allocation failed

    // Hardware errors (100-199)
    ERR_MOTOR_FAULT = 100,          ///< Motor driver fault
    ERR_HEATER_FAULT = 101,         ///< Heater control fault
    ERR_THERMISTOR_FAULT = 102,     ///< Thermistor reading fault
    ERR_SPECTRO_FAULT = 103,        ///< Spectrophotometer fault
    ERR_BARCODE_FAULT = 104,        ///< Barcode scanner fault
    ERR_LASER_FAULT = 105,          ///< Laser module fault
    ERR_I2C_FAULT = 106,            ///< I2C communication fault

    // Cartridge errors (200-299)
    ERR_CARTRIDGE_NOT_INSERTED = 200,   ///< No cartridge detected
    ERR_CARTRIDGE_INVALID = 201,        ///< Cartridge validation failed
    ERR_CARTRIDGE_EXPIRED = 202,        ///< Cartridge has expired
    ERR_CARTRIDGE_USED = 203,           ///< Cartridge already used
    ERR_BARCODE_READ_FAILED = 204,      ///< Failed to read barcode

    // Cloud/Network errors (300-399)
    ERR_CLOUD_TIMEOUT = 300,            ///< Cloud operation timed out
    ERR_CLOUD_CONNECTION = 301,         ///< Failed to connect to cloud
    ERR_CLOUD_VALIDATION = 302,         ///< Cloud validation failed
    ERR_CLOUD_UPLOAD = 303,             ///< Failed to upload results
    ERR_ASSAY_DOWNLOAD = 304,           ///< Failed to download assay
    ERR_CHECKSUM_MISMATCH = 305,        ///< Checksum validation failed

    // Test execution errors (400-499)
    ERR_TEST_CANCELLED = 400,           ///< Test was cancelled
    ERR_TEST_FAILED = 401,              ///< Test execution failed
    ERR_BCODE_INVALID = 402,            ///< Invalid BCODE instruction
    ERR_BCODE_TIMEOUT = 403,            ///< BCODE execution timeout

    // Storage errors (500-599)
    ERR_EEPROM_READ = 500,              ///< EEPROM read failed
    ERR_EEPROM_WRITE = 501,             ///< EEPROM write failed
    ERR_FILESYSTEM = 502,               ///< Filesystem operation failed
    ERR_FILE_NOT_FOUND = 503            ///< File not found
};

/**
 * @brief Radio communication states
 * @details Tracks the connection status of wireless communication interfaces.
 */
struct RadioState {
    bool wifi;      ///< WiFi connection status
    bool cellular;  ///< Cellular connection status
    bool bluetooth; ///< Bluetooth connection status

    /** @brief Default constructor - all radios off */
    RadioState() : wifi(false), cellular(false), bluetooth(false) {}
};

//==============================================================================
// PACKED STRUCTURE DEFINITIONS
//==============================================================================

#pragma pack(push, 1)

/**
 * @brief Spectrophotometer reading structure (DATA-002)
 * @details Stores a single spectrophotometer reading from the AS7341 sensor
 *          with all 10 wavelength channels plus metadata. Structure is
 *          exactly 32 bytes for binary compatibility.
 *
 * Memory Layout (32 bytes total):
 *   Offset 0:    number (1 byte)
 *   Offset 1:    channel (1 byte)
 *   Offset 2-3:  position (2 bytes)
 *   Offset 4-5:  temperature (2 bytes)
 *   Offset 6-7:  laser_output (2 bytes)
 *   Offset 8-11: msec (4 bytes)
 *   Offset 12-13: f1 (2 bytes)
 *   Offset 14-15: f2 (2 bytes)
 *   Offset 16-17: f3 (2 bytes)
 *   Offset 18-19: f4 (2 bytes)
 *   Offset 20-21: f5 (2 bytes)
 *   Offset 22-23: f6 (2 bytes)
 *   Offset 24-25: f7 (2 bytes)
 *   Offset 26-27: f8 (2 bytes)
 *   Offset 28-29: clear (2 bytes)
 *   Offset 30-31: nir (2 bytes)
 *
 * @note Size MUST be exactly 32 bytes for binary compatibility
 */
struct BrevitestSpectrophotometerReading {
    uint8_t number;         ///< Reading sequence number (0-255)
    char channel;           ///< Spectrophotometer channel ('A', 'B', or 'C')
    uint16_t position;      ///< Stage position in microsteps
    uint16_t temperature;   ///< Temperature reading (10x Celsius)
    uint16_t laser_output;  ///< Laser power output value
    uint32_t msec;          ///< Timestamp in milliseconds since test start
    uint16_t f1;            ///< AS7341 F1 channel (415nm violet)
    uint16_t f2;            ///< AS7341 F2 channel (445nm blue)
    uint16_t f3;            ///< AS7341 F3 channel (480nm cyan)
    uint16_t f4;            ///< AS7341 F4 channel (515nm green)
    uint16_t f5;            ///< AS7341 F5 channel (555nm yellow-green)
    uint16_t f6;            ///< AS7341 F6 channel (590nm orange)
    uint16_t f7;            ///< AS7341 F7 channel (630nm red)
    uint16_t f8;            ///< AS7341 F8 channel (680nm deep red)
    uint16_t clear;         ///< AS7341 Clear channel (unfiltered)
    uint16_t nir;           ///< AS7341 NIR channel (near-infrared)
};

// Compile-time size verification
static_assert(sizeof(BrevitestSpectrophotometerReading) == 32,
    "BrevitestSpectrophotometerReading must be exactly 32 bytes");

/**
 * @brief Test record structure (DATA-001)
 * @details Main test record structure storing all test data including header
 *          information and array of spectrophotometer readings. Structure is
 *          exactly 9668 bytes for binary compatibility.
 *
 * Memory Layout (9668 bytes total):
 *   Offset 0:       data_format_code (1 byte)
 *   Offset 1-37:    cartridge_id (37 bytes)
 *   Offset 38-46:   assay_id (9 bytes)
 *   Offset 47:      reserved (1 byte)
 *   Offset 48-51:   start_time (4 bytes)
 *   Offset 52-53:   duration (2 bytes)
 *   Offset 54-55:   astep (2 bytes)
 *   Offset 56:      atime (1 byte)
 *   Offset 57:      again (1 byte)
 *   Offset 58-59:   number_of_readings (2 bytes)
 *   Offset 60-61:   baseline_scans (2 bytes)
 *   Offset 62-63:   test_scans (2 bytes)
 *   Offset 64-67:   checksum (4 bytes)
 *   Offset 68-9667: reading[300] (9600 bytes)
 *
 * @note Size MUST be exactly 9668 bytes for binary compatibility
 */
struct BrevitestTestRecord {
    char data_format_code;                  ///< Format identifier (default 'J')
    char cartridge_id[BARCODE_UUID_LENGTH + 1]; ///< Cartridge barcode UUID (37 bytes)
    char assay_id[ASSAY_UUID_LENGTH + 1];   ///< Assay identifier (9 bytes)
    char reserved;                          ///< Reserved byte for alignment
    uint32_t start_time;                    ///< Test start time (Unix timestamp)
    uint16_t duration;                      ///< Test duration in seconds
    uint16_t astep;                         ///< Spectrophotometer ASTEP setting
    uint8_t atime;                          ///< Spectrophotometer ATIME setting
    uint8_t again;                          ///< Spectrophotometer AGAIN setting
    uint16_t number_of_readings;            ///< Actual number of readings taken
    uint16_t baseline_scans;                ///< Number of baseline scans
    uint16_t test_scans;                    ///< Number of test scans
    uint32_t checksum;                      ///< CRC32 checksum of record data
    BrevitestSpectrophotometerReading reading[SPECTRO_MAX_READINGS]; ///< Reading array

    /** @brief Default constructor with legacy default values */
    BrevitestTestRecord() :
        data_format_code(TEST_DATA_FORMAT_CODE),
        reserved('\0'),
        start_time(0),
        duration(0),
        astep(SPECTRO_ASTEP_DEFAULT),
        atime(SPECTRO_ATIME_DEFAULT),
        again(SPECTRO_AGAIN_DEFAULT),
        number_of_readings(0),
        baseline_scans(0),
        test_scans(0),
        checksum(0)
    {
        cartridge_id[0] = '\0';
        assay_id[0] = '\0';
    }
};

// Compile-time size verification
static_assert(sizeof(BrevitestTestRecord) == 9668,
    "BrevitestTestRecord must be exactly 9668 bytes");

/**
 * @brief Assay structure (DATA-003)
 * @details Holds downloaded test instructions (BCODE) and assay metadata.
 *          BCODE is the instruction set that controls test execution.
 */
struct BrevitestAssay {
    char id[ASSAY_UUID_LENGTH + 1];     ///< Assay identifier (9 bytes)
    int32_t duration;                    ///< Expected test duration in seconds
    uint16_t BCODE_length;              ///< Actual length of BCODE data
    char BCODE[BCODE_CAPACITY];         ///< BCODE instruction buffer

    /** @brief Default constructor */
    BrevitestAssay() :
        duration(0),
        BCODE_length(0)
    {
        id[0] = '\0';
        BCODE[0] = '\0';
    }
};

/**
 * @brief EEPROM storage structure (DATA-004)
 * @details Persistent storage structure for firmware version, test counts,
 *          and running test recovery information. Stored in device EEPROM
 *          to survive power cycles.
 *
 * @note Must be backwards compatible with existing EEPROM data
 */
struct Particle_EEPROM {
    uint8_t firmware_version;           ///< Firmware version for compatibility checking
    uint8_t data_format_version;        ///< Data format version
    int32_t lifetime_stress_test_cycles;    ///< Total stress test cycles ever run
    int32_t stress_test_cycles_since_reset; ///< Cycles since last counter reset
    int32_t stress_test_cycles;         ///< Current stress test cycle count
    int32_t stress_test_reading_count;  ///< Readings taken in stress tests
    char running_test_uuid[BARCODE_UUID_LENGTH + 1]; ///< UUID for crash recovery
    char running_assay_id[ASSAY_UUID_LENGTH + 1];    ///< Assay ID for crash recovery

    /** @brief Default constructor with version info */
    Particle_EEPROM() :
        firmware_version(FIRMWARE_VERSION),
        data_format_version(DATA_FORMAT_VERSION),
        lifetime_stress_test_cycles(0),
        stress_test_cycles_since_reset(0),
        stress_test_cycles(0),
        stress_test_reading_count(0)
    {
        running_test_uuid[0] = '\0';
        running_assay_id[0] = '\0';
    }
};

#pragma pack(pop)

//==============================================================================
// SERIALIZATION UTILITIES (DATA-006)
//==============================================================================

/**
 * @brief Calculate CRC32 checksum for data buffer
 * @param data Pointer to data buffer
 * @param length Length of data in bytes
 * @return CRC32 checksum value
 */
uint32_t calculateCRC32(const uint8_t* data, size_t length);

/**
 * @brief Calculate checksum for test record
 * @param record Pointer to test record
 * @return CRC32 checksum of record (excluding checksum field itself)
 */
uint32_t calculateTestRecordChecksum(const BrevitestTestRecord* record);

/**
 * @brief Verify test record checksum
 * @param record Pointer to test record
 * @return true if checksum is valid, false otherwise
 */
bool verifyTestRecordChecksum(const BrevitestTestRecord* record);

/**
 * @brief Serialize test record to byte array
 * @param record Pointer to test record to serialize
 * @param buffer Output buffer (must be at least 9668 bytes)
 * @param bufferSize Size of output buffer
 * @return Number of bytes written, or 0 on error
 */
size_t serializeTestRecord(const BrevitestTestRecord* record, uint8_t* buffer, size_t bufferSize);

/**
 * @brief Deserialize byte array to test record
 * @param buffer Input buffer containing serialized data
 * @param bufferSize Size of input buffer
 * @param record Pointer to test record to populate
 * @return true on success, false on error
 */
bool deserializeTestRecord(const uint8_t* buffer, size_t bufferSize, BrevitestTestRecord* record);

/**
 * @brief Calculate checksum for assay BCODE
 * @param assay Pointer to assay structure
 * @return CRC32 checksum of BCODE data
 */
uint32_t calculateAssayChecksum(const BrevitestAssay* assay);

/**
 * @brief Verify assay checksum against expected value
 * @param assay Pointer to assay structure
 * @param expectedChecksum Expected checksum value
 * @return true if checksum matches, false otherwise
 */
bool verifyAssayChecksum(const BrevitestAssay* assay, uint32_t expectedChecksum);

/**
 * @brief Encode binary data to Base64 string
 * @param data Input binary data
 * @param dataLength Length of input data
 * @param output Output buffer for Base64 string (must be at least (dataLength * 4 / 3) + 4 bytes)
 * @param outputSize Size of output buffer
 * @return Length of encoded string, or 0 on error
 */
size_t base64Encode(const uint8_t* data, size_t dataLength, char* output, size_t outputSize);

/**
 * @brief Decode Base64 string to binary data
 * @param input Base64 encoded string
 * @param inputLength Length of input string
 * @param output Output buffer for binary data
 * @param outputSize Size of output buffer
 * @return Length of decoded data, or 0 on error
 */
size_t base64Decode(const char* input, size_t inputLength, uint8_t* output, size_t outputSize);

/**
 * @brief Initialize a spectrophotometer reading structure
 * @param reading Pointer to reading structure to initialize
 */
void initSpectrophotometerReading(BrevitestSpectrophotometerReading* reading);

/**
 * @brief Initialize a test record structure
 * @param record Pointer to test record to initialize
 */
void initTestRecord(BrevitestTestRecord* record);

/**
 * @brief Copy test record with deep copy of readings
 * @param dest Destination test record
 * @param src Source test record
 */
void copyTestRecord(BrevitestTestRecord* dest, const BrevitestTestRecord* src);

//==============================================================================
// STRING CONVERSION UTILITIES
//==============================================================================

/**
 * @brief Convert DeviceMode enum to string
 * @param mode Device mode value
 * @return Constant string representation
 */
const char* deviceModeToString(DeviceMode mode);

/**
 * @brief Convert TestState enum to string
 * @param state Test state value
 * @return Constant string representation
 */
const char* testStateToString(TestState state);

/**
 * @brief Convert CartridgeState enum to string
 * @param state Cartridge state value
 * @return Constant string representation
 */
const char* cartridgeStateToString(CartridgeState state);

/**
 * @brief Convert BarcodeType enum to string
 * @param type Barcode type value
 * @return Constant string representation
 */
const char* barcodeTypeToString(BarcodeType type);

/**
 * @brief Convert ErrorCode enum to string
 * @param code Error code value
 * @return Constant string representation
 */
const char* errorCodeToString(ErrorCode code);

#endif // DATATYPES_H
