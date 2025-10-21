/*!
 * @file DeviceState.h
 * @brief Device state machine definitions for Brevitest firmware
 * @details Centralized state management to replace scattered boolean flags
 */

#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#include "application.h"

// Primary device operational state
enum class DeviceMode {
    IDLE,                    // Ready for cartridge insertion
    INITIALIZING,            // Device startup
    HEATING,                 // Waiting for heater to reach target
    BARCODE_SCANNING,        // Reading barcode
    VALIDATING_CARTRIDGE,    // Cloud validation in progress
    VALIDATING_MAGNETOMETER, // BLE magnetometer validation
    RUNNING_TEST,            // Test execution (BCODE running)
    UPLOADING_RESULTS,       // Cloud upload in progress
    RESETTING_CARTRIDGE,     // Cloud reset in progress
    STRESS_TESTING,          // Stress test mode
    ERROR_STATE              // Error condition
};

// Sub-states for test execution
enum class TestState {
    NOT_STARTED,
    RUNNING,
    COMPLETED,
    CANCELLED,
    UPLOAD_PENDING,
    UPLOAD_IN_PROGRESS,
    UPLOADED
};

// Cartridge-related states
enum class CartridgeState {
    NOT_INSERTED,
    DETECTED,
    BARCODE_READ,
    VALIDATED,
    INVALID,
    TEST_COMPLETE
};

// Device state machine container
struct DeviceStateMachine {
    DeviceMode mode;
    DeviceMode previous_mode;
    
    // Sub-state machines
    TestState test_state;
    CartridgeState cartridge_state;
    
    // Hardware states
    bool detector_on;
    bool heater_ready;
    
    // Async operation tracking
    bool cloud_operation_pending;
    unsigned long cloud_operation_start_time;
    
    // Error tracking
    String last_error;
    
    // Constructor
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
    }
    
    // State transition validation
    bool can_transition_to(DeviceMode new_mode);
    void transition_to(DeviceMode new_mode);
    void reset_to_idle();
    
    // State query methods
    bool is_idle() const { return mode == DeviceMode::IDLE; }
    bool is_testing() const { return mode == DeviceMode::RUNNING_TEST; }
    bool is_stress_testing() const { return mode == DeviceMode::STRESS_TESTING; }
    bool is_heating() const { return mode == DeviceMode::HEATING; }
    bool is_error() const { return mode == DeviceMode::ERROR_STATE; }
    bool has_cartridge() const { return cartridge_state != CartridgeState::NOT_INSERTED; }
    bool is_cartridge_validated() const { return cartridge_state == CartridgeState::VALIDATED; }
    bool is_cartridge_invalid() const { return cartridge_state == CartridgeState::INVALID; }
    
    // Cloud operation tracking
    void start_cloud_operation();
    void end_cloud_operation();
    bool is_cloud_operation_timeout(unsigned long timeout_ms = 30000) const;
    
    // Error handling
    void set_error(const String& error_msg);
    void clear_error();
};

// Helper functions for state machine
String device_mode_to_string(DeviceMode mode);
String test_state_to_string(TestState state);
String cartridge_state_to_string(CartridgeState state);

#endif // DEVICE_STATE_H
