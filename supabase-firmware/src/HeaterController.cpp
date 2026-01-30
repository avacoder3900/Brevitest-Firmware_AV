/**
 * @file HeaterController.cpp
 * @brief PID Temperature Controller implementation for Brevitest heating element
 * @author Agent THETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * Implementation of the HeaterController class providing PID temperature
 * control with safety features for the Brevitest heating element.
 *
 * SAFETY CRITICAL CODE:
 * This module controls a heating element. All safety mechanisms are
 * designed to fail-safe (heater OFF) in case of any error.
 */

#include "HeaterController.h"

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

HeaterController::HeaterController()
    : _initialized(false)
    , _enabled(false)
    , _state(HeaterState::DISABLED)
    , _lastError(HeaterError::NONE)
    , _targetTemp(HEATER_DEFAULT_TEMP_TARGET)
    , _currentTemp(0)
    , _maxTemp(HEATER_MAX_TEMPERATURE)
    , _lastRawReading(0)
    , _kpNum(DEFAULT_KP_NUM)
    , _kpDen(DEFAULT_KP_DEN)
    , _kiNum(DEFAULT_KI_NUM)
    , _kiDen(DEFAULT_KI_DEN)
    , _kdNum(DEFAULT_KD_NUM)
    , _kdDen(DEFAULT_KD_DEN)
    , _integral(0)
    , _previousError(0)
    , _lastReadTime(0)
    , _currentPower(0)
    , _defaultPower(HEATER_DEFAULT_POWER)
    , _stable(false)
    , _stableStartTime(0)
    , _readyCallbackCalled(false)
    , _readyCallback(nullptr)
    , _lastTempForRunaway(0)
    , _runawayCheckTime(0)
    , _runawayCount(0)
    , _lastPowerApplyTime(0)
    , _continuousPowerTime(0)
    , _verboseLogging(false)
{
}

HeaterController::~HeaterController()
{
    // Ensure heater is OFF when controller is destroyed
    if (_initialized) {
        HAL::setHeaterOff();
    }
}

// ============================================================================
// INITIALIZATION (HEAT-001)
// ============================================================================

bool HeaterController::init()
{
    if (_initialized) {
        return true;
    }

    // Ensure HAL is initialized
    if (!HAL::isInitialized()) {
        Log.error("HeaterController: HAL not initialized");
        return false;
    }

    // Ensure heater is off initially
    HAL::setHeaterOff();

    // Reset all state
    resetPID();
    _state = HeaterState::DISABLED;
    _lastError = HeaterError::NONE;
    _currentTemp = 0;
    _stable = false;
    _stableStartTime = 0;
    _readyCallbackCalled = false;
    _lastTempForRunaway = 0;
    _runawayCheckTime = 0;
    _runawayCount = 0;
    _continuousPowerTime = 0;

    _initialized = true;
    Log.info("HeaterController: Initialized (target=%d.%dC, max=%d.%dC)",
             _targetTemp / 10, _targetTemp % 10,
             _maxTemp / 10, _maxTemp % 10);

    return true;
}

// ============================================================================
// TEMPERATURE CONTROL (HEAT-001)
// ============================================================================

void HeaterController::setTargetTemperature(int16_t temp)
{
    // Clamp to valid range
    if (temp < 0) temp = 0;
    if (temp > _maxTemp) temp = _maxTemp;

    if (temp != _targetTemp) {
        _targetTemp = temp;
        // Reset stability when target changes
        _stable = false;
        _stableStartTime = 0;
        _readyCallbackCalled = false;

        Log.info("HeaterController: Target set to %d.%dC", temp / 10, temp % 10);
    }
}

int16_t HeaterController::getCurrentTemperatureF() const
{
    // Convert Celsius to Fahrenheit: F = (C * 9/5) + 32
    // In 10x format: F*10 = (C*10 * 9 / 5) + 320
    return ((_currentTemp * 9) / 5) + 320;
}

// ============================================================================
// PID CONTROL LOOP (HEAT-003)
// ============================================================================

