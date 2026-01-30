/**
 * @file LaserController.h
 * @brief PWM-controlled laser diode controller for three independent laser channels
 * @author Agent IOTA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module manages three laser diode channels (A, B, C) used for spectrophotometry.
 * It provides individual and bulk laser control, pulsed operation mode for synchronized
 * readings, and safety interlocks with cartridge detection.
 *
 * Features:
 * - Three independent laser channels (A=D5, B=D6, C=D7)
 * - Digital on/off control (no PWM dimming in hardware)
 * - Pulsed mode for spectrophotometer synchronization (20us on, 2ms cycle)
 * - Safety interlock with cartridge detection
 * - Maximum continuous on-time limit
 *
 * Hardware Platform: Particle Boron (NRF52840)
 *
 * User Stories Implemented:
 *   - LAS-001: LaserController class with init()
 *   - LAS-002: Individual laser control (setLaserPower, enable, disable)
 *   - LAS-003: All-laser control (enableAll, disableAll)
 *   - LAS-004: Pulsed laser mode
 *   - LAS-005: Laser safety features
 *
 * Safety Considerations:
 *   - Lasers are automatically disabled when cartridge is removed
 *   - Maximum continuous on-time prevents overheating
 *   - All lasers start in OFF state
 *   - Interlock prevents laser activation without cartridge present
 */

#ifndef LASER_CONTROLLER_H
#define LASER_CONTROLLER_H

#include "Particle.h"
#include "HardwareConfig.h"

// ============================================================================
// LASER CONSTANTS
// ============================================================================

/** @brief Maximum continuous laser on-time before safety cutoff (milliseconds) */
constexpr uint32_t LASER_MAX_CONTINUOUS_ON_MS = 30000;

/** @brief Default pulse on-time in microseconds (20us) */
constexpr uint32_t LASER_DEFAULT_PULSE_ON_US = 20;

/** @brief Default pulse cycle time in microseconds (2000us = 2ms) */
constexpr uint32_t LASER_DEFAULT_CYCLE_US = 2000;

/** @brief Minimum pulse on-time in microseconds */
constexpr uint32_t LASER_MIN_PULSE_ON_US = 5;

/** @brief Maximum pulse on-time in microseconds */
constexpr uint32_t LASER_MAX_PULSE_ON_US = 1000;

/** @brief Minimum cycle time in microseconds */
constexpr uint32_t LASER_MIN_CYCLE_US = 100;

/** @brief Maximum cycle time in microseconds */
constexpr uint32_t LASER_MAX_CYCLE_US = 100000;

// ============================================================================
// LASER CHANNEL ENUMERATION
// ============================================================================

/**
 * @brief Laser channel identifier
 * @details Identifies the three laser channels available on the device.
 *          Each channel corresponds to a specific wavelength/color.
 */
enum class LaserChannel : uint8_t {
    A = 0,  ///< Channel A (red laser, pin D5)
    B = 1,  ///< Channel B (green laser, pin D6)
    C = 2,  ///< Channel C (blue laser, pin D7)
    COUNT = 3   ///< Number of channels (for iteration)
};

// ============================================================================
// LASER CONTROLLER CLASS
// ============================================================================

/**
 * @class LaserController
 * @brief Manages laser diode operation with safety features
 *
 * The LaserController provides a safe interface for controlling the three
 * laser channels. It includes safety interlocks, continuous operation limits,
 * and pulsed mode for synchronized spectrophotometer readings.
 *
 * Usage:
 * @code
 *   LaserController laser;
 *   laser.init();
 *
 *   // Basic on/off control
 *   laser.enableLaser(LaserChannel::A);
 *   delay(100);
 *   laser.disableLaser(LaserChannel::A);
 *
 *   // Pulsed mode for readings
 *   laser.setPulseMode(20, 2000);  // 20us on, 2ms cycle
 *   laser.pulse(LaserChannel::A);   // Single pulse
 *
 *   // Safety: automatically disable all on cartridge removal
 *   laser.update();  // Call in loop() to check safety
 * @endcode
 */
class LaserController {
public:
    // ========================================================================
    // CONSTRUCTORS / INITIALIZATION
    // ========================================================================

    /**
     * @brief Default constructor
     * @details Creates LaserController with all lasers disabled and default settings.
     */
    LaserController();

    /**
     * @brief Initialize the laser controller
     * @details Configures laser pins as outputs and sets all lasers to OFF state.
     *          Must be called before using any other laser functions.
     * @return true if initialization successful, false otherwise
     * @note HAL::init() should be called before this function
     */
    bool init();

    /**
     * @brief Check if laser controller is initialized
     * @return true if init() has been called successfully
     */
    bool isInitialized() const;

