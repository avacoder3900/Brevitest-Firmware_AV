/**
 * @file LaserController.cpp
 * @brief Implementation of PWM-controlled laser diode controller
 * @author Agent IOTA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * Implementation of the LaserController class for managing three laser
 * diode channels with safety features and pulsed operation mode.
 *
 * Hardware Platform: Particle B-Series SoM (NRF52840)
 */

#include "LaserController.h"
#include "HAL.h"

// ============================================================================
// CONSTRUCTOR
// ============================================================================

LaserController::LaserController()
    : _initialized(false)
    , _interlockEnabled(true)
    , _pulseModeEnabled(false)
    , _pulseOnTimeUs(LASER_DEFAULT_PULSE_ON_US)
    , _pulseCycleTimeUs(LASER_DEFAULT_CYCLE_US)
    , _laserOnStartTime(0)
    , _wasAnyLaserOn(false)
{
    // Initialize all channels to default power, disabled
    for (uint8_t i = 0; i < static_cast<uint8_t>(LaserChannel::COUNT); i++) {
        _power[i] = LASER_DEFAULT_POWER;
        _enabled[i] = false;
    }
}

// ============================================================================
// INITIALIZATION (LAS-001)
// ============================================================================

bool LaserController::init() {
    if (_initialized) {
        return true;
    }

    // Ensure all lasers are OFF at initialization (HAL::init() should have done this)
    HAL::setAllLasersOff();

    // Reset state
    for (uint8_t i = 0; i < static_cast<uint8_t>(LaserChannel::COUNT); i++) {
        _power[i] = LASER_DEFAULT_POWER;
        _enabled[i] = false;
    }

    // Configure default pulse mode
    _pulseOnTimeUs = LASER_DEFAULT_PULSE_ON_US;
    _pulseCycleTimeUs = LASER_DEFAULT_CYCLE_US;
    _pulseModeEnabled = true;

    // Reset safety tracking
    _laserOnStartTime = 0;
    _wasAnyLaserOn = false;
    _interlockEnabled = true;

    _initialized = true;

    Log.info("LaserController: Initialized (3 channels, interlock enabled)");
    return true;
}

bool LaserController::isInitialized() const {
    return _initialized;
}

// ============================================================================
// INDIVIDUAL LASER CONTROL (LAS-002)
// ============================================================================

bool LaserController::setLaserPower(LaserChannel channel, uint8_t power) {
    if (channel >= LaserChannel::COUNT) {
        return false;
    }

    uint8_t idx = static_cast<uint8_t>(channel);
    _power[idx] = power;

    // If currently enabled and power is 0, disable
    if (_enabled[idx] && power == 0) {
        disableLaser(channel);
    }

    return true;
}

bool LaserController::setLaserPower(char channel, uint8_t power) {
    LaserChannel ch;
    if (!charToChannel(channel, ch)) {
        return false;
    }
    return setLaserPower(ch, power);
}

uint8_t LaserController::getLaserPower(LaserChannel channel) const {
    if (channel >= LaserChannel::COUNT) {
        return 0;
    }
    return _power[static_cast<uint8_t>(channel)];
}

uint8_t LaserController::getLaserPower(char channel) const {
    LaserChannel ch;
    if (!charToChannel(channel, ch)) {
        return 0;
    }
    return getLaserPower(ch);
}

bool LaserController::enableLaser(LaserChannel channel) {
    if (!_initialized) {
        Log.warn("LaserController: Not initialized");
        return false;
    }

    if (channel >= LaserChannel::COUNT) {
        return false;
    }

    // Safety interlock check
    if (!isSafeToEnable()) {
        Log.warn("LaserController: Interlock active - cartridge not present");
        return false;
    }

    uint8_t idx = static_cast<uint8_t>(channel);

    // Only enable if power > 0
    if (_power[idx] == 0) {
        Log.warn("LaserController: Cannot enable channel %c with power=0", channelToChar(channel));
        return false;
    }

    _enabled[idx] = true;
    applyHardwareState(channel);
    updateOnTimeTracking();

    Log.trace("LaserController: Channel %c enabled (power=%d)", channelToChar(channel), _power[idx]);
    return true;
}

bool LaserController::enableLaser(char channel) {
    LaserChannel ch;
    if (!charToChannel(channel, ch)) {
        return false;
    }
    return enableLaser(ch);
}

bool LaserController::disableLaser(LaserChannel channel) {
    if (channel >= LaserChannel::COUNT) {
        return false;
    }

    uint8_t idx = static_cast<uint8_t>(channel);
    _enabled[idx] = false;
    applyHardwareState(channel);
    updateOnTimeTracking();

    Log.trace("LaserController: Channel %c disabled", channelToChar(channel));
    return true;
}

bool LaserController::disableLaser(char channel) {
    LaserChannel ch;
    if (!charToChannel(channel, ch)) {
        return false;
    }
    return disableLaser(ch);
}

