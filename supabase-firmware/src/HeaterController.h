/**
 * @file HeaterController.h
 * @brief PID Temperature Controller for Brevitest heating element
 * @author Agent THETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module implements a PID controller for temperature regulation of the
 * Brevitest heating element. It provides temperature ramping, stability
 * detection, and critical safety failsafes.
 *
 * Features:
 * - PID control with configurable constants
 * - Thermistor temperature measurement via HAL
 * - Temperature stability detection for test readiness
 * - Safety failsafes (max power time, over-temperature cutoff)
 * - Enable/disable with state tracking
 *
 * Usage:
 *   HeaterController heater;
 *   heater.init();
 *   heater.setTargetTemperature(450);  // 45.0C
 *   heater.enable();
 *
 *   // In main loop:
 *   heater.update();
 *   if (heater.isReady()) {
 *       // Temperature stable, can start test
 *   }
 *
 * PID Tuning Parameters (from legacy firmware):
 *   Kp = 80/4 = 20.0
 *   Ki = 1/50000 = 0.00002
 *   Kd = 1/5 = 0.2
 *
 * SAFETY CRITICAL:
 * - Maximum continuous heater power: 2000ms
 * - Over-temperature cutoff: 60.0C (600 in 10x format)
 * - Failsafes are ALWAYS active when enabled
 */

#ifndef HEATER_CONTROLLER_H
#define HEATER_CONTROLLER_H

#include "Particle.h"
#include "HardwareConfig.h"
#include "HAL.h"

/**
 * @brief Callback function type for temperature ready notification
 */
typedef void (*HeaterReadyCallback)();

/**
 * @brief Heater error codes
 */
enum class HeaterError : uint8_t {
    NONE = 0,
    OVER_TEMPERATURE,       // Temperature exceeded maximum safe limit
    UNDER_TEMPERATURE,      // Temperature dropped unexpectedly
    SENSOR_ERROR,           // Thermistor reading out of valid range
    FAILSAFE_TRIGGERED,     // Continuous power limit exceeded
    THERMAL_RUNAWAY         // Temperature increasing uncontrollably
};

/**
 * @brief Heater controller state
 */
enum class HeaterState : uint8_t {
    DISABLED = 0,           // Heater is off, not controlling temperature
    HEATING,                // Actively heating to reach target
    STABLE,                 // Temperature stable at target
    COOLING,                // Waiting for temperature to drop (over-target)
    ERROR                   // Error state - heater disabled
};

/**
 * @class HeaterController
 * @brief PID temperature controller for the heating element
 *
 * Implements PID control algorithm with safety features for temperature
 * regulation. The controller uses the HAL layer for hardware access and
 * includes multiple safety mechanisms to prevent damage.
 */
class HeaterController {
public:
    // ========================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ========================================================================

    /**
     * @brief Default constructor
     *
     * Initializes all state variables to safe defaults.
     * Does NOT configure hardware - call init() for that.
     */
    HeaterController();

    /**
     * @brief Destructor
     *
     * Ensures heater is turned off when object is destroyed.
     */
    ~HeaterController();

    // ========================================================================
    // INITIALIZATION (HEAT-001)
    // ========================================================================

    /**
     * @brief Initialize heater controller
     *
     * Configures heater pin (D4) and sets up initial state.
     * Must be called after HAL::init().
     *
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Check if controller has been initialized
     * @return true if init() has been called successfully
     */
    bool isInitialized() const { return _initialized; }

    // ========================================================================
    // TEMPERATURE CONTROL (HEAT-001)
    // ========================================================================

    /**
     * @brief Set target temperature
     *
     * @param temp Target temperature in 10x Celsius (e.g., 450 = 45.0C)
     * @note Temperature is clamped to 0-600 (0.0C to 60.0C)
     */
    void setTargetTemperature(int16_t temp);

    /**
     * @brief Get current target temperature
     * @return Target temperature in 10x Celsius
     */
    int16_t getTargetTemperature() const { return _targetTemp; }

    /**
     * @brief Get current measured temperature
     * @return Current temperature in 10x Celsius
     */
    int16_t getCurrentTemperature() const { return _currentTemp; }

    /**
     * @brief Get current temperature in Fahrenheit (10x format)
     * @return Current temperature in 10x Fahrenheit
     */
    int16_t getCurrentTemperatureF() const;

    /**
     * @brief Get last raw ADC reading
     * @return Raw ADC value from thermistor
     */
    int16_t getRawReading() const { return _lastRawReading; }

