/**
 * @file HardwareConfig.h
 * @brief Hardware pin definitions and constants for Brevitest device
 * @author Agent BETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file contains all hardware pin mappings, timing constants, threshold values,
 * and calibration data for the Brevitest medical device running on Particle Boron (NRF52840).
 *
 * IMPORTANT: All pin numbers and constants must match legacy hardware (V56+) exactly
 * to ensure compatibility with existing Brevitest hardware.
 *
 * Hardware Platform: Particle Boron (NRF52840)
 */

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include "Particle.h"

// ============================================================================
// FIRMWARE VERSION
// ============================================================================
// Note: FIRMWARE_VERSION and DATA_FORMAT_VERSION are defined in DataTypes.h
// to avoid redefinition conflicts

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION 100        // Supabase firmware version (starts at 100)
#endif

#ifndef DATA_FORMAT_VERSION
#define DATA_FORMAT_VERSION 40      // Data format version for test records
#endif

#define LEGACY_HARDWARE_VERSION 56  // Compatible with hardware version 56+

// ============================================================================
// ANALOG PIN DEFINITIONS
// ============================================================================
// Analog pins on Particle Boron (NRF52840)
// ADC Resolution: 12-bit (0-4095), Reference: 3.3V

/**
 * @brief Buzzer output pin (PWM capable)
 * Hardware: Piezo buzzer for audio feedback
 * Connection: Buzzer positive terminal
 */
constexpr hal_pin_t PIN_BUZZER = A0;

/**
 * @brief Spectrophotometer channel A photodetector input
 * Hardware: Analog photodetector for laser A
 * Connection: Photodetector output -> voltage divider -> A2
 */
constexpr uint16_t PIN_PHOTO_A = A2;

/**
 * @brief Spectrophotometer channel B photodetector input
 * Hardware: Analog photodetector for laser B
 * Connection: Photodetector output -> voltage divider -> A3
 */
constexpr uint16_t PIN_PHOTO_B = A3;

/**
 * @brief Spectrophotometer channel C photodetector input
 * Hardware: Analog photodetector for laser C
 * Connection: Photodetector output -> voltage divider -> A4
 */
constexpr uint16_t PIN_PHOTO_C = A4;

/**
 * @brief Heater thermistor analog input
 * Hardware: NTC thermistor for heater temperature measurement
 * Connection: Thermistor in voltage divider circuit -> A6
 * Note: Uses 21-point lookup table for temperature conversion
 */
constexpr hal_pin_t PIN_HEATER_THERMISTOR = A6;

// ============================================================================
// DIGITAL PIN DEFINITIONS
// ============================================================================

/**
 * @brief Cartridge detection switch input
 * Hardware: Mechanical switch activated when cartridge is inserted
 * Connection: Switch -> D3 (active LOW with internal pull-up)
 * Note: Configured with INPUT_PULLUP, triggers interrupt on CHANGE
 */
constexpr hal_pin_t PIN_CARTRIDGE_DETECT = D3;

/**
 * @brief Heater PWM control output
 * Hardware: MOSFET-controlled heater element
 * Connection: D4 -> MOSFET gate -> heater element
 * SAFETY: Max continuous duration 2000ms (HEATER_FAILSAFE_TIMING)
 */
constexpr hal_pin_t PIN_HEATER = D4;

/**
 * @brief Laser A power control (digital on/off)
 * Hardware: Channel A laser diode (red laser)
 * Connection: D5 -> laser driver -> laser diode A
 */
constexpr hal_pin_t PIN_LASER_A = D5;

/**
 * @brief Laser B power control (digital on/off)
 * Hardware: Channel B laser diode (green laser)
 * Connection: D6 -> laser driver -> laser diode B
 */
constexpr hal_pin_t PIN_LASER_B = D6;

/**
 * @brief Laser C power control (digital on/off)
 * Hardware: Channel C laser diode (blue laser)
 * Connection: D7 -> laser driver -> laser diode C
 */
constexpr hal_pin_t PIN_LASER_C = D7;

/**
 * @brief Stepper motor direction control
 * Hardware: A4988 stepper driver DIR pin
 * Connection: D8 -> A4988 DIR
 * HIGH = forward, LOW = reverse
 */
constexpr hal_pin_t PIN_MOTOR_DIR = D8;

/**
 * @brief Stepper motor driver reset (active LOW)
 * Hardware: A4988 stepper driver RESET pin
 * Connection: D11 -> A4988 RESET
 * LOW = reset/disabled, HIGH = enabled
 */
constexpr hal_pin_t PIN_MOTOR_RESET = D11;

/**
 * @brief Stepper motor driver sleep (active LOW)
 * Hardware: A4988 stepper driver SLEEP pin
 * Connection: D12 -> A4988 SLEEP
 * LOW = sleep mode, HIGH = active
 */
