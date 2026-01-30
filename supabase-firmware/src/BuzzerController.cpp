/**
 * @file BuzzerController.cpp
 * @brief Buzzer/piezo controller implementation
 * @author Agent LAMBDA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * Implementation of non-blocking buzzer control with predefined alerts,
 * periodic alert modes, and melody playback.
 */

#include "BuzzerController.h"
#include "HAL.h"

namespace BuzzerController {

    // ========================================================================
    // PRIVATE STATE VARIABLES
    // ========================================================================

    // Initialization state
    static volatile bool _initialized = false;

    // Volume control (0-100)
    static volatile uint8_t _volumeLevel = 100;

    // Current tone state
    static volatile bool _toneActive = false;
    static volatile uint32_t _toneStartTime = 0;
    static volatile uint16_t _toneDuration = 0;
    static volatile uint16_t _toneFrequency = 0;

    // Tone queue for sequential playback
    static Tone _toneQueue[BUZZER_QUEUE_SIZE];
    static volatile uint8_t _queueHead = 0;
    static volatile uint8_t _queueTail = 0;
    static volatile uint8_t _queueCount = 0;

    // Periodic alert state
    static volatile bool _periodicAlertActive = false;
    static volatile AlertType _activeAlertType = AlertType::GENERAL;
    static volatile uint32_t _lastAlertTime = 0;
    static volatile uint16_t _alertPeriod = 0;
    static volatile bool _triggerNextAlert = false;

    // Timer for periodic alerts
    static Timer _alertTimer(1000, []() {
        _triggerNextAlert = true;
    }, false);

    // Pattern playback state
    static Tone _patternBuffer[BUZZER_MAX_PATTERN_LENGTH];
    static volatile uint8_t _patternLength = 0;
    static volatile uint8_t _patternIndex = 0;
    static volatile bool _patternActive = false;
    static PatternCompleteCallback _patternCallback = nullptr;

    // Predefined melodies
    static const Tone SUCCESS_MELODY[] = {
        Tone(523, 150),   // C5
        Tone(0, 50),      // Rest
        Tone(659, 150),   // E5
        Tone(0, 50),      // Rest
        Tone(784, 200)    // G5
    };
    static const uint8_t SUCCESS_MELODY_LENGTH = 5;

    static const Tone ERROR_MELODY[] = {
        Tone(392, 200),   // G4
        Tone(0, 50),      // Rest
        Tone(294, 300)    // D4
    };
    static const uint8_t ERROR_MELODY_LENGTH = 3;

    static const Tone STARTUP_MELODY[] = {
        Tone(440, 100),   // A4
        Tone(0, 30),      // Rest
        Tone(523, 100),   // C5
        Tone(0, 30),      // Rest
        Tone(659, 150)    // E5
    };
    static const uint8_t STARTUP_MELODY_LENGTH = 5;

    // ========================================================================
    // PRIVATE HELPER FUNCTIONS
    // ========================================================================

    /**
     * @brief Start playing a tone on the buzzer hardware
     */
    static void startBuzzer(uint16_t frequency) {
        if (frequency > 0) {
            HAL::setBuzzer(frequency);
        } else {
            HAL::setBuzzerOff();
        }
    }

    /**
     * @brief Stop the buzzer hardware
     */
    static void stopBuzzer() {
        HAL::setBuzzerOff();
    }

    /**
     * @brief Get next tone from queue
     * @return true if a tone was available
     */
    static bool dequeueNextTone(Tone& outTone) {
        if (_queueCount == 0) {
            return false;
        }

        outTone = _toneQueue[_queueHead];
        _queueHead = (_queueHead + 1) % BUZZER_QUEUE_SIZE;
        _queueCount--;
        return true;
    }

    /**
     * @brief Start playing a tone internally
     */
    static void startToneInternal(uint16_t frequency, uint16_t duration) {
        _toneFrequency = frequency;
        _toneDuration = duration;
        _toneStartTime = millis();
        _toneActive = true;

        startBuzzer(frequency);
    }

    /**
     * @brief Stop current tone and check queue for next
     */
    static void handleToneComplete() {
        stopBuzzer();
        _toneActive = false;

        // Check if we're playing a pattern
        if (_patternActive) {
            _patternIndex++;
            if (_patternIndex < _patternLength) {
                // Play next note in pattern
                Tone& nextTone = _patternBuffer[_patternIndex];
                startToneInternal(nextTone.frequency, nextTone.duration);
            } else {
                // Pattern complete
                _patternActive = false;
                if (_patternCallback != nullptr) {
                    _patternCallback();
                }
            }
            return;
        }

        // Check queue for next tone
        Tone nextTone;
        if (dequeueNextTone(nextTone)) {
            startToneInternal(nextTone.frequency, nextTone.duration);
        }
    }

