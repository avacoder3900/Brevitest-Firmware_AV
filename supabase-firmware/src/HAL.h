/**
 * @file HAL.h
 * @brief Hardware Abstraction Layer interface for Brevitest device
 * @author Agent BETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file provides the hardware abstraction interface between firmware logic
 * and physical hardware on the Particle B-Series SoM (NRF52840) platform.
 *
 * Features:
 * - GPIO initialization and configuration
 * - Analog read with averaging and noise filtering
 * - Digital I/O with debouncing
 * - Thermistor temperature conversion
 * - I2C communication helpers
 * - Safety features for heater control
 *
 * Usage:
 *   HAL::init();  // Call once in setup()
 *   int temp = HAL::readThermistorTemperature();  // Get temperature in 10x format
 */

#ifndef HAL_H
#define HAL_H

#include "Particle.h"
#include "HardwareConfig.h"

/**
 * @namespace HAL
 * @brief Hardware Abstraction Layer namespace
 *
 * Contains all low-level hardware interface functions. All functions are
 * designed to be stateless where possible, with safety features built in.
 */
namespace HAL {

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    /**
     * @brief Initialize all GPIO pins and hardware interfaces
     *
     * Configures all pins with correct modes and safe default states:
     * - All output pins set to LOW/safe state
     * - Input pins configured with appropriate pull-ups
     * - I2C bus initialized at 400kHz
     * - ADC resolution set to 12-bit
     *
     * @return true if initialization successful, false otherwise
     * @note Call this once in setup() before using any other HAL functions
     */
    bool init();

    /**
     * @brief Initialize I2C bus
     *
     * Configures I2C for 400kHz operation and verifies bus is enabled.
     *
     * @return true if I2C initialized successfully
     */
    bool initI2C();

    /**
     * @brief Check if HAL has been initialized
     * @return true if init() has been called successfully
     */
    bool isInitialized();

    // ========================================================================
    // ANALOG READ FUNCTIONS
    // ========================================================================

    /**
     * @brief Read analog value with optional averaging
     *
     * Reads an analog pin with configurable sample averaging for noise reduction.
     *
     * @param pin Analog pin to read
     * @param samples Number of samples to average (default: DEFAULT_ANALOG_SAMPLES)
     * @return Averaged ADC value (0-4095 for 12-bit resolution)
     */
    uint16_t analogReadAvg(uint16_t pin, uint8_t samples = DEFAULT_ANALOG_SAMPLES);

    /**
     * @brief Read thermistor and convert to temperature
     *
     * Reads the heater thermistor and converts the raw ADC value to temperature
     * using the 21-point lookup table with linear interpolation.
     *
     * @return Temperature in 10x Celsius (e.g., 450 = 45.0C)
     * @note Returns 0 if reading is out of range (below table minimum)
     * @note Returns 1000 (100.0C) if reading exceeds table maximum
     */
    int16_t readThermistorTemperature();

    /**
     * @brief Read raw thermistor ADC value
     *
     * @param samples Number of samples to average
     * @return Raw ADC value (0-4095)
     */
    uint16_t readThermistorRaw(uint8_t samples = THERMISTOR_SAMPLES);

    /**
     * @brief Convert raw ADC value to temperature using lookup table
     *
     * Performs linear interpolation between lookup table points.
     *
     * @param raw Raw ADC value
     * @return Temperature in 10x Celsius
     */
    int16_t rawToTemperature(int16_t raw);

    /**
     * @brief Read spectrophotometer channel
     *
     * Reads the analog photodetector for a spectrophotometer channel.
     *
     * @param channel Channel identifier ('A', 'B', or 'C')
     * @param samples Number of samples to average (default: DEFAULT_ANALOG_SAMPLES)
     * @return ADC value (0-4095), or 0 if invalid channel
     */
    uint16_t readSpectroChannel(char channel, uint8_t samples = DEFAULT_ANALOG_SAMPLES);