bool LaserController::isLaserEnabled(LaserChannel channel) const {
    if (channel >= LaserChannel::COUNT) {
        return false;
    }
    return _enabled[static_cast<uint8_t>(channel)];
}

bool LaserController::isLaserEnabled(char channel) const {
    LaserChannel ch;
    if (!charToChannel(channel, ch)) {
        return false;
    }
    return isLaserEnabled(ch);
}

// ============================================================================
// ALL-LASER CONTROL (LAS-003)
// ============================================================================

bool LaserController::enableAllLasers() {
    if (!_initialized) {
        Log.warn("LaserController: Not initialized");
        return false;
    }

    if (!isSafeToEnable()) {
        Log.warn("LaserController: Interlock active - cartridge not present");
        return false;
    }

    bool allEnabled = true;
    for (uint8_t i = 0; i < static_cast<uint8_t>(LaserChannel::COUNT); i++) {
        LaserChannel ch = static_cast<LaserChannel>(i);
        if (_power[i] > 0) {
            _enabled[i] = true;
            applyHardwareState(ch);
        } else {
            allEnabled = false;
        }
    }

    updateOnTimeTracking();
    Log.trace("LaserController: All lasers enabled");
    return allEnabled;
}

void LaserController::disableAllLasers() {
    for (uint8_t i = 0; i < static_cast<uint8_t>(LaserChannel::COUNT); i++) {
        _enabled[i] = false;
    }

    // Use HAL bulk function for efficiency
    HAL::setAllLasersOff();
    updateOnTimeTracking();

    Log.trace("LaserController: All lasers disabled");
}

void LaserController::setAllLaserPower(uint8_t power) {
    for (uint8_t i = 0; i < static_cast<uint8_t>(LaserChannel::COUNT); i++) {
        _power[i] = power;
    }

    // If power is 0, disable all
    if (power == 0) {
        disableAllLasers();
    }
}

bool LaserController::areAllLasersEnabled() const {
    for (uint8_t i = 0; i < static_cast<uint8_t>(LaserChannel::COUNT); i++) {
        if (!_enabled[i]) {
            return false;
        }
    }
    return true;
}

bool LaserController::isAnyLaserEnabled() const {
    for (uint8_t i = 0; i < static_cast<uint8_t>(LaserChannel::COUNT); i++) {
        if (_enabled[i]) {
            return true;
        }
    }
    return false;
}

// ============================================================================
// PULSED MODE CONTROL (LAS-004)
// ============================================================================

bool LaserController::setPulseMode(uint32_t onTimeUs, uint32_t cycleTimeUs) {
    // Validate parameters
    if (onTimeUs < LASER_MIN_PULSE_ON_US || onTimeUs > LASER_MAX_PULSE_ON_US) {
        Log.warn("LaserController: Invalid pulse on-time %lu (range: %lu-%lu)",
                 onTimeUs, LASER_MIN_PULSE_ON_US, LASER_MAX_PULSE_ON_US);
        return false;
    }

    if (cycleTimeUs < LASER_MIN_CYCLE_US || cycleTimeUs > LASER_MAX_CYCLE_US) {
        Log.warn("LaserController: Invalid cycle time %lu (range: %lu-%lu)",
                 cycleTimeUs, LASER_MIN_CYCLE_US, LASER_MAX_CYCLE_US);
        return false;
    }

    // On-time must be less than cycle time
    if (onTimeUs >= cycleTimeUs) {
        Log.warn("LaserController: On-time (%lu) must be less than cycle time (%lu)",
                 onTimeUs, cycleTimeUs);
        return false;
    }

    _pulseOnTimeUs = onTimeUs;
    _pulseCycleTimeUs = cycleTimeUs;
    _pulseModeEnabled = true;

    // Calculate duty cycle for logging
    float dutyCycle = (float)onTimeUs / (float)cycleTimeUs * 100.0f;
    Log.info("LaserController: Pulse mode set - on=%luus, cycle=%luus (%.2f%% duty)",
             onTimeUs, cycleTimeUs, dutyCycle);

    return true;
}

uint32_t LaserController::getPulseOnTime() const {
    return _pulseOnTimeUs;
}

uint32_t LaserController::getPulseCycleTime() const {
    return _pulseCycleTimeUs;
}

bool LaserController::pulse(LaserChannel channel) {
    if (!_initialized) {
        return false;
    }

    if (channel >= LaserChannel::COUNT) {
        return false;
    }

    // Safety check
    if (!isSafeToEnable()) {
        Log.warn("LaserController: Pulse blocked - interlock active");
        return false;
    }

    char chChar = channelToChar(channel);

    // Turn on laser
    HAL::setLaser(chChar, true);

    // Wait for pulse on-time
    delayMicroseconds(_pulseOnTimeUs);

    // Turn off laser
    HAL::setLaser(chChar, false);

    return true;
}