    // ========================================================================
    // PID CONTROL LOOP (HEAT-003)
    // ========================================================================

    /**
     * @brief Update PID control loop
     *
     * Call this function in the main loop at regular intervals.
     * Handles temperature reading, PID calculation, and safety checks.
     *
     * @return Current PWM output value (0-255), or 0 if disabled/error
     */
    uint8_t update();

    /**
     * @brief Set PID constants
     *
     * Allows runtime tuning of PID parameters. Uses numerator/denominator
     * format for integer-only calculation (matching legacy firmware).
     *
     * Default values:
     *   Kp = 80/4, Ki = 1/50000, Kd = 1/5
     *
     * @param kpNum Proportional gain numerator
     * @param kpDen Proportional gain denominator
     * @param kiNum Integral gain numerator
     * @param kiDen Integral gain denominator
     * @param kdNum Derivative gain numerator
     * @param kdDen Derivative gain denominator
     */
    void setPIDConstants(int32_t kpNum, int32_t kpDen,
                         int32_t kiNum, int32_t kiDen,
                         int32_t kdNum, int32_t kdDen);

    /**
     * @brief Reset PID state
     *
     * Clears integral accumulator and previous error.
     * Call when starting a new heating cycle.
     */
    void resetPID();

    /**
     * @brief Get current PWM output
     * @return Current PWM value (0-255)
     */
    uint8_t getCurrentPower() const { return _currentPower; }

    // ========================================================================
    // ENABLE/DISABLE FUNCTIONS (HEAT-006)
    // ========================================================================

    /**
     * @brief Enable temperature control
     *
     * Starts the PID control loop. Temperature will begin ramping
     * toward the target.
     */
    void enable();

    /**
     * @brief Disable temperature control
     *
     * Stops the PID control loop and turns off the heater safely.
     * Resets PID state for next enable.
     */
    void disable();

    /**
     * @brief Check if heater control is enabled
     * @return true if enabled
     */
    bool isEnabled() const { return _enabled; }

    /**
     * @brief Set default power level
     *
     * Power level used when first enabling, before PID takes over.
     *
     * @param power Default power (0-255), default is 64 (25%)
     */
    void setDefaultPower(uint8_t power);

    /**
     * @brief Get default power level
     * @return Default power (0-255)
     */
    uint8_t getDefaultPower() const { return _defaultPower; }

    // ========================================================================
    // STABILITY DETECTION (HEAT-005)
    // ========================================================================

    /**
     * @brief Check if temperature is stable at target
     *
     * Returns true when temperature has been within +/-10C (1.0C actual)
     * of target for at least 5 seconds.
     *
     * @return true if temperature is stable
     */
    bool isReady() const { return _stable; }

    /**
     * @brief Get time spent at target temperature
     *
     * Returns milliseconds that temperature has been within stability
     * threshold of target.
     *
     * @return Milliseconds at target, or 0 if not at target
     */
    uint32_t getTimeAtTarget() const;

    /**
     * @brief Set callback for when temperature becomes ready
     *
     * Callback is called once when temperature first stabilizes.
     *
     * @param callback Function to call when ready, or nullptr to clear
     */
    void setReadyCallback(HeaterReadyCallback callback);

    /**
     * @brief Check if temperature is within range of target
     *
     * @param tolerance Tolerance in 10x Celsius (default: HEATER_READY_TEMP_DELTA)
     * @return true if within range
     */
    bool isWithinRange(int16_t tolerance = HEATER_READY_TEMP_DELTA) const;

    // ========================================================================
    // SAFETY FUNCTIONS (HEAT-004)
    // ========================================================================

    /**
     * @brief Emergency shutdown
     *
     * Immediately turns off heater and sets error state.
     * Call when a critical safety condition is detected.
     *
     * @param error Error code for the shutdown reason
     */
    void emergencyShutdown(HeaterError error);

    /**
     * @brief Get current heater state
     * @return Current HeaterState
     */
    HeaterState getState() const { return _state; }

    /**
     * @brief Get last error code
     * @return Last HeaterError, or NONE if no error
     */
    HeaterError getLastError() const { return _lastError; }

    /**
     * @brief Clear error state
     *
     * Clears error and returns to DISABLED state.
     * Must call enable() to restart control.
     */
    void clearError();

    /**
     * @brief Check if over-temperature condition exists
     * @return true if temperature exceeds maximum safe limit
     */
    bool isOverTemperature() const;

    /**
     * @brief Check if heater failsafe has triggered
     * @return true if continuous power limit exceeded
     */
    bool isFailsafe() const;

