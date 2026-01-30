/**
 * @file BarcodeScanner.cpp
 * @brief Implementation of barcode scanner controller for Brevitest device
 * @author Agent KAPPA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module implements barcode scanner functionality for reading cartridge UUIDs.
 * Compatible with legacy firmware barcode scanning behavior.
 *
 * User Stories Implemented:
 *   - BAR-001: BarcodeScanner class with initialization
 *   - BAR-002: Scan triggering with timeout
 *   - BAR-003: Barcode parsing with UUID validation
 *   - BAR-004: Barcode type detection
 *   - BAR-005: Scan state machine
 */

#include "BarcodeScanner.h"
#include "HAL.h"
#include <cstring>
#include <cctype>

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

BarcodeScanner::BarcodeScanner() :
    _initialized(false),
    _lastType(BarcodeType::GENERAL_ERROR),
    _state(ScanState::IDLE),
    _retryCount(0),
    _maxRetries(BARCODE_MAX_RETRIES),
    _callback(nullptr)
{
    memset(_barcodeBuffer, 0, BARCODE_BUFFER_SIZE);
}

BarcodeScanner::~BarcodeScanner()
{
    // Ensure trigger is released on destruction
    if (_initialized) {
        releaseTrigger();
    }
}

// ============================================================================
// INITIALIZATION (BAR-001)
// ============================================================================

bool BarcodeScanner::init()
{
    // Configure ready pin as input with pull-up
    pinMode(PIN_BARCODE_READY, INPUT_PULLUP);

    // Configure trigger pin as output, set HIGH (inactive - trigger is active LOW)
    pinMode(PIN_BARCODE_TRIGGER, OUTPUT);
    digitalWrite(PIN_BARCODE_TRIGGER, HIGH);

    // Clear barcode buffer
    memset(_barcodeBuffer, 0, BARCODE_BUFFER_SIZE);

    // Reset state
    _state = ScanState::IDLE;
    _retryCount = 0;
    _lastType = BarcodeType::GENERAL_ERROR;

    _initialized = true;

    Log.info("BarcodeScanner: Initialized (Ready: D%d, Trigger: D%d)",
             PIN_BARCODE_READY, PIN_BARCODE_TRIGGER);

    return true;
}

bool BarcodeScanner::isInitialized() const
{
    return _initialized;
}

bool BarcodeScanner::isReady() const
{
    if (!_initialized) {
        return false;
    }

    // Use HAL function if available, otherwise read directly
    return HAL::isBarcodeReady();
}

// ============================================================================
// SCAN TRIGGERING (BAR-002)
// ============================================================================

BarcodeType BarcodeScanner::triggerScan(uint16_t timeout)
{
    if (!_initialized) {
        Log.error("BarcodeScanner: Not initialized");
        return BarcodeType::GENERAL_ERROR;
    }

    // Reset retry count for new scan operation
    _retryCount = 0;

    // Retry loop
    while (_retryCount <= _maxRetries) {
        // Clear previous barcode
        memset(_barcodeBuffer, 0, BARCODE_BUFFER_SIZE);

        // Transition to scanning state
        setState(ScanState::SCANNING);

        // Open serial port to scanner
        openSerial();

        // Activate trigger (active LOW)
        activateTrigger();

        // Wait for scan to complete or timeout
        bool scanComplete = waitForScan(timeout);

        // Release trigger
        releaseTrigger();

        if (scanComplete) {
            // Read the barcode data
            const char* barcode = readBarcode();

            // Close serial port
            closeSerial();

            if (barcode != nullptr && strlen(barcode) > 0) {
                // Determine barcode type
                _lastType = getBarcodeType(barcode);

                if (static_cast<int8_t>(_lastType) > 0) {
                    // Valid barcode type
                    setState(ScanState::SUCCESS);
                    Log.info("BarcodeScanner: Scan successful, type=%d, barcode=%s",
                             static_cast<int>(_lastType), _barcodeBuffer);
                    invokeCallback(true);
                    return _lastType;
                } else {
                    // Invalid barcode type (validation error)
                    Log.warn("BarcodeScanner: Invalid barcode format: %s (length: %d)",
                             _barcodeBuffer, strlen(_barcodeBuffer));
                    setState(ScanState::FAILED);
                }
            } else {
                // No barcode data received
                Log.warn("BarcodeScanner: No barcode data received");
                setState(ScanState::FAILED);
            }
        } else {
            // Timeout occurred
            closeSerial();
            Log.warn("BarcodeScanner: Scan timeout");
            setState(ScanState::TIMEOUT);
        }

        // Increment retry count and try again if not exhausted
        _retryCount++;
        if (_retryCount <= _maxRetries) {
            Log.info("BarcodeScanner: Retry %d/%d", _retryCount, _maxRetries);
            delay(100); // Brief delay between retries
        }
    }

    // All retries exhausted
    Log.error("BarcodeScanner: Scan failed after %d retries", _retryCount);

    // Set error message in buffer
    strncpy(_barcodeBuffer, BARCODE_ERROR_MESSAGE, BARCODE_ERROR_MESSAGE_LENGTH);
    _barcodeBuffer[BARCODE_ERROR_MESSAGE_LENGTH] = '\0';
    _lastType = BarcodeType::VALIDATION_ERROR;

    invokeCallback(false);
    return _lastType;
}

