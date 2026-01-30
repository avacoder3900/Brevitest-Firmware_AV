/**
 * @file BarcodeScanner.h
 * @brief Barcode scanner controller for Brevitest device
 * @author Agent KAPPA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module provides barcode scanner functionality for reading cartridge UUIDs.
 * Features:
 * - Scan triggering with configurable timeout
 * - UUID validation (36 character format)
 * - Barcode type detection (Cartridge, Magnetometer, Optical, StressTest, Shipping)
 * - State machine for scan process management
 * - Retry logic for failed scans
 *
 * Hardware Configuration:
 * - Ready pin: D22 (input, active HIGH when scan complete)
 * - Trigger pin: D23 (output, active LOW to start scan)
 * - Serial: 9600 baud on Serial1
 *
 * User Stories Implemented:
 *   - BAR-001: BarcodeScanner class with initialization
 *   - BAR-002: Scan triggering with timeout
 *   - BAR-003: Barcode parsing with UUID validation
 *   - BAR-004: Barcode type detection
 *   - BAR-005: Scan state machine
 */

#ifndef BARCODE_SCANNER_H
#define BARCODE_SCANNER_H

#include "Particle.h"
#include "HardwareConfig.h"
#include "DataTypes.h"

// ============================================================================
// BARCODE CONSTANTS
// ============================================================================

/** @brief Length of standard cartridge UUID (36 characters: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx) */
constexpr uint8_t BARCODE_CARTRIDGE_UUID_LENGTH = 36;

/** @brief Length of magnetometer UUID (32 characters) */
constexpr uint8_t BARCODE_MAGNETOMETER_UUID_LENGTH = 32;

/** @brief Length of stress test UUID (16 characters) */
constexpr uint8_t BARCODE_STRESS_TEST_UUID_LENGTH = 16;

/** @brief Barcode prefix length for type detection */
constexpr uint8_t BARCODE_PREFIX_LENGTH = 4;

/** @brief Magnetometer barcode prefix */
constexpr char MAGNETOMETER_PREFIX[] = "MAG-";

/** @brief Stress test barcode prefix */
constexpr char STRESS_TEST_PREFIX[] = "STRESS-TEST-";

/** @brief Stress test prefix length */
constexpr uint8_t STRESS_TEST_PREFIX_LENGTH = 12;

/** @brief Error message for failed barcode reads */
constexpr char BARCODE_ERROR_MESSAGE[] = "---BARCODE READ ERROR---";

/** @brief Length of error message */
constexpr uint8_t BARCODE_ERROR_MESSAGE_LENGTH = 24;

/** @brief Maximum buffer size for barcode string */
constexpr uint8_t BARCODE_BUFFER_SIZE = 48;

/** @brief Default scan timeout in milliseconds */
constexpr uint16_t BARCODE_DEFAULT_TIMEOUT_MS = 1000;

/** @brief Delay between serial reads in microseconds */
constexpr uint32_t BARCODE_SERIAL_DELAY_US = 100000;

/** @brief Serial baud rate for barcode scanner */
constexpr uint32_t BARCODE_SERIAL_BAUD = 9600;

/** @brief Maximum retry attempts for failed scans */
constexpr uint8_t BARCODE_MAX_RETRIES = 3;

// ============================================================================
// SCAN STATE ENUMERATION
// ============================================================================

/**
 * @brief Scanner state machine states
 * @details Represents the current state of the barcode scanning process
 */
enum class ScanState : uint8_t {
    IDLE = 0,       ///< Scanner idle, ready to scan
    SCANNING = 1,   ///< Scan in progress
    SUCCESS = 2,    ///< Scan completed successfully
    FAILED = 3,     ///< Scan failed (invalid barcode)
    TIMEOUT = 4     ///< Scan timed out
};

/**
 * @brief Callback function type for scan completion
 * @param barcode The scanned barcode string (empty if failed)
 * @param type The detected barcode type
 * @param success True if scan was successful
 */
typedef void (*ScanCompleteCallback)(const char* barcode, BarcodeType type, bool success);

// ============================================================================
// BARCODE SCANNER CLASS
// ============================================================================

/**
 * @class BarcodeScanner
 * @brief Controller class for barcode scanner hardware
 *
 * Provides complete barcode scanning functionality including:
 * - Hardware initialization
 * - Scan triggering and timeout management
 * - Barcode parsing and validation
 * - Type detection based on barcode format
 * - State machine for scan process
 *
 * Usage Example:
 * @code
 *   BarcodeScanner scanner;
 *   scanner.init();
 *
 *   if (scanner.isReady()) {
 *       BarcodeType type = scanner.triggerScan();
 *       if (type == BarcodeType::CARTRIDGE) {
 *           const char* uuid = scanner.getLastBarcode();
 *           // Process cartridge UUID
 *       }
 *   }
 * @endcode
 */
class BarcodeScanner {
public:
    // ========================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ========================================================================

    /**
     * @brief Default constructor
     */
    BarcodeScanner();

    /**
     * @brief Destructor
     */
    ~BarcodeScanner();

    // ========================================================================
    // INITIALIZATION (BAR-001)
    // ========================================================================

    /**
     * @brief Initialize the barcode scanner
     *
     * Configures scanner pins and prepares for operation:
     * - Ready pin (D22) configured as INPUT_PULLUP
     * - Trigger pin (D23) configured as OUTPUT, set HIGH (inactive)
     *
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Check if scanner is initialized
     * @return true if init() has been called successfully
     */
    bool isInitialized() const;