constexpr hal_pin_t PIN_MOTOR_SLEEP = D12;

/**
 * @brief Stepper motor step pulse output
 * Hardware: A4988 stepper driver STEP pin
 * Connection: D13 -> A4988 STEP
 * Each rising edge advances motor one microstep
 */
constexpr hal_pin_t PIN_MOTOR_STEP = D13;

/**
 * @brief Barcode scanner ready signal input
 * Hardware: Barcode scanner module ready indicator
 * Connection: Scanner READY -> D22 (active LOW)
 */
constexpr hal_pin_t PIN_BARCODE_READY = D22;

/**
 * @brief Barcode scanner trigger output
 * Hardware: Barcode scanner module trigger
 * Connection: D23 -> Scanner TRIGGER (active LOW)
 */
constexpr hal_pin_t PIN_BARCODE_TRIGGER = D23;

/**
 * @brief Stage limit switch input
 * Hardware: Mechanical limit switch at stage home position
 * Connection: Switch -> D26 (active LOW with internal pull-up)
 */
constexpr hal_pin_t PIN_STAGE_LIMIT = D26;

// ============================================================================
// I2C DEVICE ADDRESSES
// ============================================================================

/**
 * @brief AS7341 spectrophotometer I2C address
 * Hardware: AS7341 11-channel spectral sensor
 * Note: Default address, not changeable
 */
constexpr uint8_t I2C_ADDR_AS7341 = 0x39;

/**
 * @brief PCA9536 I/O expander (spectro mux) I2C address
 * Hardware: PCA9536 4-bit I/O expander for spectrophotometer selection
 * Connection: Controls which spectrophotometer is active
 */
constexpr uint8_t I2C_ADDR_SPECTRO_MUX = 0x41;

// PCA9536 Register Commands
constexpr uint8_t SPECTRO_MUX_CONFIG_CMD = 0x03;      // Configuration register
constexpr uint8_t SPECTRO_MUX_SET_OUTPUTS = 0xF0;    // Set all ports to output
constexpr uint8_t SPECTRO_MUX_INPUT_CMD = 0x00;      // Input register
constexpr uint8_t SPECTRO_MUX_OUTPUT_CMD = 0x01;     // Output register
constexpr uint8_t SPECTRO_MUX_POLARITY_CMD = 0x02;   // Polarity inversion register

// Spectrophotometer channel selection values
constexpr uint8_t SPECTRO_MUX_ALL_OFF = 0x00;        // Turn off all spectrophotometers
constexpr uint8_t SPECTRO_MUX_CHANNEL_A = 0x01;      // Turn on spectrophotometer A
constexpr uint8_t SPECTRO_MUX_CHANNEL_B = 0x02;      // Turn on spectrophotometer B
constexpr uint8_t SPECTRO_MUX_CHANNEL_C = 0x04;      // Turn on spectrophotometer C

// ============================================================================
// I2C CONFIGURATION
// ============================================================================

constexpr uint32_t I2C_CLOCK_SPEED = 400000;         // 400 kHz (Fast mode)
constexpr uint32_t I2C_TIMEOUT_MS = 100;             // I2C operation timeout
constexpr uint32_t I2C_INIT_DELAY_US = 10000;        // Delay after I2C init

// ============================================================================
// BUZZER CONSTANTS
// ============================================================================

constexpr uint16_t BUZZER_FREQUENCY = 600;           // Default frequency (Hz)
constexpr uint16_t BUZZER_DURATION = 1000;           // Default duration (ms)
constexpr uint16_t BUZZER_INSERT_FREQUENCY = 620;    // Cartridge insert notification
constexpr uint16_t BUZZER_INSERT_DURATION = 200;     // Insert notification duration
constexpr uint16_t BUZZER_REMOVE_FREQUENCY = 620;    // Cartridge remove notification
constexpr uint16_t BUZZER_REMOVE_DURATION = 500;     // Remove notification duration
constexpr uint16_t BUZZER_ALERT_FREQUENCY = 850;     // Alert/warning frequency
constexpr uint16_t BUZZER_ALERT_DURATION = 500;      // Alert duration
constexpr uint16_t BUZZER_ALERT_PERIOD = 4000;       // Alert repeat period
constexpr uint16_t BUZZER_PROBLEM_FREQUENCY = 620;   // Problem indication frequency
constexpr uint16_t BUZZER_PROBLEM_DURATION = 100;    // Problem indication duration
constexpr uint16_t BUZZER_PROBLEM_PERIOD = 777;      // Problem repeat period

// ============================================================================
// LED/LASER CONSTANTS
// ============================================================================

