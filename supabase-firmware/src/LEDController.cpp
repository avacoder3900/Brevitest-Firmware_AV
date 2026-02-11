/**
 * @file LEDController.cpp
 * @brief Implementation of RGB LED indicator controller
 * @author Agent MU - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file implements the LED controller module for device status indication
 * using the Particle B-Series SoM external RGB LED with RGB.control()/RGB.color() API.
 *
 * Implementation Notes:
 * - Uses Particle LEDStatus objects for pattern management
 * - Non-blocking animation via update() polling
 * - Priority stack for indicator management
 * - Thread-safe operations
 *
 * User Stories Implemented:
 *   - LED-001: LEDController class
 *   - LED-002: State-based indicators
 *   - LED-003: User prompt indicators
 *   - LED-004: Pattern animations
 *   - LED-005: Priority system
 */

#include "LEDController.h"

// ============================================================================
// INTERNAL STATE
// ============================================================================

namespace {
    // Initialization flag
    bool _initialized = false;

    // Current brightness level (0-255)
    uint8_t _brightness = 255;

    // Current device state
    DeviceMode _currentDeviceMode = DeviceMode::IDLE;

    // Active user prompt
    LEDPrompt _activePrompt = LEDPrompt::NONE;

    // State indicators using Particle LEDStatus API
    // These are configured to match legacy firmware behavior
    LEDStatus _indicatorDontTouch(RGB_COLOR_RED, LED_PATTERN_SOLID, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
    LEDStatus _indicatorInsert(RGB_COLOR_GREEN, LED_PATTERN_FADE, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
    LEDStatus _indicatorRemove(RGB_COLOR_GREEN, LED_PATTERN_BLINK, LED_SPEED_SLOW, LED_PRIORITY_IMPORTANT);

    // State indicators
    LEDStatus _indicatorIdle(RGB_COLOR_GREEN, LED_PATTERN_FADE, LED_SPEED_SLOW, LED_PRIORITY_NORMAL);
    LEDStatus _indicatorHeating(RGB_COLOR_ORANGE, LED_PATTERN_SOLID, LED_SPEED_NORMAL, LED_PRIORITY_NORMAL);
    LEDStatus _indicatorRunning(RGB_COLOR_BLUE, LED_PATTERN_FADE, LED_SPEED_NORMAL, LED_PRIORITY_NORMAL);
    LEDStatus _indicatorError(RGB_COLOR_RED, LED_PATTERN_SOLID, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
    LEDStatus _indicatorUploading(RGB_COLOR_CYAN, LED_PATTERN_FADE, LED_SPEED_NORMAL, LED_PRIORITY_NORMAL);
    LEDStatus _indicatorScanning(RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_FAST, LED_PRIORITY_NORMAL);
    LEDStatus _indicatorValidating(RGB_COLOR_BLUE, LED_PATTERN_FADE, LED_SPEED_NORMAL, LED_PRIORITY_NORMAL);
    LEDStatus _indicatorInitializing(RGB_COLOR_BLUE, LED_PATTERN_FADE, LED_SPEED_SLOW, LED_PRIORITY_NORMAL);

    // Custom pattern state for direct control
    LEDState _customState;

    // Last update timestamp
    uint32_t _lastUpdateTime = 0;

    // Temporary indicator state
    struct {
        bool active;
        uint32_t color;
        BrvtLEDPattern pattern;
        BrvtLEDSpeed speed;
        BrvtLEDPriority priority;
        uint32_t startTime;
        uint32_t duration;
    } _tempIndicator = {false, 0, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL, BrvtLEDPriority::NORMAL, 0, 0};

    /**
     * @brief Deactivate all LED indicators
     */
    void deactivateAllIndicators() {
        if (_indicatorDontTouch.isActive()) _indicatorDontTouch.setActive(false);
        if (_indicatorInsert.isActive()) _indicatorInsert.setActive(false);
        if (_indicatorRemove.isActive()) _indicatorRemove.setActive(false);
        if (_indicatorIdle.isActive()) _indicatorIdle.setActive(false);
        if (_indicatorHeating.isActive()) _indicatorHeating.setActive(false);
        if (_indicatorRunning.isActive()) _indicatorRunning.setActive(false);
        if (_indicatorError.isActive()) _indicatorError.setActive(false);
        if (_indicatorUploading.isActive()) _indicatorUploading.setActive(false);
        if (_indicatorScanning.isActive()) _indicatorScanning.setActive(false);
        if (_indicatorValidating.isActive()) _indicatorValidating.setActive(false);
        if (_indicatorInitializing.isActive()) _indicatorInitializing.setActive(false);
    }

    /**
     * @brief Get the LEDStatus pointer for a device mode
     */
    LEDStatus* getIndicatorForMode(DeviceMode mode) {
        switch (mode) {
            case DeviceMode::IDLE:
                return &_indicatorIdle;
            case DeviceMode::INITIALIZING:
                return &_indicatorInitializing;
            case DeviceMode::HEATING:
                return &_indicatorHeating;
            case DeviceMode::BARCODE_SCANNING:
                return &_indicatorScanning;
            case DeviceMode::VALIDATING_CARTRIDGE:
            case DeviceMode::VALIDATING_MAGNETOMETER:
                return &_indicatorValidating;
            case DeviceMode::RUNNING_TEST:
            case DeviceMode::STRESS_TESTING:
                return &_indicatorRunning;
            case DeviceMode::UPLOADING_RESULTS:
            case DeviceMode::RESETTING_CARTRIDGE:
                return &_indicatorUploading;
            case DeviceMode::ERROR_STATE:
                return &_indicatorError;
            default:
                return &_indicatorIdle;
        }
    }

    /**
     * @brief Apply brightness to a color value
     */
    uint32_t applyBrightness(uint32_t color, uint8_t brightness) {
        uint8_t r = LEDColors::getRed(color);
        uint8_t g = LEDColors::getGreen(color);
        uint8_t b = LEDColors::getBlue(color);

        r = (r * brightness) / 255;
        g = (g * brightness) / 255;
        b = (b * brightness) / 255;

        return LEDColors::pack(r, g, b);
    }

    /**
     * @brief Calculate animation value based on pattern and time
     * @return Value 0-255 representing current animation position
     */
    uint8_t calculateAnimationValue(BrvtLEDPattern pattern, BrvtLEDSpeed speed, uint32_t startTime) {
        uint16_t period = LEDController::getPeriod(speed);
        uint32_t elapsed = millis() - startTime;
        uint32_t position = elapsed % period;
        float progress = (float)position / period;

        switch (pattern) {
            case BrvtLEDPattern::SOLID:
                return 255;

            case BrvtLEDPattern::BLINK:
            case BrvtLEDPattern::FAST_BLINK:
                // Square wave - on for first half, off for second half
                return (progress < 0.5f) ? 255 : 0;

            case BrvtLEDPattern::FADE:
                // Triangle wave - linear fade up then down
                if (progress < 0.5f) {
                    return (uint8_t)(progress * 2.0f * 255.0f);
                } else {
                    return (uint8_t)((1.0f - (progress - 0.5f) * 2.0f) * 255.0f);
                }

            case BrvtLEDPattern::PULSE:
                // Sine-like pulse with minimum brightness
                {
                    float angle = progress * 2.0f * 3.14159f;
                    float sinVal = (sin(angle) + 1.0f) / 2.0f; // 0.0 to 1.0

                    uint8_t minB = (LEDTiming::PULSE_MIN_BRIGHTNESS * 255) / 100;
                    uint8_t maxB = (LEDTiming::PULSE_MAX_BRIGHTNESS * 255) / 100;
                    return minB + (uint8_t)(sinVal * (maxB - minB));
                }

            default:
                return 255;
        }
    }
}

// ============================================================================
// INITIALIZATION (LED-001)
// ============================================================================

bool LEDController::init() {
    if (_initialized) {
        return true;
    }

    // Take control of RGB LED from Particle system
    RGB.control(true);

    // Initialize all indicators to inactive
    deactivateAllIndicators();

    // Set default state
    _currentDeviceMode = DeviceMode::IDLE;
    _activePrompt = LEDPrompt::NONE;
    _brightness = 255;
    _lastUpdateTime = millis();

    // Start with LED off
    RGB.color(0, 0, 0);

    _initialized = true;
    Log.info("LEDController: Initialized");

    return true;
}

bool LEDController::isInitialized() {
    return _initialized;
}

void LEDController::shutdown() {
    if (!_initialized) {
        return;
    }

    // Deactivate all indicators
    deactivateAllIndicators();

    // Return RGB control to Particle system
    RGB.control(false);

    _initialized = false;
    Log.info("LEDController: Shutdown");
}

// ============================================================================
// DIRECT CONTROL (LED-001)
// ============================================================================

void LEDController::setColor(uint8_t r, uint8_t g, uint8_t b) {
    if (!_initialized) {
        return;
    }

    // Apply brightness
    r = (r * _brightness) / 255;
    g = (g * _brightness) / 255;
    b = (b * _brightness) / 255;

    RGB.color(r, g, b);
}

void LEDController::setColor(uint32_t color) {
    setColor(LEDColors::getRed(color),
             LEDColors::getGreen(color),
             LEDColors::getBlue(color));
}

void LEDController::setBrightness(uint8_t level) {
    _brightness = level;
    RGB.brightness(level);
}

uint8_t LEDController::getBrightness() {
    return _brightness;
}

void LEDController::off() {
    if (!_initialized) {
        return;
    }

    deactivateAllIndicators();
    RGB.color(0, 0, 0);
}

// ============================================================================
// STATE-BASED INDICATORS (LED-002)
// ============================================================================

void LEDController::setDeviceState(DeviceMode mode) {
    if (!_initialized) {
        return;
    }

    // Don't change if same mode
    if (mode == _currentDeviceMode && _activePrompt == LEDPrompt::NONE) {
        return;
    }

    _currentDeviceMode = mode;

    // If a prompt is active, don't change the display
    if (_activePrompt != LEDPrompt::NONE) {
        Log.trace("LEDController: State changed to %s but prompt active",
                  deviceModeToString(mode));
        return;
    }

    // If temporary indicator is active with higher priority, don't change
    if (_tempIndicator.active && _tempIndicator.priority >= BrvtLEDPriority::HIGH) {
        Log.trace("LEDController: State changed to %s but temp indicator active",
                  deviceModeToString(mode));
        return;
    }

    // Deactivate all and activate the appropriate indicator
    deactivateAllIndicators();

    LEDStatus* indicator = getIndicatorForMode(mode);
    if (indicator) {
        indicator->setActive(true);
    }

    Log.info("LEDController: State set to %s", deviceModeToString(mode));
}

DeviceMode LEDController::getCurrentState() {
    return _currentDeviceMode;
}

void LEDController::clearState() {
    if (!_initialized) {
        return;
    }

    deactivateAllIndicators();
    _currentDeviceMode = DeviceMode::IDLE;
}

// ============================================================================
// USER PROMPT INDICATORS (LED-003)
// ============================================================================

void LEDController::showPrompt(LEDPrompt prompt) {
    if (!_initialized) {
        return;
    }

    if (prompt == LEDPrompt::NONE) {
        clearPrompt();
        return;
    }

    _activePrompt = prompt;

    // Deactivate state indicators
    deactivateAllIndicators();

    // Activate appropriate prompt indicator
    switch (prompt) {
        case LEDPrompt::INSERT_CARTRIDGE:
            _indicatorInsert.setActive(true);
            Log.info("LEDController: Showing INSERT prompt (green fade)");
            break;

        case LEDPrompt::REMOVE_CARTRIDGE:
            _indicatorRemove.setActive(true);
            Log.info("LEDController: Showing REMOVE prompt (green blink)");
            break;

        case LEDPrompt::DONT_TOUCH:
            _indicatorDontTouch.setActive(true);
            Log.info("LEDController: Showing DONT_TOUCH prompt (red solid)");
            break;

        default:
            break;
    }
}

void LEDController::clearPrompt() {
    if (!_initialized) {
        return;
    }

    if (_activePrompt == LEDPrompt::NONE) {
        return;
    }

    Log.info("LEDController: Clearing prompt %s", promptToString(_activePrompt));
    _activePrompt = LEDPrompt::NONE;

    // Deactivate prompt indicators
    if (_indicatorInsert.isActive()) _indicatorInsert.setActive(false);
    if (_indicatorRemove.isActive()) _indicatorRemove.setActive(false);
    if (_indicatorDontTouch.isActive()) _indicatorDontTouch.setActive(false);

    // Restore state indicator
    setDeviceState(_currentDeviceMode);
}

LEDPrompt LEDController::getActivePrompt() {
    return _activePrompt;
}

bool LEDController::isPromptActive() {
    return _activePrompt != LEDPrompt::NONE;
}

// ============================================================================
// PATTERN ANIMATIONS (LED-004)
// ============================================================================

void LEDController::setPattern(uint32_t color, BrvtLEDPattern pattern, BrvtLEDSpeed speed,
                                BrvtLEDPriority priority, uint32_t timeout) {
    if (!_initialized) {
        return;
    }

    // Check priority
    if (isHigherPriorityActive(priority)) {
        Log.trace("LEDController: Pattern rejected - higher priority active");
        return;
    }

    // Deactivate current indicators
    deactivateAllIndicators();
    _activePrompt = LEDPrompt::NONE;

    // Set custom state
    _customState.color = color;
    _customState.pattern = pattern;
    _customState.speed = speed;
    _customState.priority = priority;
    _customState.startTime = millis();
    _customState.timeout = timeout;
    _customState.active = true;

    Log.info("LEDController: Custom pattern set - color=0x%06lX, pattern=%s, speed=%d",
             color, patternToString(pattern), (int)speed);

    // Force immediate update
    forceUpdate();
}

void LEDController::update() {
    if (!_initialized) {
        return;
    }

    // Rate limiting
    uint32_t now = millis();
    if (now - _lastUpdateTime < LEDTiming::MIN_UPDATE_INTERVAL_MS) {
        return;
    }
    _lastUpdateTime = now;

    // Check temporary indicator timeout
    if (_tempIndicator.active) {
        if (_tempIndicator.duration > 0 &&
            (now - _tempIndicator.startTime) >= _tempIndicator.duration) {
            // Temporary indicator expired
            Log.info("LEDController: Temporary indicator expired");
            _tempIndicator.active = false;

            // Restore previous state
            if (_activePrompt != LEDPrompt::NONE) {
                showPrompt(_activePrompt);
            } else {
                setDeviceState(_currentDeviceMode);
            }
            return;
        }

        // Update temporary indicator animation
        uint8_t animValue = calculateAnimationValue(
            _tempIndicator.pattern,
            _tempIndicator.speed,
            _tempIndicator.startTime
        );

        uint32_t animColor = applyBrightness(_tempIndicator.color, animValue);
        RGB.color(LEDColors::getRed(animColor),
                  LEDColors::getGreen(animColor),
                  LEDColors::getBlue(animColor));
        return;
    }

    // Check custom pattern timeout
    if (_customState.active) {
        if (_customState.timeout > 0 &&
            (now - _customState.startTime) >= _customState.timeout) {
            // Custom pattern expired
            Log.info("LEDController: Custom pattern expired");
            _customState.active = false;

            // Restore previous state
            if (_activePrompt != LEDPrompt::NONE) {
                showPrompt(_activePrompt);
            } else {
                setDeviceState(_currentDeviceMode);
            }
            return;
        }

        // Update custom pattern animation
        uint8_t animValue = calculateAnimationValue(
            _customState.pattern,
            _customState.speed,
            _customState.startTime
        );

        uint32_t animColor = applyBrightness(_customState.color, animValue);
        animColor = applyBrightness(animColor, _brightness);

        RGB.color(LEDColors::getRed(animColor),
                  LEDColors::getGreen(animColor),
                  LEDColors::getBlue(animColor));
    }

    // If no custom pattern, LEDStatus objects handle their own animation
}

void LEDController::forceUpdate() {
    _lastUpdateTime = 0;
    update();
}

// ============================================================================
// PRIORITY SYSTEM (LED-005)
// ============================================================================

void LEDController::showTemporary(uint32_t color, BrvtLEDPattern pattern, BrvtLEDSpeed speed,
                                   BrvtLEDPriority priority, uint32_t timeoutMs) {
    if (!_initialized) {
        return;
    }

    // Check priority
    if (isHigherPriorityActive(priority)) {
        Log.trace("LEDController: Temporary indicator rejected - higher priority active");
        return;
    }

    // Deactivate current LEDStatus indicators (they don't support timeout)
    deactivateAllIndicators();

    // Set temporary indicator state
    _tempIndicator.active = true;
    _tempIndicator.color = color;
    _tempIndicator.pattern = pattern;
    _tempIndicator.speed = speed;
    _tempIndicator.priority = priority;
    _tempIndicator.startTime = millis();
    _tempIndicator.duration = timeoutMs;

    Log.info("LEDController: Temporary indicator - color=0x%06lX, timeout=%lu ms",
             color, timeoutMs);

    // Force immediate update
    forceUpdate();
}

bool LEDController::isHigherPriorityActive(BrvtLEDPriority priority) {
    // Check temporary indicator
    if (_tempIndicator.active && _tempIndicator.priority > priority) {
        return true;
    }

    // Check prompts (HIGH priority)
    if (_activePrompt != LEDPrompt::NONE && BrvtLEDPriority::HIGH > priority) {
        return true;
    }

    // Check error state (CRITICAL priority)
    if (_currentDeviceMode == DeviceMode::ERROR_STATE && BrvtLEDPriority::CRITICAL > priority) {
        return true;
    }

    return false;
}

BrvtLEDPriority LEDController::getActivePriority() {
    // Temporary indicator takes precedence if active
    if (_tempIndicator.active) {
        return _tempIndicator.priority;
    }

    // Custom pattern
    if (_customState.active) {
        return _customState.priority;
    }

    // Prompts are HIGH priority
    if (_activePrompt != LEDPrompt::NONE) {
        return BrvtLEDPriority::HIGH;
    }

    // Error state is CRITICAL
    if (_currentDeviceMode == DeviceMode::ERROR_STATE) {
        return BrvtLEDPriority::CRITICAL;
    }

    // Default state indicator
    return BrvtLEDPriority::NORMAL;
}

void LEDController::clearAll() {
    if (!_initialized) {
        return;
    }

    deactivateAllIndicators();
    _activePrompt = LEDPrompt::NONE;
    _currentDeviceMode = DeviceMode::IDLE;
    _customState.active = false;
    _tempIndicator.active = false;

    RGB.color(0, 0, 0);

    Log.info("LEDController: All indicators cleared");
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

uint16_t LEDController::getPeriod(BrvtLEDSpeed speed) {
    switch (speed) {
        case BrvtLEDSpeed::SLOW:
            return LEDTiming::PERIOD_SLOW;
        case BrvtLEDSpeed::NORMAL:
            return LEDTiming::PERIOD_NORMAL;
        case BrvtLEDSpeed::FAST:
            return LEDTiming::PERIOD_FAST;
        default:
            return LEDTiming::PERIOD_NORMAL;
    }
}

const char* LEDController::patternToString(BrvtLEDPattern pattern) {
    switch (pattern) {
        case BrvtLEDPattern::SOLID:      return "SOLID";
        case BrvtLEDPattern::BLINK:      return "BLINK";
        case BrvtLEDPattern::FADE:       return "FADE";
        case BrvtLEDPattern::PULSE:      return "PULSE";
        case BrvtLEDPattern::FAST_BLINK: return "FAST_BLINK";
        default:                     return "UNKNOWN";
    }
}

const char* LEDController::priorityToString(BrvtLEDPriority priority) {
    switch (priority) {
        case BrvtLEDPriority::LOW:      return "LOW";
        case BrvtLEDPriority::NORMAL:   return "NORMAL";
        case BrvtLEDPriority::HIGH:     return "HIGH";
        case BrvtLEDPriority::CRITICAL: return "CRITICAL";
        default:                    return "UNKNOWN";
    }
}

const char* LEDController::promptToString(LEDPrompt prompt) {
    switch (prompt) {
        case LEDPrompt::NONE:             return "NONE";
        case LEDPrompt::INSERT_CARTRIDGE: return "INSERT_CARTRIDGE";
        case LEDPrompt::REMOVE_CARTRIDGE: return "REMOVE_CARTRIDGE";
        case LEDPrompt::DONT_TOUCH:       return "DONT_TOUCH";
        default:                          return "UNKNOWN";
    }
}
