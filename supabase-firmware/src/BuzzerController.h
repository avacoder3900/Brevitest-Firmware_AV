/**
 * @file BuzzerController.h
 * @brief Buzzer/piezo controller for audio feedback
 * @author Agent LAMBDA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module provides high-level buzzer control for the Brevitest device,
 * including predefined alert tones, periodic alerts, and melody playback.
 * All operations are non-blocking, using timer callbacks for duration control.
 *
 * Features:
 * - Predefined alert tones (cartridge insert/remove, general alert, problem alert)
 * - Periodic alert mode with configurable intervals
 * - Non-blocking tone playback using timers
 * - Melody/pattern playback with completion callback
 * - Volume control (PWM duty cycle adjustment)
 *
 * Usage:
 *   BuzzerController::init();              // Call once in setup()
 *   BuzzerController::update();            // Call in loop()
 *   BuzzerController::cartridgeInsert();   // Play insert notification
 *   BuzzerController::startPeriodicAlert(AlertType::GENERAL);  // Start repeating alert
 *
 * User Stories Implemented:
 *   - BUZ-001: BuzzerController class with initialization and basic tone functions
 *   - BUZ-002: Predefined alert tones
 *   - BUZ-003: Periodic alert mode
 *   - BUZ-004: Non-blocking tone playback
 *   - BUZ-005: Melody/pattern playback
 */

#ifndef BUZZERCONTROLLER_H
#define BUZZERCONTROLLER_H

#include "Particle.h"
#include "HardwareConfig.h"

// ============================================================================
// CONFIGURATION CONSTANTS
// ============================================================================

/** @brief Maximum number of notes in a melody pattern */
#define BUZZER_MAX_PATTERN_LENGTH 16

/** @brief Maximum number of tones in the playback queue */
#define BUZZER_QUEUE_SIZE 8

// ============================================================================
// ALERT TYPE ENUMERATION
// ============================================================================

/**
 * @brief Types of periodic alerts
 * @details Used with startPeriodicAlert() to select which alert pattern to use
 */
enum class AlertType {
    GENERAL,    ///< General alert: 850 Hz, 500ms tone, 4000ms interval
    PROBLEM     ///< Problem alert: 620 Hz, 100ms tone, 777ms interval
};

// ============================================================================
// TONE STRUCTURE
// ============================================================================

/**
 * @brief Single tone definition for patterns/melodies
 * @details Frequency of 0 indicates a rest (silence) for the given duration
 */
struct Tone {
    uint16_t frequency;     ///< Frequency in Hz (0 = rest/silence)
    uint16_t duration;      ///< Duration in milliseconds

    /**
     * @brief Default constructor - creates a silent tone
     */
    Tone() : frequency(0), duration(0) {}

    /**
     * @brief Parameterized constructor
     * @param freq Frequency in Hz (0 for rest)
     * @param dur Duration in milliseconds
     */
    Tone(uint16_t freq, uint16_t dur) : frequency(freq), duration(dur) {}
};

// ============================================================================
// PATTERN COMPLETION CALLBACK TYPE
// ============================================================================

/**
 * @brief Function pointer type for pattern completion notification
 */
typedef void (*PatternCompleteCallback)();

// ============================================================================
// BUZZER CONTROLLER NAMESPACE
// ============================================================================

/**
 * @namespace BuzzerController
 * @brief High-level buzzer control functions
 *
 * Provides non-blocking buzzer operations with predefined alerts,
 * periodic alert modes, and melody playback capabilities.
 *
 * Thread Safety:
 *   Functions use volatile state variables for timer callback safety.
 *   All state changes are atomic on the single-core Particle Boron.
 */
namespace BuzzerController {

    // ========================================================================
    // INITIALIZATION (BUZ-001)
    // ========================================================================

    /**
     * @brief Initialize the buzzer controller
     * @return true if initialization successful
     *
     * @details Configures buzzer pin and initializes timer for non-blocking
     *          playback. Must be called once in setup() before using other
     *          functions.
     *
     * @note HAL::init() should be called before this function
     */
    bool init();

    /**
     * @brief Check if buzzer controller is initialized
     * @return true if init() has been called successfully
     */
    bool isInitialized();

    // ========================================================================
    // BASIC TONE FUNCTIONS (BUZ-001)
    // ========================================================================

    /**
     * @brief Play a tone at specified frequency for specified duration
     * @param frequency Frequency in Hz
     * @param duration Duration in milliseconds
     *
     * @details Non-blocking: starts tone and returns immediately.
     *          Tone will be stopped automatically after duration.
     *          If a tone is already playing, it will be interrupted.
     */
    void playTone(uint16_t frequency, uint16_t duration);

    /**
     * @brief Stop any currently playing tone
     *
     * @details Immediately stops the buzzer and clears playback state.
     *          Also stops any periodic alert or pattern playback.
     */
    void stopTone();

    /**
     * @brief Check if a tone is currently playing
     * @return true if buzzer is active
     */
    bool isPlaying();

    /**
     * @brief Set the volume level
     * @param level Volume level 0-100 (percent)
     *
     * @details Adjusts PWM duty cycle for volume control.
     *          0 = silent, 100 = maximum volume.
     *          Default is 100 (full volume).
     *
     * @note Some piezo buzzers may not respond well to PWM volume control
     */
    void setVolume(uint8_t level);

    /**
     * @brief Get current volume level
     * @return Volume level 0-100
     */
    uint8_t getVolume();

