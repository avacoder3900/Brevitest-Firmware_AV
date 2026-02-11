/**
 * @file LEDController.h
 * @brief RGB LED indicator controller for Brevitest device status communication
 * @author Agent MU - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module manages the external RGB LED on the Particle B-Series SoM for status
 * indication. It provides state-based indicators, user prompt patterns, and
 * a priority system for display management.
 *
 * Features:
 * - State-based automatic LED indicators
 * - User prompt indicators (insert/remove/don't touch)
 * - Animated patterns (fade, blink, pulse)
 * - Priority-based display with override capability
 * - Non-blocking animation system using update() in main loop
 *
 * Color/Pattern Mappings:
 *   State Indicators:
 *     - IDLE: Slow green pulse
 *     - HEATING: Orange solid
 *     - RUNNING_TEST: Blue pulse
 *     - ERROR_STATE: Red solid
 *
 *   User Prompts:
 *     - Insert cartridge: Green fade
 *     - Remove cartridge: Green slow blink
 *     - Don't touch: Red solid
 *
 * Usage:
 *   LEDController::init();                    // Call once in setup()
 *   LEDController::setDeviceState(mode);      // Update state indicator
 *   LEDController::showPrompt(INSERT);        // Show user prompt
 *   LEDController::update();                  // Call in loop() for animations
 *
 * User Stories Implemented:
 *   - LED-001: LEDController class with init, setColor, setBrightness, off
 *   - LED-002: State-based indicators (IDLE, HEATING, RUNNING_TEST, ERROR)
 *   - LED-003: User prompt indicators (insert, remove, don't touch)
 *   - LED-004: Pattern animations (fade, blink, pulse)
 *   - LED-005: Priority system with timeout and restore
 */

#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include "Particle.h"
#include "DataTypes.h"

// ============================================================================
// LED COLOR DEFINITIONS
// ============================================================================

/**
 * @brief Standard color definitions for LED status
 * RGB values are 8-bit (0-255 per channel)
 */
namespace LEDColors {
    // Primary colors
    constexpr uint32_t RED      = 0xFF0000;
    constexpr uint32_t GREEN    = 0x00FF00;
    constexpr uint32_t BLUE     = 0x0000FF;

    // Status colors
    constexpr uint32_t ORANGE   = 0xFF6600;   // Heating indicator
    constexpr uint32_t YELLOW   = 0xFFFF00;   // Warning
    constexpr uint32_t CYAN     = 0x00FFFF;   // Info
    constexpr uint32_t MAGENTA  = 0xFF00FF;   // Special
    constexpr uint32_t WHITE    = 0xFFFFFF;   // Full on
    constexpr uint32_t OFF      = 0x000000;   // LED off

    // Extract RGB components from packed color
    inline uint8_t getRed(uint32_t color)   { return (color >> 16) & 0xFF; }
    inline uint8_t getGreen(uint32_t color) { return (color >> 8) & 0xFF; }
    inline uint8_t getBlue(uint32_t color)  { return color & 0xFF; }

    // Pack RGB components into single value
    inline uint32_t pack(uint8_t r, uint8_t g, uint8_t b) {
        return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    }
}

// ============================================================================
// LED PATTERN DEFINITIONS
// ============================================================================

/**
 * @brief LED animation pattern types
 * @note Renamed to BrvtLEDPattern to avoid collision with Particle SDK's LEDPattern
 */
enum class BrvtLEDPattern : uint8_t {
    SOLID = 0,      ///< Constant on (no animation)
    BLINK = 1,      ///< On/off blinking
    FADE = 2,       ///< Smooth fade in/out
    PULSE = 3,      ///< Breathing/pulse effect
    FAST_BLINK = 4  ///< Rapid blink for alerts
};

/**
 * @brief LED animation speed presets
 * @note Renamed to BrvtLEDSpeed to avoid collision with Particle SDK's LEDSpeed
 */
enum class BrvtLEDSpeed : uint8_t {
    SLOW = 0,       ///< 2000ms period
    NORMAL = 1,     ///< 1000ms period
    FAST = 2        ///< 500ms period
};

/**
 * @brief LED display priority levels
 * Higher values take precedence over lower values
 * @note Renamed to BrvtLEDPriority to avoid collision with Particle SDK's LEDPriority
 */
enum class BrvtLEDPriority : uint8_t {
    LOW = 0,        ///< Background indicators
    NORMAL = 1,     ///< State indicators
    HIGH = 2,       ///< User prompts
    CRITICAL = 3    ///< Error conditions, alerts
};

/**
 * @brief User prompt types for LED indication
 */