uint8_t HeaterController::update()
{
    // Safety: If not initialized or not enabled, ensure heater is off
    if (!_initialized || !_enabled) {
        if (_currentPower != 0) {
            HAL::setHeaterOff();
            _currentPower = 0;
        }
        return 0;
    }

    // Safety: If in error state, keep heater off
    if (_state == HeaterState::ERROR) {
        if (_currentPower != 0) {
            HAL::setHeaterOff();
            _currentPower = 0;
        }
        return 0;
    }

    uint32_t currentTime = millis();
    uint32_t dt = currentTime - _lastReadTime;

    // Only update at control interval
    if (dt < HEATER_CONTROL_INTERVAL) {
        // Still apply current power with safety checks
        if (_currentPower > 0) {
            applyPower(_currentPower);
        }
        return _currentPower;
    }

    // Read temperature
    if (!readTemperature()) {
        emergencyShutdown(HeaterError::SENSOR_ERROR);
        return 0;
    }

    // Check safety conditions
    if (!checkSafetyConditions()) {
        return 0;
    }

    // Calculate PID output
    int32_t output;

    if (_lastReadTime == 0) {
        // First reading - initialize PID state
        _previousError = 0;
        _integral = 0;
        output = _defaultPower;
    } else if (_targetTemp > _maxTemp) {
        // Target exceeds max - don't heat
        output = 0;
    } else {
        output = calculatePID(dt);
    }

    _lastReadTime = currentTime;

    // Clamp output to valid PWM range
    if (output < 0) output = 0;
    if (output > HEATER_MAX_POWER) output = HEATER_MAX_POWER;

    // Apply power with safety checks
    applyPower((uint8_t)output);

    // Update stability detection
    updateStability();

    // Update state based on temperature
    if (_currentTemp >= _targetTemp) {
        if (_stable) {
            _state = HeaterState::STABLE;
        } else if (_currentTemp > _targetTemp + HEATER_READY_TEMP_DELTA) {
            _state = HeaterState::COOLING;
        } else {
            _state = HeaterState::HEATING;
        }
    } else {
        _state = HeaterState::HEATING;
    }

    // Verbose logging if enabled
    if (_verboseLogging) {
        int32_t error, integral, derivative, pidOutput;
        getPIDDebugInfo(error, integral, derivative, pidOutput);
        Log.info("Heater: T=%d.%dC, target=%d.%dC, raw=%d, err=%d, int=%d, der=%d, out=%d, pwr=%d",
                 _currentTemp / 10, _currentTemp % 10,
                 _targetTemp / 10, _targetTemp % 10,
                 _lastRawReading, (int)error, (int)integral, (int)derivative, (int)pidOutput,
                 _currentPower);
    }

    return _currentPower;
}

void HeaterController::setPIDConstants(int32_t kpNum, int32_t kpDen,
                                        int32_t kiNum, int32_t kiDen,
                                        int32_t kdNum, int32_t kdDen)
{
    // Prevent division by zero
    if (kpDen == 0) kpDen = 1;
    if (kiDen == 0) kiDen = 1;
    if (kdDen == 0) kdDen = 1;

    _kpNum = kpNum;
    _kpDen = kpDen;
    _kiNum = kiNum;
    _kiDen = kiDen;
    _kdNum = kdNum;
    _kdDen = kdDen;

    Log.info("HeaterController: PID constants set Kp=%d/%d, Ki=%d/%d, Kd=%d/%d",
             (int)kpNum, (int)kpDen, (int)kiNum, (int)kiDen, (int)kdNum, (int)kdDen);
}

void HeaterController::resetPID()
{
    _integral = 0;
    _previousError = 0;
    _lastReadTime = 0;
    _currentPower = 0;
}

// ============================================================================
// ENABLE/DISABLE FUNCTIONS (HEAT-006)
// ============================================================================

void HeaterController::enable()
{
    if (!_initialized) {
        Log.error("HeaterController: Cannot enable - not initialized");
        return;
    }

    if (_state == HeaterState::ERROR) {
        Log.warn("HeaterController: Cannot enable - in error state. Call clearError() first.");
        return;
    }

    if (!_enabled) {
        _enabled = true;
        resetPID();
        _stable = false;
        _stableStartTime = 0;
        _readyCallbackCalled = false;
        _state = HeaterState::HEATING;
        _lastTempForRunaway = 0;
        _runawayCheckTime = millis();
        _runawayCount = 0;
        _continuousPowerTime = 0;

        Log.info("HeaterController: Enabled, target=%d.%dC",
                 _targetTemp / 10, _targetTemp % 10);
    }
}

void HeaterController::disable()
{
    if (_enabled) {
        _enabled = false;
        HAL::setHeaterOff();
        _currentPower = 0;
        _state = HeaterState::DISABLED;
        _stable = false;

        Log.info("HeaterController: Disabled");
    }
}

void HeaterController::setDefaultPower(uint8_t power)
{
    if (power > HEATER_MAX_POWER) {
        power = HEATER_MAX_POWER;
    }
    _defaultPower = power;
}

// ============================================================================
// STABILITY DETECTION (HEAT-005)
// ============================================================================