    /**
     * @brief Check if thermistor reading is within valid range
     *
     * @param raw Raw ADC value to check
     * @return true if within HEATER_MIN_RAW_READING to HEATER_MAX_RAW_READING
     */
    bool isThermistorReadingValid(int16_t raw);

    // ========================================================================
    // DIGITAL I/O FUNCTIONS
    // ========================================================================

    /**
     * @brief Set heater power level with safety limits
     *
     * Sets the heater PWM duty cycle with built-in safety features:
     * - Power clamped to HEATER_MAX_POWER
     * - Tracks last activation time for failsafe
     * - Use setHeaterOff() to disable
     *
     * @param power PWM value (0-255)
     * @warning Heater should not run continuously > HEATER_FAILSAFE_TIMING (2000ms)
     */
    void setHeaterPower(uint8_t power);

    /**
     * @brief Turn heater off immediately
     *
     * Disables heater output and resets safety tracking.
     */
    void setHeaterOff();

    /**
     * @brief Check if heater has exceeded failsafe timing
     *
     * @return true if heater has been on continuously > HEATER_FAILSAFE_TIMING
     */
    bool isHeaterFailsafe();

    /**
     * @brief Get time since heater was last activated
     *
     * @return Milliseconds since heater was turned on, or 0 if off
     */
    uint32_t getHeaterOnDuration();

    /**
     * @brief Set laser power for a channel
     *
     * Controls laser diode on/off state. Lasers are digital control only.
     *
     * @param channel Laser channel ('A', 'B', or 'C')
     * @param on true to turn on, false to turn off
     * @return true if channel is valid, false otherwise
     */
    bool setLaser(char channel, bool on);

    /**
     * @brief Turn on all lasers
     */
    void setAllLasersOn();

    /**
     * @brief Turn off all lasers
     */
    void setAllLasersOff();

    /**
     * @brief Read cartridge detection switch with debouncing
     *
     * Reads the cartridge presence switch. The switch is active LOW
     * (cartridge present = LOW).
     *
     * @return true if cartridge is detected (inserted)
     */
    bool readCartridgeSwitch();

    /**
     * @brief Read raw cartridge switch state (no debounce)
     *
     * @return true if switch is active (LOW)
     */
    bool readCartridgeSwitchRaw();

    /**
     * @brief Read stage limit switch with debouncing
     *
     * Reads the stage home position limit switch. Active LOW.
     *
     * @return true if limit switch is triggered (at home)
     */
    bool readLimitSwitch();

    /**
     * @brief Read raw limit switch state (no debounce)
     *
     * @return true if switch is active (LOW)
     */
    bool readLimitSwitchRaw();

    /**
     * @brief Generate a single motor step pulse
     *
     * Generates a step pulse on the motor STEP pin. Pulse width is
     * sufficient for A5985GETTR-T driver requirements.
     */
    void motorPulse();

    /**
     * @brief Set motor direction
     *
     * @param forward true for forward direction, false for reverse
     */
    void setMotorDirection(bool forward);

    /**
     * @brief Wake up motor driver
     *
     * Sets SLEEP and RESET pins to enable motor driver.
     */
    void motorWake();

    /**
     * @brief Put motor driver to sleep
     *
     * Sets SLEEP pin to put driver in low-power mode.
     */
    void motorSleep();

    /**
     * @brief Check if motor driver is awake
     *
     * @return true if motor driver is active
     */
    bool isMotorAwake();

    /**
     * @brief Trigger barcode scanner
     *
     * Activates the barcode scanner trigger (active LOW).
     */
    void triggerBarcodeScanner();

    /**
     * @brief Release barcode scanner trigger
     */
    void releaseBarcodeScanner();

    /**
     * @brief Check if barcode scanner is ready
     *
     * @return true if scanner ready signal is active
     */
    bool isBarcodeReady();

    /**
     * @brief Activate buzzer at specified frequency
     *
     * @param frequency Frequency in Hz
     */
    void setBuzzer(uint16_t frequency);

    /**
     * @brief Turn off buzzer
     */
    void setBuzzerOff();

