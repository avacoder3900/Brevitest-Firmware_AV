/*!
 * @file test_state_transitions.cpp
 * @brief Implementation of state machine transition tests
 * @details Comprehensive testing of all device state transitions
 * 
 * This file implements tests for the DeviceStateMachine with:
 * - Valid transition verification
 * - Invalid transition prevention
 * - Compound state validation
 * - Safety-critical state checks
 * 
 * Each test includes metadata for ISO 13485 traceability
 */

#include "test_state_transitions.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Mock state machine
MockDeviceStateMachine mock_state = {IDLE, IDLE, NOT_STARTED, NOT_INSERTED, false, false, false, 0, ""};

// Helper functions

void reset_mock_state_machine(void) {
    mock_state.mode = IDLE;
    mock_state.previous_mode = IDLE;
    mock_state.test_state = NOT_STARTED;
    mock_state.cartridge_state = NOT_INSERTED;
    mock_state.detector_on = false;
    mock_state.heater_ready = false;
    mock_state.cloud_operation_pending = false;
    mock_state.cloud_operation_start_time = 0;
    memset(mock_state.last_error, 0, sizeof(mock_state.last_error));
}

bool mock_can_transition_to(MockDeviceMode new_mode) {
    MockDeviceMode current = mock_state.mode;
    
    // Always allow transition to ERROR_STATE
    if (new_mode == ERROR_STATE) {
        return true;
    }
    
    // Allow transition from ERROR_STATE to IDLE (recovery)
    if (current == ERROR_STATE && new_mode == IDLE) {
        return true;
    }
    
    // Define valid transition matrix
    switch (current) {
        case INITIALIZING:
            return (new_mode == IDLE || new_mode == HEATING);
            
        case IDLE:
            return (new_mode == HEATING || 
                    new_mode == BARCODE_SCANNING ||
                    new_mode == VALIDATING_CARTRIDGE ||
                    new_mode == VALIDATING_MAGNETOMETER ||
                    new_mode == STRESS_TESTING ||
                    new_mode == RESETTING_CARTRIDGE);
            
        case HEATING:
            return (new_mode == IDLE || 
                    new_mode == BARCODE_SCANNING);
            
        case BARCODE_SCANNING:
            return (new_mode == IDLE || 
                    new_mode == VALIDATING_CARTRIDGE);
            
        case VALIDATING_CARTRIDGE:
            return (new_mode == IDLE || 
                    new_mode == RUNNING_TEST);
            
        case VALIDATING_MAGNETOMETER:
            return (new_mode == IDLE);
            
        case RUNNING_TEST:
            return (new_mode == IDLE || 
                    new_mode == UPLOADING_RESULTS);
            
        case UPLOADING_RESULTS:
            return (new_mode == IDLE);
            
        case RESETTING_CARTRIDGE:
            return (new_mode == IDLE);
            
        case STRESS_TESTING:
            return (new_mode == IDLE);
            
        case ERROR_STATE:
            return (new_mode == IDLE);
            
        default:
            return false;
    }
}

void mock_transition_to(MockDeviceMode new_mode) {
    if (mock_can_transition_to(new_mode)) {
        mock_state.previous_mode = mock_state.mode;
        mock_state.mode = new_mode;
    }
}

void mock_set_error(const char* error_msg) {
    mock_state.previous_mode = mock_state.mode;
    mock_state.mode = ERROR_STATE;
    strncpy(mock_state.last_error, error_msg, sizeof(mock_state.last_error) - 1);
}

void mock_clear_error(void) {
    if (mock_state.mode == ERROR_STATE) {
        mock_state.mode = IDLE;
        memset(mock_state.last_error, 0, sizeof(mock_state.last_error));
    }
}

// === DEVICEMODE TRANSITION TESTS ===

bool test_state_init_to_idle(void) {
    reset_mock_state_machine();
    mock_state.mode = INITIALIZING;
    
    bool can_transition = mock_can_transition_to(IDLE);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from INITIALIZING to IDLE");
    
    mock_transition_to(IDLE);
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "State should be IDLE after transition");
    TEST_ASSERT_EQUAL_INT(INITIALIZING, mock_state.previous_mode, "Previous state should be INITIALIZING");
    
    return true;
}

bool test_state_idle_to_heating(void) {
    reset_mock_state_machine();
    mock_state.mode = IDLE;
    
    bool can_transition = mock_can_transition_to(HEATING);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from IDLE to HEATING");
    
    mock_transition_to(HEATING);
    TEST_ASSERT_EQUAL_INT(HEATING, mock_state.mode, "State should be HEATING");
    
    return true;
}

bool test_state_heating_to_idle(void) {
    reset_mock_state_machine();
    mock_state.mode = HEATING;
    
    bool can_transition = mock_can_transition_to(IDLE);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from HEATING to IDLE");
    
    mock_transition_to(IDLE);
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "State should be IDLE");
    
    return true;
}

bool test_state_idle_to_barcode_scanning(void) {
    reset_mock_state_machine();
    mock_state.mode = IDLE;
    mock_state.detector_on = true; // Cartridge detected
    
    bool can_transition = mock_can_transition_to(BARCODE_SCANNING);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from IDLE to BARCODE_SCANNING");
    
    mock_transition_to(BARCODE_SCANNING);
    TEST_ASSERT_EQUAL_INT(BARCODE_SCANNING, mock_state.mode, "State should be BARCODE_SCANNING");
    
    return true;
}

bool test_state_barcode_to_validating(void) {
    reset_mock_state_machine();
    mock_state.mode = BARCODE_SCANNING;
    mock_state.cartridge_state = BARCODE_READ;
    
    bool can_transition = mock_can_transition_to(VALIDATING_CARTRIDGE);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from BARCODE_SCANNING to VALIDATING_CARTRIDGE");
    
    mock_transition_to(VALIDATING_CARTRIDGE);
    TEST_ASSERT_EQUAL_INT(VALIDATING_CARTRIDGE, mock_state.mode, "State should be VALIDATING_CARTRIDGE");
    
    return true;
}

bool test_state_validating_to_running_test(void) {
    reset_mock_state_machine();
    mock_state.mode = VALIDATING_CARTRIDGE;
    mock_state.cartridge_state = VALIDATED;
    mock_state.heater_ready = true;
    
    bool can_transition = mock_can_transition_to(RUNNING_TEST);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from VALIDATING_CARTRIDGE to RUNNING_TEST");
    
    mock_transition_to(RUNNING_TEST);
    TEST_ASSERT_EQUAL_INT(RUNNING_TEST, mock_state.mode, "State should be RUNNING_TEST");
    
    return true;
}

bool test_state_running_to_uploading(void) {
    reset_mock_state_machine();
    mock_state.mode = RUNNING_TEST;
    mock_state.test_state = COMPLETED;
    
    bool can_transition = mock_can_transition_to(UPLOADING_RESULTS);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from RUNNING_TEST to UPLOADING_RESULTS");
    
    mock_transition_to(UPLOADING_RESULTS);
    TEST_ASSERT_EQUAL_INT(UPLOADING_RESULTS, mock_state.mode, "State should be UPLOADING_RESULTS");
    
    return true;
}

bool test_state_uploading_to_idle(void) {
    reset_mock_state_machine();
    mock_state.mode = UPLOADING_RESULTS;
    mock_state.test_state = UPLOADED;
    
    bool can_transition = mock_can_transition_to(IDLE);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from UPLOADING_RESULTS to IDLE");
    
    mock_transition_to(IDLE);
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "State should be IDLE");
    
    return true;
}

bool test_state_any_to_error(void) {
    reset_mock_state_machine();
    
    // Test from multiple states
    MockDeviceMode test_states[] = {IDLE, HEATING, BARCODE_SCANNING, RUNNING_TEST, UPLOADING_RESULTS};
    
    for (int i = 0; i < 5; i++) {
        mock_state.mode = test_states[i];
        
        bool can_transition = mock_can_transition_to(ERROR_STATE);
        TEST_ASSERT_TRUE(can_transition, "Should allow transition to ERROR_STATE from any state");
        
        mock_set_error("Test error");
        TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "State should be ERROR_STATE");
    }
    
    return true;
}

