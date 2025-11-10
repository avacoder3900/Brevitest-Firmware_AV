/*!
 * @file DeviceState.cpp
 * @brief Device state machine implementation for Brevitest firmware
 * @details Implementation of state transition logic and validation
 * 
 * This file contains the implementation of the state machine methods
 * that replace the scattered boolean flags with a structured,
 * validated state management system.
 */

#include "DeviceState.h"

/**
 * @brief Validates if a state transition is allowed
 * 
 * This method implements the state transition rules to prevent
 * invalid state combinations that could cause device malfunction.
 * 
 * @param new_mode The target mode to transition to
 * @return true if transition is valid, false otherwise
 */
bool DeviceStateMachine::can_transition_to(DeviceMode new_mode) {
    // === ALWAYS ALLOWED TRANSITIONS ===
    
    // Can always transition to ERROR state (safety mechanism)
    if (new_mode == DeviceMode::ERROR_STATE) {
        return true;
    }
    
    // Can always transition to IDLE (reset/cleanup mechanism)
    if (new_mode == DeviceMode::IDLE) {
        return true;
    }
    
    // === PREVENT INVALID TRANSITIONS ===
    
    // Can't transition to same state (no-op)
    if (new_mode == mode) {
        return false;
    }
    
    // === STATE-SPECIFIC TRANSITION RULES ===
    // Each state has specific allowed transitions based on device logic
    
    switch (mode) {
        case DeviceMode::INITIALIZING:
            // Can only go to IDLE (normal startup) or HEATING (if temp not ready)
            return new_mode == DeviceMode::IDLE || new_mode == DeviceMode::HEATING;
            
        case DeviceMode::IDLE:
            // Can start barcode scanning, stress testing, or heating
            return new_mode == DeviceMode::BARCODE_SCANNING || 
                   new_mode == DeviceMode::STRESS_TESTING ||
                   new_mode == DeviceMode::HEATING;
                   
        case DeviceMode::HEATING:
            // Can go to IDLE (temp ready) or start operations
            return new_mode == DeviceMode::IDLE || 
                   new_mode == DeviceMode::BARCODE_SCANNING ||
                   new_mode == DeviceMode::STRESS_TESTING;
                   
        case DeviceMode::BARCODE_SCANNING:
            // Can validate cartridge/magnetometer, start stress test, or return to IDLE
            return new_mode == DeviceMode::VALIDATING_CARTRIDGE ||
                   new_mode == DeviceMode::VALIDATING_MAGNETOMETER ||
                   new_mode == DeviceMode::STRESS_TESTING ||
                   new_mode == DeviceMode::IDLE;
                   
        case DeviceMode::VALIDATING_CARTRIDGE:
            // Can start test, reset cartridge, or return to IDLE
            return new_mode == DeviceMode::RUNNING_TEST ||
                   new_mode == DeviceMode::RESETTING_CARTRIDGE ||
                   new_mode == DeviceMode::IDLE;
                   
        case DeviceMode::VALIDATING_MAGNETOMETER:
            // Can only return to IDLE (magnetometer validation is terminal)
            return new_mode == DeviceMode::IDLE;
            
        case DeviceMode::RUNNING_TEST:
            // Can upload results or return to IDLE (if cancelled)
            return new_mode == DeviceMode::UPLOADING_RESULTS ||
                   new_mode == DeviceMode::IDLE;
                   
        case DeviceMode::UPLOADING_RESULTS:
            // Can only return to IDLE (upload is terminal)
            return new_mode == DeviceMode::IDLE;
            
        case DeviceMode::RESETTING_CARTRIDGE:
            // Can only return to IDLE (reset is terminal)
            return new_mode == DeviceMode::IDLE;
            
        case DeviceMode::STRESS_TESTING:
            // Can only return to IDLE (stress test is terminal)
            return new_mode == DeviceMode::IDLE;
            
        case DeviceMode::ERROR_STATE:
            // Can only return to IDLE (error recovery)
            return new_mode == DeviceMode::IDLE;
            
        default:
            // Unknown state - deny all transitions
            return false;
    }
}

/**
 * @brief Performs a validated state transition
 * 
 * This method handles the actual state transition, including logging
 * and validation. It will warn about invalid transitions but won't crash.
 * 
 * @param new_mode The target mode to transition to
 */
void DeviceStateMachine::transition_to(DeviceMode new_mode) {
    // === VALIDATION CHECK ===
    if (!can_transition_to(new_mode)) {
        Log.warn("Invalid state transition from %s to %s", 
                device_mode_to_string(mode).c_str(),
                device_mode_to_string(new_mode).c_str());
        return; // Don't perform invalid transition
    }
    
    // === PERFORM TRANSITION ===
    previous_mode = mode;  // Store previous state for logging
    mode = new_mode;       // Update current state
    
    // === LOG TRANSITION ===
    Log.info("State transition: %s -> %s", 
            device_mode_to_string(previous_mode).c_str(),
            device_mode_to_string(mode).c_str());
}

/**
 * @brief Resets state machine to IDLE with clean state
 * 
 * This method is called when the device needs to return to a safe,
 * clean state. It's used when cartridges are removed or when
 * recovering from errors.
 */