uint32_t HeaterController::getTimeAtTarget() const
{
    if (!_stable && _stableStartTime == 0) {
        return 0;
    }

    if (!isWithinRange()) {
        return 0;
    }

    return HAL::elapsedSince(_stableStartTime);
}

void HeaterController::setReadyCallback(HeaterReadyCallback callback)
{
    _readyCallback = callback;
    _readyCallbackCalled = false;
}

bool HeaterController::isWithinRange(int16_t tolerance) const
{
    int16_t delta = _targetTemp - _currentTemp;
    // Only consider stable if temperature is AT or BELOW target
    // (not overheating) and within tolerance
    return (delta >= 0 && delta <= tolerance);
}

void HeaterController::updateStability()
{
    bool inRange = isWithinRange();

    if (inRange) {
        if (_stableStartTime == 0) {
            // Just entered range
            _stableStartTime = millis();
            _stable = false;
        } else {
            // Check if we've been stable long enough
            uint32_t stableTime = HAL::elapsedSince(_stableStartTime);
            if (stableTime >= STABILITY_DURATION_MS) {
                if (!_stable) {
                    _stable = true;
                    Log.info("HeaterController: Temperature stable at %d.%dC",
                             _currentTemp / 10, _currentTemp % 10);

                    // Call callback if set and not already called
                    if (_readyCallback && !_readyCallbackCalled) {
                        _readyCallbackCalled = true;
                        _readyCallback();
                    }
                }
            }
        }
    } else {
        // Out of range - reset stability
        if (_stable || _stableStartTime != 0) {
            _stable = false;
            _stableStartTime = 0;
            _readyCallbackCalled = false;
        }
    }
}

// ============================================================================
// SAFETY FUNCTIONS (HEAT-004)
// ============================================================================

void HeaterController::emergencyShutdown(HeaterError error)
{
    // IMMEDIATELY turn off heater
    HAL::setHeaterOff();
    _currentPower = 0;

    // Set error state
    _enabled = false;
    _state = HeaterState::ERROR;
    _lastError = error;
    _stable = false;

    Log.error("HeaterController: EMERGENCY SHUTDOWN - %s", getErrorString(error));
}

void HeaterController::clearError()
{
    if (_state == HeaterState::ERROR) {
        HAL::setHeaterOff();
        _currentPower = 0;
        _state = HeaterState::DISABLED;
        _lastError = HeaterError::NONE;
        _enabled = false;
        resetPID();

        Log.info("HeaterController: Error cleared");
    }
}

bool HeaterController::isOverTemperature() const
{
    return _currentTemp > _maxTemp;
}

bool HeaterController::isFailsafe() const
{
    return HAL::isHeaterFailsafe();
}

void HeaterController::setMaxTemperature(int16_t temp)
{
    // Must be at least 100 (10.0C) to be useful
    if (temp < 100) temp = 100;
    // Cannot exceed absolute maximum from hardware config
    if (temp > HEATER_MAX_TEMPERATURE) temp = HEATER_MAX_TEMPERATURE;

    _maxTemp = temp;

    // Adjust target if needed
    if (_targetTemp > _maxTemp) {
        setTargetTemperature(_maxTemp);
    }
}

bool HeaterController::checkSafetyConditions()
{
    // Check over-temperature
    if (isOverTemperature()) {
        emergencyShutdown(HeaterError::OVER_TEMPERATURE);
        return false;
    }

    // Check thermal runaway
    if (checkThermalRunaway()) {
        emergencyShutdown(HeaterError::THERMAL_RUNAWAY);
        return false;
    }

    // Check HAL failsafe (continuous power too long)
    if (isFailsafe()) {
        emergencyShutdown(HeaterError::FAILSAFE_TRIGGERED);
        return false;
    }

    return true;
}

bool HeaterController::checkThermalRunaway()
{
    uint32_t now = millis();

    // Check every second
    if (HAL::elapsedSince(_runawayCheckTime) < 1000) {
        return false;
    }

    _runawayCheckTime = now;

    // Runaway detection: if heater is off (or low power) but temperature
    // is increasing rapidly, something is wrong
    if (_currentPower < 32 && _lastTempForRunaway > 0) {
        int16_t tempIncrease = _currentTemp - _lastTempForRunaway;
        if (tempIncrease > RUNAWAY_TEMP_INCREASE) {
            _runawayCount++;
            if (_runawayCount >= RUNAWAY_THRESHOLD_COUNT) {
                return true;
            }
        } else {
            _runawayCount = 0;
        }
    } else {
        _runawayCount = 0;
    }

    _lastTempForRunaway = _currentTemp;
    return false;
}