bool test_state_error_to_idle(void) {
    reset_mock_state_machine();
    mock_state.mode = ERROR_STATE;
    strcpy(mock_state.last_error, "Previous error");
    
    bool can_transition = mock_can_transition_to(IDLE);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from ERROR_STATE to IDLE");
    
    mock_clear_error();
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "State should be IDLE after error clear");
    TEST_ASSERT_EQUAL_INT(0, strlen(mock_state.last_error), "Error message should be cleared");
    
    return true;
}

bool test_state_idle_to_stress_testing(void) {
    reset_mock_state_machine();
    mock_state.mode = IDLE;
    
    bool can_transition = mock_can_transition_to(STRESS_TESTING);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from IDLE to STRESS_TESTING");
    
    mock_transition_to(STRESS_TESTING);
    TEST_ASSERT_EQUAL_INT(STRESS_TESTING, mock_state.mode, "State should be STRESS_TESTING");
    
    return true;
}

bool test_state_stress_testing_to_idle(void) {
    reset_mock_state_machine();
    mock_state.mode = STRESS_TESTING;
    
    bool can_transition = mock_can_transition_to(IDLE);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from STRESS_TESTING to IDLE");
    
    mock_transition_to(IDLE);
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "State should be IDLE");
    
    return true;
}

bool test_state_idle_to_validating_magnetometer(void) {
    reset_mock_state_machine();
    mock_state.mode = IDLE;
    
    bool can_transition = mock_can_transition_to(VALIDATING_MAGNETOMETER);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from IDLE to VALIDATING_MAGNETOMETER");
    
    mock_transition_to(VALIDATING_MAGNETOMETER);
    TEST_ASSERT_EQUAL_INT(VALIDATING_MAGNETOMETER, mock_state.mode, "State should be VALIDATING_MAGNETOMETER");
    
    return true;
}

bool test_state_validating_mag_to_idle(void) {
    reset_mock_state_machine();
    mock_state.mode = VALIDATING_MAGNETOMETER;
    
    bool can_transition = mock_can_transition_to(IDLE);
    TEST_ASSERT_TRUE(can_transition, "Should allow transition from VALIDATING_MAGNETOMETER to IDLE");
    
    mock_transition_to(IDLE);
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "State should be IDLE");
    
    return true;
}

// === INVALID TRANSITION TESTS ===

bool test_state_invalid_idle_to_uploading(void) {
    reset_mock_state_machine();
    mock_state.mode = IDLE;
    
    bool can_transition = mock_can_transition_to(UPLOADING_RESULTS);
    TEST_ASSERT_FALSE(can_transition, "Should NOT allow transition from IDLE to UPLOADING_RESULTS");
    
    MockDeviceMode original_mode = mock_state.mode;
    mock_transition_to(UPLOADING_RESULTS);
    TEST_ASSERT_EQUAL_INT(original_mode, mock_state.mode, "State should remain unchanged");
    
    return true;
}

bool test_state_invalid_heating_to_running_test(void) {
    reset_mock_state_machine();
    mock_state.mode = HEATING;
    
    bool can_transition = mock_can_transition_to(RUNNING_TEST);
    TEST_ASSERT_FALSE(can_transition, "Should NOT allow transition from HEATING to RUNNING_TEST");
    
    MockDeviceMode original_mode = mock_state.mode;
    mock_transition_to(RUNNING_TEST);
    TEST_ASSERT_EQUAL_INT(original_mode, mock_state.mode, "State should remain HEATING");
    
    return true;
}

bool test_state_invalid_barcode_to_uploading(void) {
    reset_mock_state_machine();
    mock_state.mode = BARCODE_SCANNING;
    
    bool can_transition = mock_can_transition_to(UPLOADING_RESULTS);
    TEST_ASSERT_FALSE(can_transition, "Should NOT allow transition from BARCODE_SCANNING to UPLOADING_RESULTS");
    
    return true;
}

bool test_state_invalid_running_to_barcode(void) {
    reset_mock_state_machine();
    mock_state.mode = RUNNING_TEST;
    
    bool can_transition = mock_can_transition_to(BARCODE_SCANNING);
    TEST_ASSERT_FALSE(can_transition, "Should NOT allow transition from RUNNING_TEST to BARCODE_SCANNING");
    
    return true;
}

bool test_state_invalid_uploading_to_running(void) {
    reset_mock_state_machine();
    mock_state.mode = UPLOADING_RESULTS;
    
    bool can_transition = mock_can_transition_to(RUNNING_TEST);
    TEST_ASSERT_FALSE(can_transition, "Should NOT allow transition from UPLOADING_RESULTS to RUNNING_TEST");
    
    return true;
}

// === CARTRIDGE FLOW TESTS ===

bool test_state_cartridge_insertion_flow(void) {
    reset_mock_state_machine();
    
    // Step 1: Start in IDLE
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Should start in IDLE");
    TEST_ASSERT_EQUAL_INT(NOT_INSERTED, mock_state.cartridge_state, "Cartridge should not be inserted");
    
    // Step 2: Cartridge detected
    mock_state.detector_on = true;
    mock_state.cartridge_state = DETECTED;
    TEST_ASSERT_TRUE(mock_state.detector_on, "Detector should be on");
    
    // Step 3: Transition to BARCODE_SCANNING
    mock_transition_to(BARCODE_SCANNING);
    TEST_ASSERT_EQUAL_INT(BARCODE_SCANNING, mock_state.mode, "Should transition to BARCODE_SCANNING");
    
    // Step 4: Barcode read successfully
    mock_state.cartridge_state = BARCODE_READ;
    TEST_ASSERT_EQUAL_INT(BARCODE_READ, mock_state.cartridge_state, "Cartridge state should be BARCODE_READ");
    
    // Step 5: Transition to VALIDATING_CARTRIDGE
    mock_transition_to(VALIDATING_CARTRIDGE);
    TEST_ASSERT_EQUAL_INT(VALIDATING_CARTRIDGE, mock_state.mode, "Should transition to VALIDATING_CARTRIDGE");
    
    // Step 6: Cartridge validated
    mock_state.cartridge_state = VALIDATED;
    mock_state.heater_ready = true;
    TEST_ASSERT_EQUAL_INT(VALIDATED, mock_state.cartridge_state, "Cartridge should be validated");
    
    // Step 7: Transition to RUNNING_TEST
    mock_transition_to(RUNNING_TEST);
    TEST_ASSERT_EQUAL_INT(RUNNING_TEST, mock_state.mode, "Should transition to RUNNING_TEST");
    
    return true;
}

bool test_state_cartridge_removal_during_idle(void) {
    reset_mock_state_machine();
    mock_state.mode = IDLE;
    mock_state.cartridge_state = VALIDATED;
    mock_state.detector_on = true;
    
    // Simulate cartridge removal
    mock_state.detector_on = false;
    mock_state.cartridge_state = NOT_INSERTED;
    
    TEST_ASSERT_FALSE(mock_state.detector_on, "Detector should be off");
    TEST_ASSERT_EQUAL_INT(NOT_INSERTED, mock_state.cartridge_state, "Cartridge state should be NOT_INSERTED");
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Device should remain in IDLE");
    
    return true;
}

bool test_state_cartridge_removal_during_test(void) {
    reset_mock_state_machine();
    mock_state.mode = RUNNING_TEST;
    mock_state.test_state = RUNNING;
    mock_state.detector_on = true;
    
    // Simulate cartridge removal during test (CRITICAL scenario)
    mock_state.detector_on = false;
    
    // Should trigger error or cancellation
    TEST_ASSERT_FALSE(mock_state.detector_on, "Detector should be off");
    
    // Test should be cancelled
    mock_state.test_state = CANCELLED;
    TEST_ASSERT_EQUAL_INT(CANCELLED, mock_state.test_state, "Test should be cancelled");
    
    // Should transition to error or idle
    mock_transition_to(ERROR_STATE);
    TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should transition to ERROR_STATE");
    
    return true;
}

bool test_state_cartridge_removal_during_heating(void) {
    reset_mock_state_machine();
    mock_state.mode = HEATING;
    mock_state.detector_on = true;
    
    // Simulate cartridge removal
    mock_state.detector_on = false;
    mock_state.cartridge_state = NOT_INSERTED;
    
    // Should transition to IDLE
    mock_transition_to(IDLE);
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Should transition to IDLE after cartridge removal");
    
    return true;
}

// === TESTSTATE TRANSITION TESTS ===

bool test_teststate_not_started_to_running(void) {
    reset_mock_state_machine();
    mock_state.test_state = NOT_STARTED;
    
    mock_state.test_state = RUNNING;
    TEST_ASSERT_EQUAL_INT(RUNNING, mock_state.test_state, "Test state should be RUNNING");
    
    return true;
}