    // ========================================================================
    // INDIVIDUAL LASER CONTROL (LAS-002)
    // ========================================================================

    /**
     * @brief Set laser power level for a channel
     * @details Sets the power level for the specified channel. The laser
     *          will use this power level when enabled. Note that the current
     *          hardware uses digital on/off control, so this sets the
     *          conceptual power level for future PWM support.
     * @param channel Laser channel to configure
     * @param power Power level (0-255), 0 = off, 255 = full power
     * @return true if channel is valid, false otherwise
     * @note Current hardware is digital on/off; power > 0 means ON
     */
    bool setLaserPower(LaserChannel channel, uint8_t power);

    /**
     * @brief Set laser power level using character channel identifier
     * @param channel Channel identifier ('A', 'B', or 'C')
     * @param power Power level (0-255)
     * @return true if channel is valid, false otherwise
     */
    bool setLaserPower(char channel, uint8_t power);

    /**
     * @brief Get current power setting for a channel
     * @param channel Laser channel to query
     * @return Power level (0-255), or 0 if invalid channel
     */
    uint8_t getLaserPower(LaserChannel channel) const;

    /**
     * @brief Get current power setting using character channel identifier
     * @param channel Channel identifier ('A', 'B', or 'C')
     * @return Power level (0-255), or 0 if invalid channel
     */
    uint8_t getLaserPower(char channel) const;

    /**
     * @brief Enable a laser channel
     * @details Turns on the specified laser at its configured power level.
     *          Will fail if cartridge is not inserted (safety interlock).
     * @param channel Laser channel to enable
     * @return true if laser was enabled, false if failed (no cartridge or invalid channel)
     */
    bool enableLaser(LaserChannel channel);

    /**
     * @brief Enable a laser channel using character identifier
     * @param channel Channel identifier ('A', 'B', or 'C')
     * @return true if laser was enabled, false otherwise
     */
    bool enableLaser(char channel);

    /**
     * @brief Disable a laser channel
     * @details Turns off the specified laser immediately.
     * @param channel Laser channel to disable
     * @return true if channel is valid, false otherwise
     */
    bool disableLaser(LaserChannel channel);

    /**
     * @brief Disable a laser channel using character identifier
     * @param channel Channel identifier ('A', 'B', or 'C')
     * @return true if channel is valid, false otherwise
     */
    bool disableLaser(char channel);

    /**
     * @brief Check if a laser channel is currently enabled
     * @param channel Laser channel to query
     * @return true if laser is on, false if off or invalid channel
     */
    bool isLaserEnabled(LaserChannel channel) const;

    /**
     * @brief Check if a laser channel is currently enabled using character identifier
     * @param channel Channel identifier ('A', 'B', or 'C')
     * @return true if laser is on, false if off or invalid channel
     */
    bool isLaserEnabled(char channel) const;

    // ========================================================================
    // ALL-LASER CONTROL (LAS-003)
    // ========================================================================

    /**
     * @brief Enable all laser channels
     * @details Turns on all three lasers at their configured power levels.
     *          Will fail if cartridge is not inserted (safety interlock).
     * @return true if all lasers enabled, false if failed (no cartridge)
     */
    bool enableAllLasers();

    /**
     * @brief Disable all laser channels
     * @details Turns off all lasers immediately. Always succeeds.
     */
    void disableAllLasers();

    /**
     * @brief Set power level for all laser channels
     * @param power Power level (0-255) to set for all channels
     */
    void setAllLaserPower(uint8_t power);

    /**
     * @brief Check if all laser channels are enabled
     * @return true if all three lasers are on, false otherwise
     */
    bool areAllLasersEnabled() const;

    /**
     * @brief Check if any laser channel is enabled
     * @return true if at least one laser is on, false if all off
     */
    bool isAnyLaserEnabled() const;

    // ========================================================================
    // PULSED MODE CONTROL (LAS-004)
    // ========================================================================

    /**
     * @brief Configure pulsed mode timing
     * @details Sets up the pulse timing for synchronized spectrophotometer
     *          readings. Default is 20us on, 2ms total cycle (0.5% duty).
     * @param onTimeUs Pulse on-time in microseconds (5-1000us)
     * @param cycleTimeUs Total cycle time in microseconds (100-100000us)
     * @return true if parameters valid, false if out of range
     */
    bool setPulseMode(uint32_t onTimeUs, uint32_t cycleTimeUs);

    /**
     * @brief Get current pulse on-time setting
     * @return Pulse on-time in microseconds
     */
    uint32_t getPulseOnTime() const;

    /**
     * @brief Get current pulse cycle time setting
     * @return Pulse cycle time in microseconds
     */
    uint32_t getPulseCycleTime() const;