// ============================================================================
// PRIVATE HELPER FUNCTIONS
// ============================================================================

bool HeaterController::readTemperature()
{
    // Turn off heater briefly to get accurate reading
    // (as done in legacy firmware)
    uint8_t savedPower = _currentPower;
    HAL::setHeaterOff();
    delayMicroseconds(HEATER_STABILIZATION_TIME_US);

    // Read raw ADC value
    _lastRawReading = HAL::readThermistorRaw(THERMISTOR_SAMPLES);

    // Restore heater power if needed
    if (savedPower > 0 && _enabled && _state != HeaterState::ERROR) {
        HAL::setHeaterPower(savedPower);
        _lastPowerApplyTime = millis();
    }

    // Check if reading is valid
    if (!HAL::isThermistorReadingValid(_lastRawReading)) {
        Log.error("HeaterController: Invalid thermistor reading: %d", _lastRawReading);
        return false;
    }

    // Convert to temperature
    _currentTemp = HAL::rawToTemperature(_lastRawReading);

    return true;
}

int32_t HeaterController::calculatePID(int32_t dt)
{
    // Calculate error (positive = need more heat)
    int32_t error = _targetTemp - _currentTemp;

    // Calculate integral with anti-windup
    _integral += (error * dt) / 1000;
    if (_integral > MAX_INTEGRAL) _integral = MAX_INTEGRAL;
    if (_integral < MIN_INTEGRAL) _integral = MIN_INTEGRAL;

    // Calculate derivative (using dt in ms, multiply error by 1000 for precision)
    int32_t derivative = 0;
    if (dt > 0) {
        derivative = (1000 * (error - _previousError)) / dt;
    }

    // Calculate output using integer math (matching legacy)
    int32_t output = (_kpNum * error) / _kpDen;
    output += (_kiNum * _integral) / _kiDen;
    output += (_kdNum * derivative) / _kdDen;

    // Store for next iteration
    _previousError = error;

    return output;
}

void HeaterController::applyPower(uint8_t power)
{
    uint32_t now = millis();

    // Track continuous power time for failsafe
    if (power > 0) {
        if (_currentPower == 0) {
            // Just turned on
            _lastPowerApplyTime = now;
            _continuousPowerTime = 0;
        } else {
            _continuousPowerTime = HAL::elapsedSince(_lastPowerApplyTime);
        }

        // Safety: If approaching failsafe timing, briefly turn off
        // This ensures we never exceed the 2000ms continuous limit
        if (_continuousPowerTime >= (HEATER_FAILSAFE_TIMING - 100)) {
            HAL::setHeaterOff();
            delayMicroseconds(1000);  // Brief pause
            _lastPowerApplyTime = now;
            _continuousPowerTime = 0;
        }

        HAL::setHeaterPower(power);
    } else {
        HAL::setHeaterOff();
        _continuousPowerTime = 0;
    }

    _currentPower = power;
}

// ============================================================================
// DEBUG / DIAGNOSTICS
// ============================================================================

void HeaterController::getPIDDebugInfo(int32_t& error, int32_t& integral,
                                        int32_t& derivative, int32_t& output) const
{
    error = _targetTemp - _currentTemp;
    integral = _integral;

    // Approximate derivative (we don't store it, so estimate)
    derivative = (1000 * error) / HEATER_CONTROL_INTERVAL;

    // Calculate approximate output
    output = (_kpNum * error) / _kpDen;
    output += (_kiNum * integral) / _kiDen;
    output += (_kdNum * derivative) / _kdDen;
}

const char* HeaterController::getStateString() const
{
    switch (_state) {
        case HeaterState::DISABLED: return "DISABLED";
        case HeaterState::HEATING:  return "HEATING";
        case HeaterState::STABLE:   return "STABLE";
        case HeaterState::COOLING:  return "COOLING";
        case HeaterState::ERROR:    return "ERROR";
        default:                    return "UNKNOWN";
    }
}

const char* HeaterController::getErrorString(HeaterError error)
{
    switch (error) {
        case HeaterError::NONE:              return "NONE";
        case HeaterError::OVER_TEMPERATURE:  return "OVER_TEMPERATURE";
        case HeaterError::UNDER_TEMPERATURE: return "UNDER_TEMPERATURE";
        case HeaterError::SENSOR_ERROR:      return "SENSOR_ERROR";
        case HeaterError::FAILSAFE_TRIGGERED: return "FAILSAFE_TRIGGERED";
        case HeaterError::THERMAL_RUNAWAY:   return "THERMAL_RUNAWAY";
        default:                             return "UNKNOWN";
    }
}