void DeviceStateMachine::reset_to_idle() {
    // === RESET PRIMARY STATE ===
    mode = DeviceMode::IDLE;
    previous_mode = DeviceMode::IDLE;
    
    // === RESET SUB-STATES ===
    test_state = TestState::NOT_STARTED;
    cartridge_state = CartridgeState::NOT_INSERTED;
    
    // === RESET ASYNC OPERATIONS ===
    cloud_operation_pending = false;
    cloud_operation_start_time = 0;
    
    // === CLEAR ERROR STATE ===
    last_error = "";
    
    Log.info("Device state reset to IDLE");
}

/**
 * @brief Start tracking a cloud operation
 * 
 * Call this when initiating a cloud request (validate, reset, upload).
 * Used for timeout detection and preventing duplicate requests.
 */
void DeviceStateMachine::start_cloud_operation() {
    cloud_operation_pending = true;
    cloud_operation_start_time = millis();
    Log.info("Cloud operation started");
}

/**
 * @brief End tracking a cloud operation
 * 
 * Call this when receiving a cloud response or on error.
 * Clears the pending flag and resets timeout tracking.
 */
void DeviceStateMachine::end_cloud_operation() {
    cloud_operation_pending = false;
    cloud_operation_start_time = 0;
    Log.info("Cloud operation completed");
}

/**
 * @brief Check if cloud operation has timed out
 * 
 * @param timeout_ms Timeout in milliseconds (default 30 seconds)
 * @return true if operation has timed out
 * 
 * Used to detect when cloud operations take too long and may need
 * to be retried or cancelled.
 */
bool DeviceStateMachine::is_cloud_operation_timeout(unsigned long timeout_ms) const {
    if (!cloud_operation_pending) {
        return false; // No operation pending
    }
    return (millis() - cloud_operation_start_time) > timeout_ms;
}

/**
 * @brief Set error state with message
 * 
 * @param error_msg Description of the error
 * 
 * Transitions device to ERROR_STATE and stores error message.
 * Use this for any error condition that requires attention.
 */
void DeviceStateMachine::set_error(const String& error_msg) {
    last_error = error_msg;
    mode = DeviceMode::ERROR_STATE;
    Log.error("Device error: %s", error_msg.c_str());
}

/**
 * @brief Clear error state
 * 
 * Transitions from ERROR_STATE back to IDLE and clears error message.
 * Use this when error condition is resolved.
 */
void DeviceStateMachine::clear_error() {
    last_error = "";
    if (mode == DeviceMode::ERROR_STATE) {
        mode = DeviceMode::IDLE;
    }
}

/**
 * @brief Convert DeviceMode enum to human-readable string
 * 
 * @param mode The device mode to convert
 * @return String representation of the mode
 * 
 * Used for logging and debugging. Returns "UNKNOWN" for invalid modes.
 */
String device_mode_to_string(DeviceMode mode) {
    switch (mode) {
        case DeviceMode::IDLE: return "IDLE";
        case DeviceMode::INITIALIZING: return "INITIALIZING";
        case DeviceMode::HEATING: return "HEATING";
        case DeviceMode::BARCODE_SCANNING: return "BARCODE_SCANNING";
        case DeviceMode::VALIDATING_CARTRIDGE: return "VALIDATING_CARTRIDGE";
        case DeviceMode::VALIDATING_MAGNETOMETER: return "VALIDATING_MAGNETOMETER";
        case DeviceMode::RUNNING_TEST: return "RUNNING_TEST";
        case DeviceMode::UPLOADING_RESULTS: return "UPLOADING_RESULTS";
        case DeviceMode::RESETTING_CARTRIDGE: return "RESETTING_CARTRIDGE";
        case DeviceMode::STRESS_TESTING: return "STRESS_TESTING";
        case DeviceMode::ERROR_STATE: return "ERROR_STATE";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert TestState enum to human-readable string
 * 
 * @param state The test state to convert
 * @return String representation of the state
 * 
 * Used for logging and debugging. Returns "UNKNOWN" for invalid states.
 */
String test_state_to_string(TestState state) {
    switch (state) {
        case TestState::NOT_STARTED: return "NOT_STARTED";
        case TestState::RUNNING: return "RUNNING";
        case TestState::COMPLETED: return "COMPLETED";
        case TestState::CANCELLED: return "CANCELLED";
        case TestState::UPLOAD_PENDING: return "UPLOAD_PENDING";
        case TestState::UPLOAD_IN_PROGRESS: return "UPLOAD_IN_PROGRESS";
        case TestState::UPLOADED: return "UPLOADED";
        default: return "UNKNOWN";
    }
}

/**
 * @brief Convert CartridgeState enum to human-readable string
 * 
 * @param state The cartridge state to convert
 * @return String representation of the state
 * 
 * Used for logging and debugging. Returns "UNKNOWN" for invalid states.
 */
String cartridge_state_to_string(CartridgeState state) {
    switch (state) {
        case CartridgeState::NOT_INSERTED: return "NOT_INSERTED";
        case CartridgeState::DETECTED: return "DETECTED";
        case CartridgeState::BARCODE_READ: return "BARCODE_READ";
        case CartridgeState::VALIDATED: return "VALIDATED";
        case CartridgeState::INVALID: return "INVALID";
        case CartridgeState::TEST_COMPLETE: return "TEST_COMPLETE";
        default: return "UNKNOWN";
    }
}