constexpr uint8_t LED_DEFAULT_POWER = 115;           // Default LED power (0-255)
constexpr uint16_t LED_WARMUP_DELAY_MS = 1000;       // LED warmup time
constexpr uint16_t LED_DURATION = 500;               // Default LED on duration
constexpr uint8_t LASER_MAX_POWER = 255;             // Maximum laser power
constexpr uint8_t LASER_DEFAULT_POWER = 128;         // Default laser power (50%)
constexpr uint16_t LASER_PWM_FREQUENCY = 500;        // Laser PWM frequency (Hz)
constexpr uint32_t LASER_PWM_ON_US = 20000;          // Laser PWM on time (us)
constexpr uint32_t LASER_PWM_TOTAL_US = 2000;        // Laser PWM cycle time (us)

// ============================================================================
// MOTOR CONSTANTS
// ============================================================================

constexpr uint16_t MOTOR_MICRONS_PER_EIGHTH_STEP = 25;   // Microns per 1/8 step
constexpr uint32_t MOTOR_MOVE_DURATION_UNIT = 25000;     // Movement duration unit (us)
constexpr uint16_t MOTOR_MINIMUM_STEP_DELAY = 250;       // Minimum step delay (us)
constexpr uint16_t MOTOR_RESET_STEP_DELAY = 290;         // Reset movement step delay (us)
constexpr uint16_t MOTOR_BOUNCE_STEP_DELAY = 350;        // Bounce movement step delay (us)
constexpr uint16_t MOTOR_FAST_STEP_DELAY = 290;          // Fast movement step delay (us)
constexpr uint16_t MOTOR_SLOW_STEP_DELAY = 600;          // Slow movement step delay (us)
constexpr uint16_t MOTOR_OSCILLATION_STEP_DELAY = 350;   // Oscillation step delay (us)
constexpr uint16_t MOTOR_SENSOR_STEP_DELAY = 1000;       // Sensor reading step delay (us)

// ============================================================================
// STAGE POSITION CONSTANTS
// ============================================================================

constexpr int32_t STAGE_RESET_STEPS = -60000;            // Steps to move during reset
constexpr int32_t STAGE_POSITION_LIMIT = 45000;          // Maximum stage position (microns)
constexpr int32_t STAGE_MICRONS_TO_INITIAL = 1000;       // Initial position from home
constexpr int32_t STAGE_MICRONS_TO_TEST_START = 7860;    // Position for test start
constexpr int32_t STAGE_SHIPPING_BOLT_LOCATION = 28000;  // Shipping bolt check position
constexpr int32_t STAGE_MICRONS_TO_MAG_START = 12800;    // Magnetometer start position

// ============================================================================
// HEATER CONSTANTS
// ============================================================================

constexpr uint8_t HEATER_MAX_POWER = 255;                // Maximum heater PWM
constexpr uint8_t HEATER_DEFAULT_POWER = 64;             // Default heater power (25%)
constexpr uint16_t HEATER_PWM_FREQUENCY = 150;           // Heater PWM frequency (Hz)
constexpr int16_t HEATER_MAX_TEMPERATURE = 600;          // Max temp (60.0C in 10x format)
constexpr uint16_t HEATER_CONTROL_INTERVAL = 1000;       // PID control interval (ms)
constexpr uint16_t HEATER_PULSE_DURATION = 800;          // Default pulse duration (ms)
constexpr int16_t HEATER_DEFAULT_TEMP_TARGET = 450;      // Default target (45.0C in 10x)
constexpr int16_t HEATER_READY_TEMP_DELTA = 10;          // Ready tolerance (1.0C in 10x)
constexpr uint16_t HEATER_READY_DEBOUNCE_DELAY = 5000;   // Temp stable debounce (ms)
constexpr int16_t HEATER_MAX_RAW_READING = 890;          // Max valid ADC reading
constexpr int16_t HEATER_MIN_RAW_READING = 550;          // Min valid ADC reading
constexpr uint16_t HEATER_FAILSAFE_TIMING = 2000;        // MAX continuous heater ON (ms)
constexpr uint32_t HEATER_STABILIZATION_TIME_US = 5000;  // ADC stabilization time (us)

// ============================================================================
// THERMISTOR LOOKUP TABLE
// ============================================================================
// Temperature values are in 10x format (e.g., 450 = 45.0C)
// Raw values are 12-bit ADC readings (0-4095 range, typical 122-2438)
// Table provides linear interpolation between points

constexpr uint8_t THERMISTOR_TABLE_LENGTH = 21;
constexpr int16_t THERMISTOR_SCALE = 10000;

