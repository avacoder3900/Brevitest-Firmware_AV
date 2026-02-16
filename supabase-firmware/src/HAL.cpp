/**
 * @file HAL.cpp
 * @brief Hardware Abstraction Layer implementation for Brevitest device
 * @author Agent BETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * Implementation of all HAL functions for Particle B-Series SoM (NRF52840).
 */

#include "HAL.h"

namespace HAL {

    // ========================================================================
    // PRIVATE STATE VARIABLES
    // ========================================================================

    static bool _initialized = false;
    static bool _motorAwake = false;
    static uint32_t _heaterOnTime = 0;
    static bool _heaterActive = false;

    // Debounce state for switches
    static uint32_t _cartridgeDebounceTime = 0;
    static bool _cartridgeLastState = false;
    static uint32_t _limitDebounceTime = 0;
    static bool _limitLastState = false;

    // ========================================================================
    // PRIVATE HELPER FUNCTIONS
    // ========================================================================

    /**
     * @brief Initialize an analog pin
     */
    static void initAnalogPin(uint16_t pin, PinMode mode, uint8_t value = 0) {
        pinMode(pin, mode);
        if (mode == OUTPUT) {
            analogWrite(pin, value);
        }
    }

    /**
     * @brief Initialize a digital pin
     */
    static void initDigitalPin(hal_pin_t pin, PinMode mode, uint8_t value = LOW) {
        pinMode(pin, mode);
        if (mode == OUTPUT) {
            digitalWrite(pin, value);
        }
    }

    // ========================================================================
    // INITIALIZATION FUNCTIONS
    // ========================================================================

    bool init() {
        if (_initialized) {
            return true;
        }

        // Note: Gen 3 devices (M.2 SoM) have fixed 12-bit ADC resolution
        // analogReadResolution() is not available on these platforms

        // === ANALOG PINS ===
        // Buzzer output (PWM)
        initAnalogPin(PIN_BUZZER, OUTPUT, 0);

        // Spectrophotometer photodetector inputs
        initAnalogPin(PIN_PHOTO_A, INPUT);
        initAnalogPin(PIN_PHOTO_B, INPUT);
        initAnalogPin(PIN_PHOTO_C, INPUT);

        // Thermistor input
        initAnalogPin(PIN_HEATER_THERMISTOR, INPUT);

        // === DIGITAL PINS ===
        // Cartridge detection (external 10k pull-up on board, active LOW)
        initDigitalPin(PIN_CARTRIDGE_DETECT, INPUT);

        // Heater control (output, start OFF for safety)
        initDigitalPin(PIN_HEATER, OUTPUT, LOW);

        // Laser outputs (start OFF)
        initDigitalPin(PIN_LASER_A, OUTPUT, LOW);
        initDigitalPin(PIN_LASER_B, OUTPUT, LOW);
        initDigitalPin(PIN_LASER_C, OUTPUT, LOW);

        // Motor control pins
        initDigitalPin(PIN_MOTOR_DIR, OUTPUT, LOW);
        initDigitalPin(PIN_MOTOR_RESET, OUTPUT, HIGH);   // Active LOW, so HIGH = not reset
        initDigitalPin(PIN_MOTOR_SLEEP, OUTPUT, LOW);    // Active LOW, so LOW = sleeping
        initDigitalPin(PIN_MOTOR_STEP, OUTPUT, LOW);

        // Barcode scanner
        initDigitalPin(PIN_BARCODE_TRIGGER, OUTPUT, HIGH);  // Active LOW, HIGH = not triggered
        initDigitalPin(PIN_BARCODE_READY, INPUT_PULLUP);

        // Stage limit switch (input with pull-up, active LOW)
        initDigitalPin(PIN_STAGE_LIMIT, INPUT_PULLUP);

        // Initialize I2C
        if (!initI2C()) {
            Log.warn("HAL: I2C initialization failed");
            // Continue anyway - device may work without I2C devices
        }

        // Initialize spectrophotometer sensor power switch (PCA9536)
        initSpectroPower();

        // Reset state variables
        _motorAwake = false;
        _heaterOnTime = 0;
        _heaterActive = false;

        _initialized = true;
        Log.info("HAL: Initialization complete");
        return true;
    }

    bool initI2C() {
        if (Wire.isEnabled()) {
            return true;
        }

        Wire.setSpeed(CLOCK_SPEED_400KHZ);
        Wire.begin();
        delayMicroseconds(I2C_INIT_DELAY_US);

        return Wire.isEnabled();
    }

    bool isInitialized() {
        return _initialized;
    }

    // ========================================================================
    // ANALOG READ FUNCTIONS
    // ========================================================================

    uint16_t analogReadAvg(uint16_t pin, uint8_t samples) {
        if (samples == 0) samples = 1;
        if (samples > 64) samples = 64;  // Limit to prevent overflow

        uint32_t sum = 0;
        for (uint8_t i = 0; i < samples; i++) {
            sum += analogRead(pin);
        }
        return (uint16_t)(sum / samples);
    }

