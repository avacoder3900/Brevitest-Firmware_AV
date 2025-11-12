/*!
 * @file DeviceState.h
 * @brief Device state machine definitions for Brevitest firmware
 * @details Centralized state management to replace scattered boolean flags
 * 
 * This file defines the state machine that replaces 26+ scattered boolean flags
 * with a structured, validated state management system. The state machine
 * prevents invalid state transitions and provides clear visibility into
 * device operations.
 */

#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#include "Particle.h"

/**
 * @brief Primary device operational states
 *
 * These represent the main operational modes of the device. Only one
 * DeviceMode can be active at a time, and transitions between modes
 * are validated to prevent invalid state combinations.
 */
enum class DeviceMode {
    IDLE,                    // Ready for cartridge insertion - normal standby state
    INITIALIZING,            // Device startup - occurs during setup()
    HEATING,                 // Waiting for heater to reach target temperature
    BARCODE_SCANNING,        // Reading barcode from inserted cartridge
    VALIDATING_CARTRIDGE,    // Cloud validation in progress - waiting for server response
    VALIDATING_MAGNETOMETER, // BLE magnetometer validation - checking magnetometer data
    RUNNING_TEST,            // Test execution (BCODE running) - actual test in progress
    UPLOADING_RESULTS,       // Cloud upload in progress - sending test results to server
    RESETTING_CARTRIDGE,     // Cloud reset in progress - resetting cartridge on server
    STRESS_TESTING,          // Stress test mode - automated testing cycles
    ERROR_STATE              // Error condition - something went wrong, needs attention
};

/**
 * @brief Test execution sub-states
 * 
 * These track the progress of individual test runs. A test can be
 * in various stages from not started to fully uploaded to the cloud.
 */
enum class TestState {
    NOT_STARTED,        // No test has been initiated
    RUNNING,            // Test is currently executing (BCODE running)
    COMPLETED,          // Test finished successfully
    CANCELLED,          // Test was cancelled (e.g., cartridge removed)
    UPLOAD_PENDING,     // Test completed, waiting to upload to cloud
    UPLOAD_IN_PROGRESS, // Currently uploading test results to cloud
    UPLOADED            // Test results successfully uploaded to cloud
};

/**
 * @brief Cartridge-related states
 * 
 * These track the status of cartridge insertion and validation.
 * Cartridges go through a sequence: detected -> barcode read -> validated.
 */
enum class CartridgeState {
    NOT_INSERTED,   // No cartridge detected
    DETECTED,       // Cartridge physically detected (hardware interrupt)
    BARCODE_READ,   // Barcode successfully scanned
    VALIDATED,      // Cartridge validated by cloud server
    INVALID,        // Cartridge validation failed
    TEST_COMPLETE   // Test completed for this cartridge
};

/**
 * @brief State transition history entry
 * 
 * Stores information about a single state transition for diagnostic purposes.
 * Includes barcode association to track transitions for specific test cartridges.
 */
struct StateTransitionEntry {
    DeviceMode from_mode;           // State transitioned from
    DeviceMode to_mode;             // State transitioned to
    time_t timestamp;               // Unix timestamp when transition occurred (seconds since epoch)
    char barcode_id[37];            // Associated barcode UUID (36 chars + null terminator)
    
    StateTransitionEntry() : from_mode(DeviceMode::INITIALIZING), 
                            to_mode(DeviceMode::INITIALIZING), 
                            timestamp(0) {
        barcode_id[0] = '\0';  // Empty string by default
    }
};

/**
 * @brief Centralized device state machine
 * 
 * This struct replaces 26+ scattered boolean flags with a single,
 * organized state management system. It provides:
 * - State transition validation
 * - Clear state visibility
 * - Race condition prevention
 * - Comprehensive error tracking
 * - State transition history logging
 */
struct DeviceStateMachine {
    // === PRIMARY STATE ===
    DeviceMode mode;                    // Current operational mode
    DeviceMode previous_mode;           // Previous mode (for transition logging)
    
