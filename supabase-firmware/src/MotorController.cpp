/**
 * @file MotorController.cpp
 * @brief Stepper motor controller implementation
 * @author Agent ETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * Implementation of MotorController class for Brevitest linear stage.
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
    _cancelled(false),
    _oscillating(false),
    _oscillationAmplitude(0),
    _oscillationDelay(MotorSpeed::OSCILLATION),
    _oscillationForward(true),
    _oscillationStart(0),
    _oscillationCallback(nullptr)
{
}

MotorController::~MotorController() {
    // Ensure motor is disabled on destruction
    if (_initialized) {
        disable();
    }
}

// ============================================================================
// INITIALIZATION (MOT-001)
// ============================================================================

bool MotorController::init() {
    if (_initialized) {
        return true;
    }

    // Ensure HAL is initialized (motor pins are configured in HAL::init())
    if (!HAL::isInitialized()) {
        if (!HAL::init()) {
            Log.error("MotorController: HAL initialization failed");
            return false;
        }
    }

    // Motor starts disabled (sleeping)
    _initialized = true;
    _homed = false;
    _position = 0;
    _positionError = 0;
    _state = MotorState::IDLE;
    _cancelled = false;
    _oscillating = false;

    Log.info("MotorController: Initialized");
    return true;
}

bool MotorController::isInitialized() const {
    return _initialized;
}

void MotorController::enable() {
    if (!_initialized) return;
    HAL::motorWake();
    Log.trace("MotorController: Motor enabled");
}

void MotorController::disable() {
    if (!_initialized) return;
    HAL::motorSleep();
    Log.trace("MotorController: Motor disabled");
}

bool MotorController::isEnabled() const {
    return _initialized && HAL::isMotorAwake();
}

// ============================================================================
// POSITION TRACKING (MOT-002)
// ============================================================================

int32_t MotorController::getCurrentPosition() const {
    return _position;
}

void MotorController::setPosition(int32_t position) {
    _position = position;
    _positionError = 0;
    Log.info("MotorController: Position set to %ld microns", position);
}

bool MotorController::isValidPosition(int32_t position) const {
    return (position >= StagePosition::MIN && position <= StagePosition::MAX);
}

bool MotorController::isHomed() const {
    return _homed;
}

int16_t MotorController::getPositionError() const {
    return _positionError;
}

// ============================================================================
// HOMING (MOT-003)
// ============================================================================

MotorResult MotorController::home(bool sleepAfter) {
    if (!_initialized) {
        Log.error("MotorController: Cannot home - not initialized");
        return MotorResult::ERROR;
    }

    Log.info("MotorController: Starting homing sequence");
    _state = MotorState::HOMING;
    _cancelled = false;

    // Wake motor if sleeping
    if (!isEnabled()) {
        enable();
    }

    // Move until limit switch is triggered
    MotorResult result = moveUntilLimit(MotorSpeed::RESET);

    if (result == MotorResult::AT_LIMIT || result == MotorResult::SUCCESS) {
        // Set position to 0 at limit
        _position = 0;
        _positionError = 0;
        Log.info("MotorController: At limit, position zeroed");

        // Move to initial position
        result = moveRelative(StagePosition::INITIAL, MotorSpeed::RESET);

        if (result == MotorResult::SUCCESS) {
            _homed = true;
            Log.info("MotorController: Homing complete, position = %ld", _position);
        }
    }

    // Sleep motor if requested
    if (sleepAfter) {
        disable();
    }

    _state = MotorState::IDLE;
    return result;
}

MotorResult MotorController::reset() {
    return home(true);
}

bool MotorController::isAtLimit() const {
    return HAL::readLimitSwitch();
}

// ============================================================================
// ABSOLUTE POSITIONING (MOT-004)
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
        return MotorResult::SUCCESS;  // Already at position
    }

    Log.trace("MotorController: Moving to %ld (distance %ld)", position, distance);
    return moveRelative(distance, stepDelay);
}

MotorResult MotorController::moveToPositionAsync(int32_t position, uint16_t stepDelay, MotorCallback callback) {
    // Currently implemented as blocking with callback
    MotorResult result = moveToPosition(position, stepDelay);

    if (callback != nullptr) {
        callback(result, _position);
    }

    return result;
}

// ============================================================================
// RELATIVE MOVEMENT (MOT-005)
// ============================================================================

MotorResult MotorController::moveRelative(int32_t microns, uint16_t stepDelay) {
    if (!_initialized) {
        return MotorResult::ERROR;
    }

    // Check if movement is cancelled
    if (_cancelled) {
        _cancelled = false;
        return MotorResult::CANCELLED;
    }

    // Wake motor if sleeping
    if (!isEnabled()) {
        enable();
    }

    // Determine direction: negative microns = proximal (toward limit), positive = distal
    bool forward = (microns >= 0);
    int32_t absMicrons = forward ? microns : -microns;

    // Add accumulated error from previous movements
    absMicrons += _positionError;

    // Calculate number of 1/8 steps
    int32_t steps = absMicrons / MOTOR_MICRONS_PER_EIGHTH_STEP;
    _positionError = absMicrons % MOTOR_MICRONS_PER_EIGHTH_STEP;

    // Clamp step delay to minimum
    stepDelay = clampStepDelay(stepDelay);

    // Set direction
    // In legacy: HIGH = proximal (toward limit), LOW = distal (away from limit)
    HAL::setMotorDirection(!forward);  // Inverted: forward=true means distal=LOW

    _state = MotorState::MOVING;

    // Execute steps
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

MotorResult MotorController::stepRelative(int32_t steps, uint16_t stepDelay) {
    // Convert steps to microns and use moveRelative
    int32_t microns = steps * MOTOR_MICRONS_PER_EIGHTH_STEP;
    return moveRelative(microns, stepDelay);
}

bool MotorController::stepOnce(bool forward, uint16_t stepDelay) {
    if (!_initialized) {
        return false;
    }

    // Wake motor if sleeping
    if (!isEnabled()) {
        enable();
    }

    // Set direction
    HAL::setMotorDirection(!forward);

    return executeStep(forward, clampStepDelay(stepDelay));
}

// ============================================================================
// OSCILLATION MODE (MOT-006)
// ============================================================================

MotorResult MotorController::oscillate(int32_t amplitude, uint16_t stepDelay, uint16_t cycles) {
    if (!_initialized) {
        return MotorResult::ERROR;
    }

    if (amplitude == 0 || cycles == 0) {
        return MotorResult::SUCCESS;  // Nothing to do
    }

    // Wake motor if sleeping
    if (!isEnabled()) {
        enable();
    }

    _state = MotorState::OSCILLATING;
    _cancelled = false;

    for (uint16_t i = 0; i < cycles; i++) {
        // Move forward
        MotorResult result = moveRelative(amplitude, stepDelay);
        if (result != MotorResult::SUCCESS || _cancelled) {
            _state = MotorState::IDLE;
            return _cancelled ? MotorResult::CANCELLED : result;
        }

        // Allow system to process (important for BCODE/cloud operations)
        Particle.process();

        // Move backward
        result = moveRelative(-amplitude, stepDelay);
        if (result != MotorResult::SUCCESS || _cancelled) {
            _state = MotorState::IDLE;
            return _cancelled ? MotorResult::CANCELLED : result;
        }

        // Allow system to process
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

MotorResult MotorController::startOscillation(int32_t amplitude, uint16_t stepDelay, OscillationCallback callback) {
    if (!_initialized) {
        return MotorResult::ERROR;
    }

    // Wake motor if sleeping
    if (!isEnabled()) {
        enable();
    }

    _oscillating = true;
    _oscillationAmplitude = amplitude;
    _oscillationDelay = clampStepDelay(stepDelay);
    _oscillationForward = true;
    _oscillationStart = _position;
    _oscillationCallback = callback;
    _state = MotorState::OSCILLATING;

    Log.info("MotorController: Starting oscillation, amplitude=%ld, delay=%d", amplitude, stepDelay);
    return MotorResult::SUCCESS;
}

void MotorController::stopOscillation() {
    _oscillating = false;
    _state = MotorState::IDLE;
    _oscillationCallback = nullptr;
    Log.info("MotorController: Oscillation stopped");
}

bool MotorController::isOscillating() const {
    return _oscillating;
}

void MotorController::updateOscillation() {
    if (!_oscillating || _state != MotorState::OSCILLATING) {
        return;
    }

    // Perform oscillation movement
    int32_t target;
    if (_oscillationForward) {
        target = _oscillationStart + _oscillationAmplitude;
    } else {
        target = _oscillationStart - _oscillationAmplitude;
    }

    // Move one step toward target
    int32_t distance = target - _position;
    if (distance == 0 || (distance > 0) != _oscillationForward) {
        // Reached target or passed it, reverse direction
        _oscillationForward = !_oscillationForward;

        if (_oscillationCallback != nullptr) {
            _oscillationCallback(_oscillationForward, _position);
        }
    } else {
        // Take one step
        stepOnce(_oscillationForward, _oscillationDelay);
    }
}

// ============================================================================
// PREDEFINED POSITIONS (MOT-007)
// ============================================================================

MotorResult MotorController::moveToHome(uint16_t stepDelay) {
    return moveToPosition(StagePosition::HOME, stepDelay);
}

MotorResult MotorController::moveToTestStart(uint16_t stepDelay) {
    return moveToPosition(StagePosition::TEST_START, stepDelay);
}

MotorResult MotorController::moveToOpticalRead(uint16_t stepDelay) {
    return moveToPosition(StagePosition::OPTICAL_READ, stepDelay);
}

MotorResult MotorController::moveToMagnetometer(uint16_t stepDelay) {
    return moveToPosition(StagePosition::MAGNETOMETER, stepDelay);
}

MotorResult MotorController::moveToShippingBolt(uint16_t stepDelay) {
    return moveToPosition(StagePosition::SHIPPING_BOLT, stepDelay);
}

// ============================================================================
// STATE AND DIAGNOSTICS
// ============================================================================

MotorState MotorController::getState() const {
    return _state;
}

void MotorController::cancel() {
    _cancelled = true;
    if (_oscillating) {
        stopOscillation();
    }
    Log.info("MotorController: Operation cancelled");
}

int32_t MotorController::micronsToSteps(int32_t microns) {
    return microns / MOTOR_MICRONS_PER_EIGHTH_STEP;
}

int32_t MotorController::stepsToMicrons(int32_t steps) {
    return steps * MOTOR_MICRONS_PER_EIGHTH_STEP;
}

uint16_t MotorController::calculateStepDelayForIntegration(uint8_t atime, uint16_t astep) {
    // Calculate integration time in microseconds: (ATIME + 1) * (ASTEP + 1) * 2.78
    uint32_t integrationTimeUs = (uint32_t)(atime + 1) * (uint32_t)(astep + 1) * 2780 / 1000;

    // Well length is SPECTRO_WELL_LENGTH (5000 microns)
    // Each step is MOTOR_MICRONS_PER_EIGHTH_STEP (25 microns)
    int32_t stepsNeeded = SPECTRO_WELL_LENGTH / MOTOR_MICRONS_PER_EIGHTH_STEP;  // 200 steps

    // Time per step in microseconds
    uint32_t timePerStepUs = integrationTimeUs / stepsNeeded;

    // Step delay is half the time per step (since step duration = 2 * step_delay)
    uint16_t calculatedDelay = (uint16_t)(timePerStepUs / 2);

    // Enforce minimum step delay
    if (calculatedDelay < MotorSpeed::MINIMUM_DELAY) {
        calculatedDelay = MotorSpeed::MINIMUM_DELAY;
    }

    return calculatedDelay;
}

// ============================================================================
// PRIVATE METHODS
// ============================================================================

MotorResult MotorController::moveUntilLimit(uint16_t stepDelay) {
    // Calculate maximum steps to prevent infinite loop
    // Maximum possible travel is from MAX to 0, plus some margin
    int32_t maxSteps = (StagePosition::MAX / MOTOR_MICRONS_PER_EIGHTH_STEP) + 40;

    stepDelay = clampStepDelay(stepDelay);

    // Set direction to proximal (toward limit switch)
    // In legacy: HIGH = proximal
    HAL::setMotorDirection(true);

    int32_t count = 0;
    while (!HAL::readLimitSwitchRaw() && count < maxSteps) {
        // Generate step pulse with timing
        digitalWrite(PIN_MOTOR_STEP, HIGH);
        delayMicroseconds(stepDelay);
        digitalWrite(PIN_MOTOR_STEP, LOW);
        delayMicroseconds(stepDelay);

        // Update position
        _position -= MOTOR_MICRONS_PER_EIGHTH_STEP;
        if (_position < 0) {
            _position = 0;
        }
        count++;
    }

    // Check if we hit the limit
    if (HAL::readLimitSwitchRaw()) {
        _position = 0;
        _positionError = 0;
        return MotorResult::AT_LIMIT;
    }

    // Timeout / max steps reached without hitting limit
    Log.warn("MotorController: moveUntilLimit timeout after %ld steps", count);
    return MotorResult::TIMEOUT;
}

bool MotorController::executeStep(bool forward, uint16_t stepDelay) {
    // Check limits before moving
    if (!forward) {
        // Moving proximal (toward limit)
        if (HAL::readLimitSwitchRaw()) {
            // Double-check with debounce
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
        // Moving distal (away from limit)
        if (_position >= StagePosition::MAX) {
            return false;
        }
    }

    // Generate step pulse
    digitalWrite(PIN_MOTOR_STEP, HIGH);
    delayMicroseconds(stepDelay);
    digitalWrite(PIN_MOTOR_STEP, LOW);
    delayMicroseconds(stepDelay);

    // Update position
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