bool BarcodeScanner::waitForScan(uint16_t timeout)
{
    uint32_t startTime = millis();
    uint32_t endTime = startTime + timeout;

    // Wait for scanner ready signal to go HIGH (scan complete)
    // Ready pin is LOW during scan, HIGH when complete
    while (digitalRead(PIN_BARCODE_READY) == LOW && millis() < endTime) {
        delayMicroseconds(BARCODE_SERIAL_DELAY_US);
    }

    // Return true if ready signal went HIGH before timeout
    return (digitalRead(PIN_BARCODE_READY) == HIGH);
}

void BarcodeScanner::cancelScan()
{
    // Release trigger immediately
    releaseTrigger();

    // Close serial if open
    closeSerial();

    // Reset state
    setState(ScanState::IDLE);
    _retryCount = 0;

    Log.info("BarcodeScanner: Scan cancelled");
}

// ============================================================================
// BARCODE PARSING (BAR-003)
// ============================================================================

const char* BarcodeScanner::readBarcode()
{
    int index = 0;
    int bytesRead;
    bool dataAvailable = false;

    // Wait for data to be available with timeout
    uint32_t timeout = millis() + 500; // 500ms timeout for data
    while (!Serial1.available() && millis() < timeout) {
        delayMicroseconds(BARCODE_SERIAL_DELAY_US);
    }

    // Brief delay to allow buffer to fill
    delayMicroseconds(BARCODE_SERIAL_DELAY_US);

    // Read data from serial
    while (Serial1.available() && index < (BARCODE_BUFFER_SIZE - 1)) {
        bytesRead = Serial1.read();
        if (bytesRead >= 0) {
            _barcodeBuffer[index++] = static_cast<char>(bytesRead);
            dataAvailable = true;
        }
    }

    // Null terminate
    if (index > 0) {
        // Remove trailing newline/carriage return if present
        while (index > 0 && (_barcodeBuffer[index - 1] == '\n' ||
                            _barcodeBuffer[index - 1] == '\r')) {
            index--;
        }
        _barcodeBuffer[index] = '\0';
    } else {
        _barcodeBuffer[0] = '\0';
    }

    // Strip whitespace
    trimWhitespace(_barcodeBuffer);

    if (!dataAvailable || strlen(_barcodeBuffer) == 0) {
        return nullptr;
    }

    return _barcodeBuffer;
}

const char* BarcodeScanner::getLastBarcode() const
{
    return _barcodeBuffer;
}

BarcodeType BarcodeScanner::getLastBarcodeType() const
{
    return _lastType;
}

void BarcodeScanner::clearLastBarcode()
{
    memset(_barcodeBuffer, 0, BARCODE_BUFFER_SIZE);
    _lastType = BarcodeType::GENERAL_ERROR;
}

bool BarcodeScanner::isValidUUID(const char* barcode)
{
    if (barcode == nullptr) {
        return false;
    }

    size_t len = strlen(barcode);

    // UUID must be exactly 36 characters
    if (len != BARCODE_CARTRIDGE_UUID_LENGTH) {
        return false;
    }

    // Check UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    // Positions 8, 13, 18, 23 should be hyphens
    const int hyphenPositions[] = {8, 13, 18, 23};

    for (int i = 0; i < 4; i++) {
        if (barcode[hyphenPositions[i]] != '-') {
            return false;
        }
    }

    // Check that all other characters are hexadecimal
    for (size_t i = 0; i < len; i++) {
        // Skip hyphen positions
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            continue;
        }

        char c = barcode[i];
        if (!isxdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }

    return true;
}

bool BarcodeScanner::isValidLength(const char* barcode, BarcodeType type)
{
    if (barcode == nullptr) {
        return false;
    }

    size_t len = strlen(barcode);

    switch (type) {
        case BarcodeType::CARTRIDGE:
            return len == BARCODE_CARTRIDGE_UUID_LENGTH;

        case BarcodeType::MAGNETOMETER:
            return len == BARCODE_MAGNETOMETER_UUID_LENGTH;

        case BarcodeType::STRESS_TEST:
            return len == BARCODE_STRESS_TEST_UUID_LENGTH;

        case BarcodeType::OPTICAL:
        case BarcodeType::SHIPPING:
            // No specific length requirements defined
            return len > 0;

        default:
            return false;
    }
}

// ============================================================================
// BARCODE TYPE DETECTION (BAR-004)
// ============================================================================

