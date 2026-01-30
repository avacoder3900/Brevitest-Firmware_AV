/**
 * @file StateMachine.cpp
 * @brief Implementation of core state machine for Brevitest firmware
 * @details Implements all state transition logic, validation, history tracking,
 *          and state management functions.
 *
 * @version 1.0
 * @date 2026-01-29
 *
 * User Stories Implemented:
 *   - SM-001: DeviceState class implementation
 *   - SM-002: State transition validation matrix
 *   - SM-003: Transition history tracking (50-entry circular buffer)
 *   - SM-004: setMode() with validation
 *   - SM-005: Test state management
 *   - SM-006: Cartridge state management
 *   - SM-007: State query functions
 */

#include "StateMachine.h"
#include <string.h>
#include <stdio.h>

//==============================================================================
// PLATFORM-SPECIFIC TIME FUNCTIONS
//==============================================================================

// Forward declarations for platform-specific functions
// These should be provided by Particle.h in production or mocked in tests
#ifdef PARTICLE
#include "Particle.h"
#define GET_MILLIS() millis()
#define GET_TIME() (Time.isValid() ? Time.now() : (millis() / 1000))
#else
// For testing without Particle SDK
static uint32_t _mockMillis = 0;
static uint32_t _mockTime = 1706500000; // Jan 29, 2024
#define GET_MILLIS() _mockMillis
#define GET_TIME() _mockTime
#endif

//==============================================================================
// GLOBAL STATE MACHINE INSTANCE
//==============================================================================

StateMachine deviceState;

StateMachine& getStateMachine() {
    return deviceState;
}

//==============================================================================
// STATE TRANSITION ENTRY IMPLEMENTATION
//==============================================================================

StateTransitionEntry::StateTransitionEntry()
    : from_mode(DeviceMode::INITIALIZING)
    , to_mode(DeviceMode::INITIALIZING)
    , timestamp(0)
{
    cartridge_id[0] = '\0';
}

StateTransitionEntry::StateTransitionEntry(DeviceMode from, DeviceMode to,
                                           uint32_t time, const char* cartridge)
    : from_mode(from)
    , to_mode(to)
    , timestamp(time)
{
    if (cartridge != nullptr && cartridge[0] != '\0') {
        strncpy(cartridge_id, cartridge, CARTRIDGE_ID_MAX_LENGTH - 1);
        cartridge_id[CARTRIDGE_ID_MAX_LENGTH - 1] = '\0';
    } else {
        cartridge_id[0] = '\0';
    }
}

//==============================================================================
// STATE MACHINE CONSTRUCTOR AND INITIALIZATION
//==============================================================================

StateMachine::StateMachine()
    : _initialized(false)
    , _currentMode(DeviceMode::INITIALIZING)
    , _previousMode(DeviceMode::INITIALIZING)
    , _lastTransitionTime(0)
    , _testState(TestState::NOT_STARTED)
    , _cartridgeState(CartridgeState::NOT_INSERTED)
    , _cloudOperationPending(false)
    , _cloudOperationStartTime(0)
    , _lastErrorCode(ErrorCode::SUCCESS)
    , _historyIndex(0)
    , _historyCount(0)
    , _observerCount(0)
{
    _currentCartridgeId[0] = '\0';
    _lastErrorMessage[0] = '\0';

    // Initialize observer array
    for (int i = 0; i < MAX_OBSERVERS; i++) {
        _observers[i] = nullptr;
    }
}

bool StateMachine::init() {
    if (_initialized) {
        return true; // Already initialized
    }

    // Reset all state to known values
    _currentMode = DeviceMode::INITIALIZING;
    _previousMode = DeviceMode::INITIALIZING;
    _lastTransitionTime = getCurrentTimestamp();
    _testState = TestState::NOT_STARTED;
    _cartridgeState = CartridgeState::NOT_INSERTED;
    _cloudOperationPending = false;
    _cloudOperationStartTime = 0;
    _lastErrorCode = ErrorCode::SUCCESS;
    _lastErrorMessage[0] = '\0';
    _currentCartridgeId[0] = '\0';
    _historyIndex = 0;
    _historyCount = 0;

    _initialized = true;
    return true;
}

bool StateMachine::isInitialized() const {
    return _initialized;
}

//==============================================================================
// STATE TRANSITION VALIDATION (SM-002)
//==============================================================================

