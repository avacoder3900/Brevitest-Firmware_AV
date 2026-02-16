/**
 * @file HardwareConfig.h
 * @brief Hardware pin definitions and constants for Brevitest device
 * @date February 2026
 *
 * All hardware pin mappings, timing constants, threshold values, and calibration
 * data for the Brevitest Acuity GEN2 medical device.
 *
 * IMPORTANT: All pin numbers verified against Eagle .brd board files:
 *   - Acuity GEN2 Main Board R5 (main MCU, motor, lasers, buzzer, barcode)
 *   - Acuity GEN2 Stage Board R5 (3x AS7341 sensors, heater, thermistor, limit switch)
 *
 * Hardware Platform: Particle B-Series SoM (NRF52840) via TE 21992304 M.2 connector
 * Motor Driver:      Allegro A5985GETTR-T Dual Full Bridge (ENABLE tied to GND)
 * Laser Driver:      IS32LT3143-ZLA3-TR 3-channel linear LED driver
 * Laser Diodes:      3x SLD3232VF (TO-18 with built-in photodiodes)
 * Buzzer:            BSS138P N-channel MOSFET, 12V supply, 1k current limiter
 * Heater:            IRLML6344TRPBF N-channel MOSFET on stage board, 5V supply
 * Spectro Sensors:   3x AS7341-DLGM, power-switched via PCA9536DR (1.8V)
 * Thermistor:        NTC, divider: 3.3V -> Thermistor -> 1K (R2) -> GND
 * Cartridge Switch:  ESE-24SV3, external 10k pull-up to 3.3V
 * RGB LED:           External, common anode 3.3V, 150 ohm resistors
 * Inter-board:       12-pin TE 1-104074-1 (I2C, heater, thermistor, limit, power)
 *
 * User Stories Implemented:
 *   - BETA-001: Board-verified pin definitions and hardware documentation
 */

#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include "Particle.h"

// ============================================================================
// ANALOG PIN DEFINITIONS
// ============================================================================
// Analog pins on Particle B-Series SoM (NRF52840)
// ADC Resolution: 12-bit (0-4095), Reference: 3.3V

/**
 * @brief Buzzer output pin (PWM capable)
 * Hardware: BSS138P N-channel MOSFET drives piezo buzzer from 12V supply
 * Connection: A0 -> 1k resistor -> BSS138P gate -> buzzer (12V)
 */
constexpr hal_pin_t PIN_BUZZER = A0;

/**
 * @brief Spectrophotometer channel A photodetector input
 * Hardware: SLD3232VF built-in photodiode with 3k pull-down resistor
 * Connection: Photodiode anode -> A2 (3k to GND)
 */
constexpr uint16_t PIN_PHOTO_A = A2;

/**
 * @brief Spectrophotometer channel B photodetector input
 * Hardware: SLD3232VF built-in photodiode with 3k pull-down resistor
 * Connection: Photodiode anode -> A3 (3k to GND)
 */
constexpr uint16_t PIN_PHOTO_B = A3;

/**
 * @brief Spectrophotometer channel C photodetector input
 * Hardware: SLD3232VF built-in photodiode with 3k pull-down resistor
 * Connection: Photodiode anode -> A4 (3k to GND)
 */
constexpr uint16_t PIN_PHOTO_C = A4;

/**
 * @brief Heater thermistor analog input
 * Hardware: NTC thermistor on stage board
 * Connection: 3.3V -> NTC Thermistor -> A6 -> R2 (1K) -> GND, 2.2uF filter cap
 * Note: Uses 21-point lookup table for temperature conversion
 */
constexpr hal_pin_t PIN_HEATER_THERMISTOR = A6;

// ============================================================================
// DIGITAL PIN DEFINITIONS
// ============================================================================

/**
 * @brief Cartridge detection switch input
 * Hardware: ESE-24SV3 mechanical switch with external 10k pull-up to 3.3V
 * Connection: Switch -> D3 (active LOW)
 * Note: Use INPUT mode (NOT INPUT_PULLUP) - external pull-up on board
 */