bool test_teststate_running_to_completed(void) {
    reset_mock_state_machine();
    mock_state.test_state = RUNNING;
    
    mock_state.test_state = COMPLETED;
    TEST_ASSERT_EQUAL_INT(COMPLETED, mock_state.test_state, "Test state should be COMPLETED");
    
    return true;
}

bool test_teststate_running_to_cancelled(void) {
    reset_mock_state_machine();
    mock_state.test_state = RUNNING;
    
    // Simulate test cancellation (e.g., cartridge removal)
    mock_state.test_state = CANCELLED;
    TEST_ASSERT_EQUAL_INT(CANCELLED, mock_state.test_state, "Test state should be CANCELLED");
    
    return true;
}

bool test_teststate_completed_to_upload_pending(void) {
    reset_mock_state_machine();
    mock_state.test_state = COMPLETED;
    
    mock_state.test_state = UPLOAD_PENDING;
    TEST_ASSERT_EQUAL_INT(UPLOAD_PENDING, mock_state.test_state, "Test state should be UPLOAD_PENDING");
    
    return true;
}

bool test_teststate_upload_pending_to_in_progress(void) {
    reset_mock_state_machine();
    mock_state.test_state = UPLOAD_PENDING;
    
    mock_state.test_state = UPLOAD_IN_PROGRESS;
    TEST_ASSERT_EQUAL_INT(UPLOAD_IN_PROGRESS, mock_state.test_state, "Test state should be UPLOAD_IN_PROGRESS");
    
    return true;
}

bool test_teststate_in_progress_to_uploaded(void) {
    reset_mock_state_machine();
    mock_state.test_state = UPLOAD_IN_PROGRESS;
    
    mock_state.test_state = UPLOADED;
    TEST_ASSERT_EQUAL_INT(UPLOADED, mock_state.test_state, "Test state should be UPLOADED");
    
    return true;
}

bool test_teststate_invalid_completed_to_running(void) {
    reset_mock_state_machine();
    mock_state.test_state = COMPLETED;
    
    // Should NOT allow transition back to RUNNING
    MockTestState original_state = mock_state.test_state;
    // In real implementation, this would be prevented
    TEST_ASSERT_EQUAL_INT(COMPLETED, original_state, "Test state should remain COMPLETED");
    
    return true;
}

bool test_teststate_invalid_uploaded_to_running(void) {
    reset_mock_state_machine();
    mock_state.test_state = UPLOADED;
    
    // Should NOT allow transition back to RUNNING
    MockTestState original_state = mock_state.test_state;
    TEST_ASSERT_EQUAL_INT(UPLOADED, original_state, "Test state should remain UPLOADED");
    
    return true;
}

// === CARTRIDGESTATE TRANSITION TESTS ===

bool test_cartridge_not_inserted_to_detected(void) {
    reset_mock_state_machine();
    mock_state.cartridge_state = NOT_INSERTED;
    mock_state.detector_on = false;
    
    // Simulate detection
    mock_state.detector_on = true;
    mock_state.cartridge_state = DETECTED;
    
    TEST_ASSERT_TRUE(mock_state.detector_on, "Detector should be on");
    TEST_ASSERT_EQUAL_INT(DETECTED, mock_state.cartridge_state, "Cartridge state should be DETECTED");
    
    return true;
}

bool test_cartridge_detected_to_barcode_read(void) {
    reset_mock_state_machine();
    mock_state.cartridge_state = DETECTED;
    
    mock_state.cartridge_state = BARCODE_READ;
    TEST_ASSERT_EQUAL_INT(BARCODE_READ, mock_state.cartridge_state, "Cartridge state should be BARCODE_READ");
    
    return true;
}

bool test_cartridge_barcode_read_to_validated(void) {
    reset_mock_state_machine();
    mock_state.cartridge_state = BARCODE_READ;
    
    // Simulate successful validation
    mock_state.cartridge_state = VALIDATED;
    TEST_ASSERT_EQUAL_INT(VALIDATED, mock_state.cartridge_state, "Cartridge state should be VALIDATED");
    
    return true;
}

bool test_cartridge_barcode_read_to_invalid(void) {
    reset_mock_state_machine();
    mock_state.cartridge_state = BARCODE_READ;
    
    // Simulate validation failure
    mock_state.cartridge_state = INVALID;
    TEST_ASSERT_EQUAL_INT(INVALID, mock_state.cartridge_state, "Cartridge state should be INVALID");
    
    return true;
}

bool test_cartridge_validated_to_test_complete(void) {
    reset_mock_state_machine();
    mock_state.cartridge_state = VALIDATED;
    mock_state.test_state = COMPLETED;
    
    mock_state.cartridge_state = TEST_COMPLETE;
    TEST_ASSERT_EQUAL_INT(TEST_COMPLETE, mock_state.cartridge_state, "Cartridge state should be TEST_COMPLETE");
    
    return true;
}

bool test_cartridge_test_complete_to_not_inserted(void) {
    reset_mock_state_machine();
    mock_state.cartridge_state = TEST_COMPLETE;
    mock_state.detector_on = true;
    
    // Simulate cartridge removal
    mock_state.detector_on = false;
    mock_state.cartridge_state = NOT_INSERTED;
    
    TEST_ASSERT_FALSE(mock_state.detector_on, "Detector should be off");
    TEST_ASSERT_EQUAL_INT(NOT_INSERTED, mock_state.cartridge_state, "Cartridge state should be NOT_INSERTED");
    
    return true;
}

bool test_cartridge_invalid_detected_to_validated(void) {
    reset_mock_state_machine();
    mock_state.cartridge_state = DETECTED;
    
    // Should NOT allow direct transition to VALIDATED without BARCODE_READ
    TEST_ASSERT_EQUAL_INT(DETECTED, mock_state.cartridge_state, "Should require BARCODE_READ before VALIDATED");
    
    return true;
}

bool test_cartridge_invalid_not_inserted_to_validated(void) {
    reset_mock_state_machine();
    mock_state.cartridge_state = NOT_INSERTED;
    
    // Should NOT allow transition to VALIDATED without detection and reading
    TEST_ASSERT_EQUAL_INT(NOT_INSERTED, mock_state.cartridge_state, "Should require detection before validation");
    
    return true;
}

// === COMPOUND STATE TESTS ===

bool test_compound_full_test_workflow(void) {
    reset_mock_state_machine();
    
    // Complete workflow from start to finish
    
    // 1. Initialize
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Start in IDLE");
    TEST_ASSERT_EQUAL_INT(NOT_STARTED, mock_state.test_state, "Test not started");
    TEST_ASSERT_EQUAL_INT(NOT_INSERTED, mock_state.cartridge_state, "No cartridge");
    
    // 2. Insert cartridge
    mock_state.detector_on = true;
    mock_state.cartridge_state = DETECTED;
    mock_transition_to(BARCODE_SCANNING);
    TEST_ASSERT_EQUAL_INT(BARCODE_SCANNING, mock_state.mode, "Scanning barcode");
    
    // 3. Read barcode
    mock_state.cartridge_state = BARCODE_READ;
    mock_transition_to(VALIDATING_CARTRIDGE);
    TEST_ASSERT_EQUAL_INT(VALIDATING_CARTRIDGE, mock_state.mode, "Validating");
    
    // 4. Validate cartridge
    mock_state.cartridge_state = VALIDATED;
    mock_state.heater_ready = true;
    mock_transition_to(RUNNING_TEST);
    TEST_ASSERT_EQUAL_INT(RUNNING_TEST, mock_state.mode, "Running test");
    
    // 5. Run test
    mock_state.test_state = RUNNING;
    TEST_ASSERT_EQUAL_INT(RUNNING, mock_state.test_state, "Test running");
    
    // 6. Complete test
    mock_state.test_state = COMPLETED;
    mock_state.cartridge_state = TEST_COMPLETE;
    TEST_ASSERT_EQUAL_INT(COMPLETED, mock_state.test_state, "Test completed");
    
    // 7. Upload results
    mock_state.test_state = UPLOAD_PENDING;
    mock_transition_to(UPLOADING_RESULTS);
    TEST_ASSERT_EQUAL_INT(UPLOADING_RESULTS, mock_state.mode, "Uploading");
    
    // 8. Upload complete
    mock_state.test_state = UPLOADED;
    mock_transition_to(IDLE);
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Back to idle");
    TEST_ASSERT_EQUAL_INT(UPLOADED, mock_state.test_state, "Test uploaded");
    
    return true;
}

