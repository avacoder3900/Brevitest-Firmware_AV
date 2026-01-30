/**
 * @file MotorController.h
 * @brief Stepper motor controller for linear stage movement
 * @author Agent ETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module provides high-level stepper motor control for the Brevitest linear stage.
 * It handles position tracking, homing, absolute/relative movements, and oscillation modes.
 *
 * Hardware: A4988 stepper driver with 8 microsteps per full step
 * Resolution: 25 microns per 1/8 step
 *
 * Features:
 * - Position tracking in microns
 * - Homing sequence with limit switch
 * - Absolute and relative positioning
 * - Oscillation mode for spectrophotometer scanning
 * - Predefined positions for common operations
 * - Non-blocking movement with callback support
 *
 * Usage:
 *   MotorController motor;
 *   motor.init();
 *   motor.home();
 *   motor.moveToPosition(7860);  // Move to test start
 */

#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "Particle.h"
#include "HAL.h"
#include "HardwareConfig.h"

// ============================================================================
// MOTOR CONTROLLER CONSTANTS
// ============================================================================

/**
 * @brief Predefined stage positions (in microns)
 *
 * These positions correspond to key locations on the cartridge:
 * - HOME: Default position after homing
 * - TEST_START: Position to begin test sequence
 * - OPTICAL_READ: Starting position for spectrophotometer readings
 * - MAGNETOMETER: Position for magnetometer readings
 * - SHIPPING_BOLT: Position for shipping bolt check
 * - LIMIT: Physical limit switch position (0 microns)
 */
namespace StagePosition {
    constexpr int32_t LIMIT = 0;                   // Limit switch position
    constexpr int32_t INITIAL = 1000;              // Initial position after reset
    constexpr int32_t HOME = 21000;                // Home position (optical read start)
    constexpr int32_t TEST_START = 7860;           // Test start position
    constexpr int32_t OPTICAL_READ = 21000;        // Spectrophotometer start position
    constexpr int32_t MAGNETOMETER = 12800;        // Magnetometer start position
    constexpr int32_t SHIPPING_BOLT = 28000;       // Shipping bolt position
    constexpr int32_t MIN = -60000;                // Minimum position (reset steps)
    constexpr int32_t MAX = 45000;                 // Maximum position limit
}

/**
 * @brief Speed presets for motor movement
 */
namespace MotorSpeed {
    constexpr uint16_t MINIMUM_DELAY = 250;        // Fastest speed (minimum delay)
    constexpr uint16_t FAST = 290;                 // Fast movement
    constexpr uint16_t RESET = 290;                // Reset/homing speed
    constexpr uint16_t OSCILLATION = 350;          // Oscillation speed
    constexpr uint16_t BOUNCE = 350;               // Bounce movement
    constexpr uint16_t SLOW = 600;                 // Slow/precise movement
    constexpr uint16_t SENSOR = 1000;              // Sensor reading speed
}

/**
 * @brief Motor state enumeration
 */
enum class MotorState {
    IDLE,           // Motor is not moving
    HOMING,         // Performing homing sequence
    MOVING,         // Moving to position
    OSCILLATING,    // In oscillation mode
    ERROR           // Error state
};

/**
 * @brief Motor movement result codes
 */
enum class MotorResult {
    SUCCESS,                // Operation completed successfully
    AT_LIMIT,               // Reached limit switch
    AT_BOUNDARY,            // Reached position boundary
    TIMEOUT,                // Operation timed out
    CANCELLED,              // Operation was cancelled
    NOT_HOMED,              // Must home before moving
    INVALID_POSITION,       // Position out of range
    ERROR                   // General error
};

/**
 * @brief Callback function type for movement completion
 * @param result The result of the movement operation
 * @param position Current position after movement
 */
typedef void (*MotorCallback)(MotorResult result, int32_t position);

/**
 * @brief Callback function type for oscillation direction change
 * @param forward True if now moving forward, false if reverse
 * @param position Current position
 */
typedef void (*OscillationCallback)(bool forward, int32_t position);

// ============================================================================
// MOTOR CONTROLLER CLASS
// ============================================================================

/**
 * @class MotorController
 * @brief High-level stepper motor controller for linear stage
 *
 * This class provides position-aware motor control with safety features.
 * It tracks position in microns and enforces movement limits.
 */
class MotorController {
public:
    // ========================================================================
    // CONSTRUCTOR / DESTRUCTOR
    // ========================================================================

    /**
     * @brief Construct a new Motor Controller object
     */
    MotorController();

    /**
     * @brief Destroy the Motor Controller object
     */
    ~MotorController();

    // ========================================================================
    // INITIALIZATION (MOT-001)
    // ========================================================================

    /**
     * @brief Initialize the motor controller
     *
     * Configures motor pins via HAL and sets initial state.
     * Does NOT perform homing - call home() separately.
     *
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Check if motor controller is initialized
     * @return true if init() has been called successfully
     */
    bool isInitialized() const;