enum class LEDPrompt : uint8_t {
    NONE = 0,           ///< No prompt active
    INSERT_CARTRIDGE,   ///< Signal user to insert cartridge
    REMOVE_CARTRIDGE,   ///< Signal user to remove cartridge
    DONT_TOUCH          ///< Signal user not to touch device
};

// ============================================================================
// LED TIMING CONSTANTS
// ============================================================================

namespace LEDTiming {
    // Animation periods (milliseconds)
    constexpr uint16_t PERIOD_SLOW = 2000;      ///< Slow animation period
    constexpr uint16_t PERIOD_NORMAL = 1000;    ///< Normal animation period
    constexpr uint16_t PERIOD_FAST = 500;       ///< Fast animation period

    // Blink duty cycles
    constexpr uint8_t BLINK_ON_PERCENT = 50;    ///< Blink on duty cycle

    // Pulse parameters
    constexpr uint8_t PULSE_MIN_BRIGHTNESS = 30;  ///< Minimum pulse brightness (%)
    constexpr uint8_t PULSE_MAX_BRIGHTNESS = 100; ///< Maximum pulse brightness (%)

    // Default timeout for temporary indicators
    constexpr uint32_t DEFAULT_TIMEOUT_MS = 0;  ///< 0 = no timeout

    // Update rate limiter
    constexpr uint16_t MIN_UPDATE_INTERVAL_MS = 20; ///< Minimum ms between updates
}

// ============================================================================
// LED STATE STRUCTURE
// ============================================================================

/**
 * @brief Complete LED display state
 */
struct LEDState {
    uint32_t color;         ///< RGB color value
    BrvtLEDPattern pattern;     ///< Animation pattern
    BrvtLEDSpeed speed;         ///< Animation speed
    BrvtLEDPriority priority;   ///< Display priority
    uint32_t startTime;     ///< Animation start time (millis)
    uint32_t timeout;       ///< Timeout duration (0 = no timeout)
    bool active;            ///< Is this state currently active

    LEDState() :
        color(LEDColors::OFF),
        pattern(BrvtLEDPattern::SOLID),
        speed(BrvtLEDSpeed::NORMAL),
        priority(BrvtLEDPriority::NORMAL),
        startTime(0),
        timeout(0),
        active(false)
    {}
};

// ============================================================================
// LED CONTROLLER NAMESPACE
// ============================================================================

/**
 * @namespace LEDController
 * @brief RGB LED indicator controller
 *
 * Provides state-based LED indicators using Particle's built-in RGB LED.
 * All functions are designed to be non-blocking and thread-safe.
 */
namespace LEDController {

    // ========================================================================
    // INITIALIZATION (LED-001)
    // ========================================================================

    /**
     * @brief Initialize LED controller
     *
     * Sets up the Particle RGB LED for custom control and initializes
     * internal state. Must be called once in setup().
     *
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Check if LED controller has been initialized
     * @return true if init() has been called successfully
     */
    bool isInitialized();

    /**
     * @brief Shutdown LED controller and release RGB control
     *
     * Returns control of the RGB LED to the Particle system.
     */
    void shutdown();

    // ========================================================================
    // DIRECT CONTROL (LED-001)
    // ========================================================================

    /**
     * @brief Set LED to a specific RGB color
     *
     * Directly sets the LED color. This bypasses the state system and
     * is intended for low-level control.
     *
     * @param r Red component (0-255)
     * @param g Green component (0-255)
     * @param b Blue component (0-255)
     */
    void setColor(uint8_t r, uint8_t g, uint8_t b);

    /**
     * @brief Set LED to a packed RGB color
     *
     * @param color Packed RGB color (0xRRGGBB)
     */
    void setColor(uint32_t color);

    /**
     * @brief Set LED brightness level
     *
     * Adjusts the overall brightness of the LED.
     *
     * @param level Brightness level (0-255)
     */
    void setBrightness(uint8_t level);

    /**
     * @brief Get current brightness level
     *
     * @return Current brightness (0-255)
     */
    uint8_t getBrightness();

    /**
     * @brief Turn off LED
     *
     * Immediately turns off the LED. Equivalent to setColor(0, 0, 0).
     */
    void off();

    // ========================================================================
    // STATE-BASED INDICATORS (LED-002)
    // ========================================================================

    /**
     * @brief Set LED indicator based on device state
     *
     * Automatically selects appropriate color and pattern based on
     * the current device operating mode.
     *
     * State mappings:
     *   - IDLE: Slow green pulse
     *   - INITIALIZING: Blue pulse
     *   - HEATING: Orange solid
     *   - BARCODE_SCANNING: Blue fast blink
     *   - VALIDATING_CARTRIDGE: Blue pulse
     *   - RUNNING_TEST: Blue pulse
     *   - UPLOADING_RESULTS: Cyan pulse
     *   - ERROR_STATE: Red solid
     *
     * @param mode Device operating mode
     */
    void setDeviceState(DeviceMode mode);