bool LaserController::pulse(char channel) {
    LaserChannel ch;
    if (!charToChannel(channel, ch)) {
        return false;
    }
    return pulse(ch);
}

bool LaserController::pulseAll() {
    if (!_initialized) {
        return false;
    }

    // Safety check
    if (!isSafeToEnable()) {
        Log.warn("LaserController: Pulse blocked - interlock active");
        return false;
    }

    // Turn on all lasers
    HAL::setAllLasersOn();

    // Wait for pulse on-time
    delayMicroseconds(_pulseOnTimeUs);

    // Turn off all lasers
    HAL::setAllLasersOff();

    return true;
}

bool LaserController::isPulseModeEnabled() const {
    return _pulseModeEnabled;
}

// ============================================================================
// SAFETY FEATURES (LAS-005)
// ============================================================================

void LaserController::update() {
    if (!_initialized) {
        return;
    }

    // Check cartridge presence - disable lasers if removed
    if (_interlockEnabled && !isCartridgePresent()) {
        if (isAnyLaserEnabled()) {
            Log.warn("LaserController: Cartridge removed - disabling all lasers");
            disableAllLasers();
        }
    }

    // Check maximum continuous on-time
    if (isOvertime()) {
        Log.warn("LaserController: Maximum on-time exceeded (%lu ms) - disabling lasers",
                 LASER_MAX_CONTINUOUS_ON_MS);
        disableAllLasers();
    }
}

void LaserController::setInterlockEnabled(bool enabled) {
    _interlockEnabled = enabled;
    if (enabled) {
        Log.info("LaserController: Safety interlock ENABLED");
    } else {
        Log.warn("LaserController: Safety interlock DISABLED - use with caution!");
    }
}

bool LaserController::isInterlockEnabled() const {
    return _interlockEnabled;
}

bool LaserController::isCartridgePresent() const {
    return HAL::readCartridgeSwitch();
}

bool LaserController::isOvertime() const {
    if (!_wasAnyLaserOn || _laserOnStartTime == 0) {
        return false;
    }

    uint32_t duration = HAL::elapsedSince(_laserOnStartTime);
    return duration >= LASER_MAX_CONTINUOUS_ON_MS;
}

uint32_t LaserController::getContinuousOnTime() const {
    if (!_wasAnyLaserOn || _laserOnStartTime == 0) {
        return 0;
    }
    return HAL::elapsedSince(_laserOnStartTime);
}

void LaserController::emergencyStop() {
    Log.warn("LaserController: EMERGENCY STOP");

    // Disable all lasers immediately
    for (uint8_t i = 0; i < static_cast<uint8_t>(LaserChannel::COUNT); i++) {
        _enabled[i] = false;
    }
    HAL::setAllLasersOff();

    // Reset safety tracking
    _laserOnStartTime = 0;
    _wasAnyLaserOn = false;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

bool LaserController::charToChannel(char c, LaserChannel& channel) {
    switch (c) {
        case 'A':
        case 'a':
            channel = LaserChannel::A;
            return true;
        case 'B':
        case 'b':
            channel = LaserChannel::B;
            return true;
        case 'C':
        case 'c':
            channel = LaserChannel::C;
            return true;
        default:
            return false;
    }
}

char LaserController::channelToChar(LaserChannel channel) {
    switch (channel) {
        case LaserChannel::A:
            return 'A';
        case LaserChannel::B:
            return 'B';
        case LaserChannel::C:
            return 'C';
        default:
            return '?';
    }
}

hal_pin_t LaserController::getChannelPin(LaserChannel channel) {
    switch (channel) {
        case LaserChannel::A:
            return PIN_LASER_A;
        case LaserChannel::B:
            return PIN_LASER_B;
        case LaserChannel::C:
            return PIN_LASER_C;
        default:
            return 0;
    }
}

// ============================================================================
// PRIVATE HELPER METHODS
// ============================================================================

bool LaserController::isSafeToEnable() const {
    // If interlock is disabled, always safe
    if (!_interlockEnabled) {
        return true;
    }

    // Check cartridge presence
    return isCartridgePresent();
}

void LaserController::applyHardwareState(LaserChannel channel) {
    if (channel >= LaserChannel::COUNT) {
        return;
    }

    uint8_t idx = static_cast<uint8_t>(channel);
    char chChar = channelToChar(channel);

    // Current hardware is digital on/off (no PWM)
    // Laser is ON if enabled and power > 0
    bool shouldBeOn = _enabled[idx] && (_power[idx] > 0);

    HAL::setLaser(chChar, shouldBeOn);
}

void LaserController::updateOnTimeTracking() {
    bool anyOn = isAnyLaserEnabled();

    if (anyOn && !_wasAnyLaserOn) {
        // Lasers just turned on - start tracking
        _laserOnStartTime = millis();
    } else if (!anyOn && _wasAnyLaserOn) {
        // Lasers just turned off - reset tracking
        _laserOnStartTime = 0;
    }

    _wasAnyLaserOn = anyOn;
}