BarcodeType BarcodeScanner::getBarcodeType(const char* barcode)
{
    if (barcode == nullptr || strlen(barcode) == 0) {
        return BarcodeType::GENERAL_ERROR;
    }

    size_t len = strlen(barcode);

    // Check by length first (most common case)
    switch (len) {
        case BARCODE_CARTRIDGE_UUID_LENGTH:
            // 36 characters - standard cartridge UUID
            if (isValidUUID(barcode)) {
                return BarcodeType::CARTRIDGE;
            }
            break;

        case BARCODE_MAGNETOMETER_UUID_LENGTH:
            // 32 characters - magnetometer validation cartridge
            // Check for MAG- prefix
            if (strncmp(barcode, MAGNETOMETER_PREFIX, BARCODE_PREFIX_LENGTH) == 0) {
                return BarcodeType::MAGNETOMETER;
            }
            break;

        case BARCODE_STRESS_TEST_UUID_LENGTH:
            // 16 characters - stress test cartridge
            // Check for STRESS-TEST- prefix
            if (strncmp(barcode, STRESS_TEST_PREFIX, STRESS_TEST_PREFIX_LENGTH) == 0) {
                return BarcodeType::STRESS_TEST;
            }
            break;

        default:
            // Unknown length - check prefixes for other types
            break;
    }

    // Additional prefix checks for non-standard lengths
    if (len >= BARCODE_PREFIX_LENGTH) {
        if (strncmp(barcode, MAGNETOMETER_PREFIX, BARCODE_PREFIX_LENGTH) == 0) {
            return BarcodeType::MAGNETOMETER;
        }
    }

    if (len >= STRESS_TEST_PREFIX_LENGTH) {
        if (strncmp(barcode, STRESS_TEST_PREFIX, STRESS_TEST_PREFIX_LENGTH) == 0) {
            return BarcodeType::STRESS_TEST;
        }
    }

    // If we got here, barcode format is not recognized
    Log.warn("BarcodeScanner: Unknown barcode format - length=%d, barcode=%s", len, barcode);
    return BarcodeType::VALIDATION_ERROR;
}

// ============================================================================
// SCAN STATE MACHINE (BAR-005)
// ============================================================================

ScanState BarcodeScanner::getScanState() const
{
    return _state;
}

uint8_t BarcodeScanner::getRetryCount() const
{
    return _retryCount;
}

void BarcodeScanner::setMaxRetries(uint8_t maxRetries)
{
    _maxRetries = maxRetries;
}

uint8_t BarcodeScanner::getMaxRetries() const
{
    return _maxRetries;
}

void BarcodeScanner::setCallback(ScanCompleteCallback callback)
{
    _callback = callback;
}

void BarcodeScanner::reset()
{
    // Release trigger if active
    releaseTrigger();

    // Clear buffer
    memset(_barcodeBuffer, 0, BARCODE_BUFFER_SIZE);

    // Reset state
    _state = ScanState::IDLE;
    _retryCount = 0;
    _lastType = BarcodeType::GENERAL_ERROR;

    Log.info("BarcodeScanner: Reset to IDLE state");
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

void BarcodeScanner::activateTrigger()
{
    // Use HAL function if available
    HAL::triggerBarcodeScanner();
}

void BarcodeScanner::releaseTrigger()
{
    // Use HAL function if available
    HAL::releaseBarcodeScanner();
}

void BarcodeScanner::openSerial()
{
    Serial1.begin(BARCODE_SERIAL_BAUD);
}

void BarcodeScanner::closeSerial()
{
    Serial1.end();
}

void BarcodeScanner::setState(ScanState newState)
{
    if (_state != newState) {
        Log.trace("BarcodeScanner: State %s -> %s",
                  scanStateToString(_state),
                  scanStateToString(newState));
        _state = newState;
    }
}

void BarcodeScanner::invokeCallback(bool success)
{
    if (_callback != nullptr) {
        _callback(_barcodeBuffer, _lastType, success);
    }
}

void BarcodeScanner::trimWhitespace(char* str)
{
    if (str == nullptr || strlen(str) == 0) {
        return;
    }

    // Trim leading whitespace
    char* start = str;
    while (*start && isspace(static_cast<unsigned char>(*start))) {
        start++;
    }

    // Trim trailing whitespace
    char* end = str + strlen(str) - 1;
    while (end > start && isspace(static_cast<unsigned char>(*end))) {
        end--;
    }

    // Null terminate at new end
    *(end + 1) = '\0';

    // Shift string if leading whitespace was removed
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

const char* scanStateToString(ScanState state)
{
    switch (state) {
        case ScanState::IDLE:       return "IDLE";
        case ScanState::SCANNING:   return "SCANNING";
        case ScanState::SUCCESS:    return "SUCCESS";
        case ScanState::FAILED:     return "FAILED";
        case ScanState::TIMEOUT:    return "TIMEOUT";
        default:                    return "UNKNOWN";
    }
}
