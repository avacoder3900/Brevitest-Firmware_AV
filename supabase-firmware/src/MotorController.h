/**
 * @file MotorController.h
 * @brief Stepper motor controller for linear stage movement
 *
 * Hardware: A5985GETTR-T dual full bridge, MS1/MS2/MS3 tied HIGH (1/8 microstep)
 * Resolution: 25 microns per 1/8 step
 */

#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include "Particle.h"
#include "HAL.h"
#include "HardwareConfig.h"

// ============================================================================
// MOTOR CONTROLLER CONSTANTS
// ============================================================================

namespace StagePosition {
    constexpr int32_t LIMIT = 0;
    constexpr int32_t INITIAL = 1000;
    constexpr int32_t HOME = 21000;
    constexpr int32_t TEST_START = 7860;
    constexpr int32_t OPTICAL_READ = 21000;
    constexpr int32_t MAGNETOMETER = 12800;
    constexpr int32_t SHIPPING_BOLT = 28000;
    constexpr int32_t MIN = -60000;
    constexpr int32_t MAX = 45000;
}

namespace MotorSpeed {
    constexpr uint16_t MINIMUM_DELAY = 250;
    constexpr uint16_t FAST = 290;
    constexpr uint16_t RESET = 290;
    constexpr uint16_t OSCILLATION = 350;
    constexpr uint16_t BOUNCE = 350;
    constexpr uint16_t SLOW = 600;
    constexpr uint16_t SENSOR = 1000;
}

enum class MotorState {
    IDLE,
    HOMING,
    MOVING,
    OSCILLATING,
    ERROR
};

enum class MotorResult {
    SUCCESS,
    AT_LIMIT,
    AT_BOUNDARY,
    TIMEOUT,
    CANCELLED,
    NOT_HOMED,
    INVALID_POSITION,
    ERROR
};

// ============================================================================
// MOTOR CONTROLLER CLASS
// ============================================================================

class MotorController {
public:
    MotorController();
    ~MotorController();

    // Initialization
    bool init();
    bool isInitialized() const;
    void enable();
    void disable();

    // Position tracking
    int32_t getCurrentPosition() const;
    int16_t getPositionError() const;

    // Homing
    MotorResult home(bool sleepAfter = true);

    // Movement
    MotorResult moveToPosition(int32_t position, uint16_t stepDelay = MotorSpeed::SLOW);
    MotorResult moveRelative(int32_t microns, uint16_t stepDelay = MotorSpeed::SLOW);

    // Oscillation (blocking)
    MotorResult oscillate(int32_t amplitude, uint16_t stepDelay = MotorSpeed::OSCILLATION, uint16_t cycles = 1);

private:
    bool _initialized;
    bool _homed;
    int32_t _position;
    int16_t _positionError;
    MotorState _state;
    bool _cancelled;

    bool isEnabled() const;
    bool isValidPosition(int32_t position) const;
    MotorResult moveUntilLimit(uint16_t stepDelay);
    bool executeStep(bool forward, uint16_t stepDelay);
    uint16_t clampStepDelay(uint16_t stepDelay) const;
};

#endif // MOTOR_CONTROLLER_H