bool StateMachine::isValidTransition(DeviceMode from_mode, DeviceMode to_mode) {
    // Same state is not a transition
    if (from_mode == to_mode) {
        return false;
    }

    // Can ALWAYS transition to ERROR_STATE (safety mechanism)
    if (to_mode == DeviceMode::ERROR_STATE) {
        return true;
    }

    // Can ALWAYS transition to IDLE (reset/cleanup mechanism)
    if (to_mode == DeviceMode::IDLE) {
        return true;
    }

    // State-specific transition rules based on legacy firmware
    switch (from_mode) {
        case DeviceMode::INITIALIZING:
            // INITIALIZING -> IDLE (normal startup)
            // INITIALIZING -> HEATING (if temp not ready during startup)
            return to_mode == DeviceMode::IDLE ||
                   to_mode == DeviceMode::HEATING;

        case DeviceMode::IDLE:
            // IDLE -> BARCODE_SCANNING (cartridge inserted)
            // IDLE -> STRESS_TESTING (stress test cartridge detected)
            // IDLE -> HEATING (temperature drops below threshold)
            // IDLE -> UPLOADING_RESULTS (cached results to upload)
            return to_mode == DeviceMode::BARCODE_SCANNING ||
                   to_mode == DeviceMode::STRESS_TESTING ||
                   to_mode == DeviceMode::HEATING ||
                   to_mode == DeviceMode::UPLOADING_RESULTS;

        case DeviceMode::HEATING:
            // HEATING -> IDLE (temp ready, no cartridge)
            // HEATING -> BARCODE_SCANNING (temp ready, cartridge present)
            // HEATING -> STRESS_TESTING (temp ready, stress test mode)
            return to_mode == DeviceMode::IDLE ||
                   to_mode == DeviceMode::BARCODE_SCANNING ||
                   to_mode == DeviceMode::STRESS_TESTING;

        case DeviceMode::BARCODE_SCANNING:
            // BARCODE_SCANNING -> VALIDATING_CARTRIDGE (test cartridge scanned)
            // BARCODE_SCANNING -> VALIDATING_MAGNETOMETER (magnetometer scanned)
            // BARCODE_SCANNING -> STRESS_TESTING (stress test barcode)
            // BARCODE_SCANNING -> IDLE (cartridge removed, scan failed)
            return to_mode == DeviceMode::VALIDATING_CARTRIDGE ||
                   to_mode == DeviceMode::VALIDATING_MAGNETOMETER ||
                   to_mode == DeviceMode::STRESS_TESTING ||
                   to_mode == DeviceMode::IDLE;

        case DeviceMode::VALIDATING_CARTRIDGE:
            // VALIDATING_CARTRIDGE -> RUNNING_TEST (validation success)
            // VALIDATING_CARTRIDGE -> RESETTING_CARTRIDGE (already used)
            // VALIDATING_CARTRIDGE -> IDLE (validation failed, cartridge removed)
            return to_mode == DeviceMode::RUNNING_TEST ||
                   to_mode == DeviceMode::RESETTING_CARTRIDGE ||
                   to_mode == DeviceMode::IDLE;

        case DeviceMode::VALIDATING_MAGNETOMETER:
            // VALIDATING_MAGNETOMETER -> IDLE (validation complete or failed)
            return to_mode == DeviceMode::IDLE;

        case DeviceMode::RUNNING_TEST:
            // RUNNING_TEST -> UPLOADING_RESULTS (test complete)
            // RUNNING_TEST -> IDLE (test cancelled, cartridge removed)
            return to_mode == DeviceMode::UPLOADING_RESULTS ||
                   to_mode == DeviceMode::IDLE;

        case DeviceMode::UPLOADING_RESULTS:
            // UPLOADING_RESULTS -> IDLE (upload complete or failed)
            return to_mode == DeviceMode::IDLE;

        case DeviceMode::RESETTING_CARTRIDGE:
            // RESETTING_CARTRIDGE -> IDLE (reset complete)
            return to_mode == DeviceMode::IDLE;

        case DeviceMode::STRESS_TESTING:
            // STRESS_TESTING -> IDLE (stress test stopped)
            return to_mode == DeviceMode::IDLE;

        case DeviceMode::ERROR_STATE:
            // ERROR_STATE -> IDLE (error cleared)
            return to_mode == DeviceMode::IDLE;

        default:
            // Unknown state - deny all transitions
            return false;
    }
}

bool StateMachine::canTransitionTo(DeviceMode target_mode) const {
    return isValidTransition(_currentMode, target_mode);
}

