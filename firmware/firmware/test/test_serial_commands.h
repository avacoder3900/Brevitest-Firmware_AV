/*!
 * @file test_serial_commands.h
 * @brief Unit tests for serial command processing
 * @details Tests all 80+ serial commands with full traceability
 * 
 * Test Coverage:
 * - Low Level Commands (1-9)
 * - Serial Port Messaging (10-12)
 * - Stage Motion (20-29)
 * - Laser Diodes (30-33)
 * - Buzzer (40-43)
 * - Heater (50-55)
 * - Barcode Scanner (60)
 * - Magnetometer (70-73)
 * - Stress Test (90-93)
 * - Bluetooth LE (200)
 * - Spectrophotometer (301-311)
 * - Cloud Functions (400-405)
 * - Communication (8000-8999)
 * 
 * Compliance: ISO 13485:2016 Section 7.3.5
 * Risk Level: CRITICAL (commands control medical device functions)
 */

#ifndef TEST_SERIAL_COMMANDS_H
#define TEST_SERIAL_COMMANDS_H

#include "test_framework.h"
#include <stdbool.h>

// Mock hardware state for testing
typedef struct {
    int eeprom_data;
    int digital_pin_state[50];
    int analog_pin_value[20];
    int stage_position;
    int heater_temperature;
    bool motor_awake;
    bool heater_on;
    bool laser_on[3];
    bool buzzer_on;
    int test_result;
} MockHardwareState;

extern MockHardwareState mock_hw;

// Test suite runner
void run_serial_commands_test_suite(void);

// === LOW LEVEL COMMANDS (1-9) ===
bool test_cmd_001_system_reset(void);
bool test_cmd_002_reset_eeprom(void);
bool test_cmd_003_check_cache(void);
bool test_cmd_004_check_digital_pin(void);
bool test_cmd_005_limit_switch_state(void);
bool test_cmd_006_set_digital_pin(void);
bool test_cmd_007_toggle_pin(void);
bool test_cmd_008_check_analog_pin(void);
bool test_cmd_009_clear_wifi_credentials(void);

// === SERIAL PORT MESSAGING (10-12) ===
bool test_cmd_010_enable_serial_messaging(void);
bool test_cmd_011_disable_serial_messaging(void);
bool test_cmd_012_display_device_id(void);

// === STAGE MOTION (20-29) ===
bool test_cmd_020_reset_stage(void);
bool test_cmd_021_wake_motor(void);
bool test_cmd_022_move_stage_microns(void);
bool test_cmd_022_move_stage_boundary_test(void);
bool test_cmd_023_move_to_position(void);
bool test_cmd_024_move_to_test_start(void);
bool test_cmd_025_move_to_optical_read(void);
bool test_cmd_026_oscillate_stage(void);
bool test_cmd_027_move_to_shipping_bolt(void);
bool test_cmd_028_move_to_zero(void);
bool test_cmd_029_sleep_motor(void);

// === LASER DIODES (30-33) ===
bool test_cmd_030_laser_a_on(void);
bool test_cmd_031_laser_b_on(void);
bool test_cmd_032_laser_c_on(void);
bool test_cmd_033_all_lasers_on(void);
bool test_cmd_03x_laser_safety_boundaries(void);

// === BUZZER (40-43) ===
bool test_cmd_040_buzzer_on(void);
bool test_cmd_041_buzzer_alert(void);
bool test_cmd_042_buzzer_problem(void);
bool test_cmd_043_buzzer_off(void);

// === HEATER (50-55) ===
bool test_cmd_050_read_temperature(void);
bool test_cmd_051_heater_on(void);
bool test_cmd_052_heater_off(void);
bool test_cmd_053_set_target_temperature(void);
bool test_cmd_053_temperature_boundary_test(void);
bool test_cmd_054_start_temp_control(void);
bool test_cmd_055_stop_temp_control(void);

// === BARCODE SCANNER (60) ===
bool test_cmd_060_scan_barcode(void);
bool test_cmd_060_barcode_validation(void);

// === MAGNETOMETER (70-73) ===
bool test_cmd_070_validate_magnets(void);
bool test_cmd_071_list_validation_files(void);
bool test_cmd_072_clear_validation_files(void);
bool test_cmd_073_load_latest_validation(void);

// === STRESS TEST (90-93) ===
bool test_cmd_090_reset_and_start_stress_test(void);
bool test_cmd_091_start_stress_test_wifi_off(void);
bool test_cmd_092_start_stress_test(void);
bool test_cmd_093_stop_stress_test(void);

// === BLUETOOTH LE (200) ===
bool test_cmd_200_ble_scan(void);

// === SPECTROPHOTOMETER (301-311) ===
bool test_cmd_301_set_spectro_params(void);
bool test_cmd_301_spectro_param_validation(void);
bool test_cmd_303_power_on_channel(void);
bool test_cmd_304_power_off_all_spectro(void);
bool test_cmd_305_baseline_scan(void);
bool test_cmd_306_test_scan(void);
bool test_cmd_307_set_pulse_params(void);
bool test_cmd_308_take_readings(void);
bool test_cmd_309_output_raw_readings(void);
bool test_cmd_310_take_readings_no_lasers(void);
bool test_cmd_311_baseline_continuous_scan(void);

// === CLOUD FUNCTIONS (400-405) ===
bool test_cmd_400_check_cache(void);
bool test_cmd_401_clear_cache(void);
bool test_cmd_402_load_cached_record(void);
bool test_cmd_403_upload_test(void);
bool test_cmd_404_check_assay_files(void);
bool test_cmd_405_clear_assay_files(void);

// === COMMUNICATION COMMANDS (8000-8999) ===
bool test_cmd_8000_display_help(void);
bool test_cmd_8001_display_status(void);
bool test_cmd_8100_wifi_off(void);
bool test_cmd_8101_wifi_on(void);
bool test_cmd_8200_cellular_off(void);
bool test_cmd_8201_cellular_on(void);
bool test_cmd_8300_bluetooth_off(void);
bool test_cmd_8301_bluetooth_on(void);
bool test_cmd_8900_all_radios_off(void);
bool test_cmd_8901_all_radios_on(void);
bool test_cmd_8500_test_wifi_only(void);
bool test_cmd_8501_test_cellular_only(void);
bool test_cmd_8502_test_bluetooth_only(void);
bool test_cmd_8503_test_fcc_compliance(void);
bool test_cmd_8999_emission_check(void);

// === INTEGRATION TESTS ===
bool test_cmd_integration_heater_sequence(void);
bool test_cmd_integration_spectro_workflow(void);
bool test_cmd_integration_stage_motion_sequence(void);

// === SAFETY AND BOUNDARY TESTS ===
bool test_cmd_safety_temperature_limits(void);
bool test_cmd_safety_stage_limits(void);
bool test_cmd_safety_concurrent_operations(void);
bool test_cmd_boundary_invalid_commands(void);
bool test_cmd_boundary_parameter_overflow(void);

#endif // TEST_SERIAL_COMMANDS_H