    /**
     * @brief Handle periodic alert trigger
     */
    static void triggerPeriodicAlertTone() {
        if (!_periodicAlertActive) {
            return;
        }

        switch (_activeAlertType) {
            case AlertType::GENERAL:
                startToneInternal(BUZZER_ALERT_FREQUENCY, BUZZER_ALERT_DURATION);
                break;
            case AlertType::PROBLEM:
                startToneInternal(BUZZER_PROBLEM_FREQUENCY, BUZZER_PROBLEM_DURATION);
                break;
        }
    }

    // ========================================================================
    // INITIALIZATION (BUZ-001)
    // ========================================================================

    bool init() {
        if (_initialized) {
            return true;
        }

        // Ensure buzzer is off
        stopBuzzer();

        // Reset all state
        _volumeLevel = 100;
        _toneActive = false;
        _queueHead = 0;
        _queueTail = 0;
        _queueCount = 0;
        _periodicAlertActive = false;
        _patternActive = false;
        _patternCallback = nullptr;
        _triggerNextAlert = false;

        _initialized = true;
        Log.info("BuzzerController: Initialized");
        return true;
    }

    bool isInitialized() {
        return _initialized;
    }

    // ========================================================================
    // BASIC TONE FUNCTIONS (BUZ-001)
    // ========================================================================

    void playTone(uint16_t frequency, uint16_t duration) {
        // Stop any current playback
        _patternActive = false;
        stopPeriodicAlert();
        clearQueue();

        startToneInternal(frequency, duration);
    }

    void stopTone() {
        _toneActive = false;
        _patternActive = false;
        stopPeriodicAlert();
        clearQueue();
        stopBuzzer();
    }

    bool isPlaying() {
        return _toneActive;
    }

    void setVolume(uint8_t level) {
        if (level > 100) level = 100;
        _volumeLevel = level;
        // Note: Volume control via PWM is limited on piezo buzzers
        // This is provided for interface completeness but may have
        // limited effect depending on hardware
    }

    uint8_t getVolume() {
        return _volumeLevel;
    }

    // ========================================================================
    // PREDEFINED ALERT TONES (BUZ-002)
    // ========================================================================

    void cartridgeInsert() {
        playTone(BUZZER_INSERT_FREQUENCY, BUZZER_INSERT_DURATION);
    }

    void cartridgeRemove() {
        playTone(BUZZER_REMOVE_FREQUENCY, BUZZER_REMOVE_DURATION);
    }

    void generalAlert() {
        playTone(BUZZER_ALERT_FREQUENCY, BUZZER_ALERT_DURATION);
    }

    void problemAlert() {
        playTone(BUZZER_PROBLEM_FREQUENCY, BUZZER_PROBLEM_DURATION);
    }

    void standardBeep() {
        playTone(BUZZER_FREQUENCY, BUZZER_DURATION);
    }

    // ========================================================================
    // PERIODIC ALERT MODE (BUZ-003)
    // ========================================================================

    void startPeriodicAlert(AlertType type) {
        // If already running this alert type, ensure timer is active
        if (_periodicAlertActive && _activeAlertType == type) {
            if (!_alertTimer.isActive()) {
                _alertTimer.start();
            }
            return;
        }

        // Stop any other playback
        _patternActive = false;
        clearQueue();

        _activeAlertType = type;
        _periodicAlertActive = true;
        _lastAlertTime = millis();

        // Set period based on alert type
        switch (type) {
            case AlertType::GENERAL:
                _alertPeriod = BUZZER_ALERT_PERIOD;
                break;
            case AlertType::PROBLEM:
                _alertPeriod = BUZZER_PROBLEM_PERIOD;
                break;
        }

        // Configure and start timer
        _alertTimer.changePeriod(_alertPeriod);
        _alertTimer.reset();

        if (!_alertTimer.isActive()) {
            _alertTimer.start();
        }

        // Play first alert immediately
        triggerPeriodicAlertTone();

        Log.trace("BuzzerController: Started periodic alert type %d, period %dms",
                  (int)type, _alertPeriod);
    }

    void stopPeriodicAlert() {
        if (!_periodicAlertActive) {
            return;
        }

        _alertTimer.stop();
        _periodicAlertActive = false;
        _triggerNextAlert = false;

        Log.trace("BuzzerController: Stopped periodic alert");
    }