//==============================================================================
// STATE TRANSITION METHODS (SM-004)
//==============================================================================

ErrorCode StateMachine::setMode(DeviceMode new_mode) {
    // Validate the transition
    if (!canTransitionTo(new_mode)) {
        return ErrorCode::ERR_INVALID_STATE;
    }

    // Store previous mode
    DeviceMode old_mode = _currentMode;

    // Log the transition to history
    logTransition(old_mode, new_mode);

    // Update state
    _previousMode = old_mode;
    _currentMode = new_mode;
    _lastTransitionTime = getCurrentTimestamp();

    // Notify observers
    notifyObservers(old_mode, new_mode);

    return ErrorCode::SUCCESS;
}

ErrorCode StateMachine::forceMode(DeviceMode new_mode) {
    // Store previous mode
    DeviceMode old_mode = _currentMode;

    // Log the transition to history (even forced ones)
    logTransition(old_mode, new_mode);

    // Update state without validation
    _previousMode = old_mode;
    _currentMode = new_mode;
    _lastTransitionTime = getCurrentTimestamp();

    // Notify observers
    notifyObservers(old_mode, new_mode);

    return ErrorCode::SUCCESS;
}

void StateMachine::resetToIdle() {
    // Force transition to IDLE
    DeviceMode old_mode = _currentMode;

    // Log transition
    logTransition(old_mode, DeviceMode::IDLE);

    // Reset primary state
    _previousMode = old_mode;
    _currentMode = DeviceMode::IDLE;
    _lastTransitionTime = getCurrentTimestamp();

    // Reset sub-states
    _testState = TestState::NOT_STARTED;
    _cartridgeState = CartridgeState::NOT_INSERTED;

    // Clear cloud operation
    _cloudOperationPending = false;
    _cloudOperationStartTime = 0;

    // Clear error state
    _lastErrorCode = ErrorCode::SUCCESS;
    _lastErrorMessage[0] = '\0';

    // Clear cartridge ID
    _currentCartridgeId[0] = '\0';

    // Notify observers
    notifyObservers(old_mode, DeviceMode::IDLE);
}

//==============================================================================
// STATE QUERY METHODS (SM-007)
//==============================================================================

DeviceMode StateMachine::getCurrentMode() const {
    return _currentMode;
}

DeviceMode StateMachine::getPreviousMode() const {
    return _previousMode;
}

const char* StateMachine::getCurrentModeString() const {
    return deviceModeToString(_currentMode);
}

uint32_t StateMachine::getLastTransitionTime() const {
    return _lastTransitionTime;
}

size_t StateMachine::getStateAsJson(char* buffer, size_t bufferSize) const {
    if (buffer == nullptr || bufferSize == 0) {
        return 0;
    }

    int written = snprintf(buffer, bufferSize,
        "{"
        "\"mode\":\"%s\","
        "\"mode_code\":%d,"
        "\"previous_mode\":\"%s\","
        "\"test_state\":\"%s\","
        "\"cartridge_state\":\"%s\","
        "\"cartridge_id\":\"%s\","
        "\"cloud_pending\":%s,"
        "\"error_code\":%d,"
        "\"last_transition\":%lu,"
        "\"transition_count\":%d"
        "}",
        deviceModeToString(_currentMode),
        static_cast<int>(_currentMode),
        deviceModeToString(_previousMode),
        testStateToString(_testState),
        cartridgeStateToString(_cartridgeState),
        _currentCartridgeId,
        _cloudOperationPending ? "true" : "false",
        static_cast<int>(_lastErrorCode),
        (unsigned long)_lastTransitionTime,
        _historyCount
    );

    if (written < 0 || written >= (int)bufferSize) {
        buffer[0] = '\0';
        return 0;
    }

    return (size_t)written;
}

// Convenience query functions
bool StateMachine::isIdle() const {
    return _currentMode == DeviceMode::IDLE;
}

bool StateMachine::isInitializing() const {
    return _currentMode == DeviceMode::INITIALIZING;
}

bool StateMachine::isHeating() const {
    return _currentMode == DeviceMode::HEATING;
}

bool StateMachine::isBarcodeScanning() const {
    return _currentMode == DeviceMode::BARCODE_SCANNING;
}

bool StateMachine::isValidatingCartridge() const {
    return _currentMode == DeviceMode::VALIDATING_CARTRIDGE;
}

bool StateMachine::isRunningTest() const {
    return _currentMode == DeviceMode::RUNNING_TEST;
}