constexpr hal_pin_t PIN_CARTRIDGE_DETECT = D3;

/**
 * @brief Heater PWM control output
 * Hardware: IRLML6344TRPBF N-channel MOSFET on stage board, 5V supply
 * Connection: D4 -> inter-board connector -> MOSFET gate -> heater (5V)
 * SAFETY: Max continuous duration 2000ms (HEATER_FAILSAFE_TIMING)
 */
constexpr hal_pin_t PIN_HEATER = D4;

/**
 * @brief Laser A enable (digital on/off)
 * Hardware: IS32LT3143-ZLA3-TR channel 1, SLD3232VF diode (R_EXT=8.16k)
 * Connection: D5 -> IS32LT3143 EN1 -> SLD3232VF laser A
 */
constexpr hal_pin_t PIN_LASER_A = D5;

/**
 * @brief Laser B enable (digital on/off)
 * Hardware: IS32LT3143-ZLA3-TR channel 2, SLD3232VF diode (R_EXT=8.16k)
 * Connection: D6 -> IS32LT3143 EN2 -> SLD3232VF laser B
 */
constexpr hal_pin_t PIN_LASER_B = D6;

/**
 * @brief Laser C enable (digital on/off)
 * Hardware: IS32LT3143-ZLA3-TR channel 3, SLD3232VF diode (R_EXT=8.16k)
 * Connection: D7 -> IS32LT3143 EN3 -> SLD3232VF laser C
 */
constexpr hal_pin_t PIN_LASER_C = D7;

/**
 * @brief Stepper motor direction control
 * Hardware: A5985GETTR-T dual full bridge DIR pin
 * Connection: D8 -> A5985 DIR
 * HIGH = forward, LOW = reverse
 */
constexpr hal_pin_t PIN_MOTOR_DIR = D8;

/**
 * @brief Stepper motor driver reset (active LOW)
 * Hardware: A5985GETTR-T dual full bridge RESET pin
 * Connection: D11 -> A5985 RESET
 * LOW = reset/disabled, HIGH = enabled
 */
constexpr hal_pin_t PIN_MOTOR_RESET = D11;

/**
 * @brief Stepper motor driver sleep (active LOW)
 * Hardware: A5985GETTR-T dual full bridge SLEEP pin
 * Connection: D12 -> A5985 SLEEP
 * LOW = sleep mode, HIGH = active
 * Note: ENABLE pin is hardware-tied to GND (always enabled). Control via SLEEP/RESET only.
 * Note: MS1/MS2/MS3 are hardware-tied HIGH (1/8 microstepping fixed).
 */
constexpr hal_pin_t PIN_MOTOR_SLEEP = D12;

/**
 * @brief Stepper motor step pulse output
 * Hardware: A5985GETTR-T dual full bridge STEP pin
 * Connection: D13 -> A5985 STEP
 * Each rising edge advances motor one microstep (1/8 step, 25 microns)
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
 * Hardware: SDS004R limit switch on stage board
 * Connection: Switch -> D26 via inter-board connector (active LOW)
 */
constexpr hal_pin_t PIN_STAGE_LIMIT = D26;

// ============================================================================
// I2C DEVICE ADDRESSES
// ============================================================================

/**
 * @brief AS7341-DLGM spectrophotometer I2C address
 * Hardware: 3x AS7341-DLGM 11-channel spectral sensors on stage board
 * Note: All 3 sensors share address 0x39 - only one powered at a time via PCA9536
 */
constexpr uint8_t I2C_ADDR_AS7341 = 0x39;

/**
 * @brief PCA9536DR I/O expander (sensor power switch) I2C address
 * Hardware: PCA9536DR 4-bit I/O expander on stage board (powered at 1.8V via MIC5365 LDO)
 * Function: Controls VDD power to individual AS7341 sensors (NOT signal mux)
 * IO0 = Sensor A power, IO1 = Sensor B power, IO2 = Sensor C power
 * Only one sensor may be powered at a time (shared I2C address 0x39)
 */
constexpr uint8_t I2C_ADDR_SPECTRO_POWER = 0x41;