    /**
     * @brief Enable the motor driver (wake from sleep)
     */
    void enable();

    /**
     * @brief Disable the motor driver (put to sleep)
     */
    void disable();

    /**
     * @brief Check if motor driver is enabled
     * @return true if motor is awake/enabled
     */
    bool isEnabled() const;

    // ========================================================================
    // POSITION TRACKING (MOT-002)
    // ========================================================================

    /**
     * @brief Get current position in microns
     * @return Current position (0 = limit switch)
     */
    int32_t getCurrentPosition() const;

    /**
     * @brief Set the current position (for calibration)
     *
     * Use with caution - this overrides position tracking.
     *
     * @param position Position in microns
     */
    void setPosition(int32_t position);

    /**
     * @brief Check if position is within valid range
     * @param position Position to check (microns)
     * @return true if position is valid
     */
    bool isValidPosition(int32_t position) const;

    /**
     * @brief Check if stage has been homed
     * @return true if home() completed successfully
     */
    bool isHomed() const;

    /**
     * @brief Get accumulated position error from rounding
     * @return Error in microns (0-24)
     */
    int16_t getPositionError() const;

    // ========================================================================
    // HOMING (MOT-003)
    // ========================================================================

    /**
     * @brief Perform homing sequence
     *
     * Moves stage until limit switch is triggered, sets position to 0,
     * then moves to initial position (1000 microns).
     *
     * @param sleepAfter If true, put motor to sleep after homing
     * @return MotorResult indicating success or failure
     */
    MotorResult home(bool sleepAfter = true);

    /**
     * @brief Reset stage (alias for home with sleep)
     * @return MotorResult indicating success or failure
     */
    MotorResult reset();

    /**
     * @brief Check if limit switch is triggered
     * @return true if limit switch is active (stage at home)
     */
    bool isAtLimit() const;

    // ========================================================================
    // ABSOLUTE POSITIONING (MOT-004)
    // ========================================================================

    /**
     * @brief Move to an absolute position
     *
     * Calculates the required movement and direction to reach the target.
     *
     * @param position Target position in microns
     * @param stepDelay Step delay in microseconds (default: slow)
     * @return MotorResult indicating success or failure
     */
    MotorResult moveToPosition(int32_t position, uint16_t stepDelay = MotorSpeed::SLOW);

    /**
     * @brief Move to position with completion callback (non-blocking)
     *
     * Note: Currently implemented as blocking - callback is called
     * immediately after movement completes.
     *
     * @param position Target position in microns
     * @param stepDelay Step delay in microseconds
     * @param callback Function to call on completion
     * @return MotorResult::SUCCESS if movement started
     */
    MotorResult moveToPositionAsync(int32_t position, uint16_t stepDelay, MotorCallback callback);

    // ========================================================================
    // RELATIVE MOVEMENT (MOT-005)
    // ========================================================================

    /**
     * @brief Move relative to current position
     *
     * Positive values move distally (away from limit), negative values
     * move proximally (toward limit).
     *
     * @param microns Distance to move (positive = distal, negative = proximal)
     * @param stepDelay Step delay in microseconds
     * @return MotorResult indicating success or failure
     */
    MotorResult moveRelative(int32_t microns, uint16_t stepDelay = MotorSpeed::SLOW);

    /**
     * @brief Move a specific number of 1/8 steps
     *
     * Low-level function for precise step control.
     *
     * @param steps Number of steps (positive = distal, negative = proximal)
     * @param stepDelay Step delay in microseconds
     * @return MotorResult indicating success or failure
     */
    MotorResult stepRelative(int32_t steps, uint16_t stepDelay = MotorSpeed::SLOW);

    /**
     * @brief Move one 1/8 step in the specified direction
     *
     * Checks limits before moving. Updates position tracking.
     *
     * @param forward True for distal (forward), false for proximal (reverse)
     * @param stepDelay Step delay in microseconds
     * @return true if step was taken, false if at limit
     */
    bool stepOnce(bool forward, uint16_t stepDelay);

    // ========================================================================
    // OSCILLATION MODE (MOT-006)
    // ========================================================================

    /**
     * @brief Start oscillation mode
     *
     * Oscillates the stage back and forth for a specified number of cycles.
     * Each cycle consists of moving amplitude in one direction, then back.
     *
     * @param amplitude Distance to oscillate (microns)
     * @param stepDelay Step delay in microseconds
     * @param cycles Number of complete cycles (0 = continuous)
     * @return MotorResult indicating success or failure
     */
    MotorResult oscillate(int32_t amplitude, uint16_t stepDelay = MotorSpeed::OSCILLATION, uint16_t cycles = 1);