bool StateMachine::isUploadingResults() const {
    return _currentMode == DeviceMode::UPLOADING_RESULTS;
}

bool StateMachine::isStressTesting() const {
    return _currentMode == DeviceMode::STRESS_TESTING;
}

bool StateMachine::isInErrorState() const {
    return _currentMode == DeviceMode::ERROR_STATE;
}

bool StateMachine::canAcceptCartridge() const {
    // Can accept cartridge when idle and no cartridge present
    return isIdle() && !hasCartridge();
}

bool StateMachine::isHeatingRequired() const {
    // Heating is required when in HEATING mode
    return isHeating();
}

bool StateMachine::isCloudOperationInProgress() const {
    return _cloudOperationPending ||
           _currentMode == DeviceMode::VALIDATING_CARTRIDGE ||
           _currentMode == DeviceMode::VALIDATING_MAGNETOMETER ||
           _currentMode == DeviceMode::UPLOADING_RESULTS ||
           _currentMode == DeviceMode::RESETTING_CARTRIDGE;
}

bool StateMachine::isBusy() const {
    return _currentMode != DeviceMode::IDLE &&
           _currentMode != DeviceMode::ERROR_STATE;
}

//==============================================================================
// TEST STATE MANAGEMENT (SM-005)
//==============================================================================

TestState StateMachine::getTestState() const {
    return _testState;
}

const char* StateMachine::getTestStateString() const {
    return testStateToString(_testState);
}

ErrorCode StateMachine::setTestState(TestState state) {
    // Validate test state transitions
    TestState current = _testState;

    // Define valid test state transitions
    bool valid = false;

    switch (current) {
        case TestState::NOT_STARTED:
            valid = (state == TestState::RUNNING);
            break;

        case TestState::RUNNING:
            valid = (state == TestState::COMPLETED ||
                     state == TestState::CANCELLED);
            break;

        case TestState::COMPLETED:
            valid = (state == TestState::UPLOAD_PENDING ||
                     state == TestState::UPLOAD_IN_PROGRESS ||
                     state == TestState::NOT_STARTED); // Reset for new test
            break;

        case TestState::CANCELLED:
            valid = (state == TestState::NOT_STARTED); // Reset for new test
            break;

        case TestState::UPLOAD_PENDING:
            valid = (state == TestState::UPLOAD_IN_PROGRESS ||
                     state == TestState::NOT_STARTED);
            break;

        case TestState::UPLOAD_IN_PROGRESS:
            valid = (state == TestState::UPLOADED ||
                     state == TestState::UPLOAD_PENDING); // Retry
            break;

        case TestState::UPLOADED:
            valid = (state == TestState::NOT_STARTED); // Reset for new test
            break;

        default:
            valid = false;
    }

    // Also allow resetting to NOT_STARTED from any state (for cleanup)
    if (state == TestState::NOT_STARTED) {
        valid = true;
    }

    if (!valid) {
        return ErrorCode::ERR_INVALID_STATE;
    }

    _testState = state;
    return ErrorCode::SUCCESS;
}

bool StateMachine::isTestInProgress() const {
    return _testState == TestState::RUNNING;
}

bool StateMachine::canStartNewTest() const {
    return _testState == TestState::NOT_STARTED ||
           _testState == TestState::UPLOADED ||
           _testState == TestState::CANCELLED;
}

bool StateMachine::hasResultsPendingUpload() const {
    return _testState == TestState::COMPLETED ||
           _testState == TestState::UPLOAD_PENDING;
}

//==============================================================================
// CARTRIDGE STATE MANAGEMENT (SM-006)
//==============================================================================

CartridgeState StateMachine::getCartridgeState() const {
    return _cartridgeState;
}

const char* StateMachine::getCartridgeStateString() const {
    return cartridgeStateToString(_cartridgeState);
}

ErrorCode StateMachine::setCartridgeState(CartridgeState state) {
    _cartridgeState = state;
    return ErrorCode::SUCCESS;
}

const char* StateMachine::getCurrentCartridgeId() const {
    return _currentCartridgeId;
}

void StateMachine::setCurrentCartridgeId(const char* cartridge_id) {
    if (cartridge_id != nullptr && cartridge_id[0] != '\0') {
        strncpy(_currentCartridgeId, cartridge_id, CARTRIDGE_ID_MAX_LENGTH - 1);
        _currentCartridgeId[CARTRIDGE_ID_MAX_LENGTH - 1] = '\0';
    }
}