    int16_t readThermistorTemperature() {
        uint16_t raw = readThermistorRaw(THERMISTOR_SAMPLES);
        return rawToTemperature(raw);
    }

    uint16_t readThermistorRaw(uint8_t samples) {
        // Allow ADC to stabilize
        delayMicroseconds(HEATER_STABILIZATION_TIME_US);
        return analogReadAvg(PIN_HEATER_THERMISTOR, samples);
    }

    int16_t rawToTemperature(int16_t raw) {
        // Handle out of range - above maximum temperature
        if (raw > THERMISTOR_RAW_TABLE[0]) {
            return THERMISTOR_TEMP_TABLE[0];  // Return max temp (100.0C)
        }

        // Handle out of range - below minimum temperature
        if (raw < THERMISTOR_RAW_TABLE[THERMISTOR_TABLE_LENGTH - 1]) {
            return 0;  // Return 0 (0.0C)
        }

        // Linear interpolation through lookup table
        for (uint8_t i = 0; i < THERMISTOR_TABLE_LENGTH - 1; i++) {
            if (raw <= THERMISTOR_RAW_TABLE[i] && raw > THERMISTOR_RAW_TABLE[i + 1]) {
                // Linear interpolation formula:
                // result = temp[i] + ((raw - raw[i]) * (temp[i+1] - temp[i])) / (raw[i+1] - raw[i])
                int32_t temp1 = THERMISTOR_TEMP_TABLE[i];
                int32_t temp2 = THERMISTOR_TEMP_TABLE[i + 1];
                int32_t raw1 = THERMISTOR_RAW_TABLE[i];
                int32_t raw2 = THERMISTOR_RAW_TABLE[i + 1];

                int32_t result = temp1 + ((raw - raw1) * (temp2 - temp1)) / (raw2 - raw1);
                return (int16_t)result;
            }
        }

        return 0;  // Should not reach here
    }

    bool isThermistorReadingValid(int16_t raw) {
        return (raw >= HEATER_MIN_RAW_READING && raw <= HEATER_MAX_RAW_READING);
    }

    // ========================================================================
    // DIGITAL I/O FUNCTIONS
    // ========================================================================

    void setHeaterPower(uint8_t power) {
        // Clamp power to maximum
        if (power > HEATER_MAX_POWER) {
            power = HEATER_MAX_POWER;
        }

        // Track heater activation time for failsafe
        if (power > 0) {
            if (!_heaterActive) {
                _heaterOnTime = millis();
                _heaterActive = true;
            }
        } else {
            _heaterActive = false;
            _heaterOnTime = 0;
        }

        analogWrite(PIN_HEATER, power);
    }

    void setHeaterOff() {
        analogWrite(PIN_HEATER, 0);
        digitalWrite(PIN_HEATER, LOW);
        _heaterActive = false;
        _heaterOnTime = 0;
    }

    bool isHeaterFailsafe() {
        if (!_heaterActive) {
            return false;
        }
        return (millis() - _heaterOnTime) > HEATER_FAILSAFE_TIMING;
    }

    uint32_t getHeaterOnDuration() {
        if (!_heaterActive) {
            return 0;
        }
        return millis() - _heaterOnTime;
    }

    bool setLaser(char channel, bool on) {
        hal_pin_t pin = getLaserPin(channel);
        if (pin == 0) {
            Log.warn("HAL: Invalid laser channel '%c'", channel);
            return false;
        }
        digitalWrite(pin, on ? HIGH : LOW);
        return true;
    }

    void setAllLasersOn() {
        digitalWrite(PIN_LASER_A, HIGH);
        digitalWrite(PIN_LASER_B, HIGH);
        digitalWrite(PIN_LASER_C, HIGH);
    }

    void setAllLasersOff() {
        digitalWrite(PIN_LASER_A, LOW);
        digitalWrite(PIN_LASER_B, LOW);
        digitalWrite(PIN_LASER_C, LOW);
    }

    bool readCartridgeSwitch() {
        bool currentState = (digitalRead(PIN_CARTRIDGE_DETECT) == LOW);
        uint32_t now = millis();

        // If state changed, reset debounce timer
        if (currentState != _cartridgeLastState) {
            _cartridgeDebounceTime = now;
            _cartridgeLastState = currentState;
        }

        // Return stable state if debounce period has elapsed
        if ((now - _cartridgeDebounceTime) >= DETECTOR_DEBOUNCE_DELAY) {
            return currentState;
        }

        // During debounce, return previous stable state
        return !currentState;  // Return opposite of changing state
    }

    bool readCartridgeSwitchRaw() {
        return (digitalRead(PIN_CARTRIDGE_DETECT) == LOW);
    }

    bool readLimitSwitch() {
        bool currentState = (digitalRead(PIN_STAGE_LIMIT) == LOW);
        uint32_t now = millis();

        if (currentState != _limitLastState) {
            _limitDebounceTime = now;
            _limitLastState = currentState;
        }

        if ((now - _limitDebounceTime) >= DETECTOR_DEBOUNCE_DELAY) {
            return currentState;
        }

        return !currentState;
    }

