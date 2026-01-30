/*!
 * @file test_state_transitions.h
 * @brief Unit tests for device state machine transitions
 * @details Comprehensive testing of all state transitions with validation
 * 
 * Test Coverage:
 * - DeviceMode transitions (11 states)
 * - TestState transitions (7 states)
 * - CartridgeState transitions (6 states)
 * - Valid transition paths
 * - Invalid transition prevention
 * - State machine integrity
 * - Concurrent state handling
 * 
 * Compliance: ISO 13485:2016 Section 7.3.5
 * Risk Level: CRITICAL (state machine controls device behavior)
 */

#ifndef TEST_STATE_TRANSITIONS_H
#define TEST_STATE_TRANSITIONS_H

#include "test_framework.h"
#include <stdbool.h>

// Mock DeviceMode enum (matches firmware)
typedef enum {
    IDLE,
    INITIALIZING,
    HEATING,
    BARCODE_SCANNING,
    VALIDATING_CARTRIDGE,
    VALIDATING_MAGNETOMETER,
    RUNNING_TEST,
    UPLOADING_RESULTS,
    RESETTING_CARTRIDGE,
    STRESS_TESTING,
    ERROR_STATE
} MockDeviceMode;

// Mock TestState enum (matches firmware)
typedef enum {
    NOT_STARTED,
    RUNNING,
    COMPLETED,
    CANCELLED,
    UPLOAD_PENDING,
    UPLOAD_IN_PROGRESS,
    UPLOADED
} MockTestState;

// Mock CartridgeState enum (matches firmware)
typedef enum {
    NOT_INSERTED,
    DETECTED,
    BARCODE_READ,
    VALIDATED,
    INVALID,
    TEST_COMPLETE
} MockCartridgeState;

// Mock state machine structure
typedef struct {
    MockDeviceMode mode;
    MockDeviceMode previous_mode;
    MockTestState test_state;
    MockCartridgeState cartridge_state;
    bool detector_on;
    bool heater_ready;
    bool cloud_operation_pending;
    unsigned long cloud_operation_start_time;
    char last_error[128];
} MockDeviceStateMachine;

extern MockDeviceStateMachine mock_state;

// Test suite runner
void run_state_transitions_test_suite(void);

// === DEVICEMODE TRANSITION TESTS ===

// Valid transitions
bool test_state_init_to_idle(void);
bool test_state_idle_to_heating(void);
bool test_state_heating_to_idle(void);
bool test_state_idle_to_barcode_scanning(void);
bool test_state_barcode_to_validating(void);
bool test_state_validating_to_running_test(void);
bool test_state_running_to_uploading(void);
bool test_state_uploading_to_idle(void);
bool test_state_any_to_error(void);
bool test_state_error_to_idle(void);
bool test_state_idle_to_stress_testing(void);
bool test_state_stress_testing_to_idle(void);
bool test_state_idle_to_validating_magnetometer(void);
bool test_state_validating_mag_to_idle(void);

// Invalid transitions (should be prevented)
bool test_state_invalid_idle_to_uploading(void);
bool test_state_invalid_heating_to_running_test(void);
bool test_state_invalid_barcode_to_uploading(void);
bool test_state_invalid_running_to_barcode(void);
bool test_state_invalid_uploading_to_running(void);

// Cartridge insertion/removal transitions
bool test_state_cartridge_insertion_flow(void);
bool test_state_cartridge_removal_during_idle(void);
bool test_state_cartridge_removal_during_test(void);
bool test_state_cartridge_removal_during_heating(void);

// === TESTSTATE TRANSITION TESTS ===

bool test_teststate_not_started_to_running(void);
bool test_teststate_running_to_completed(void);
bool test_teststate_running_to_cancelled(void);
bool test_teststate_completed_to_upload_pending(void);
bool test_teststate_upload_pending_to_in_progress(void);
bool test_teststate_in_progress_to_uploaded(void);
bool test_teststate_invalid_completed_to_running(void);
bool test_teststate_invalid_uploaded_to_running(void);

// === CARTRIDGESTATE TRANSITION TESTS ===

bool test_cartridge_not_inserted_to_detected(void);
bool test_cartridge_detected_to_barcode_read(void);
bool test_cartridge_barcode_read_to_validated(void);
bool test_cartridge_barcode_read_to_invalid(void);
bool test_cartridge_validated_to_test_complete(void);
bool test_cartridge_test_complete_to_not_inserted(void);
bool test_cartridge_invalid_detected_to_validated(void);
bool test_cartridge_invalid_not_inserted_to_validated(void);

// === COMPOUND STATE TESTS ===

bool test_compound_full_test_workflow(void);
bool test_compound_validation_failure_workflow(void);
bool test_compound_cartridge_removal_recovery(void);
bool test_compound_error_recovery_workflow(void);
bool test_compound_stress_test_workflow(void);

// === CLOUD OPERATION TRACKING ===

bool test_cloud_op_start_tracking(void);
bool test_cloud_op_end_tracking(void);
bool test_cloud_op_timeout_detection(void);
bool test_cloud_op_prevent_duplicate_requests(void);

// === ERROR STATE HANDLING ===

bool test_error_state_transition_from_any_state(void);
bool test_error_state_message_storage(void);
bool test_error_state_recovery(void);
bool test_error_state_clear(void);

// === STATE QUERY FUNCTIONS ===

bool test_query_is_idle(void);
bool test_query_is_testing(void);
bool test_query_is_heating(void);
bool test_query_is_error(void);
bool test_query_has_cartridge(void);
bool test_query_is_cartridge_validated(void);

// === CONCURRENT STATE VALIDATION ===

bool test_concurrent_device_and_test_states(void);
bool test_concurrent_device_and_cartridge_states(void);
bool test_concurrent_all_state_machines(void);

// === STATE PERSISTENCE ===

bool test_state_previous_mode_tracking(void);
bool test_state_transition_logging(void);

// === BOUNDARY AND EDGE CASES ===

bool test_boundary_rapid_state_transitions(void);
bool test_boundary_state_during_power_cycle(void);
bool test_edge_simultaneous_cartridge_events(void);
bool test_edge_timeout_during_cloud_op(void);
bool test_edge_cartridge_removal_multiple_states(void);

// === SAFETY-CRITICAL STATE TESTS ===

bool test_safety_no_test_without_validation(void);
bool test_safety_no_heating_without_cartridge(void);
bool test_safety_emergency_stop_from_any_state(void);
bool test_safety_state_after_hardware_failure(void);

#endif // TEST_STATE_TRANSITIONS_H