    // === STATE TRANSITION HISTORY ===
    static const int TRANSITION_HISTORY_SIZE = 50;  // Number of transitions to keep
    StateTransitionEntry transition_history[50];     // Circular buffer of transitions
    int history_index;                              // Current position in circular buffer
    int history_count;                              // Total number of transitions logged
    
    // === SUB-STATE MACHINES ===
    TestState test_state;               // Current test execution state
    CartridgeState cartridge_state;    // Current cartridge status
    
    // === HARDWARE STATE ===
    bool detector_on;                   // Physical cartridge detector state
    bool heater_ready;                  // Heater temperature ready for operation
    
    // === ASYNC OPERATION TRACKING ===
    bool cloud_operation_pending;       // True when waiting for cloud response
    unsigned long cloud_operation_start_time; // Timestamp for timeout detection
    
    // === ERROR TRACKING ===
    String last_error;                  // Most recent error message
    
    // === BARCODE TRACKING ===
    char current_barcode[37];           // Currently scanned barcode UUID (36 chars + null)
    
    /**
     * @brief Constructor - initializes state machine to safe defaults
     * 
     * Sets all states to safe initial values. The device starts in
     * INITIALIZING mode and transitions to IDLE after setup completes.
     */
    DeviceStateMachine() {
        mode = DeviceMode::INITIALIZING;
        previous_mode = DeviceMode::INITIALIZING;
        test_state = TestState::NOT_STARTED;
        cartridge_state = CartridgeState::NOT_INSERTED;
        detector_on = false;
        heater_ready = false;
        cloud_operation_pending = false;
        cloud_operation_start_time = 0;
        last_error = "";
        history_index = 0;
        history_count = 0;
        current_barcode[0] = '\0';  // Initialize to empty string
    }
    
    // === STATE TRANSITION METHODS ===
    
    /**
     * @brief Validates if a state transition is allowed
     * @param new_mode The target mode to transition to
     * @return true if transition is valid, false otherwise
     * 
     * Prevents invalid state combinations that could cause device
     * malfunction or undefined behavior.
     */
    bool can_transition_to(DeviceMode new_mode);
    
    /**
     * @brief Performs a validated state transition
     * @param new_mode The target mode to transition to
     * 
     * Logs the transition and updates the state machine.
     * Will warn if transition is invalid but won't crash.
     */
    void transition_to(DeviceMode new_mode);
    
    /**
     * @brief Resets state machine to IDLE with clean state
     * 
     * Used when cartridge is removed or device needs to reset.
     * Clears all sub-states and returns to safe IDLE condition.
     */
    void reset_to_idle();
    
    // === STATE QUERY METHODS ===
    
    /** @brief Check if device is in IDLE state (ready for cartridge) */
    bool is_idle() const { return mode == DeviceMode::IDLE; }
    
    /** @brief Check if device is running a test */
    bool is_testing() const { return mode == DeviceMode::RUNNING_TEST; }
    
    /** @brief Check if device is in stress test mode */
    bool is_stress_testing() const { return mode == DeviceMode::STRESS_TESTING; }
    
    /** @brief Check if device is heating up */
    bool is_heating() const { return mode == DeviceMode::HEATING; }
    
    /** @brief Check if device is in error state */
    bool is_error() const { return mode == DeviceMode::ERROR_STATE; }
    
    /** @brief Check if cartridge is physically present */
    bool has_cartridge() const { return cartridge_state != CartridgeState::NOT_INSERTED; }
    
    /** @brief Check if cartridge is validated by cloud */
    bool is_cartridge_validated() const { return cartridge_state == CartridgeState::VALIDATED; }
    
    /** @brief Check if cartridge validation failed */
    bool is_cartridge_invalid() const { return cartridge_state == CartridgeState::INVALID; }
    
    // === CLOUD OPERATION TRACKING ===
    
