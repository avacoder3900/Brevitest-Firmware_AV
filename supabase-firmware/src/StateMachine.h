/**
 * @file StateMachine.h
 * @brief Core state machine interface for Brevitest firmware
 * @details This module provides centralized state management for the device,
 *          replacing scattered boolean flags with a structured, validated
 *          state machine. It handles DeviceMode transitions with validation,
 *          history tracking, and error handling.
 *
 * Key Features:
 *   - Single source of truth for device state
 *   - Validated state transitions with error rejection
 *   - 50-entry circular buffer for transition history
 *   - Thread-safe state access via atomic operations
 *   - Integration with test and cartridge sub-states
 *
 * @note All state access should go through this module's API
 *
 * @version 2.0
 * @date 2026-02-11
 *
 * User Stories Implemented:
 *   - ALPHA-002: Validated state transitions and observer pattern
 *   - SM-001: DeviceState class with state members
 *   - SM-002: State transition validation matrix
 *   - SM-003: Transition history tracking (50-entry circular buffer)
 *   - SM-004: setMode() with validation
 *   - SM-005: Test state management
 *   - SM-006: Cartridge state management
 *   - SM-007: State query functions
 */

#ifndef STATEMACHINE_H
#define STATEMACHINE_H

#include "DataTypes.h"
#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// CONFIGURATION CONSTANTS
//==============================================================================

/** @brief Size of the transition history circular buffer */
#define TRANSITION_HISTORY_SIZE 50

/** @brief Maximum length of cartridge ID string (36 chars + null) */
#define CARTRIDGE_ID_MAX_LENGTH 37

/** @brief Default cloud operation timeout in milliseconds */
#define DEFAULT_CLOUD_TIMEOUT_MS 30000

//==============================================================================
// STATE TRANSITION ENTRY STRUCTURE
//==============================================================================

/**
 * @brief Single entry in the state transition history buffer
 * @details Stores complete information about a state transition including
 *          source mode, destination mode, timestamp, and associated cartridge.
 */
struct StateTransitionEntry {
    DeviceMode from_mode;                       ///< State transitioned from
    DeviceMode to_mode;                         ///< State transitioned to
    uint32_t timestamp;                         ///< Unix timestamp when transition occurred
    char cartridge_id[CARTRIDGE_ID_MAX_LENGTH]; ///< Associated cartridge UUID (36 chars + null)

    /**
     * @brief Default constructor - initializes to safe values
     */
    StateTransitionEntry();

    /**
     * @brief Parameterized constructor
     * @param from Source mode
     * @param to Destination mode
     * @param time Unix timestamp
     * @param cartridge Cartridge ID string (can be nullptr)
     */
    StateTransitionEntry(DeviceMode from, DeviceMode to, uint32_t time, const char* cartridge);
};

//==============================================================================
// STATE CHANGE CALLBACK TYPE
//==============================================================================

/**
 * @brief Function pointer type for state change observers
 * @param from Previous device mode
 * @param to New device mode
 */
typedef void (*StateChangeCallback)(DeviceMode from, DeviceMode to);

//==============================================================================
// DEVICE STATE MACHINE CLASS (SM-001)
//==============================================================================

/**
 * @brief Centralized device state machine
 * @details This class replaces scattered boolean flags with a structured,
 *          validated state management system. It provides:
 *          - State transition validation
 *          - Clear state visibility
 *          - Race condition prevention via atomic operations
 *          - Comprehensive error tracking
 *          - State transition history logging
 *
 * Thread Safety:
 *   State access is protected via volatile members and atomic-like operations
 *   for the single-core Particle B-Series SoM. For true multi-threaded environments,
 *   additional mutex protection would be needed.
 */
class StateMachine {
public:
    //==========================================================================
    // LIFECYCLE
    //==========================================================================

    /**
     * @brief Constructor - initializes state machine to safe defaults
     * @details Sets device to INITIALIZING mode with all sub-states reset.
     *          Should be called early in setup() before any state operations.
     */
    StateMachine();

