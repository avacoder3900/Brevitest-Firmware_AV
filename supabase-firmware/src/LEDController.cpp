/**
 * @file LEDController.cpp
 * @brief Implementation of RGB LED indicator controller
 *
 * Uses Particle's RGB.control()/RGB.color() API for custom LED patterns.
 * Non-blocking animation via update() polling.
 */

#include "LEDController.h"

// ============================================================================
// INTERNAL STATE
// ============================================================================

namespace {
    bool _initialized = false;
    uint8_t _brightness = 255;
    uint32_t _lastUpdateTime = 0;

    // Current pattern state
    struct {
        uint32_t color;
        BrvtLEDPattern pattern;
        BrvtLEDSpeed speed;
        uint32_t startTime;
        bool active;
    } _currentPattern = {LEDColors::OFF, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL, 0, false};

    uint32_t applyBrightness(uint32_t color, uint8_t brightness) {
        uint8_t r = (LEDColors::getRed(color) * brightness) / 255;
        uint8_t g = (LEDColors::getGreen(color) * brightness) / 255;
        uint8_t b = (LEDColors::getBlue(color) * brightness) / 255;
        return LEDColors::pack(r, g, b);
    }

    uint16_t getPeriod(BrvtLEDSpeed speed) {
        switch (speed) {
            case BrvtLEDSpeed::SLOW:   return LEDTiming::PERIOD_SLOW;
            case BrvtLEDSpeed::NORMAL: return LEDTiming::PERIOD_NORMAL;
            case BrvtLEDSpeed::FAST:   return LEDTiming::PERIOD_FAST;
            default:                   return LEDTiming::PERIOD_NORMAL;
        }
    }

    uint8_t calculateAnimationValue(BrvtLEDPattern pattern, BrvtLEDSpeed speed, uint32_t startTime) {
        uint16_t period = getPeriod(speed);
        uint32_t elapsed = millis() - startTime;
        uint32_t position = elapsed % period;
        float progress = (float)position / period;

        switch (pattern) {
            case BrvtLEDPattern::SOLID:
                return 255;

            case BrvtLEDPattern::BLINK:
            case BrvtLEDPattern::FAST_BLINK:
                return (progress < 0.5f) ? 255 : 0;

            case BrvtLEDPattern::FADE:
                if (progress < 0.5f) {
                    return (uint8_t)(progress * 2.0f * 255.0f);
                } else {
                    return (uint8_t)((1.0f - (progress - 0.5f) * 2.0f) * 255.0f);
                }

            case BrvtLEDPattern::PULSE:
                {
                    float angle = progress * 2.0f * 3.14159f;
                    float sinVal = (sin(angle) + 1.0f) / 2.0f;
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
// PUBLIC API
// ============================================================================

bool LEDController::init() {
    if (_initialized) {
        return true;
    }

    RGB.control(true);

    _brightness = 255;
    _lastUpdateTime = millis();
    _currentPattern.active = false;

    RGB.color(0, 0, 0);

    _initialized = true;
    Log.info("LEDController: Initialized");
    return true;
}

void LEDController::setPattern(uint32_t color, BrvtLEDPattern pattern, BrvtLEDSpeed speed) {
    if (!_initialized) {
        return;
    }

    _currentPattern.color = color;
    _currentPattern.pattern = pattern;
    _currentPattern.speed = speed;
    _currentPattern.startTime = millis();
    _currentPattern.active = true;

    // Force immediate update
    _lastUpdateTime = 0;
    update();
}

void LEDController::update() {
    if (!_initialized) {
        return;
    }

    uint32_t now = millis();
    if (now - _lastUpdateTime < LEDTiming::MIN_UPDATE_INTERVAL_MS) {
        return;
    }
    _lastUpdateTime = now;

    if (_currentPattern.active) {
        uint8_t animValue = calculateAnimationValue(
            _currentPattern.pattern,
            _currentPattern.speed,
            _currentPattern.startTime
        );

        uint32_t animColor = applyBrightness(_currentPattern.color, animValue);
        animColor = applyBrightness(animColor, _brightness);

        RGB.color(LEDColors::getRed(animColor),
                  LEDColors::getGreen(animColor),
                  LEDColors::getBlue(animColor));
    }
}