    /**
     * @brief Execute a single pulse on a laser channel
     * @details Turns on the laser for the configured on-time, then turns it off.
     *          This is a blocking call that waits for the pulse to complete.
     * @param channel Laser channel to pulse
     * @return true if pulse executed, false if failed (no cartridge or invalid)
     */
    bool pulse(LaserChannel channel);

    /**
     * @brief Execute a single pulse using character channel identifier
     * @param channel Channel identifier ('A', 'B', or 'C')
     * @return true if pulse executed, false otherwise
     */
    bool pulse(char channel);

    /**
     * @brief Execute a single pulse on all laser channels simultaneously
     * @return true if pulse executed, false if failed (no cartridge)
     */
    bool pulseAll();

    /**
     * @brief Check if pulse mode is active
     * @return true if pulse mode is configured (always true after init)
     */
    bool isPulseModeEnabled() const;

    // ========================================================================
    // SAFETY FEATURES (LAS-005)
    // ========================================================================

    /**
     * @brief Update safety checks
     * @details Should be called periodically (e.g., in loop()) to check
     *          safety conditions and disable lasers if necessary.
     *          Checks:
     *          - Cartridge removal (disable all lasers)
     *          - Maximum continuous on-time exceeded
     */
    void update();

    /**
     * @brief Set safety interlock state
     * @details Allows external code to enable/disable safety interlock.
     *          When interlock is enabled, lasers cannot be turned on
     *          unless cartridge is present.
     * @param enabled true to enable interlock (default), false to disable
     * @warning Disabling interlock bypasses safety - use with caution!
     */
    void setInterlockEnabled(bool enabled);

    /**
     * @brief Check if safety interlock is enabled
     * @return true if interlock is enabled
     */
    bool isInterlockEnabled() const;

    /**
     * @brief Check if cartridge is present (for interlock)
     * @return true if cartridge is detected
     */
    bool isCartridgePresent() const;

    /**
     * @brief Check if laser has exceeded maximum continuous on-time
     * @return true if any laser has been on too long
     */
    bool isOvertime() const;

    /**
     * @brief Get duration that any laser has been continuously on
     * @return Milliseconds of continuous laser operation, 0 if all off
     */
    uint32_t getContinuousOnTime() const;

    /**
     * @brief Emergency stop - disable all lasers immediately
     * @details Disables all lasers and resets safety timers.
     *          Use in error conditions.
     */
    void emergencyStop();

    // ========================================================================
    // UTILITY FUNCTIONS
    // ========================================================================

    /**
     * @brief Convert character channel to enum
     * @param c Channel character ('A', 'B', 'C', 'a', 'b', 'c')
     * @param channel Output enum value
     * @return true if valid character, false otherwise
     */
    static bool charToChannel(char c, LaserChannel& channel);

    /**
     * @brief Convert enum channel to character
     * @param channel Enum channel value
     * @return Character ('A', 'B', 'C'), or '?' if invalid
     */
    static char channelToChar(LaserChannel channel);

    /**
     * @brief Get the hardware pin for a laser channel
     * @param channel Laser channel
     * @return Pin number, or 0 if invalid channel
     */
    static hal_pin_t getChannelPin(LaserChannel channel);

private:
    // ========================================================================
    // PRIVATE MEMBER VARIABLES
    // ========================================================================

    bool _initialized;                          ///< Initialization flag
    bool _interlockEnabled;                     ///< Safety interlock flag
    bool _pulseModeEnabled;                     ///< Pulse mode configured flag

    uint8_t _power[static_cast<uint8_t>(LaserChannel::COUNT)];    ///< Power settings per channel
    bool _enabled[static_cast<uint8_t>(LaserChannel::COUNT)];     ///< Enabled state per channel

    uint32_t _pulseOnTimeUs;                    ///< Pulse on-time in microseconds
    uint32_t _pulseCycleTimeUs;                 ///< Pulse cycle time in microseconds

    uint32_t _laserOnStartTime;                 ///< Time when first laser turned on
    bool _wasAnyLaserOn;                        ///< Tracking for continuous on-time

    // ========================================================================
    // PRIVATE HELPER METHODS
    // ========================================================================

    /**
     * @brief Check if it's safe to enable lasers
     * @return true if safe (cartridge present or interlock disabled)
     */
    bool isSafeToEnable() const;

    /**
     * @brief Apply the current enabled state to hardware
     * @param channel Channel to update
     */
    void applyHardwareState(LaserChannel channel);

    /**
     * @brief Update continuous on-time tracking
     */
    void updateOnTimeTracking();
};

#endif // LASER_CONTROLLER_H