void StateMachine::clearCurrentCartridgeId() {
    _currentCartridgeId[0] = '\0';
}

void StateMachine::onCartridgeInserted() {
    _cartridgeState = CartridgeState::DETECTED;
}

void StateMachine::onCartridgeRemoved() {
    // If test was running, cancel it
    if (_testState == TestState::RUNNING) {
        _testState = TestState::CANCELLED;
    }

    // Clear cartridge state
    _cartridgeState = CartridgeState::NOT_INSERTED;
    clearCurrentCartridgeId();

    // Transition to IDLE if we're in a state that requires cartridge
    if (_currentMode == DeviceMode::BARCODE_SCANNING ||
        _currentMode == DeviceMode::VALIDATING_CARTRIDGE ||
        _currentMode == DeviceMode::RUNNING_TEST) {
        // Use setMode for proper logging
        setMode(DeviceMode::IDLE);
    }
}

bool StateMachine::hasCartridge() const {
    return _cartridgeState != CartridgeState::NOT_INSERTED;
}

bool StateMachine::isCartridgeValidated() const {
    return _cartridgeState == CartridgeState::VALIDATED;
}

bool StateMachine::isCartridgeInvalid() const {
    return _cartridgeState == CartridgeState::INVALID;
}

//==============================================================================
// CLOUD OPERATION TRACKING
//==============================================================================

void StateMachine::startCloudOperation() {
    _cloudOperationPending = true;
    _cloudOperationStartTime = GET_MILLIS();
}

void StateMachine::endCloudOperation() {
    _cloudOperationPending = false;
    _cloudOperationStartTime = 0;
}

bool StateMachine::isCloudOperationPending() const {
    return _cloudOperationPending;
}

bool StateMachine::isCloudOperationTimeout(uint32_t timeout_ms) const {
    if (!_cloudOperationPending) {
        return false;
    }
    return (GET_MILLIS() - _cloudOperationStartTime) > timeout_ms;
}

uint32_t StateMachine::getCloudOperationElapsed() const {
    if (!_cloudOperationPending) {
        return 0;
    }
    return GET_MILLIS() - _cloudOperationStartTime;
}

//==============================================================================
// ERROR HANDLING
//==============================================================================

void StateMachine::setError(ErrorCode error_code, const char* error_msg) {
    _lastErrorCode = error_code;

    if (error_msg != nullptr) {
        strncpy(_lastErrorMessage, error_msg, sizeof(_lastErrorMessage) - 1);
        _lastErrorMessage[sizeof(_lastErrorMessage) - 1] = '\0';
    } else {
        _lastErrorMessage[0] = '\0';
    }

    // Transition to error state
    forceMode(DeviceMode::ERROR_STATE);
}

void StateMachine::clearError() {
    _lastErrorCode = ErrorCode::SUCCESS;
    _lastErrorMessage[0] = '\0';

    if (_currentMode == DeviceMode::ERROR_STATE) {
        setMode(DeviceMode::IDLE);
    }
}

ErrorCode StateMachine::getLastErrorCode() const {
    return _lastErrorCode;
}

const char* StateMachine::getLastErrorMessage() const {
    return _lastErrorMessage;
}

//==============================================================================
// TRANSITION HISTORY (SM-003)
//==============================================================================

int StateMachine::getTransitionCount() const {
    return _historyCount;
}

const StateTransitionEntry* StateMachine::getTransition(int index) const {
    if (index < 0 || index >= _historyCount) {
        return nullptr;
    }

    // Calculate actual position in circular buffer
    // Index 0 = most recent, which is at (historyIndex - 1)
    int actualIndex = (_historyIndex - 1 - index + TRANSITION_HISTORY_SIZE) % TRANSITION_HISTORY_SIZE;
    return &_transitionHistory[actualIndex];
}

int StateMachine::getTransitionHistory(StateTransitionEntry* entries, int maxEntries) const {
    if (entries == nullptr || maxEntries <= 0) {
        return 0;
    }

    int count = (maxEntries < _historyCount) ? maxEntries : _historyCount;

    for (int i = 0; i < count; i++) {
        const StateTransitionEntry* entry = getTransition(i);
        if (entry != nullptr) {
            entries[i] = *entry;
        }
    }

    return count;
}