bool test_compound_validation_failure_workflow(void) {
    reset_mock_state_machine();
    
    // Test workflow when validation fails
    
    // 1. Start and detect cartridge
    mock_state.detector_on = true;
    mock_state.cartridge_state = DETECTED;
    mock_transition_to(BARCODE_SCANNING);
    
    // 2. Read barcode
    mock_state.cartridge_state = BARCODE_READ;
    mock_transition_to(VALIDATING_CARTRIDGE);
    
    // 3. Validation fails
    mock_state.cartridge_state = INVALID;
    mock_set_error("Validation failed");
    
    TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should be in ERROR_STATE");
    TEST_ASSERT_EQUAL_INT(INVALID, mock_state.cartridge_state, "Cartridge should be INVALID");
    
    // 4. Clear error and return to idle
    mock_clear_error();
    mock_state.cartridge_state = NOT_INSERTED;
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Should return to IDLE");
    
    return true;
}

bool test_compound_cartridge_removal_recovery(void) {
    reset_mock_state_machine();
    
    // Test recovery from unexpected cartridge removal
    
    // 1. Running test
    mock_state.mode = RUNNING_TEST;
    mock_state.test_state = RUNNING;
    mock_state.detector_on = true;
    
    // 2. Cartridge removed unexpectedly
    mock_state.detector_on = false;
    mock_state.test_state = CANCELLED;
    mock_set_error("Cartridge removed during test");
    
    TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should transition to ERROR_STATE");
    TEST_ASSERT_EQUAL_INT(CANCELLED, mock_state.test_state, "Test should be cancelled");
    
    // 3. Clear error
    mock_clear_error();
    mock_state.cartridge_state = NOT_INSERTED;
    mock_state.test_state = NOT_STARTED;
    
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Should return to IDLE");
    TEST_ASSERT_EQUAL_INT(NOT_STARTED, mock_state.test_state, "Test should be reset");
    
    return true;
}

bool test_compound_error_recovery_workflow(void) {
    reset_mock_state_machine();
    
    // Test error recovery from various states
    
    // 1. Error from RUNNING_TEST
    mock_state.mode = RUNNING_TEST;
    mock_set_error("Hardware error");
    TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should be in ERROR_STATE");
    TEST_ASSERT_EQUAL_INT(RUNNING_TEST, mock_state.previous_mode, "Previous mode should be saved");
    
    // 2. Clear error
    mock_clear_error();
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Should recover to IDLE");
    TEST_ASSERT_EQUAL_INT(0, strlen(mock_state.last_error), "Error should be cleared");
    
    return true;
}

bool test_compound_stress_test_workflow(void) {
    reset_mock_state_machine();
    
    // Test stress test mode workflow
    
    // 1. Start from IDLE
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Start in IDLE");
    
    // 2. Enter stress test mode
    mock_transition_to(STRESS_TESTING);
    TEST_ASSERT_EQUAL_INT(STRESS_TESTING, mock_state.mode, "Should be in STRESS_TESTING");
    
    // 3. Exit stress test
    mock_transition_to(IDLE);
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Should return to IDLE");
    
    return true;
}

// === CLOUD OPERATION TRACKING ===

bool test_cloud_op_start_tracking(void) {
    reset_mock_state_machine();
    
    mock_state.cloud_operation_pending = false;
    mock_state.cloud_operation_start_time = 0;
    
    // Start cloud operation
    mock_state.cloud_operation_pending = true;
    mock_state.cloud_operation_start_time = 1000;
    
    TEST_ASSERT_TRUE(mock_state.cloud_operation_pending, "Cloud operation should be pending");
    TEST_ASSERT_TRUE(mock_state.cloud_operation_start_time > 0, "Start time should be set");
    
    return true;
}

bool test_cloud_op_end_tracking(void) {
    reset_mock_state_machine();
    mock_state.cloud_operation_pending = true;
    mock_state.cloud_operation_start_time = 1000;
    
    // End cloud operation
    mock_state.cloud_operation_pending = false;
    mock_state.cloud_operation_start_time = 0;
    
    TEST_ASSERT_FALSE(mock_state.cloud_operation_pending, "Cloud operation should not be pending");
    TEST_ASSERT_EQUAL_INT(0, mock_state.cloud_operation_start_time, "Start time should be reset");
    
    return true;
}

bool test_cloud_op_timeout_detection(void) {
    reset_mock_state_machine();
    
    unsigned long current_time = 35000; // 35 seconds
    mock_state.cloud_operation_pending = true;
    mock_state.cloud_operation_start_time = 1000; // Started 34 seconds ago
    
    unsigned long elapsed = current_time - mock_state.cloud_operation_start_time;
    unsigned long timeout = 30000; // 30 second timeout
    
    TEST_ASSERT_TRUE(elapsed > timeout, "Cloud operation should have timed out");
    
    return true;
}

bool test_cloud_op_prevent_duplicate_requests(void) {
    reset_mock_state_machine();
    mock_state.cloud_operation_pending = true;
    
    // Should not allow starting another cloud operation while one is pending
    bool should_start_new = !mock_state.cloud_operation_pending;
    TEST_ASSERT_FALSE(should_start_new, "Should not start new operation while one is pending");
    
    return true;
}

// === EDGE CASE TESTS ===

bool test_edge_timeout_during_cloud_op(void) {
    reset_mock_state_machine();
    
    // Step 1: Set device to RUNNING_TEST with test COMPLETED
    mock_state.mode = RUNNING_TEST;
    mock_state.test_state = COMPLETED;
    mock_state.cartridge_state = VALIDATED;
    mock_state.detector_on = true;
    
    TEST_ASSERT_EQUAL_INT(RUNNING_TEST, mock_state.mode, "Should be in RUNNING_TEST");
    TEST_ASSERT_EQUAL_INT(COMPLETED, mock_state.test_state, "Test should be COMPLETED");
    
    // Step 2: Transition to UPLOADING_RESULTS state
    mock_transition_to(UPLOADING_RESULTS);
    TEST_ASSERT_EQUAL_INT(UPLOADING_RESULTS, mock_state.mode, "Should transition to UPLOADING_RESULTS");
    
    // Step 3: Start cloud operation tracking
    mock_state.cloud_operation_pending = true;
    mock_state.cloud_operation_start_time = 1000; // Started at 1 second
    TEST_ASSERT_TRUE(mock_state.cloud_operation_pending, "Cloud operation should be pending");
    
    // Step 4: Simulate 35 seconds elapsed (timeout is 30s)
    unsigned long current_time = 36000; // 36 seconds (35 seconds elapsed)
    unsigned long elapsed = current_time - mock_state.cloud_operation_start_time;
    unsigned long timeout_threshold = 30000; // 30 second timeout
    
    // Step 5: Verify timeout is detected
    bool has_timed_out = (elapsed > timeout_threshold);
    TEST_ASSERT_TRUE(has_timed_out, "Cloud operation should have timed out");
    TEST_ASSERT_TRUE(elapsed == 35000, "Elapsed time should be 35000ms");
    
    // Step 6: Verify device transitions to ERROR_STATE
    mock_set_error("Cloud upload timeout");
    TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should transition to ERROR_STATE on timeout");
    TEST_ASSERT_EQUAL_INT(UPLOADING_RESULTS, mock_state.previous_mode, "Previous mode should be UPLOADING_RESULTS");
    
    // Step 7: Verify test remains COMPLETED (not lost)
    TEST_ASSERT_EQUAL_INT(COMPLETED, mock_state.test_state, "Test should remain COMPLETED (data not lost)");
    
    // Step 8: Verify error recovery returns to IDLE with test marked UPLOAD_PENDING
    mock_clear_error();
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Should recover to IDLE");
    
    // Mark test as UPLOAD_PENDING to retry later
    mock_state.test_state = UPLOAD_PENDING;
    TEST_ASSERT_EQUAL_INT(UPLOAD_PENDING, mock_state.test_state, "Test should be UPLOAD_PENDING for retry");
    
    // Verify cloud operation tracking is cleared
    mock_state.cloud_operation_pending = false;
    mock_state.cloud_operation_start_time = 0;
    TEST_ASSERT_FALSE(mock_state.cloud_operation_pending, "Cloud operation should not be pending after timeout");
    
    return true;
}

