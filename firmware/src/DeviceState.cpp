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
            // Can start barcode scanning, stress testing, heating, or upload cached tests
            return new_mode == DeviceMode::BARCODE_SCANNING || 
                   new_mode == DeviceMode::STRESS_TESTING ||
                   new_mode == DeviceMode::HEATING ||
                   new_mode == DeviceMode::UPLOADING_RESULTS;
                   
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
    
    // === STORE TRANSITION IN HISTORY ===
    transition_history[history_index].from_mode = mode;
    transition_history[history_index].to_mode = new_mode;
    transition_history[history_index].timestamp = Time.now();  // Unix timestamp (seconds since epoch)
    
    // Copy current barcode to this transition
    strncpy(transition_history[history_index].barcode_id, current_barcode, 36);
    transition_history[history_index].barcode_id[36] = '\0';  // Ensure null termination
    
    // Update circular buffer index
    history_index = (history_index + 1) % TRANSITION_HISTORY_SIZE;
    if (history_count < TRANSITION_HISTORY_SIZE) {
        history_count++;
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

// === BARCODE TRACKING METHODS ===

/**
 * @brief Set the current barcode being processed
 * 
 * @param barcode The barcode UUID string (up to 36 characters)
 * 
 * Call this when a barcode is scanned. All subsequent state transitions
 * will be associated with this barcode until it's cleared.
 */
void DeviceStateMachine::set_current_barcode(const char* barcode) {
    if (barcode && barcode[0] != '\0') {
        strncpy(current_barcode, barcode, 36);
        current_barcode[36] = '\0';  // Ensure null termination
        Log.info("Barcode set: %s", current_barcode);
    }
}

/**
 * @brief Clear the current barcode
 * 
 * Call this when a cartridge is removed or test is complete.
 * Subsequent transitions will have no barcode association.
 */
void DeviceStateMachine::clear_current_barcode() {
    current_barcode[0] = '\0';
    Log.info("Barcode cleared");
}

/**
 * @brief Get transitions associated with a specific barcode
 * 
 * @param barcode The barcode UUID to search for
 * @param results Array to store matching transition indices (most recent first)
 * @param max_results Maximum number of results to return
 * @return Number of matching transitions found
 * 
 * Searches through the transition history and returns indices of all
 * transitions that match the given barcode.
 */
int DeviceStateMachine::get_transitions_for_barcode(const char* barcode, int* results, int max_results) const {
    if (!barcode || barcode[0] == '\0' || max_results <= 0) {
        return 0;
    }
    
    int found = 0;
    
    // Search from most recent to oldest
    for (int i = 0; i < history_count && found < max_results; i++) {
        const StateTransitionEntry* entry = get_transition(i);
        if (entry && strcmp(entry->barcode_id, barcode) == 0) {
            results[found++] = i;
        }
    }
    
    return found;
}

/**
 * @brief Print state transition history for a specific barcode
 * 
 * @param barcode The barcode UUID to filter by
 * 
 * Prints a formatted table showing only transitions associated with
 * the specified barcode. Includes transitions before the barcode scan
 * if they occurred in the same session.
 */
void DeviceStateMachine::print_barcode_history(const char* barcode) const {
    if (!barcode || barcode[0] == '\0') {
        Serial.println("ERROR: No barcode specified");
        return;
    }
    
    // Find all transitions for this barcode
    int results[TRANSITION_HISTORY_SIZE];
    int count = get_transitions_for_barcode(barcode, results, TRANSITION_HISTORY_SIZE);
    
    Serial.println("\n╔══════════════════════════════════════════════════════════════════════════════════════════════════════╗");
    Serial.printlnf("║                     STATE TRANSITIONS FOR BARCODE: %-46s║", barcode);
    Serial.println("╠══════════════════════════════════════════════════════════════════════════════════════════════════════╣");
    Serial.printlnf("║ Total Matching Transitions: %-77d║", count);
    Serial.println("╠══════════════════════════════════════════════════════════════════════════════════════════════════════╣");
    
    if (count == 0) {
        Serial.println("║ No transitions found for this barcode                                                                ║");
    } else {
        Serial.println("║  #  │ Date & Time             │ From State            │ To State                                   ║");
        Serial.println("╟─────┼─────────────────────────┼───────────────────────┼────────────────────────────────────────────╢");
        
        // Print in chronological order (reverse of search order)
        for (int i = count - 1; i >= 0; i--) {
            const StateTransitionEntry* entry = get_transition(results[i]);
            if (entry) {
                String time_str = Time.format(entry->timestamp, "%Y-%m-%d %H:%M:%S");
                Serial.printlnf("║ %3d │ %s │ %-21s │ %-42s ║", 
                    count - i,
                    time_str.c_str(),
                    device_mode_to_string(entry->from_mode).c_str(),
                    device_mode_to_string(entry->to_mode).c_str());
            }
        }
    }
    
    Serial.println("╚══════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
}

/**
 * @brief Get formatted history string for a specific barcode
 * 
 * @param barcode The barcode UUID to filter by
 * @return Formatted string with barcode-specific history
 * 
 * Returns a compact string suitable for Particle Cloud events.
 * Format: Barcode:UUID|Total:N|DateTime:FROM>TO;DateTime:FROM>TO;...
 */
String DeviceStateMachine::get_barcode_history_string(const char* barcode) const {
    if (!barcode || barcode[0] == '\0') {
        return "Error:No barcode specified";
    }
    
    // Find all transitions for this barcode
    int results[TRANSITION_HISTORY_SIZE];
    int count = get_transitions_for_barcode(barcode, results, TRANSITION_HISTORY_SIZE);
    
    String result = String::format("Barcode:%s|Total:%d|", barcode, count);
    
    if (count > 0) {
        // Add transitions in chronological order
        for (int i = count - 1; i >= 0; i--) {
            const StateTransitionEntry* entry = get_transition(results[i]);
            if (entry) {
                if (i < count - 1) result += ";";
                String time_str = Time.format(entry->timestamp, "%Y-%m-%d %H:%M:%S");
                result += String::format("%s:%s>%s", 
                    time_str.c_str(),
                    device_mode_to_string(entry->from_mode).c_str(),
                    device_mode_to_string(entry->to_mode).c_str());
            }
        }
    }
    
    return result;
}

// === STATE TRANSITION HISTORY METHODS ===

/**
 * @brief Get the number of state transitions logged
 * 
 * @return Number of transitions in history (up to TRANSITION_HISTORY_SIZE)
 * 
 * Returns the total number of transitions stored. This will be less than
 * or equal to TRANSITION_HISTORY_SIZE.
 */
int DeviceStateMachine::get_transition_count() const {
    return history_count;
}

/**
 * @brief Get a specific transition from history
 * 
 * @param index Index into history (0 = most recent, 1 = second most recent, etc.)
 * @return Pointer to transition entry, or NULL if index out of bounds
 * 
 * This method allows you to retrieve specific transitions from the history
 * buffer. Index 0 is the most recent transition, 1 is the second most recent,
 * and so on.
 */
const StateTransitionEntry* DeviceStateMachine::get_transition(int index) const {
    if (index < 0 || index >= history_count) {
        return NULL;  // Index out of bounds
    }
    
    // Calculate the actual position in the circular buffer
    // Most recent is at (history_index - 1), going backwards from there
    int actual_index = (history_index - 1 - index + TRANSITION_HISTORY_SIZE) % TRANSITION_HISTORY_SIZE;
    return &transition_history[actual_index];
}

/**
 * @brief Print state transition history to Serial
 * 
 * @param count Number of most recent transitions to print (0 = all)
 * 
 * Prints a formatted table of state transitions to the Serial port.
 * Useful for debugging and diagnostics.
 */
void DeviceStateMachine::print_transition_history(int count) const {
    int num_to_print = (count == 0 || count > history_count) ? history_count : count;
    
    Serial.println("\n╔══════════════════════════════════════════════════════════════════════════════════════════════════════╗");
    Serial.println("║                           DEVICE STATE TRANSITION HISTORY                                            ║");
    Serial.println("╠══════════════════════════════════════════════════════════════════════════════════════════════════════╣");
    Serial.printlnf("║ Total Transitions: %-86d║", history_count);
    Serial.printlnf("║ Showing: %-94d║", num_to_print);
    Serial.println("╠══════════════════════════════════════════════════════════════════════════════════════════════════════╣");
    
    if (num_to_print == 0) {
        Serial.println("║ No transitions recorded yet                                                                          ║");
    } else {
        Serial.println("║  #  │ Date & Time             │ From State            │ To State          │ Barcode            ║");
        Serial.println("╟─────┼─────────────────────────┼───────────────────────┼───────────────────┼────────────────────╢");
        
        for (int i = 0; i < num_to_print; i++) {
            const StateTransitionEntry* entry = get_transition(i);
            if (entry) {
                // Format time as "YYYY-MM-DD HH:MM:SS"
                String time_str = Time.format(entry->timestamp, "%Y-%m-%d %H:%M:%S");
                
                // Truncate barcode for display if too long
                String barcode_display = String(entry->barcode_id);
                if (barcode_display.length() == 0) {
                    barcode_display = "(none)";
                } else if (barcode_display.length() > 18) {
                    barcode_display = barcode_display.substring(0, 15) + "...";
                }
                
                Serial.printlnf("║ %3d │ %s │ %-21s │ %-17s │ %-18s ║", 
                    i + 1,
                    time_str.c_str(),
                    device_mode_to_string(entry->from_mode).c_str(),
                    device_mode_to_string(entry->to_mode).c_str(),
                    barcode_display.c_str());
            }
        }
    }
    
    Serial.println("╚══════════════════════════════════════════════════════════════════════════════════════════════════════╝\n");
}

/**
 * @brief Clear state transition history
 * 
 * Resets the transition history buffer. Useful when you want to start
 * fresh tracking from a specific point in time.
 */
void DeviceStateMachine::clear_transition_history() {
    history_index = 0;
    history_count = 0;
    Log.info("State transition history cleared");
}

/**
 * @brief Get formatted state transition history as String
 * 
 * @param count Number of most recent transitions to include (0 = all)
 * @return Formatted string with transition history
 * 
 * Returns the transition history as a formatted string suitable for
 * Particle Cloud variable or function response. Format is compact
 * to fit within Particle Cloud constraints.
 */
String DeviceStateMachine::get_transition_history_string(int count) const {
    int num_to_include = (count == 0 || count > history_count) ? history_count : count;
    String result = String::format("Total:%d|", history_count);
    
    for (int i = 0; i < num_to_include; i++) {
        const StateTransitionEntry* entry = get_transition(i);
        if (entry) {
            if (i > 0) result += ";";
            // Format: DateTime:FROM>TO[Barcode]
            String time_str = Time.format(entry->timestamp, "%Y-%m-%d %H:%M:%S");
            String barcode = String(entry->barcode_id);
            if (barcode.length() == 0) {
                barcode = "none";
            }
            result += String::format("%s:%s>%s[%s]", 
                time_str.c_str(),
                device_mode_to_string(entry->from_mode).c_str(),
                device_mode_to_string(entry->to_mode).c_str(),
                barcode.c_str());
        }
    }
    
    return result;
}