int StateMachine::getHistoryByCartridge(const char* cartridge_id,
                                        StateTransitionEntry* entries,
                                        int maxEntries) const {
    if (cartridge_id == nullptr || cartridge_id[0] == '\0' ||
        entries == nullptr || maxEntries <= 0) {
        return 0;
    }

    int found = 0;

    for (int i = 0; i < _historyCount && found < maxEntries; i++) {
        const StateTransitionEntry* entry = getTransition(i);
        if (entry != nullptr && strcmp(entry->cartridge_id, cartridge_id) == 0) {
            entries[found++] = *entry;
        }
    }

    return found;
}

void StateMachine::clearHistory() {
    _historyIndex = 0;
    _historyCount = 0;
}

size_t StateMachine::getHistoryAsString(char* buffer, size_t bufferSize, int maxEntries) const {
    if (buffer == nullptr || bufferSize == 0) {
        return 0;
    }

    int entriesToShow = (maxEntries == 0 || maxEntries > _historyCount) ? _historyCount : maxEntries;

    // Start with header
    int written = snprintf(buffer, bufferSize, "Total:%d|", _historyCount);
    if (written < 0 || written >= (int)bufferSize) {
        buffer[0] = '\0';
        return 0;
    }

    size_t offset = (size_t)written;

    // Add entries (most recent first)
    for (int i = 0; i < entriesToShow && offset < bufferSize - 1; i++) {
        const StateTransitionEntry* entry = getTransition(i);
        if (entry == nullptr) continue;

        // Add separator
        if (i > 0 && offset < bufferSize - 1) {
            buffer[offset++] = ';';
        }

        // Format: timestamp:FROM>TO[cartridge_id]
        const char* cartridgeStr = (entry->cartridge_id[0] != '\0') ? entry->cartridge_id : "none";

        int entryLen = snprintf(buffer + offset, bufferSize - offset,
            "%lu:%s>%s[%s]",
            (unsigned long)entry->timestamp,
            deviceModeToString(entry->from_mode),
            deviceModeToString(entry->to_mode),
            cartridgeStr);

        if (entryLen < 0 || entryLen >= (int)(bufferSize - offset)) {
            break; // Not enough space
        }

        offset += (size_t)entryLen;
    }

    return offset;
}

//==============================================================================
// OBSERVER PATTERN
//==============================================================================

bool StateMachine::registerStateChangeCallback(StateChangeCallback callback) {
    if (callback == nullptr) {
        return false;
    }

    // Check if already registered
    for (int i = 0; i < _observerCount; i++) {
        if (_observers[i] == callback) {
            return true; // Already registered
        }
    }

    // Add if room available
    if (_observerCount < MAX_OBSERVERS) {
        _observers[_observerCount++] = callback;
        return true;
    }

    return false; // No room
}

bool StateMachine::unregisterStateChangeCallback(StateChangeCallback callback) {
    if (callback == nullptr) {
        return false;
    }

    for (int i = 0; i < _observerCount; i++) {
        if (_observers[i] == callback) {
            // Shift remaining observers down
            for (int j = i; j < _observerCount - 1; j++) {
                _observers[j] = _observers[j + 1];
            }
            _observerCount--;
            return true;
        }
    }

    return false; // Not found
}

//==============================================================================
// PRIVATE METHODS
//==============================================================================

void StateMachine::logTransition(DeviceMode from, DeviceMode to) {
    // Create new entry
    StateTransitionEntry entry(from, to, getCurrentTimestamp(), _currentCartridgeId);

    // Add to circular buffer
    _transitionHistory[_historyIndex] = entry;

    // Update index
    _historyIndex = (_historyIndex + 1) % TRANSITION_HISTORY_SIZE;

    // Update count
    if (_historyCount < TRANSITION_HISTORY_SIZE) {
        _historyCount++;
    }
}

void StateMachine::notifyObservers(DeviceMode from, DeviceMode to) {
    for (int i = 0; i < _observerCount; i++) {
        if (_observers[i] != nullptr) {
            _observers[i](from, to);
        }
    }
}

uint32_t StateMachine::getCurrentTimestamp() const {
    return GET_TIME();
}

//==============================================================================
// TEST SUPPORT FUNCTIONS (not for production use)
//==============================================================================

#ifndef PARTICLE
// Mock time control for testing
void setMockTime(uint32_t timestamp) {
    _mockTime = timestamp;
}

void setMockMillis(uint32_t ms) {
    _mockMillis = ms;
}

void advanceMockMillis(uint32_t ms) {
    _mockMillis += ms;
}
#endif