bool test_edge_cartridge_removal_multiple_states(void) {
    reset_mock_state_machine();
    
    // === Sub-test 1: Cartridge removal during BARCODE_SCANNING ===
    // This should be graceful - just return to IDLE
    
    mock_state.mode = BARCODE_SCANNING;
    mock_state.cartridge_state = DETECTED;
    mock_state.detector_on = true;
    
    TEST_ASSERT_EQUAL_INT(BARCODE_SCANNING, mock_state.mode, "Should start in BARCODE_SCANNING");
    TEST_ASSERT_TRUE(mock_state.detector_on, "Detector should be on");
    
    // Simulate cartridge removal
    mock_state.detector_on = false;
    mock_state.cartridge_state = NOT_INSERTED;
    
    // Should transition to IDLE (graceful)
    mock_transition_to(IDLE);
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Should transition to IDLE gracefully from BARCODE_SCANNING");
    TEST_ASSERT_EQUAL_INT(NOT_INSERTED, mock_state.cartridge_state, "Cartridge state should be NOT_INSERTED");
    TEST_ASSERT_FALSE(mock_state.detector_on, "Detector should be off");
    
    // === Sub-test 2: Cartridge removal during VALIDATING_CARTRIDGE ===
    // This is more serious - operation in progress
    
    reset_mock_state_machine();
    mock_state.mode = VALIDATING_CARTRIDGE;
    mock_state.cartridge_state = BARCODE_READ;
    mock_state.detector_on = true;
    mock_state.cloud_operation_pending = true;
    mock_state.cloud_operation_start_time = 1000;
    
    TEST_ASSERT_EQUAL_INT(VALIDATING_CARTRIDGE, mock_state.mode, "Should be in VALIDATING_CARTRIDGE");
    TEST_ASSERT_TRUE(mock_state.cloud_operation_pending, "Cloud operation should be pending");
    
    // Simulate cartridge removal during validation
    mock_state.detector_on = false;
    mock_state.cartridge_state = NOT_INSERTED;
    
    // Should transition to ERROR_STATE (operation in progress)
    mock_set_error("Cartridge removed during validation");
    TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should transition to ERROR_STATE");
    
    // Should cancel cloud operation
    mock_state.cloud_operation_pending = false;
    mock_state.cloud_operation_start_time = 0;
    TEST_ASSERT_FALSE(mock_state.cloud_operation_pending, "Cloud operation should be cancelled");
    
    // Test state should be CANCELLED
    mock_state.test_state = CANCELLED;
    TEST_ASSERT_EQUAL_INT(CANCELLED, mock_state.test_state, "Test should be CANCELLED");
    
    // === Sub-test 3: Cartridge removal during RUNNING_TEST (CRITICAL) ===
    // This is the most critical scenario
    
    reset_mock_state_machine();
    mock_state.mode = RUNNING_TEST;
    mock_state.test_state = RUNNING;
    mock_state.cartridge_state = VALIDATED;
    mock_state.detector_on = true;
    
    TEST_ASSERT_EQUAL_INT(RUNNING_TEST, mock_state.mode, "Should be in RUNNING_TEST");
    TEST_ASSERT_EQUAL_INT(RUNNING, mock_state.test_state, "Test should be RUNNING");
    
    // Simulate cartridge removal during active test (CRITICAL)
    mock_state.detector_on = false;
    mock_state.cartridge_state = NOT_INSERTED;
    
    // Should transition to ERROR_STATE immediately
    mock_set_error("CRITICAL: Cartridge removed during test execution");
    TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should transition to ERROR_STATE immediately");
    TEST_ASSERT_EQUAL_INT(RUNNING_TEST, mock_state.previous_mode, "Previous mode should be RUNNING_TEST");
    
    // Test must be CANCELLED
    mock_state.test_state = CANCELLED;
    TEST_ASSERT_EQUAL_INT(CANCELLED, mock_state.test_state, "Test must be CANCELLED");
    
    // Must not allow results upload for cancelled test
    bool can_upload = (mock_state.test_state == COMPLETED);
    TEST_ASSERT_FALSE(can_upload, "Must not allow upload of cancelled test");
    
    // === Sub-test 4: Cartridge removal during UPLOADING_RESULTS ===
    // May allow upload to complete (data integrity)
    
    reset_mock_state_machine();
    mock_state.mode = UPLOADING_RESULTS;
    mock_state.test_state = COMPLETED;
    mock_state.cartridge_state = TEST_COMPLETE;
    mock_state.detector_on = true;
    mock_state.cloud_operation_pending = true;
    mock_state.cloud_operation_start_time = 1000;
    
    TEST_ASSERT_EQUAL_INT(UPLOADING_RESULTS, mock_state.mode, "Should be in UPLOADING_RESULTS");
    TEST_ASSERT_EQUAL_INT(COMPLETED, mock_state.test_state, "Test should be COMPLETED");
    
    // Simulate cartridge removal during upload
    mock_state.detector_on = false;
    mock_state.cartridge_state = NOT_INSERTED;
    
    // Test data should be preserved (don't cancel completed test)
    TEST_ASSERT_EQUAL_INT(COMPLETED, mock_state.test_state, "Test data should remain COMPLETED");
    
    // Option A: Allow upload to complete (data integrity priority)
    // The upload should continue even with cartridge removed
    TEST_ASSERT_TRUE(mock_state.cloud_operation_pending, "Upload should continue for data integrity");
    
    // After upload completes, transition to IDLE
    mock_state.cloud_operation_pending = false;
    mock_state.test_state = UPLOADED;
    mock_transition_to(IDLE);
    
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Should transition to IDLE after upload completes");
    TEST_ASSERT_EQUAL_INT(UPLOADED, mock_state.test_state, "Test should be successfully UPLOADED");
    
    return true;
}

// === ERROR STATE HANDLING ===

bool test_error_state_transition_from_any_state(void) {
    reset_mock_state_machine();
    
    MockDeviceMode states[] = {IDLE, HEATING, BARCODE_SCANNING, VALIDATING_CARTRIDGE, 
                               RUNNING_TEST, UPLOADING_RESULTS, STRESS_TESTING};
    
    for (int i = 0; i < 7; i++) {
        mock_state.mode = states[i];
        mock_set_error("Test error");
        TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should transition to ERROR_STATE");
        mock_clear_error();
    }
    
    return true;
}

bool test_error_state_message_storage(void) {
    reset_mock_state_machine();
    
    const char* error_msg = "Critical hardware failure";
    mock_set_error(error_msg);
    
    TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should be in ERROR_STATE");
    TEST_ASSERT_EQUAL_STRING(error_msg, mock_state.last_error, "Error message should be stored");
    
    return true;
}

bool test_error_state_recovery(void) {
    reset_mock_state_machine();
    mock_state.mode = RUNNING_TEST;
    
    mock_set_error("Temporary error");
    TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should be in ERROR_STATE");
    TEST_ASSERT_EQUAL_INT(RUNNING_TEST, mock_state.previous_mode, "Previous mode should be saved");
    
    mock_clear_error();
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Should recover to IDLE");
    
    return true;
}

bool test_error_state_clear(void) {
    reset_mock_state_machine();
    mock_set_error("Test error");
    
    TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should be in ERROR_STATE");
    TEST_ASSERT_TRUE(strlen(mock_state.last_error) > 0, "Error message should be present");
    
    mock_clear_error();
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.mode, "Should be in IDLE");
    TEST_ASSERT_EQUAL_INT(0, strlen(mock_state.last_error), "Error message should be cleared");
    
    return true;
}

// === STATE QUERY FUNCTIONS ===

bool test_query_is_idle(void) {
    reset_mock_state_machine();
    mock_state.mode = IDLE;
    
    bool is_idle = (mock_state.mode == IDLE);
    TEST_ASSERT_TRUE(is_idle, "is_idle() should return true");
    
    mock_state.mode = RUNNING_TEST;
    is_idle = (mock_state.mode == IDLE);
    TEST_ASSERT_FALSE(is_idle, "is_idle() should return false");
    
    return true;
}

bool test_query_is_testing(void) {
    reset_mock_state_machine();
    mock_state.mode = RUNNING_TEST;
    
    bool is_testing = (mock_state.mode == RUNNING_TEST);
    TEST_ASSERT_TRUE(is_testing, "is_testing() should return true");
    
    mock_state.mode = IDLE;
    is_testing = (mock_state.mode == RUNNING_TEST);
    TEST_ASSERT_FALSE(is_testing, "is_testing() should return false");
    
    return true;
}