    bool readLimitSwitchRaw() {
        return (digitalRead(PIN_STAGE_LIMIT) == LOW);
    }

    void motorPulse() {
        // A5985 requires minimum 1us pulse width
        digitalWrite(PIN_MOTOR_STEP, HIGH);
        delayMicroseconds(2);
        digitalWrite(PIN_MOTOR_STEP, LOW);
        delayMicroseconds(2);
    }

    void setMotorDirection(bool forward) {
        digitalWrite(PIN_MOTOR_DIR, forward ? HIGH : LOW);
    }

    void motorWake() {
        digitalWrite(PIN_MOTOR_SLEEP, HIGH);  // Exit sleep mode
        digitalWrite(PIN_MOTOR_RESET, HIGH);  // Ensure not in reset
        delayMicroseconds(10000);             // Allow driver to wake up (legacy: 10ms)
        _motorAwake = true;
    }

    void motorSleep() {
        digitalWrite(PIN_MOTOR_SLEEP, LOW);
        _motorAwake = false;
    }

    bool isMotorAwake() {
        return _motorAwake;
    }

    void triggerBarcodeScanner() {
        digitalWrite(PIN_BARCODE_TRIGGER, LOW);  // Active LOW
    }

    void releaseBarcodeScanner() {
        digitalWrite(PIN_BARCODE_TRIGGER, HIGH);
    }

    bool isBarcodeReady() {
        return (digitalRead(PIN_BARCODE_READY) == LOW);  // Active LOW
    }

    void setBuzzer(uint16_t frequency) {
        if (frequency > 0) {
            tone(PIN_BUZZER, frequency);
        } else {
            noTone(PIN_BUZZER);
        }
    }

    void setBuzzerOff() {
        noTone(PIN_BUZZER);
        analogWrite(PIN_BUZZER, 0);
    }

    void buzzForDuration(uint16_t duration, uint16_t frequency) {
        tone(PIN_BUZZER, frequency, duration);
    }

    // ========================================================================
    // I2C COMMUNICATION FUNCTIONS
    // ========================================================================

    bool i2cWriteRegister(uint8_t address, uint8_t reg, uint8_t value) {
        Wire.beginTransmission(address);
        Wire.write(reg);
        Wire.write(value);
        return (Wire.endTransmission() == 0);
    }

    bool i2cReadRegister(uint8_t address, uint8_t reg, uint8_t* value) {
        Wire.beginTransmission(address);
        Wire.write(reg);
        if (Wire.endTransmission() != 0) {
            return false;
        }

        Wire.requestFrom(address, (uint8_t)1);
        if (Wire.available()) {
            *value = Wire.read();
            return true;
        }
        return false;
    }

    // ========================================================================
    // SPECTROPHOTOMETER SENSOR POWER CONTROL (PCA9536DR)
    // ========================================================================

    bool initSpectroPower() {
        // Configure PCA9536 for output mode on all pins
        if (!i2cWriteRegister(I2C_ADDR_SPECTRO_POWER, SPECTRO_PWR_CONFIG_CMD, SPECTRO_PWR_SET_OUTPUTS)) {
            Log.warn("HAL: Failed to configure spectro power switch");
            return false;
        }

        // Power off all sensors initially
        return spectroAllOff();
    }

    bool selectSpectroChannel(char channel) {
        // Power on one AS7341 sensor at a time via PCA9536 (shared I2C address 0x39)
        uint8_t value;
        switch (channel) {
            case 'A':
            case 'a':
                value = SPECTRO_PWR_SENSOR_A;
                break;
            case 'B':
            case 'b':
                value = SPECTRO_PWR_SENSOR_B;
                break;
            case 'C':
            case 'c':
                value = SPECTRO_PWR_SENSOR_C;
                break;
            case 0:
            case '0':
                value = SPECTRO_PWR_ALL_OFF;
                break;
            default:
                Log.warn("HAL: Invalid spectro channel '%c'", channel);
                return false;
        }

        return i2cWriteRegister(I2C_ADDR_SPECTRO_POWER, SPECTRO_PWR_OUTPUT_CMD, value);
    }

    bool spectroAllOff() {
        return i2cWriteRegister(I2C_ADDR_SPECTRO_POWER, SPECTRO_PWR_OUTPUT_CMD, SPECTRO_PWR_ALL_OFF);
    }

    // ========================================================================
    // UTILITY FUNCTIONS
    // ========================================================================

    hal_pin_t getLaserPin(char channel) {
        switch (channel) {
            case 'A':
            case 'a':
                return PIN_LASER_A;
            case 'B':
            case 'b':
                return PIN_LASER_B;
            case 'C':
            case 'c':
                return PIN_LASER_C;
            default:
                return 0;
        }
    }

    uint32_t elapsedSince(uint32_t startTime) {
        uint32_t now = millis();
        if (now >= startTime) {
            return now - startTime;
        }
        // Handle overflow
        return (0xFFFFFFFF - startTime) + now + 1;
    }

} // namespace HAL