    // ========================================================================
    // PREDEFINED ALERT TONES (BUZ-002)
    // ========================================================================

    /**
     * @brief Play cartridge insertion notification
     * @details 620 Hz, 200ms duration
     */
    void cartridgeInsert();

    /**
     * @brief Play cartridge removal notification
     * @details 620 Hz, 500ms duration
     */
    void cartridgeRemove();

    /**
     * @brief Play general alert tone (single beep)
     * @details 850 Hz, 500ms duration
     */
    void generalAlert();

    /**
     * @brief Play problem alert tone (single beep)
     * @details 620 Hz, 100ms duration
     */
    void problemAlert();

    /**
     * @brief Play standard beep
     * @details 600 Hz, 1000ms duration
     */
    void standardBeep();

    // ========================================================================
    // PERIODIC ALERT MODE (BUZ-003)
    // ========================================================================

    /**
     * @brief Start a periodic alert that repeats at intervals
     * @param type Alert type (GENERAL or PROBLEM)
     *
     * @details Starts the appropriate alert tone and repeats it at the
     *          configured interval:
     *          - GENERAL: 850 Hz, 500ms tone, 4000ms interval
     *          - PROBLEM: 620 Hz, 100ms tone, 777ms interval
     *
     *          First alert plays immediately upon activation.
     *          Call stopPeriodicAlert() to stop the repeating alerts.
     */
    void startPeriodicAlert(AlertType type);

    /**
     * @brief Stop any active periodic alert
     *
     * @details Stops the current tone and cancels the repeating timer.
     */
    void stopPeriodicAlert();

    /**
     * @brief Check if a periodic alert is currently active
     * @return true if periodic alert is running
     */
    bool isPeriodicAlertActive();

    /**
     * @brief Get the currently active alert type
     * @return Current AlertType, or GENERAL if no alert active
     */
    AlertType getActiveAlertType();

    // ========================================================================
    // NON-BLOCKING PLAYBACK (BUZ-004)
    // ========================================================================

    /**
     * @brief Update function - must be called in loop()
     *
     * @details Handles timer-based tone duration and pattern advancement.
     *          Should be called frequently in the main loop for responsive
     *          buzzer control.
     */
    void update();

    /**
     * @brief Queue a tone for sequential playback
     * @param frequency Frequency in Hz
     * @param duration Duration in milliseconds
     * @return true if tone was queued successfully, false if queue is full
     *
     * @details Adds a tone to the playback queue. If no tone is currently
     *          playing, starts immediately. Otherwise, plays after current
     *          tone(s) complete.
     */
    bool queueTone(uint16_t frequency, uint16_t duration);

    /**
     * @brief Clear the tone queue
     *
     * @details Removes all queued tones. Does not stop the currently
     *          playing tone.
     */
    void clearQueue();

    /**
     * @brief Get number of tones in the queue
     * @return Number of pending tones
     */
    uint8_t getQueueLength();

    /**
     * @brief Interrupt current tone and play a new one immediately
     * @param frequency Frequency in Hz
     * @param duration Duration in milliseconds
     *
     * @details Stops current playback, clears the queue, and starts
     *          the new tone immediately. Use for urgent notifications.
     */
    void interruptWithTone(uint16_t frequency, uint16_t duration);

    // ========================================================================
    // MELODY/PATTERN PLAYBACK (BUZ-005)
    // ========================================================================

    /**
     * @brief Play a pattern of tones
     * @param tones Array of Tone structures
     * @param count Number of tones in the array (max BUZZER_MAX_PATTERN_LENGTH)
     * @return true if pattern started successfully
     *
     * @details Plays the sequence of tones non-blocking. Each tone plays
     *          for its specified duration before advancing to the next.
     *          A frequency of 0 creates a rest (silence) for that duration.
     */
    bool playPattern(const Tone* tones, uint8_t count);

    /**
     * @brief Play the success melody
     *
     * @details Ascending three-note pattern indicating successful completion
     */
    void playSuccessMelody();

    /**
     * @brief Play the error melody
     *
     * @details Descending two-note pattern indicating an error
     */
    void playErrorMelody();

    /**
     * @brief Play the startup melody
     *
     * @details Short ascending melody played on device boot
     */
    void playStartupMelody();

    /**
     * @brief Register a callback for pattern completion
     * @param callback Function to call when pattern finishes
     *
     * @details The callback is invoked when a pattern or melody completes.
     *          Set to nullptr to disable callbacks.
     */
    void setPatternCompleteCallback(PatternCompleteCallback callback);

    /**
     * @brief Check if a pattern is currently playing
     * @return true if pattern playback is active
     */
    bool isPatternPlaying();

    /**
     * @brief Stop pattern playback
     *
     * @details Immediately stops the current pattern. The completion
     *          callback will NOT be invoked when stopped manually.
     */
    void stopPattern();

    // ========================================================================
    // DIAGNOSTICS
    // ========================================================================

    /**
     * @brief Run a buzzer self-test
     * @return true if test passed
     *
     * @details Plays a quick test tone. Useful for hardware verification.
     */
    bool selfTest();

    /**
     * @brief Get state information as formatted string
     * @param buffer Output buffer
     * @param bufferSize Size of output buffer
     * @return Number of characters written
     */
    size_t getStateAsString(char* buffer, size_t bufferSize);

} // namespace BuzzerController

#endif // BUZZERCONTROLLER_H