    /**
     * @brief Check if scanner is ready to scan
     *
     * Checks the scanner ready signal to determine if a barcode
     * can be read.
     *
     * @return true if scanner hardware is ready
     */
    bool isReady() const;

    // ========================================================================
    // SCAN TRIGGERING (BAR-002)
    // ========================================================================

    /**
     * @brief Trigger a barcode scan
     *
     * Initiates a scan, waits for completion or timeout, reads the barcode
     * data, and determines the barcode type. This is a blocking call.
     *
     * @param timeout Timeout in milliseconds (default: BARCODE_DEFAULT_TIMEOUT_MS)
     * @return BarcodeType indicating the type of barcode read, or error code
     */
    BarcodeType triggerScan(uint16_t timeout = BARCODE_DEFAULT_TIMEOUT_MS);

    /**
     * @brief Wait for a scan to complete
     *
     * Blocks until scanner signals completion or timeout occurs.
     *
     * @param timeout Maximum wait time in milliseconds
     * @return true if scan completed, false if timed out
     */
    bool waitForScan(uint16_t timeout);

    /**
     * @brief Cancel an in-progress scan
     *
     * Releases the trigger and resets the scan state.
     */
    void cancelScan();

    // ========================================================================
    // BARCODE PARSING (BAR-003)
    // ========================================================================

    /**
     * @brief Read barcode data from serial
     *
     * Reads the scanned barcode string from the serial interface,
     * strips whitespace, and validates the format.
     *
     * @return Pointer to the barcode string (internal buffer), or nullptr if failed
     */
    const char* readBarcode();

    /**
     * @brief Get the last successfully scanned barcode
     * @return Pointer to the last barcode string, or empty string if none
     */
    const char* getLastBarcode() const;

    /**
     * @brief Get the last barcode type detected
     * @return BarcodeType of the last scan
     */
    BarcodeType getLastBarcodeType() const;

    /**
     * @brief Clear the last barcode data
     */
    void clearLastBarcode();

    /**
     * @brief Validate UUID format
     *
     * Checks if the string is a valid UUID format:
     * xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx (36 characters)
     *
     * @param barcode The barcode string to validate
     * @return true if valid UUID format
     */
    static bool isValidUUID(const char* barcode);

    /**
     * @brief Validate barcode length for given type
     *
     * @param barcode The barcode string to validate
     * @param type Expected barcode type
     * @return true if length is valid for the type
     */
    static bool isValidLength(const char* barcode, BarcodeType type);

    // ========================================================================
    // BARCODE TYPE DETECTION (BAR-004)
    // ========================================================================

    /**
     * @brief Determine barcode type from scanned data
     *
     * Analyzes the barcode string to identify its type based on:
     * - Length (36 chars = Cartridge, 32 chars = Magnetometer, etc.)
     * - Prefix (MAG- = Magnetometer, STRESS-TEST- = Stress test)
     *
     * @param barcode The barcode string to analyze
     * @return BarcodeType indicating the detected type
     */
    static BarcodeType getBarcodeType(const char* barcode);

    // ========================================================================
    // SCAN STATE MACHINE (BAR-005)
    // ========================================================================

    /**
     * @brief Get current scan state
     * @return Current ScanState
     */
    ScanState getScanState() const;

    /**
     * @brief Get number of retries performed
     * @return Number of retries in current scan operation
     */
    uint8_t getRetryCount() const;

    /**
     * @brief Set maximum retry count
     * @param maxRetries Maximum number of retry attempts
     */
    void setMaxRetries(uint8_t maxRetries);

    /**
     * @brief Get maximum retry count
     * @return Maximum retry attempts configured
     */
    uint8_t getMaxRetries() const;

    /**
     * @brief Register callback for scan completion
     *
     * The callback will be invoked when a scan completes (success or failure).
     *
     * @param callback Function pointer to callback, or nullptr to clear
     */
    void setCallback(ScanCompleteCallback callback);

    /**
     * @brief Reset scanner state to IDLE
     *
     * Clears retry count and resets state machine.
     */
    void reset();

private:
    // ========================================================================
    // PRIVATE MEMBER VARIABLES
    // ========================================================================

    bool _initialized;                              ///< Initialization flag
    char _barcodeBuffer[BARCODE_BUFFER_SIZE];       ///< Buffer for barcode string
    BarcodeType _lastType;                          ///< Last detected barcode type
    ScanState _state;                               ///< Current scan state
    uint8_t _retryCount;                            ///< Current retry count
    uint8_t _maxRetries;                            ///< Maximum retry attempts
    ScanCompleteCallback _callback;                 ///< Scan completion callback

    // ========================================================================
    // PRIVATE METHODS
    // ========================================================================

    /**
     * @brief Activate scanner trigger (pull LOW)
     */
    void activateTrigger();

    /**
     * @brief Release scanner trigger (set HIGH)
     */
    void releaseTrigger();

    /**
     * @brief Open serial port for scanner communication
     */
    void openSerial();

    /**
     * @brief Close serial port
     */
    void closeSerial();

    /**
     * @brief Transition to new state
     * @param newState The state to transition to
     */
    void setState(ScanState newState);

    /**
     * @brief Invoke registered callback
     * @param success Whether scan was successful
     */
    void invokeCallback(bool success);

    /**
     * @brief Strip leading and trailing whitespace from string
     * @param str String to trim (modified in place)
     */
    static void trimWhitespace(char* str);
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Convert ScanState enum to string
 * @param state Scan state value
 * @return Constant string representation
 */
const char* scanStateToString(ScanState state);

#endif // BARCODE_SCANNER_H