    /**
     * @brief Initialize the state machine
     * @return true on success
     */
    bool init();

    /**
     * @brief Check if state machine is initialized
     * @return true if init() has been called successfully
     */
    bool isInitialized() const;

    //==========================================================================
    // STATE TRANSITION METHODS (SM-002, SM-004)
    //==========================================================================

    /**
     * @brief Check if a transition from current state to target is valid
     * @param target_mode The target mode to transition to
     * @return true if transition is valid, false otherwise
     *
     * @note Uses the transition validation matrix from legacy firmware
     */
    bool canTransitionTo(DeviceMode target_mode) const;

    /**
     * @brief Check if a transition between two specific modes is valid
     * @param from_mode Source mode
     * @param to_mode Target mode
     * @return true if transition is valid, false otherwise
     */
    static bool isValidTransition(DeviceMode from_mode, DeviceMode to_mode);

    /**
     * @brief Perform a validated state transition
     * @param new_mode The target mode to transition to
     * @return ErrorCode::SUCCESS on success, error code on failure
     *
     * @details Validates the transition, logs to history, updates timestamp,
     *          and notifies observers. Invalid transitions are rejected.
     */
    ErrorCode setMode(DeviceMode new_mode);

    /**
     * @brief Force a state transition without validation
     * @param new_mode The target mode to transition to
     * @return ErrorCode::SUCCESS (always succeeds)
     *
     * @warning Use only for error recovery or testing. Bypasses validation.
     */
    ErrorCode forceMode(DeviceMode new_mode);

    /**
     * @brief Reset state machine to IDLE with clean state
     * @details Clears all sub-states and returns device to safe IDLE.
     *          Used when cartridge is removed or device needs reset.
     */
    void resetToIdle();

    //==========================================================================
    // STATE QUERY METHODS (SM-007)
    //==========================================================================

    /**
     * @brief Get the current device mode
     * @return Current DeviceMode enum value
     */
    DeviceMode getCurrentMode() const;

    /**
     * @brief Get the previous device mode
     * @return Previous DeviceMode enum value (before last transition)
     */
    DeviceMode getPreviousMode() const;

    /**
     * @brief Get the current mode as a string
     * @return Constant string representation of current mode
     */
    const char* getCurrentModeString() const;

    /**
     * @brief Get current state as JSON-formatted string
     * @param buffer Output buffer for JSON string
     * @param bufferSize Size of output buffer
     * @return Number of characters written, or 0 on error
     */
    size_t getStateAsJson(char* buffer, size_t bufferSize) const;

    // Convenience query functions
    bool isIdle() const;                      ///< Check if in IDLE state
    bool isInitializing() const;              ///< Check if in INITIALIZING state
    bool isHeating() const;                   ///< Check if in HEATING state
    bool isBarcodeScanning() const;           ///< Check if in BARCODE_SCANNING state
    bool isValidatingCartridge() const;       ///< Check if in VALIDATING_CARTRIDGE state
    bool isRunningTest() const;               ///< Check if in RUNNING_TEST state
    bool isUploadingResults() const;          ///< Check if in UPLOADING_RESULTS state
    bool isStressTesting() const;             ///< Check if in STRESS_TESTING state
    bool isInErrorState() const;              ///< Check if in ERROR_STATE

    // Compound state queries
    bool canAcceptCartridge() const;          ///< Check if device can accept a new cartridge
    bool isHeatingRequired() const;           ///< Check if heating is needed before test
    bool isCloudOperationInProgress() const;  ///< Check if cloud operation is active
    bool isBusy() const;                      ///< Check if device is performing an operation

    //==========================================================================
    // TEST STATE MANAGEMENT (SM-005)
    //==========================================================================

    /**
     * @brief Get the current test state
     * @return Current TestState enum value
     */
    TestState getTestState() const;