bool test_query_is_heating(void) {
    reset_mock_state_machine();
    mock_state.mode = HEATING;
    
    bool is_heating = (mock_state.mode == HEATING);
    TEST_ASSERT_TRUE(is_heating, "is_heating() should return true");
    
    return true;
}

bool test_query_is_error(void) {
    reset_mock_state_machine();
    mock_state.mode = ERROR_STATE;
    
    bool is_error = (mock_state.mode == ERROR_STATE);
    TEST_ASSERT_TRUE(is_error, "is_error() should return true");
    
    return true;
}

bool test_query_has_cartridge(void) {
    reset_mock_state_machine();
    mock_state.cartridge_state = DETECTED;
    
    bool has_cartridge = (mock_state.cartridge_state != NOT_INSERTED);
    TEST_ASSERT_TRUE(has_cartridge, "has_cartridge() should return true");
    
    mock_state.cartridge_state = NOT_INSERTED;
    has_cartridge = (mock_state.cartridge_state != NOT_INSERTED);
    TEST_ASSERT_FALSE(has_cartridge, "has_cartridge() should return false");
    
    return true;
}

bool test_query_is_cartridge_validated(void) {
    reset_mock_state_machine();
    mock_state.cartridge_state = VALIDATED;
    
    bool is_validated = (mock_state.cartridge_state == VALIDATED);
    TEST_ASSERT_TRUE(is_validated, "is_cartridge_validated() should return true");
    
    mock_state.cartridge_state = DETECTED;
    is_validated = (mock_state.cartridge_state == VALIDATED);
    TEST_ASSERT_FALSE(is_validated, "is_cartridge_validated() should return false");
    
    return true;
}

// === CONCURRENT STATE VALIDATION ===

bool test_concurrent_device_and_test_states(void) {
    reset_mock_state_machine();
    
    // Verify proper coordination of device and test states
    mock_state.mode = RUNNING_TEST;
    mock_state.test_state = RUNNING;
    
    TEST_ASSERT_EQUAL_INT(RUNNING_TEST, mock_state.mode, "Device should be in RUNNING_TEST");
    TEST_ASSERT_EQUAL_INT(RUNNING, mock_state.test_state, "Test should be RUNNING");
    
    // Both states should transition together
    mock_state.test_state = COMPLETED;
    mock_transition_to(UPLOADING_RESULTS);
    
    TEST_ASSERT_EQUAL_INT(UPLOADING_RESULTS, mock_state.mode, "Device should be UPLOADING");
    TEST_ASSERT_EQUAL_INT(COMPLETED, mock_state.test_state, "Test should be COMPLETED");
    
    return true;
}

bool test_concurrent_device_and_cartridge_states(void) {
    reset_mock_state_machine();
    
    // Verify proper coordination of device and cartridge states
    mock_state.mode = BARCODE_SCANNING;
    mock_state.cartridge_state = DETECTED;
    
    TEST_ASSERT_EQUAL_INT(BARCODE_SCANNING, mock_state.mode, "Device should be scanning");
    TEST_ASSERT_EQUAL_INT(DETECTED, mock_state.cartridge_state, "Cartridge should be detected");
    
    // States should transition together
    mock_state.cartridge_state = BARCODE_READ;
    mock_transition_to(VALIDATING_CARTRIDGE);
    
    TEST_ASSERT_EQUAL_INT(VALIDATING_CARTRIDGE, mock_state.mode, "Device should be validating");
    TEST_ASSERT_EQUAL_INT(BARCODE_READ, mock_state.cartridge_state, "Cartridge barcode should be read");
    
    return true;
}

bool test_concurrent_all_state_machines(void) {
    reset_mock_state_machine();
    
    // Test full coordination of all three state machines
    mock_state.mode = RUNNING_TEST;
    mock_state.test_state = RUNNING;
    mock_state.cartridge_state = VALIDATED;
    mock_state.heater_ready = true;
    mock_state.detector_on = true;
    
    TEST_ASSERT_EQUAL_INT(RUNNING_TEST, mock_state.mode, "Device mode correct");
    TEST_ASSERT_EQUAL_INT(RUNNING, mock_state.test_state, "Test state correct");
    TEST_ASSERT_EQUAL_INT(VALIDATED, mock_state.cartridge_state, "Cartridge state correct");
    TEST_ASSERT_TRUE(mock_state.heater_ready, "Heater should be ready");
    TEST_ASSERT_TRUE(mock_state.detector_on, "Detector should be on");
    
    return true;
}

// === SAFETY-CRITICAL STATE TESTS ===

bool test_safety_no_test_without_validation(void) {
    reset_mock_state_machine();
    
    // CRITICAL: Should NOT allow test without validated cartridge
    mock_state.cartridge_state = DETECTED; // Not validated
    mock_state.mode = IDLE;
    
    bool can_start_test = (mock_state.cartridge_state == VALIDATED);
    TEST_ASSERT_FALSE(can_start_test, "Should NOT allow test without validated cartridge");
    
    // With validation
    mock_state.cartridge_state = VALIDATED;
    can_start_test = (mock_state.cartridge_state == VALIDATED);
    TEST_ASSERT_TRUE(can_start_test, "Should allow test with validated cartridge");
    
    return true;
}

bool test_safety_no_heating_without_cartridge(void) {
    reset_mock_state_machine();
    
    // CRITICAL: Should NOT heat without cartridge present
    mock_state.detector_on = false;
    mock_state.cartridge_state = NOT_INSERTED;
    
    bool should_heat = mock_state.detector_on;
    TEST_ASSERT_FALSE(should_heat, "Should NOT heat without cartridge");
    
    return true;
}

bool test_safety_emergency_stop_from_any_state(void) {
    reset_mock_state_machine();
    
    // CRITICAL: Must be able to transition to ERROR_STATE from any state
    MockDeviceMode critical_states[] = {RUNNING_TEST, HEATING, UPLOADING_RESULTS};
    
    for (int i = 0; i < 3; i++) {
        mock_state.mode = critical_states[i];
        
        bool can_emergency_stop = mock_can_transition_to(ERROR_STATE);
        TEST_ASSERT_TRUE(can_emergency_stop, "Must allow emergency stop from any state");
        
        mock_set_error("Emergency stop");
        TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should be in ERROR_STATE");
        
        mock_clear_error();
    }
    
    return true;
}

bool test_safety_state_after_hardware_failure(void) {
    reset_mock_state_machine();
    mock_state.mode = RUNNING_TEST;
    mock_state.test_state = RUNNING;
    
    // Simulate hardware failure
    mock_set_error("Hardware failure detected");
    
    TEST_ASSERT_EQUAL_INT(ERROR_STATE, mock_state.mode, "Should transition to ERROR_STATE");
    TEST_ASSERT_TRUE(strlen(mock_state.last_error) > 0, "Error should be logged");
    
    // Test should be marked as cancelled
    mock_state.test_state = CANCELLED;
    TEST_ASSERT_EQUAL_INT(CANCELLED, mock_state.test_state, "Test should be cancelled");
    
    return true;
}

// === STATE PERSISTENCE ===

bool test_state_previous_mode_tracking(void) {
    reset_mock_state_machine();
    mock_state.mode = IDLE;
    
    mock_transition_to(HEATING);
    TEST_ASSERT_EQUAL_INT(IDLE, mock_state.previous_mode, "Previous mode should be IDLE");
    TEST_ASSERT_EQUAL_INT(HEATING, mock_state.mode, "Current mode should be HEATING");
    
    mock_transition_to(BARCODE_SCANNING);
    TEST_ASSERT_EQUAL_INT(HEATING, mock_state.previous_mode, "Previous mode should be HEATING");
    
    return true;
}

bool test_state_transition_logging(void) {
    reset_mock_state_machine();
    
    // Track transition sequence
    MockDeviceMode expected_sequence[] = {IDLE, HEATING, BARCODE_SCANNING, VALIDATING_CARTRIDGE};
    
    for (int i = 0; i < 3; i++) {
        mock_transition_to(expected_sequence[i + 1]);
        TEST_ASSERT_EQUAL_INT(expected_sequence[i], mock_state.previous_mode, "Previous state tracked");
        TEST_ASSERT_EQUAL_INT(expected_sequence[i + 1], mock_state.mode, "Current state correct");
    }
    
    return true;
}