// Temperature lookup table (10x degrees Celsius)
// Range: 0.0C to 100.0C in 5C increments (descending order for ADC matching)
constexpr int16_t THERMISTOR_TEMP_TABLE[THERMISTOR_TABLE_LENGTH] = {
    1000,   // 100.0C
    950,    // 95.0C
    900,    // 90.0C
    850,    // 85.0C
    800,    // 80.0C
    750,    // 75.0C
    700,    // 70.0C
    650,    // 65.0C
    600,    // 60.0C
    550,    // 55.0C
    500,    // 50.0C
    450,    // 45.0C
    400,    // 40.0C
    350,    // 35.0C
    300,    // 30.0C
    250,    // 25.0C
    200,    // 20.0C
    150,    // 15.0C
    100,    // 10.0C
    50,     // 5.0C
    0       // 0.0C
};

// Raw ADC value lookup table (12-bit ADC, descending with temperature)
// Higher ADC values = higher temperature (NTC thermistor behavior)
constexpr int16_t THERMISTOR_RAW_TABLE[THERMISTOR_TABLE_LENGTH] = {
    2438,   // 100.0C
    2290,   // 95.0C
    2136,   // 90.0C
    1977,   // 85.0C
    1814,   // 80.0C
    1651,   // 75.0C
    1488,   // 70.0C
    1329,   // 65.0C
    1174,   // 60.0C
    1028,   // 55.0C
    890,    // 50.0C
    763,    // 45.0C
    647,    // 40.0C
    544,    // 35.0C
    452,    // 30.0C
    372,    // 25.0C
    304,    // 20.0C
    245,    // 15.0C
    196,    // 10.0C
    155,    // 5.0C
    122     // 0.0C
};

// ============================================================================
// SPECTROPHOTOMETER CONSTANTS
// ============================================================================
// Note: SPECTRO_ASTEP_DEFAULT, SPECTRO_ATIME_DEFAULT, SPECTRO_AGAIN_DEFAULT,
// and SPECTRO_MAX_READINGS are defined as macros in DataTypes.h

constexpr int32_t SPECTRO_WELL_LENGTH = 5000;            // Well length (microns)
constexpr int32_t SPECTRO_STARTING_STAGE_POS = 21000;    // Starting position (microns)
constexpr uint8_t SPECTRO_NUMBER_OF_READINGS = 5;        // Readings per sample
constexpr uint16_t SPECTRO_TIMEOUT = 2000;               // Reading timeout (ms)
constexpr uint8_t SPECTRO_MAX_CYCLES = 255;              // Max reading cycles
constexpr uint8_t SPECTRO_READING_CYCLES = 10;           // Default reading cycles
constexpr uint16_t SPECTRO_RAW_MAX_CYCLES = 300;         // Raw reading max cycles

// ============================================================================
// BARCODE SCANNER CONSTANTS
// ============================================================================

constexpr uint32_t BARCODE_DELAY_US = 100000;            // Barcode scan delay (us)
constexpr uint16_t BARCODE_READ_TIMEOUT = 1000;          // Read timeout (ms)

// Barcode type identifiers
constexpr int8_t BARCODE_TYPE_CARTRIDGE = 1;
constexpr int8_t BARCODE_TYPE_MAGNETOMETER = 2;
constexpr int8_t BARCODE_TYPE_OPTICAL = 3;
constexpr int8_t BARCODE_TYPE_STRESS_TEST = 4;
constexpr int8_t BARCODE_TYPE_SHIPPING = 5;
constexpr int8_t BARCODE_TYPE_GENERAL_ERROR = -1;
constexpr int8_t BARCODE_TYPE_VALIDATION_ERROR = -2;
constexpr int8_t BARCODE_TYPE_OPTICAL_ERROR = -3;

// ============================================================================
// DEBOUNCE AND TIMING CONSTANTS
// ============================================================================

constexpr uint16_t DETECTOR_DEBOUNCE_DELAY = 10;         // Cartridge detect debounce (ms)
constexpr uint16_t ASYNC_COMMAND_DEFAULT_INTERVAL = 5000; // Async command interval (ms)

// ============================================================================
// ADC CONFIGURATION
// ============================================================================

constexpr uint8_t ADC_RESOLUTION_BITS = 12;              // ADC resolution (bits)
constexpr uint16_t ADC_MAX_VALUE = 4095;                 // Max ADC value (2^12 - 1)
constexpr float ADC_REFERENCE_VOLTAGE = 3.3f;            // Reference voltage (V)

// ============================================================================
// DEFAULT SAMPLE AVERAGING
// ============================================================================

constexpr uint8_t DEFAULT_ANALOG_SAMPLES = 8;            // Default samples for averaging
constexpr uint8_t THERMISTOR_SAMPLES = 16;               // Samples for thermistor reading

#endif // HARDWARE_CONFIG_H