    /**
     * @brief Get test state as string
     * @return Constant string representation of test state
     */
    const char* getTestStateString() const;

    /**
     * @brief Set the test state
     * @param state New test state
     * @return ErrorCode::SUCCESS on success, error code on invalid transition
     *
     * @note Valid test state transitions:
     *       NOT_STARTED -> RUNNING -> COMPLETED -> UPLOAD_PENDING -> UPLOADED
     *                               -> CANCELLED (from RUNNING on cartridge removal)
     */
    ErrorCode setTestState(TestState state);

    /**
     * @brief Check if a test is currently in progress
     * @return true if test is RUNNING
     */
    bool isTestInProgress() const;

    /**
     * @brief Check if device can start a new test
     * @return true if test state allows starting a new test
     */
    bool canStartNewTest() const;

    /**
     * @brief Check if test results are pending upload
     * @return true if test is completed but not yet uploaded
     */
    bool hasResultsPendingUpload() const;

    //==========================================================================
    // CARTRIDGE STATE MANAGEMENT (SM-006)
    //==========================================================================

    /**
     * @brief Get the current cartridge state
     * @return Current CartridgeState enum value
     */
    CartridgeState getCartridgeState() const;

    /**
     * @brief Get cartridge state as string
     * @return Constant string representation of cartridge state
     */
    const char* getCartridgeStateString() const;

    /**
     * @brief Set the cartridge state
     * @param state New cartridge state
     * @return ErrorCode::SUCCESS on success
     */
    ErrorCode setCartridgeState(CartridgeState state);

    /**
     * @brief Get the current cartridge ID
     * @return Pointer to cartridge ID string (may be empty)
     */
    const char* getCurrentCartridgeId() const;

    /**
     * @brief Set the current cartridge ID
     * @param cartridge_id Cartridge UUID string (max 36 characters)
     */
    void setCurrentCartridgeId(const char* cartridge_id);

    /**
     * @brief Clear the current cartridge ID
     */
    void clearCurrentCartridgeId();

    /**
     * @brief Event handler for cartridge insertion
     * @details Called when hardware detects cartridge insertion.
     *          Sets cartridge state to DETECTED.
     */
    void onCartridgeInserted();

    /**
     * @brief Event handler for cartridge removal
     * @details Called when hardware detects cartridge removal.
     *          Cancels any running test and resets to IDLE.
     */
    void onCartridgeRemoved();

    /**
     * @brief Check if cartridge is physically present
     * @return true if cartridge state is not NOT_INSERTED
     */
    bool hasCartridge() const;

    /**
     * @brief Check if cartridge has been validated
     * @return true if cartridge state is VALIDATED
     */
    bool isCartridgeValidated() const;

    /**
     * @brief Check if cartridge validation failed
     * @return true if cartridge state is INVALID
     */
    bool isCartridgeInvalid() const;

    //==========================================================================
    // CLOUD OPERATION TRACKING
    //==========================================================================

    /**
     * @brief Start tracking a cloud operation
     * @details Call when initiating a cloud request. Used for timeout detection.
     */
    void startCloudOperation();

    /**
     * @brief End tracking a cloud operation
     * @details Call when receiving cloud response or on error.
     */
    void endCloudOperation();

    /**
     * @brief Check if a cloud operation is pending
     * @return true if cloud operation is in progress
     */
    bool isCloudOperationPending() const;

    /**
     * @brief Check if cloud operation has timed out
     * @param timeout_ms Timeout in milliseconds (default 30 seconds)
     * @return true if operation has exceeded timeout
     */
    bool isCloudOperationTimeout(uint32_t timeout_ms = DEFAULT_CLOUD_TIMEOUT_MS) const;

    /**
     * @brief Get elapsed time for current cloud operation
     * @return Milliseconds since cloud operation started, or 0 if not active
     */
    uint32_t getCloudOperationElapsed() const;

    //==========================================================================
    // ERROR HANDLING
    //==========================================================================