    bool isPeriodicAlertActive() {
        return _periodicAlertActive;
    }

    AlertType getActiveAlertType() {
        return _activeAlertType;
    }

    // ========================================================================
    // NON-BLOCKING PLAYBACK (BUZ-004)
    // ========================================================================

    void update() {
        // Check for tone duration completion
        if (_toneActive) {
            uint32_t elapsed = millis() - _toneStartTime;
            if (elapsed >= _toneDuration) {
                handleToneComplete();
            }
        }

        // Check for queued tones if nothing playing
        if (!_toneActive && _queueCount > 0) {
            Tone nextTone;
            if (dequeueNextTone(nextTone)) {
                startToneInternal(nextTone.frequency, nextTone.duration);
            }
        }

        // Handle periodic alert triggers from timer callback
        if (_triggerNextAlert && _periodicAlertActive) {
            _triggerNextAlert = false;
            triggerPeriodicAlertTone();
        }
    }

    bool queueTone(uint16_t frequency, uint16_t duration) {
        if (_queueCount >= BUZZER_QUEUE_SIZE) {
            return false;
        }

        _toneQueue[_queueTail] = Tone(frequency, duration);
        _queueTail = (_queueTail + 1) % BUZZER_QUEUE_SIZE;
        _queueCount++;

        // If nothing playing, start immediately
        if (!_toneActive && !_patternActive) {
            Tone nextTone;
            if (dequeueNextTone(nextTone)) {
                startToneInternal(nextTone.frequency, nextTone.duration);
            }
        }

        return true;
    }

    void clearQueue() {
        _queueHead = 0;
        _queueTail = 0;
        _queueCount = 0;
    }

    uint8_t getQueueLength() {
        return _queueCount;
    }

    void interruptWithTone(uint16_t frequency, uint16_t duration) {
        // Stop everything
        _patternActive = false;
        stopPeriodicAlert();
        clearQueue();
        stopBuzzer();
        _toneActive = false;

        // Start new tone
        startToneInternal(frequency, duration);
    }

    // ========================================================================
    // MELODY/PATTERN PLAYBACK (BUZ-005)
    // ========================================================================

    bool playPattern(const Tone* tones, uint8_t count) {
        if (tones == nullptr || count == 0) {
            return false;
        }

        if (count > BUZZER_MAX_PATTERN_LENGTH) {
            count = BUZZER_MAX_PATTERN_LENGTH;
        }

        // Stop any current playback
        stopPeriodicAlert();
        clearQueue();
        stopBuzzer();
        _toneActive = false;

        // Copy pattern to buffer
        for (uint8_t i = 0; i < count; i++) {
            _patternBuffer[i] = tones[i];
        }
        _patternLength = count;
        _patternIndex = 0;
        _patternActive = true;

        // Start first note
        startToneInternal(_patternBuffer[0].frequency, _patternBuffer[0].duration);

        return true;
    }

    void playSuccessMelody() {
        playPattern(SUCCESS_MELODY, SUCCESS_MELODY_LENGTH);
    }

    void playErrorMelody() {
        playPattern(ERROR_MELODY, ERROR_MELODY_LENGTH);
    }

    void playStartupMelody() {
        playPattern(STARTUP_MELODY, STARTUP_MELODY_LENGTH);
    }

    void setPatternCompleteCallback(PatternCompleteCallback callback) {
        _patternCallback = callback;
    }

    bool isPatternPlaying() {
        return _patternActive;
    }

    void stopPattern() {
        _patternActive = false;
        _toneActive = false;
        stopBuzzer();
    }

    // ========================================================================
    // DIAGNOSTICS
    // ========================================================================

    bool selfTest() {
        // Play a quick test tone
        playTone(BUZZER_FREQUENCY, 100);

        // Wait for completion
        uint32_t startTime = millis();
        while (isPlaying() && (millis() - startTime) < 200) {
            update();
            delay(10);
        }

        Log.info("BuzzerController: Self-test complete");
        return true;
    }

    size_t getStateAsString(char* buffer, size_t bufferSize) {
        if (buffer == nullptr || bufferSize == 0) {
            return 0;
        }

        return snprintf(buffer, bufferSize,
            "BuzzerController: init=%s, playing=%s, periodic=%s, pattern=%s, queue=%d, volume=%d",
            _initialized ? "yes" : "no",
            _toneActive ? "yes" : "no",
            _periodicAlertActive ? "yes" : "no",
            _patternActive ? "yes" : "no",
            _queueCount,
            _volumeLevel
        );
    }

} // namespace BuzzerController