    /**
     * @brief Set over-temperature threshold
     *
     * @param temp Maximum safe temperature in 10x Celsius
     */
    void setMaxTemperature(int16_t temp);

    /**
     * @brief Get over-temperature threshold
     * @return Maximum safe temperature in 10x Celsius
     */
    int16_t getMaxTemperature() const { return _maxTemp; }

    // ========================================================================
    // DEBUG / DIAGNOSTICS
    // ========================================================================

    /**
     * @brief Get PID debug information
     *
     * Fills provided variables with current PID state for debugging.
     *
     * @param error Output: current error value
     * @param integral Output: integral accumulator
     * @param derivative Output: derivative term
     * @param output Output: calculated output before clamping
     */
    void getPIDDebugInfo(int32_t& error, int32_t& integral,
                         int32_t& derivative, int32_t& output) const;

    /**
     * @brief Enable/disable verbose logging
     * @param enable true to enable logging
     */
    void setVerboseLogging(bool enable) { _verboseLogging = enable; }

    /**
     * @brief Get string representation of current state
     * @return State name as string
     */
    const char* getStateString() const;

    /**
     * @brief Get string representation of error code
     * @param error Error code to convert
     * @return Error name as string
     */
    static const char* getErrorString(HeaterError error);

private:
    // ========================================================================
    // PRIVATE HELPER FUNCTIONS
    // ========================================================================

    /**
     * @brief Read temperature from thermistor
     * @return true if reading was valid
     */
    bool readTemperature();

    /**
     * @brief Calculate PID output
     * @param dt Time delta in milliseconds
     * @return Calculated output value
     */
    int32_t calculatePID(int32_t dt);

    /**
     * @brief Apply power output with safety checks
     * @param power Desired power level (0-255)
     */
    void applyPower(uint8_t power);

    /**
     * @brief Check for thermal runaway condition
     * @return true if runaway detected
     */
    bool checkThermalRunaway();

    /**
     * @brief Update stability detection
     */
    void updateStability();

    /**
     * @brief Check and handle safety conditions
     * @return true if safe to continue, false if error
     */
    bool checkSafetyConditions();

    // ========================================================================
    // STATE VARIABLES
    // ========================================================================

    // Initialization
    bool _initialized;

    // Control state
    bool _enabled;
    HeaterState _state;
    HeaterError _lastError;

    // Temperature values (all in 10x Celsius format)
    int16_t _targetTemp;
    int16_t _currentTemp;
    int16_t _maxTemp;
    int16_t _lastRawReading;

    // PID constants (numerator/denominator for integer math)
    int32_t _kpNum, _kpDen;
    int32_t _kiNum, _kiDen;
    int32_t _kdNum, _kdDen;

    // PID state
    int32_t _integral;
    int32_t _previousError;
    uint32_t _lastReadTime;
    uint8_t _currentPower;
    uint8_t _defaultPower;

    // Stability detection
    bool _stable;
    uint32_t _stableStartTime;
    bool _readyCallbackCalled;
    HeaterReadyCallback _readyCallback;

    // Thermal runaway detection
    int16_t _lastTempForRunaway;
    uint32_t _runawayCheckTime;
    uint8_t _runawayCount;

    // Power application timing
    uint32_t _lastPowerApplyTime;
    uint32_t _continuousPowerTime;

    // Logging
    bool _verboseLogging;

    // ========================================================================
    // CONSTANTS
    // ========================================================================

    // PID default values (matching legacy: Kp=80/4, Ki=1/50000, Kd=1/5)
    static constexpr int32_t DEFAULT_KP_NUM = 80;
    static constexpr int32_t DEFAULT_KP_DEN = 4;
    static constexpr int32_t DEFAULT_KI_NUM = 1;
    static constexpr int32_t DEFAULT_KI_DEN = 50000;
    static constexpr int32_t DEFAULT_KD_NUM = 1;
    static constexpr int32_t DEFAULT_KD_DEN = 5;

    // Safety constants
    static constexpr int32_t MAX_INTEGRAL = 100000;  // Prevent integral windup
    static constexpr int32_t MIN_INTEGRAL = -100000;
    static constexpr uint8_t RUNAWAY_THRESHOLD_COUNT = 5;
    static constexpr int16_t RUNAWAY_TEMP_INCREASE = 50;  // 5.0C increase = runaway

    // Stability constants
    static constexpr uint32_t STABILITY_DURATION_MS = 5000;  // 5 seconds
};

#endif // HEATER_CONTROLLER_H