    /**
     * @brief Transition to error state with message
     * @param error_code Error code to record
     * @param error_msg Description of the error
     */
    void setError(ErrorCode error_code, const char* error_msg);

    /**
     * @brief Clear error state and return to IDLE
     */
    void clearError();

    /**
     * @brief Get the last error code
     * @return Last recorded error code
     */
    ErrorCode getLastErrorCode() const;

    /**
     * @brief Get the last error message
     * @return Pointer to error message string
     */
    const char* getLastErrorMessage() const;

    //==========================================================================
    // TRANSITION HISTORY (SM-003)
    //==========================================================================

    /**
     * @brief Get the total number of transitions logged
     * @return Number of transitions in history (up to TRANSITION_HISTORY_SIZE)
     */
    int getTransitionCount() const;

    /**
     * @brief Get a specific transition from history
     * @param index Index into history (0 = most recent)
     * @return Pointer to transition entry, or nullptr if index out of bounds
     */
    const StateTransitionEntry* getTransition(int index) const;

    /**
     * @brief Clear all transition history
     */
    void clearHistory();

    /**
     * @brief Get history as formatted string
     * @param buffer Output buffer
     * @param bufferSize Size of buffer
     * @param maxEntries Maximum entries to include (0 = all)
     * @return Number of characters written
     */
    size_t getHistoryAsString(char* buffer, size_t bufferSize, int maxEntries = 0) const;

    //==========================================================================
    // OBSERVER PATTERN
    //==========================================================================

    /**
     * @brief Register a callback for state change notifications
     * @param callback Function to call on state changes
     * @return true if callback registered successfully
     */
    bool registerStateChangeCallback(StateChangeCallback callback);

private:
    //==========================================================================
    // PRIVATE MEMBERS
    //==========================================================================

    // Initialization flag
    volatile bool _initialized;

    // Primary state (SM-001)
    volatile DeviceMode _currentMode;
    volatile DeviceMode _previousMode;
    volatile uint32_t _lastTransitionTime;

    // Sub-states (SM-005, SM-006)
    volatile TestState _testState;
    volatile CartridgeState _cartridgeState;
    char _currentCartridgeId[CARTRIDGE_ID_MAX_LENGTH];

    // Cloud operation tracking
    volatile bool _cloudOperationPending;
    volatile uint32_t _cloudOperationStartTime;

    // Error tracking
    volatile ErrorCode _lastErrorCode;
    char _lastErrorMessage[64];

    // Transition history (SM-003)
    StateTransitionEntry _transitionHistory[TRANSITION_HISTORY_SIZE];
    volatile int _historyIndex;
    volatile int _historyCount;

    // Observer callbacks (maximum 4 observers)
    static const int MAX_OBSERVERS = 4;
    StateChangeCallback _observers[MAX_OBSERVERS];
    int _observerCount;

    //==========================================================================
    // PRIVATE METHODS
    //==========================================================================

    /**
     * @brief Log a transition to the history buffer
     * @param from Source mode
     * @param to Destination mode
     */
    void logTransition(DeviceMode from, DeviceMode to);

    /**
     * @brief Notify all registered observers of state change
     * @param from Source mode
     * @param to Destination mode
     */
    void notifyObservers(DeviceMode from, DeviceMode to);

    /**
     * @brief Get current Unix timestamp
     * @return Current time as Unix timestamp, or millis()/1000 if time not synced
     */
    uint32_t getCurrentTimestamp() const;
};

//==============================================================================
// GLOBAL STATE MACHINE INSTANCE
//==============================================================================

/**
 * @brief Global state machine instance
 * @details Single source of truth for device state. Access via this global
 *          or through getter function.
 */
extern StateMachine deviceState;

/**
 * @brief Get reference to the global state machine
 * @return Reference to global StateMachine instance
 */
StateMachine& getStateMachine();

#endif // STATEMACHINE_H
