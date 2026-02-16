/**
 * @file MotorController.cpp
 * @brief Stepper motor controller implementation
 */

#include "MotorController.h"

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

MotorController::MotorController() :
    _initialized(false),
    _homed(false),
    _position(0),
    _positionError(0),
    _state(MotorState::IDLE),
    _cancelled(false)
{
}

MotorController::~MotorController() {
    if (_initialized) {
        disable();
    }
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool MotorController::init() {
    if (_initialized) {
        return true;
    }

    if (!HAL::isInitialized()) {
        if (!HAL::init()) {
            Log.error("MotorController: HAL initialization failed");
            return false;
        }
    }

    _initialized = true;
    _homed = false;
    _position = 0;
    _positionError = 0;
    _state = MotorState::IDLE;
    _cancelled = false;

    Log.info("MotorController: Initialized");
    return true;
}

bool MotorController::isInitialized() const {
    return _initialized;
}

void MotorController::enable() {
    if (!_initialized) return;
    HAL::motorWake();
}

void MotorController::disable() {
    if (!_initialized) return;
    HAL::motorSleep();
}

bool MotorController::isEnabled() const {
    return _initialized && HAL::isMotorAwake();
}

// ============================================================================
// POSITION TRACKING
// ============================================================================

int32_t MotorController::getCurrentPosition() const {
    return _position;
}

bool MotorController::isValidPosition(int32_t position) const {
    return (position >= StagePosition::MIN && position <= StagePosition::MAX);
}

int16_t MotorController::getPositionError() const {
    return _positionError;
}

// ============================================================================
// HOMING
// ============================================================================

MotorResult MotorController::home(bool sleepAfter) {
    if (!_initialized) {
        Log.error("MotorController: Cannot home - not initialized");
        return MotorResult::ERROR;
    }

    Log.info("MotorController: Starting homing sequence");
    _state = MotorState::HOMING;
    _cancelled = false;

    if (!isEnabled()) {
        enable();
    }

    MotorResult result = moveUntilLimit(MotorSpeed::RESET);

    if (result == MotorResult::AT_LIMIT || result == MotorResult::SUCCESS) {
        _position = 0;
        _positionError = 0;
        Log.info("MotorController: At limit, position zeroed");

        result = moveRelative(StagePosition::INITIAL, MotorSpeed::RESET);

        if (result == MotorResult::SUCCESS) {
            _homed = true;
            Log.info("MotorController: Homing complete, position = %ld", _position);
        }
    }

    if (sleepAfter) {
        disable();
    }

    _state = MotorState::IDLE;
    return result;
}

// ============================================================================
// MOVEMENT
// ============================================================================

MotorResult MotorController::moveToPosition(int32_t position, uint16_t stepDelay) {
    if (!_initialized) {
        return MotorResult::ERROR;
    }

    if (!isValidPosition(position)) {
        Log.warn("MotorController: Invalid position %ld", position);
        return MotorResult::INVALID_POSITION;
    }

    int32_t distance = position - _position;

    if (distance == 0) {
        return MotorResult::SUCCESS;
    }

    return moveRelative(distance, stepDelay);
}

MotorResult MotorController::moveRelative(int32_t microns, uint16_t stepDelay) {
    if (!_initialized) {
        return MotorResult::ERROR;
    }

    if (_cancelled) {
        _cancelled = false;
        return MotorResult::CANCELLED;
    }

    if (!isEnabled()) {
        enable();
    }

    bool forward = (microns >= 0);
    int32_t absMicrons = forward ? microns : -microns;

    absMicrons += _positionError;

    int32_t steps = absMicrons / MOTOR_MICRONS_PER_EIGHTH_STEP;
    _positionError = absMicrons % MOTOR_MICRONS_PER_EIGHTH_STEP;

    stepDelay = clampStepDelay(stepDelay);

    // In legacy: HIGH = proximal (toward limit), LOW = distal (away from limit)
    HAL::setMotorDirection(!forward);

    _state = MotorState::MOVING;

    for (int32_t i = 0; i < steps; i++) {
        if (_cancelled) {
            _state = MotorState::IDLE;
            _cancelled = false;
            return MotorResult::CANCELLED;
        }

        if (!executeStep(forward, stepDelay)) {
            _state = MotorState::IDLE;
            return forward ? MotorResult::AT_BOUNDARY : MotorResult::AT_LIMIT;
        }
    }

    _state = MotorState::IDLE;
    return MotorResult::SUCCESS;
}

// ============================================================================
// OSCILLATION (blocking)
// ============================================================================

MotorResult MotorController::oscillate(int32_t amplitude, uint16_t stepDelay, uint16_t cycles) {
    if (!_initialized) {
        return MotorResult::ERROR;
    }

    if (amplitude == 0 || cycles == 0) {
        return MotorResult::SUCCESS;
    }

    if (!isEnabled()) {
        enable();
    }

    _state = MotorState::OSCILLATING;
    _cancelled = false;

    for (uint16_t i = 0; i < cycles; i++) {
        MotorResult result = moveRelative(amplitude, stepDelay);
        if (result != MotorResult::SUCCESS || _cancelled) {
            _state = MotorState::IDLE;
            return _cancelled ? MotorResult::CANCELLED : result;
        }

        Particle.process();

        result = moveRelative(-amplitude, stepDelay);
        if (result != MotorResult::SUCCESS || _cancelled) {
            _state = MotorState::IDLE;
            return _cancelled ? MotorResult::CANCELLED : result;
        }

        Particle.process();

        if (_cancelled) {
            _state = MotorState::IDLE;
            _cancelled = false;
            return MotorResult::CANCELLED;
        }
    }

    _state = MotorState::IDLE;
    return MotorResult::SUCCESS;
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

MotorResult MotorController::moveUntilLimit(uint16_t stepDelay) {
    int32_t maxSteps = (StagePosition::MAX / MOTOR_MICRONS_PER_EIGHTH_STEP) + 40;

    stepDelay = clampStepDelay(stepDelay);

    // Set direction to proximal (toward limit switch)
    HAL::setMotorDirection(true);

    int32_t count = 0;
    while (!HAL::readLimitSwitchRaw() && count < maxSteps) {
        digitalWrite(PIN_MOTOR_STEP, HIGH);
        delayMicroseconds(stepDelay);
        digitalWrite(PIN_MOTOR_STEP, LOW);
        delayMicroseconds(stepDelay);

        _position -= MOTOR_MICRONS_PER_EIGHTH_STEP;
        if (_position < 0) {
            _position = 0;
        }
        count++;
    }

    if (HAL::readLimitSwitchRaw()) {
        _position = 0;
        _positionError = 0;
        return MotorResult::AT_LIMIT;
    }

    Log.warn("MotorController: moveUntilLimit timeout after %ld steps", count);
    return MotorResult::TIMEOUT;
}

bool MotorController::executeStep(bool forward, uint16_t stepDelay) {
    if (!forward) {
        if (HAL::readLimitSwitchRaw()) {
            delayMicroseconds(10000);
            if (HAL::readLimitSwitchRaw()) {
                _position = 0;
                _positionError = 0;
                return false;
            }
        }
        if (_position <= 0) {
            _position = 0;
            _positionError = 0;
            return false;
        }
    } else {
        if (_position >= StagePosition::MAX) {
            return false;
        }
    }

    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(stepDelay);

    if (forward) {
        _position += MOTOR_MICRONS_PER_EIGHTH_STEP;
    } else {
        _position -= MOTOR_MICRONS_PER_EIGHTH_STEP;
        if (_position <= 0) {
            _position = 0;
            _positionError = 0;
        }
    }

    return true;
}

uint16_t MotorController::clampStepDelay(uint16_t stepDelay) const {
    if (stepDelay < MotorSpeed::MINIMUM_DELAY) {
        return MotorSpeed::MINIMUM_DELAY;
    }
    return stepDelay;
}