    /**
     * @brief Start continuous oscillation with callback
     *
     * Oscillates until stopOscillation() is called. Calls callback on
     * each direction change.
     *
     * @param amplitude Distance to oscillate (microns)
     * @param stepDelay Step delay in microseconds
     * @param callback Function to call on direction change (optional)
     * @return MotorResult::SUCCESS if oscillation started
     */
    MotorResult startOscillation(int32_t amplitude, uint16_t stepDelay, OscillationCallback callback = nullptr);

    /**
     * @brief Stop oscillation mode
     */
    void stopOscillation();

    /**
     * @brief Check if currently oscillating
     * @return true if in oscillation mode
     */
    bool isOscillating() const;

    /**
     * @brief Update oscillation (call from main loop)
     *
     * Must be called periodically when oscillating to process movement.
     */
    void updateOscillation();

    // ========================================================================
    // PREDEFINED POSITIONS (MOT-007)
    // ========================================================================

    /**
     * @brief Move to home position (21000 microns)
     * @param stepDelay Step delay in microseconds
     * @return MotorResult indicating success or failure
     */
    MotorResult moveToHome(uint16_t stepDelay = MotorSpeed::SLOW);

    /**
     * @brief Move to test start position (7860 microns)
     * @param stepDelay Step delay in microseconds
     * @return MotorResult indicating success or failure
     */
    MotorResult moveToTestStart(uint16_t stepDelay = MotorSpeed::SLOW);

    /**
     * @brief Move to optical read position (21000 microns)
     * @param stepDelay Step delay in microseconds
     * @return MotorResult indicating success or failure
     */
    MotorResult moveToOpticalRead(uint16_t stepDelay = MotorSpeed::SLOW);

    /**
     * @brief Move to magnetometer position (12800 microns)
     * @param stepDelay Step delay in microseconds
     * @return MotorResult indicating success or failure
     */
    MotorResult moveToMagnetometer(uint16_t stepDelay = MotorSpeed::SLOW);

    /**
     * @brief Move to shipping bolt position (28000 microns)
     * @param stepDelay Step delay in microseconds
     * @return MotorResult indicating success or failure
     */
    MotorResult moveToShippingBolt(uint16_t stepDelay = MotorSpeed::SLOW);

    // ========================================================================
    // STATE AND DIAGNOSTICS
    // ========================================================================

    /**
     * @brief Get current motor state
     * @return Current MotorState
     */
    MotorState getState() const;

    /**
     * @brief Cancel current operation
     *
     * Stops any ongoing movement and returns motor to IDLE state.
     */
    void cancel();

    /**
     * @brief Convert microns to 1/8 steps
     * @param microns Distance in microns
     * @return Equivalent number of 1/8 steps
     */
    static int32_t micronsToSteps(int32_t microns);

    /**
     * @brief Convert 1/8 steps to microns
     * @param steps Number of 1/8 steps
     * @return Equivalent distance in microns
     */
    static int32_t stepsToMicrons(int32_t steps);

    /**
     * @brief Calculate step delay for given integration time
     *
     * Used for spectrophotometer scanning to match motor speed
     * with sensor integration time.
     *
     * @param atime AS7341 ATIME value
     * @param astep AS7341 ASTEP value
     * @return Calculated step delay in microseconds
     */
    static uint16_t calculateStepDelayForIntegration(uint8_t atime, uint16_t astep);

private:
    // ========================================================================
    // PRIVATE MEMBERS
    // ========================================================================

    bool _initialized;              // True if init() called
    bool _homed;                    // True if homing completed
    int32_t _position;              // Current position in microns
    int16_t _positionError;         // Accumulated rounding error
    MotorState _state;              // Current motor state
    bool _cancelled;                // Operation cancelled flag

    // Oscillation state
    bool _oscillating;              // True if oscillating
    int32_t _oscillationAmplitude;  // Current oscillation amplitude
    uint16_t _oscillationDelay;     // Current oscillation step delay
    bool _oscillationForward;       // Current oscillation direction
    int32_t _oscillationStart;      // Oscillation center position
    OscillationCallback _oscillationCallback;  // Direction change callback

    // ========================================================================
    // PRIVATE METHODS
    // ========================================================================

    /**
     * @brief Move until limit switch is triggered
     * @param stepDelay Step delay in microseconds
     * @return MotorResult indicating success or timeout
     */
    MotorResult moveUntilLimit(uint16_t stepDelay);

    /**
     * @brief Execute a single step with timing
     * @param forward Direction (true = distal)
     * @param stepDelay Delay between steps
     * @return true if step completed, false if at limit
     */
    bool executeStep(bool forward, uint16_t stepDelay);

    /**
     * @brief Clamp step delay to valid range
     * @param stepDelay Requested step delay
     * @return Clamped step delay (>= MINIMUM_DELAY)
     */
    uint16_t clampStepDelay(uint16_t stepDelay) const;
};

#endif // MOTOR_CONTROLLER_H