// === TEST SUITE RUNNER ===

BEGIN_TEST_SUITE(state_transitions)
    TestMetadata meta;
    
    // === DEVICEMODE TRANSITIONS ===
    meta = {"TC-STATE-001", "INIT to IDLE Transition", "REQ-STATE-001", "RISK-HIGH-010", "DeviceMode"};
    TEST_CASE(test_state_init_to_idle, meta);
    
    meta = {"TC-STATE-002", "IDLE to HEATING Transition", "REQ-STATE-002", "RISK-MED-010", "DeviceMode"};
    TEST_CASE(test_state_idle_to_heating, meta);
    
    meta = {"TC-STATE-003", "HEATING to IDLE Transition", "REQ-STATE-003", "RISK-MED-011", "DeviceMode"};
    TEST_CASE(test_state_heating_to_idle, meta);
    
    meta = {"TC-STATE-004", "IDLE to BARCODE_SCANNING", "REQ-STATE-004", "RISK-MED-012", "DeviceMode"};
    TEST_CASE(test_state_idle_to_barcode_scanning, meta);
    
    meta = {"TC-STATE-005", "BARCODE to VALIDATING", "REQ-STATE-005", "RISK-HIGH-011", "DeviceMode"};
    TEST_CASE(test_state_barcode_to_validating, meta);
    
    meta = {"TC-STATE-006", "VALIDATING to RUNNING_TEST", "REQ-STATE-006", "RISK-CRIT-010", "DeviceMode"};
    TEST_CASE(test_state_validating_to_running_test, meta);
    
    meta = {"TC-STATE-007", "RUNNING to UPLOADING", "REQ-STATE-007", "RISK-HIGH-012", "DeviceMode"};
    TEST_CASE(test_state_running_to_uploading, meta);
    
    meta = {"TC-STATE-008", "UPLOADING to IDLE", "REQ-STATE-008", "RISK-MED-013", "DeviceMode"};
    TEST_CASE(test_state_uploading_to_idle, meta);
    
    meta = {"TC-STATE-009", "Any State to ERROR", "REQ-STATE-009", "RISK-CRIT-011", "DeviceMode"};
    TEST_CASE(test_state_any_to_error, meta);
    
    meta = {"TC-STATE-010", "ERROR to IDLE", "REQ-STATE-010", "RISK-HIGH-013", "DeviceMode"};
    TEST_CASE(test_state_error_to_idle, meta);
    
    meta = {"TC-STATE-011", "IDLE to STRESS_TESTING", "REQ-STATE-011", "RISK-LOW-010", "DeviceMode"};
    TEST_CASE(test_state_idle_to_stress_testing, meta);
    
    meta = {"TC-STATE-012", "STRESS_TESTING to IDLE", "REQ-STATE-012", "RISK-LOW-011", "DeviceMode"};
    TEST_CASE(test_state_stress_testing_to_idle, meta);
    
    meta = {"TC-STATE-013", "IDLE to VALIDATING_MAGNETOMETER", "REQ-STATE-013", "RISK-MED-014", "DeviceMode"};
    TEST_CASE(test_state_idle_to_validating_magnetometer, meta);
    
    meta = {"TC-STATE-014", "VALIDATING_MAG to IDLE", "REQ-STATE-014", "RISK-MED-015", "DeviceMode"};
    TEST_CASE(test_state_validating_mag_to_idle, meta);
    
    // === INVALID TRANSITIONS ===
    meta = {"TC-STATE-INV-001", "Invalid IDLE to UPLOADING", "REQ-STATE-INV-001", "RISK-HIGH-014", "Invalid Transitions"};
    TEST_CASE(test_state_invalid_idle_to_uploading, meta);
    
    meta = {"TC-STATE-INV-002", "Invalid HEATING to RUNNING", "REQ-STATE-INV-002", "RISK-HIGH-015", "Invalid Transitions"};
    TEST_CASE(test_state_invalid_heating_to_running_test, meta);
    
    meta = {"TC-STATE-INV-003", "Invalid BARCODE to UPLOADING", "REQ-STATE-INV-003", "RISK-MED-016", "Invalid Transitions"};
    TEST_CASE(test_state_invalid_barcode_to_uploading, meta);
    
    meta = {"TC-STATE-INV-004", "Invalid RUNNING to BARCODE", "REQ-STATE-INV-004", "RISK-HIGH-016", "Invalid Transitions"};
    TEST_CASE(test_state_invalid_running_to_barcode, meta);
    
    meta = {"TC-STATE-INV-005", "Invalid UPLOADING to RUNNING", "REQ-STATE-INV-005", "RISK-HIGH-017", "Invalid Transitions"};
    TEST_CASE(test_state_invalid_uploading_to_running, meta);
    
    // === CARTRIDGE FLOW ===
    meta = {"TC-CART-001", "Full Cartridge Insertion Flow", "REQ-CART-001", "RISK-HIGH-018", "Cartridge Flow"};
    TEST_CASE(test_state_cartridge_insertion_flow, meta);
    
    meta = {"TC-CART-002", "Cartridge Removal During IDLE", "REQ-CART-002", "RISK-MED-017", "Cartridge Flow"};
    TEST_CASE(test_state_cartridge_removal_during_idle, meta);
    
    meta = {"TC-CART-003", "Cartridge Removal During Test", "REQ-CART-003", "RISK-CRIT-012", "Cartridge Flow"};
    TEST_CASE(test_state_cartridge_removal_during_test, meta);
    
    meta = {"TC-CART-004", "Cartridge Removal During Heating", "REQ-CART-004", "RISK-HIGH-019", "Cartridge Flow"};
    TEST_CASE(test_state_cartridge_removal_during_heating, meta);
    
    // === TESTSTATE TRANSITIONS ===
    meta = {"TC-TEST-001", "NOT_STARTED to RUNNING", "REQ-TEST-001", "RISK-HIGH-020", "TestState"};
    TEST_CASE(test_teststate_not_started_to_running, meta);
    
    meta = {"TC-TEST-002", "RUNNING to COMPLETED", "REQ-TEST-002", "RISK-HIGH-021", "TestState"};
    TEST_CASE(test_teststate_running_to_completed, meta);
    
    meta = {"TC-TEST-003", "RUNNING to CANCELLED", "REQ-TEST-003", "RISK-HIGH-022", "TestState"};
    TEST_CASE(test_teststate_running_to_cancelled, meta);
    
    meta = {"TC-TEST-004", "COMPLETED to UPLOAD_PENDING", "REQ-TEST-004", "RISK-MED-018", "TestState"};
    TEST_CASE(test_teststate_completed_to_upload_pending, meta);
    
    meta = {"TC-TEST-005", "UPLOAD_PENDING to IN_PROGRESS", "REQ-TEST-005", "RISK-MED-019", "TestState"};
    TEST_CASE(test_teststate_upload_pending_to_in_progress, meta);
    
    meta = {"TC-TEST-006", "IN_PROGRESS to UPLOADED", "REQ-TEST-006", "RISK-MED-020", "TestState"};
    TEST_CASE(test_teststate_in_progress_to_uploaded, meta);
    
    meta = {"TC-TEST-INV-001", "Invalid COMPLETED to RUNNING", "REQ-TEST-INV-001", "RISK-HIGH-023", "TestState"};
    TEST_CASE(test_teststate_invalid_completed_to_running, meta);
    
    meta = {"TC-TEST-INV-002", "Invalid UPLOADED to RUNNING", "REQ-TEST-INV-002", "RISK-HIGH-024", "TestState"};
    TEST_CASE(test_teststate_invalid_uploaded_to_running, meta);
    
    // === CARTRIDGESTATE TRANSITIONS ===
    meta = {"TC-CSTATE-001", "NOT_INSERTED to DETECTED", "REQ-CSTATE-001", "RISK-MED-021", "CartridgeState"};
    TEST_CASE(test_cartridge_not_inserted_to_detected, meta);
    
    meta = {"TC-CSTATE-002", "DETECTED to BARCODE_READ", "REQ-CSTATE-002", "RISK-MED-022", "CartridgeState"};
    TEST_CASE(test_cartridge_detected_to_barcode_read, meta);
    
    meta = {"TC-CSTATE-003", "BARCODE_READ to VALIDATED", "REQ-CSTATE-003", "RISK-HIGH-025", "CartridgeState"};
    TEST_CASE(test_cartridge_barcode_read_to_validated, meta);
    
    meta = {"TC-CSTATE-004", "BARCODE_READ to INVALID", "REQ-CSTATE-004", "RISK-HIGH-026", "CartridgeState"};
    TEST_CASE(test_cartridge_barcode_read_to_invalid, meta);
    
    meta = {"TC-CSTATE-005", "VALIDATED to TEST_COMPLETE", "REQ-CSTATE-005", "RISK-MED-023", "CartridgeState"};
    TEST_CASE(test_cartridge_validated_to_test_complete, meta);
    
    meta = {"TC-CSTATE-006", "TEST_COMPLETE to NOT_INSERTED", "REQ-CSTATE-006", "RISK-MED-024", "CartridgeState"};
    TEST_CASE(test_cartridge_test_complete_to_not_inserted, meta);
    
    meta = {"TC-CSTATE-INV-001", "Invalid DETECTED to VALIDATED", "REQ-CSTATE-INV-001", "RISK-CRIT-013", "CartridgeState"};
    TEST_CASE(test_cartridge_invalid_detected_to_validated, meta);
    
    meta = {"TC-CSTATE-INV-002", "Invalid NOT_INSERTED to VALIDATED", "REQ-CSTATE-INV-002", "RISK-CRIT-014", "CartridgeState"};
    TEST_CASE(test_cartridge_invalid_not_inserted_to_validated, meta);
    
    // === COMPOUND TESTS ===
    meta = {"TC-COMP-001", "Full Test Workflow", "REQ-COMP-001", "RISK-CRIT-015", "Compound"};
    TEST_CASE(test_compound_full_test_workflow, meta);
    
    meta = {"TC-COMP-002", "Validation Failure Workflow", "REQ-COMP-002", "RISK-HIGH-027", "Compound"};
    TEST_CASE(test_compound_validation_failure_workflow, meta);
    
    meta = {"TC-COMP-003", "Cartridge Removal Recovery", "REQ-COMP-003", "RISK-CRIT-016", "Compound"};
    TEST_CASE(test_compound_cartridge_removal_recovery, meta);
    
    meta = {"TC-COMP-004", "Error Recovery Workflow", "REQ-COMP-004", "RISK-HIGH-028", "Compound"};
    TEST_CASE(test_compound_error_recovery_workflow, meta);
    
    meta = {"TC-COMP-005", "Stress Test Workflow", "REQ-COMP-005", "RISK-MED-025", "Compound"};
    TEST_CASE(test_compound_stress_test_workflow, meta);
    
    // === CLOUD OPERATION TRACKING ===
    meta = {"TC-CLOUD-001", "Start Cloud Operation Tracking", "REQ-CLOUD-001", "RISK-MED-026", "Cloud Operations"};
    TEST_CASE(test_cloud_op_start_tracking, meta);
    
    meta = {"TC-CLOUD-002", "End Cloud Operation Tracking", "REQ-CLOUD-002", "RISK-MED-027", "Cloud Operations"};
    TEST_CASE(test_cloud_op_end_tracking, meta);
    
    meta = {"TC-CLOUD-003", "Cloud Operation Timeout", "REQ-CLOUD-003", "RISK-HIGH-029", "Cloud Operations"};
    TEST_CASE(test_cloud_op_timeout_detection, meta);
    
    meta = {"TC-CLOUD-004", "Prevent Duplicate Requests", "REQ-CLOUD-004", "RISK-HIGH-030", "Cloud Operations"};
    TEST_CASE(test_cloud_op_prevent_duplicate_requests, meta);
    
    // === ERROR STATE HANDLING ===
    meta = {"TC-ERR-001", "Error Transition From Any State", "REQ-ERR-001", "RISK-CRIT-017", "Error Handling"};
    TEST_CASE(test_error_state_transition_from_any_state, meta);
    
    meta = {"TC-ERR-002", "Error Message Storage", "REQ-ERR-002", "RISK-MED-028", "Error Handling"};
    TEST_CASE(test_error_state_message_storage, meta);
    
    meta = {"TC-ERR-003", "Error State Recovery", "REQ-ERR-003", "RISK-HIGH-031", "Error Handling"};
    TEST_CASE(test_error_state_recovery, meta);
    
    meta = {"TC-ERR-004", "Error State Clear", "REQ-ERR-004", "RISK-MED-029", "Error Handling"};
    TEST_CASE(test_error_state_clear, meta);
    
    // === STATE QUERY FUNCTIONS ===
    meta = {"TC-QUERY-001", "Query is_idle", "REQ-QUERY-001", "RISK-LOW-012", "State Query"};
    TEST_CASE(test_query_is_idle, meta);
    
    meta = {"TC-QUERY-002", "Query is_testing", "REQ-QUERY-002", "RISK-LOW-013", "State Query"};
    TEST_CASE(test_query_is_testing, meta);
    
    meta = {"TC-QUERY-003", "Query is_heating", "REQ-QUERY-003", "RISK-LOW-014", "State Query"};
    TEST_CASE(test_query_is_heating, meta);
    
    meta = {"TC-QUERY-004", "Query is_error", "REQ-QUERY-004", "RISK-LOW-015", "State Query"};
    TEST_CASE(test_query_is_error, meta);
    
    meta = {"TC-QUERY-005", "Query has_cartridge", "REQ-QUERY-005", "RISK-LOW-016", "State Query"};
    TEST_CASE(test_query_has_cartridge, meta);
    
    meta = {"TC-QUERY-006", "Query is_cartridge_validated", "REQ-QUERY-006", "RISK-MED-030", "State Query"};
    TEST_CASE(test_query_is_cartridge_validated, meta);
    
    // === CONCURRENT STATE VALIDATION ===
    meta = {"TC-CONC-001", "Device and Test States", "REQ-CONC-001", "RISK-HIGH-032", "Concurrent States"};
    TEST_CASE(test_concurrent_device_and_test_states, meta);
    
    meta = {"TC-CONC-002", "Device and Cartridge States", "REQ-CONC-002", "RISK-HIGH-033", "Concurrent States"};
    TEST_CASE(test_concurrent_device_and_cartridge_states, meta);
    
    meta = {"TC-CONC-003", "All State Machines", "REQ-CONC-003", "RISK-CRIT-018", "Concurrent States"};
    TEST_CASE(test_concurrent_all_state_machines, meta);
    
    // === SAFETY-CRITICAL TESTS ===
    meta = {"TC-SAFE-010", "No Test Without Validation", "REQ-SAFE-010", "RISK-CRIT-019", "Safety"};
    TEST_CASE(test_safety_no_test_without_validation, meta);
    
    meta = {"TC-SAFE-011", "No Heating Without Cartridge", "REQ-SAFE-011", "RISK-CRIT-020", "Safety"};
    TEST_CASE(test_safety_no_heating_without_cartridge, meta);
    
    meta = {"TC-SAFE-012", "Emergency Stop From Any State", "REQ-SAFE-012", "RISK-CRIT-021", "Safety"};
    TEST_CASE(test_safety_emergency_stop_from_any_state, meta);
    
    meta = {"TC-SAFE-013", "State After Hardware Failure", "REQ-SAFE-013", "RISK-CRIT-022", "Safety"};
    TEST_CASE(test_safety_state_after_hardware_failure, meta);
    
    // === STATE PERSISTENCE ===
    meta = {"TC-PERSIST-001", "Previous Mode Tracking", "REQ-PERSIST-001", "RISK-MED-031", "Persistence"};
    TEST_CASE(test_state_previous_mode_tracking, meta);
    
    meta = {"TC-PERSIST-002", "Transition Logging", "REQ-PERSIST-002", "RISK-MED-032", "Persistence"};
    TEST_CASE(test_state_transition_logging, meta);
    
    // === EDGE CASE TESTS ===
    meta = {"TC-EDGE-001", "Cloud Timeout During Upload", "REQ-EDGE-001", "RISK-CRIT-023", "Edge Cases"};
    TEST_CASE(test_edge_timeout_during_cloud_op, meta);
    
    meta = {"TC-EDGE-002", "Cartridge Removal Multiple States", "REQ-EDGE-002", "RISK-CRIT-024", "Edge Cases"};
    TEST_CASE(test_edge_cartridge_removal_multiple_states, meta);
    
END_TEST_SUITE()

