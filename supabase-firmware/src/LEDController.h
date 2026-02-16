/**
 * @file LEDController.h
 * @brief RGB LED indicator controller for Brevitest device status communication
 *
 * Manages the external RGB LED on the Particle B-Series SoM for status
 * indication. Provides pattern-based animated indicators using update() polling.
 */

#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include "Particle.h"
#include "DataTypes.h"

// ============================================================================
// LED COLOR DEFINITIONS
// ============================================================================

namespace LEDColors {
    constexpr uint32_t RED      = 0xFF0000;
    constexpr uint32_t GREEN    = 0x00FF00;
    constexpr uint32_t BLUE     = 0x0000FF;
    constexpr uint32_t ORANGE   = 0xFF6600;
    constexpr uint32_t YELLOW   = 0xFFFF00;
    constexpr uint32_t CYAN     = 0x00FFFF;
    constexpr uint32_t MAGENTA  = 0xFF00FF;
    constexpr uint32_t WHITE    = 0xFFFFFF;
    constexpr uint32_t OFF      = 0x000000;

    inline uint8_t getRed(uint32_t color)   { return (color >> 16) & 0xFF; }
    inline uint8_t getGreen(uint32_t color) { return (color >> 8) & 0xFF; }
    inline uint8_t getBlue(uint32_t color)  { return color & 0xFF; }
    inline uint32_t pack(uint8_t r, uint8_t g, uint8_t b) {
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
}

// ============================================================================
// LED PATTERN DEFINITIONS
// ============================================================================

/**
 * @note Prefixed with Brvt to avoid collision with Particle SDK's LEDPattern
 */
enum class BrvtLEDPattern : uint8_t {
    SOLID = 0,
    BLINK = 1,
    FADE = 2,
    PULSE = 3,
    FAST_BLINK = 4
};

enum class BrvtLEDSpeed : uint8_t {
    SLOW = 0,       ///< 2000ms period
    NORMAL = 1,     ///< 1000ms period
    FAST = 2        ///< 500ms period
};

// ============================================================================
// LED TIMING CONSTANTS
// ============================================================================

namespace LEDTiming {
    constexpr uint16_t PERIOD_SLOW = 2000;
    constexpr uint16_t PERIOD_NORMAL = 1000;
    constexpr uint16_t PERIOD_FAST = 500;
    constexpr uint8_t PULSE_MIN_BRIGHTNESS = 30;
    constexpr uint8_t PULSE_MAX_BRIGHTNESS = 100;
    constexpr uint16_t MIN_UPDATE_INTERVAL_MS = 20;
}

// ============================================================================
// LED CONTROLLER NAMESPACE
// ============================================================================

namespace LEDController {

    bool init();

    /**
     * @brief Set LED color and animation pattern
     * @param color RGB color value (0xRRGGBB)
     * @param pattern Animation pattern
     * @param speed Animation speed
     */
    void setPattern(uint32_t color, BrvtLEDPattern pattern, BrvtLEDSpeed speed);

    /**
     * @brief Update LED animations (call in loop)
     */
    void update();

} // namespace LEDController

#endif // LED_CONTROLLER_H