    /**
     * @brief Play buzzer tone for specified duration
     *
     * Non-blocking: sets up buzzer and returns immediately.
     * Use with timer for duration control.
     *
     * @param duration Duration in milliseconds
     * @param frequency Frequency in Hz
     */
    void buzzForDuration(uint16_t duration, uint16_t frequency);

    // ========================================================================
    // I2C COMMUNICATION FUNCTIONS
    // ========================================================================

    /**
     * @brief Write a single byte to an I2C register
     *
     * @param address I2C device address (7-bit)
     * @param reg Register address
     * @param value Byte value to write
     * @return true if write successful
     */
    bool i2cWriteRegister(uint8_t address, uint8_t reg, uint8_t value);

    /**
     * @brief Write multiple bytes to an I2C device
     *
     * @param address I2C device address (7-bit)
     * @param data Pointer to data buffer
     * @param length Number of bytes to write
     * @return true if write successful
     */
    bool i2cWrite(uint8_t address, const uint8_t* data, uint8_t length);

    /**
     * @brief Read a single byte from an I2C register
     *
     * @param address I2C device address (7-bit)
     * @param reg Register address
     * @param value Pointer to store read value
     * @return true if read successful
     */
    bool i2cReadRegister(uint8_t address, uint8_t reg, uint8_t* value);

    /**
     * @brief Read multiple bytes from an I2C device
     *
     * @param address I2C device address (7-bit)
     * @param reg Starting register address
     * @param buffer Pointer to buffer for read data
     * @param length Number of bytes to read
     * @return Number of bytes actually read
     */
    uint8_t i2cReadBytes(uint8_t address, uint8_t reg, uint8_t* buffer, uint8_t length);

    /**
     * @brief Scan I2C bus for devices
     *
     * Scans all 7-bit addresses (1-127) and logs found devices.
     * Useful for debugging I2C connectivity issues.
     *
     * @return Number of devices found
     */
    uint8_t i2cScan();

    /**
     * @brief Check if an I2C device is present
     *
     * @param address I2C device address (7-bit)
     * @return true if device acknowledges
     */
    bool i2cDevicePresent(uint8_t address);

    // ========================================================================
    // SPECTROPHOTOMETER SENSOR POWER CONTROL (PCA9536DR)
    // ========================================================================

    /**
     * @brief Initialize the spectrophotometer power switch (PCA9536DR)
     *
     * Configures the PCA9536 I/O expander for output mode to control
     * individual AS7341 sensor VDD power on the stage board.
     *
     * @return true if initialization successful
     */
    bool initSpectroPower();

    /**
     * @brief Select spectrophotometer channel
     *
     * @param channel Channel to select ('A', 'B', 'C') or 0 for all off
     * @return true if selection successful
     */
    bool selectSpectroChannel(char channel);

    /**
     * @brief Turn off all spectrophotometer channels
     *
     * @return true if successful
     */
    bool spectroAllOff();

    // ========================================================================
    // UTILITY FUNCTIONS
    // ========================================================================

    /**
     * @brief Get pin number for spectrophotometer channel
     *
     * @param channel Channel identifier ('A', 'B', or 'C')
     * @return Pin number, or 0 if invalid channel
     */
    uint16_t getSpectroPin(char channel);

    /**
     * @brief Get laser pin for channel
     *
     * @param channel Channel identifier ('A', 'B', or 'C')
     * @return Pin number, or 0 if invalid channel
     */
    hal_pin_t getLaserPin(char channel);

    /**
     * @brief Delay in microseconds with yield
     *
     * Uses delayMicroseconds() but yields to system for longer delays.
     *
     * @param us Microseconds to delay
     */
    void delayUs(uint32_t us);

    /**
     * @brief Get elapsed time since a timestamp
     *
     * Handles millis() overflow correctly.
     *
     * @param startTime Starting timestamp from millis()
     * @return Elapsed milliseconds
     */
    uint32_t elapsedSince(uint32_t startTime);

} // namespace HAL

#endif // HAL_H