// PCA9536 Register Commands
constexpr uint8_t SPECTRO_PWR_CONFIG_CMD = 0x03;       // Configuration register
constexpr uint8_t SPECTRO_PWR_SET_OUTPUTS = 0xF0;     // Set IO0-IO3 as output (lower nibble)
constexpr uint8_t SPECTRO_PWR_INPUT_CMD = 0x00;       // Input register
constexpr uint8_t SPECTRO_PWR_OUTPUT_CMD = 0x01;      // Output register
constexpr uint8_t SPECTRO_PWR_POLARITY_CMD = 0x02;    // Polarity inversion register

// Spectrophotometer sensor power control values
constexpr uint8_t SPECTRO_PWR_ALL_OFF = 0x00;         // Power off all sensors
constexpr uint8_t SPECTRO_PWR_SENSOR_A = 0x01;        // Power on sensor A (IO0)
constexpr uint8_t SPECTRO_PWR_SENSOR_B = 0x02;        // Power on sensor B (IO1)
constexpr uint8_t SPECTRO_PWR_SENSOR_C = 0x04;        // Power on sensor C (IO2)

// ============================================================================
// I2C CONFIGURATION
// ============================================================================

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

constexpr uint16_t LED_DURATION = 500;               // Default LED on duration
constexpr uint8_t LASER_DEFAULT_POWER = 128;         // Default laser power (50%)

// ============================================================================
// MOTOR CONSTANTS
// ============================================================================

constexpr uint16_t MOTOR_MICRONS_PER_EIGHTH_STEP = 25;   // Microns per 1/8 step
constexpr uint16_t MOTOR_MINIMUM_STEP_DELAY = 250;       // Minimum step delay (us)
constexpr uint16_t MOTOR_RESET_STEP_DELAY = 290;         // Reset movement step delay (us)
constexpr uint16_t MOTOR_FAST_STEP_DELAY = 290;          // Fast movement step delay (us)
constexpr uint16_t MOTOR_SLOW_STEP_DELAY = 600;          // Slow movement step delay (us)
constexpr uint16_t MOTOR_OSCILLATION_STEP_DELAY = 350;   // Oscillation step delay (us)
constexpr uint16_t MOTOR_SENSOR_STEP_DELAY = 1000;       // Sensor reading step delay (us)

// ============================================================================
// STAGE POSITION CONSTANTS
// ============================================================================

constexpr int32_t STAGE_RESET_STEPS = -60000;            // Steps to move during reset
constexpr int32_t STAGE_MICRONS_TO_TEST_START = 7860;    // Position for test start

// ============================================================================
// HEATER CONSTANTS
// ============================================================================

constexpr uint8_t HEATER_MAX_POWER = 255;                // Maximum heater PWM
constexpr uint8_t HEATER_DEFAULT_POWER = 64;             // Default heater power (25%)
constexpr int16_t HEATER_MAX_TEMPERATURE = 600;          // Max temp (60.0C in 10x format)
constexpr uint16_t HEATER_CONTROL_INTERVAL = 1000;       // PID control interval (ms)
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
constexpr uint16_t SPECTRO_TIMEOUT = 2000;               // Reading timeout (ms)

// ============================================================================
// BARCODE SCANNER CONSTANTS
// ============================================================================

constexpr uint32_t BARCODE_DELAY_US = 100000;            // Barcode scan delay (us)
constexpr uint16_t BARCODE_READ_TIMEOUT = 1000;          // Read timeout (ms)

// ============================================================================
// DEBOUNCE AND TIMING CONSTANTS
// ============================================================================

constexpr uint16_t DETECTOR_DEBOUNCE_DELAY = 10;         // Cartridge detect debounce (ms)

// ============================================================================
// DEFAULT SAMPLE AVERAGING
// ============================================================================

constexpr uint8_t DEFAULT_ANALOG_SAMPLES = 8;            // Default samples for averaging
constexpr uint8_t THERMISTOR_SAMPLES = 16;               // Samples for thermistor reading

#endif // HARDWARE_CONFIG_H