    /**
     * @brief Get the current device state being displayed
     *
     * @return Currently displayed device mode
     */
    DeviceMode getCurrentState();

    /**
     * @brief Clear state indicator
     *
     * Removes the current state indicator, allowing lower priority
     * indicators to show.
     */
    void clearState();

    // ========================================================================
    // USER PROMPT INDICATORS (LED-003)
    // ========================================================================

    /**
     * @brief Show a user prompt indicator
     *
     * Displays a high-priority LED pattern to prompt user action.
     * Overrides state indicators until cleared.
     *
     * Prompt patterns:
     *   - INSERT_CARTRIDGE: Green fade
     *   - REMOVE_CARTRIDGE: Green slow blink
     *   - DONT_TOUCH: Red solid
     *
     * @param prompt Type of prompt to display
     */
    void showPrompt(LEDPrompt prompt);

    /**
     * @brief Clear active user prompt
     *
     * Removes the user prompt and restores state indicator.
     */
    void clearPrompt();

    /**
     * @brief Get currently active prompt
     *
     * @return Active prompt type, or NONE if no prompt
     */
    LEDPrompt getActivePrompt();

    /**
     * @brief Check if a prompt is currently active
     *
     * @return true if a user prompt is being displayed
     */
    bool isPromptActive();

    // ========================================================================
    // PATTERN ANIMATIONS (LED-004)
    // ========================================================================

    /**
     * @brief Set custom LED pattern
     *
     * Configure a custom color and pattern combination.
     *
     * @param color RGB color value
     * @param pattern Animation pattern
     * @param speed Animation speed
     * @param priority Display priority
     * @param timeout Timeout in ms (0 = no timeout)
     */
    void setPattern(uint32_t color, BrvtLEDPattern pattern, BrvtLEDSpeed speed,
                    BrvtLEDPriority priority = BrvtLEDPriority::NORMAL,
                    uint32_t timeout = 0);

    /**
     * @brief Update LED animations
     *
     * Must be called regularly in loop() to update animations.
     * Handles timing, pattern transitions, and timeout expiration.
     *
     * @note This is non-blocking and rate-limited internally
     */
    void update();

    /**
     * @brief Force immediate LED update
     *
     * Bypasses the rate limiter for immediate visual feedback.
     */
    void forceUpdate();

    // ========================================================================
    // PRIORITY SYSTEM (LED-005)
    // ========================================================================

    /**
     * @brief Set a temporary indicator with automatic restore
     *
     * Shows an indicator that automatically clears after timeout,
     * restoring the previous indicator.
     *
     * @param color RGB color value
     * @param pattern Animation pattern
     * @param speed Animation speed
     * @param priority Display priority
     * @param timeoutMs Duration before auto-clear (ms)
     */
    void showTemporary(uint32_t color, BrvtLEDPattern pattern, BrvtLEDSpeed speed,
                       BrvtLEDPriority priority, uint32_t timeoutMs);

    /**
     * @brief Check if a higher priority indicator is active
     *
     * @param priority Priority level to check against
     * @return true if an indicator with higher priority is active
     */
    bool isHigherPriorityActive(BrvtLEDPriority priority);

    /**
     * @brief Get current active priority level
     *
     * @return Highest active priority level
     */
    BrvtLEDPriority getActivePriority();

    /**
     * @brief Clear all indicators and turn off LED
     *
     * Resets all state and turns off the LED.
     */
    void clearAll();

    // ========================================================================
    // UTILITY FUNCTIONS
    // ========================================================================

    /**
     * @brief Get animation period for speed preset
     *
     * @param speed Speed preset
     * @return Period in milliseconds
     */
    uint16_t getPeriod(BrvtLEDSpeed speed);

    /**
     * @brief Convert BrvtLEDPattern to string for debugging
     *
     * @param pattern Pattern to convert
     * @return Constant string representation
     */
    const char* patternToString(BrvtLEDPattern pattern);

    /**
     * @brief Convert BrvtLEDPriority to string for debugging
     *
     * @param priority Priority to convert
     * @return Constant string representation
     */
    const char* priorityToString(BrvtLEDPriority priority);

    /**
     * @brief Convert LEDPrompt to string for debugging
     *
     * @param prompt Prompt to convert
     * @return Constant string representation
     */
    const char* promptToString(LEDPrompt prompt);

} // namespace LEDController

#endif // LED_CONTROLLER_H