    /**
     * @brief Start tracking a cloud operation
     * 
     * Call this when initiating a cloud request (validate, reset, upload).
     * Used for timeout detection and preventing duplicate requests.
     */
    void start_cloud_operation();
    
    /**
     * @brief End tracking a cloud operation
     * 
     * Call this when receiving a cloud response or on error.
     * Clears the pending flag and resets timeout tracking.
     */
    void end_cloud_operation();
    
    /**
     * @brief Check if cloud operation has timed out
     * @param timeout_ms Timeout in milliseconds (default 30 seconds)
     * @return true if operation has timed out
     */
    bool is_cloud_operation_timeout(unsigned long timeout_ms = 30000) const;
    
    // === STATE TRANSITION HISTORY ===
    
    /**
     * @brief Get the number of state transitions logged
     * @return Number of transitions in history (up to TRANSITION_HISTORY_SIZE)
     */
    int get_transition_count() const;
    
    /**
     * @brief Get a specific transition from history
     * @param index Index into history (0 = most recent, 1 = second most recent, etc.)
     * @return Pointer to transition entry, or NULL if index out of bounds
     */
    const StateTransitionEntry* get_transition(int index) const;
    
    /**
     * @brief Print state transition history to Serial
     * @param count Number of most recent transitions to print (0 = all)
     */
    void print_transition_history(int count = 0) const;
    
    /**
     * @brief Clear state transition history
     */
    void clear_transition_history();
    
    /**
     * @brief Get formatted state transition history as String
     * @param count Number of most recent transitions to include (0 = all)
     * @return Formatted string with transition history
     */
    String get_transition_history_string(int count = 0) const;
    
    // === BARCODE TRACKING ===
    
    /**
     * @brief Set the current barcode being processed
     * @param barcode The barcode UUID string (up to 36 characters)
     */
    void set_current_barcode(const char* barcode);
    
    /**
     * @brief Clear the current barcode (e.g., when cartridge removed)
     */
    void clear_current_barcode();
    
    /**
     * @brief Get transitions associated with a specific barcode
     * @param barcode The barcode UUID to search for
     * @param results Array to store matching transition indices
     * @param max_results Maximum number of results to return
     * @return Number of matching transitions found
     */
    int get_transitions_for_barcode(const char* barcode, int* results, int max_results) const;
    
    /**
     * @brief Print state transition history for a specific barcode
     * @param barcode The barcode UUID to filter by
     */
    void print_barcode_history(const char* barcode) const;
    
    /**
     * @brief Get formatted history string for a specific barcode
     * @param barcode The barcode UUID to filter by
     * @return Formatted string with barcode-specific history
     */
    String get_barcode_history_string(const char* barcode) const;
    
    // === ERROR HANDLING ===
    
    /**
     * @brief Set error state with message
     * @param error_msg Description of the error
     * 
     * Transitions device to ERROR_STATE and stores error message.
     * Use this for any error condition that requires attention.
     */
    void set_error(const String& error_msg);
    
    /**
     * @brief Clear error state
     * 
     * Transitions from ERROR_STATE back to IDLE and clears error message.
     * Use this when error condition is resolved.
     */
    void clear_error();
};

// === HELPER FUNCTIONS ===

/**
 * @brief Convert DeviceMode enum to human-readable string
 * @param mode The device mode to convert
 * @return String representation of the mode
 * 
 * Used for logging and debugging. Returns "UNKNOWN" for invalid modes.
 */
String device_mode_to_string(DeviceMode mode);

/**
 * @brief Convert TestState enum to human-readable string
 * @param state The test state to convert
 * @return String representation of the state
 * 
 * Used for logging and debugging. Returns "UNKNOWN" for invalid states.
 */
String test_state_to_string(TestState state);

/**
 * @brief Convert CartridgeState enum to human-readable string
 * @param state The cartridge state to convert
 * @return String representation of the state
 * 
 * Used for logging and debugging. Returns "UNKNOWN" for invalid states.
 */
String cartridge_state_to_string(CartridgeState state);

#endif // DEVICE_STATE_H
