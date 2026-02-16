/**
 * @file BuzzerController.cpp
 * @brief Buzzer/piezo controller implementation
 *
 * Non-blocking buzzer control with predefined alerts, periodic alert modes,
 * and melody playback.
 */

#include "BuzzerController.h"
#include "HAL.h"

// ============================================================================
// INTERNAL CONSTANTS AND TYPES
// ============================================================================

#define BUZZER_MAX_PATTERN_LENGTH 16

struct Tone {
    uint16_t frequency;
    uint16_t duration;
    Tone() : frequency(0), duration(0) {}
    Tone(uint16_t freq, uint16_t dur) : frequency(freq), duration(dur) {}
};

namespace BuzzerController {

    // ========================================================================
    // PRIVATE STATE
    // ========================================================================

    static volatile bool _initialized = false;

    // Current tone state
    static volatile bool _toneActive = false;
    static volatile uint32_t _toneStartTime = 0;
    static volatile uint16_t _toneDuration = 0;
    static volatile uint16_t _toneFrequency = 0;

    // Periodic alert state
    static volatile bool _periodicAlertActive = false;
    static volatile AlertType _activeAlertType = AlertType::GENERAL;
    static volatile uint32_t _lastAlertTime = 0;
    static volatile uint16_t _alertPeriod = 0;
    static volatile bool _triggerNextAlert = false;

    static Timer _alertTimer(1000, []() {
        _triggerNextAlert = true;
    }, false);

    // Pattern playback state
    static Tone _patternBuffer[BUZZER_MAX_PATTERN_LENGTH];
    static volatile uint8_t _patternLength = 0;
    static volatile uint8_t _patternIndex = 0;
    static volatile bool _patternActive = false;

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

    // ========================================================================
    // PRIVATE HELPERS
    // ========================================================================

    static void startBuzzer(uint16_t frequency) {
        if (frequency > 0) {
            HAL::setBuzzer(frequency);
        } else {
            HAL::setBuzzerOff();
        }
    }

    static void stopBuzzer() {
        HAL::setBuzzerOff();
    }

    static void startToneInternal(uint16_t frequency, uint16_t duration) {
        _toneFrequency = frequency;
        _toneDuration = duration;
        _toneStartTime = millis();
        _toneActive = true;
        startBuzzer(frequency);
    }

    static void handleToneComplete() {
        stopBuzzer();
        _toneActive = false;

        // Advance pattern if active
        if (_patternActive) {
            _patternIndex++;
            if (_patternIndex < _patternLength) {
                Tone& nextTone = _patternBuffer[_patternIndex];
                startToneInternal(nextTone.frequency, nextTone.duration);
            } else {
                _patternActive = false;
            }
        }
    }

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

    static bool playPatternInternal(const Tone* tones, uint8_t count) {
        if (tones == nullptr || count == 0) {
            return false;
        }

        if (count > BUZZER_MAX_PATTERN_LENGTH) {
            count = BUZZER_MAX_PATTERN_LENGTH;
        }

        // Stop any current playback
        stopPeriodicAlert();
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

    // ========================================================================
    // PUBLIC API
    // ========================================================================

    bool init() {
        if (_initialized) {
            return true;
        }

        stopBuzzer();

        _toneActive = false;
        _periodicAlertActive = false;
        _patternActive = false;
        _triggerNextAlert = false;

        _initialized = true;
        Log.info("BuzzerController: Initialized");
        return true;
    }

    bool isInitialized() {
        return _initialized;
    }

    void playTone(uint16_t frequency, uint16_t duration) {
        _patternActive = false;
        stopPeriodicAlert();
        startToneInternal(frequency, duration);
    }

    void stopTone() {
        _toneActive = false;
        _patternActive = false;
        stopPeriodicAlert();
        stopBuzzer();
    }

    void startPeriodicAlert(AlertType type) {
        if (_periodicAlertActive && _activeAlertType == type) {
            if (!_alertTimer.isActive()) {
                _alertTimer.start();
            }
            return;
        }

        _patternActive = false;

        _activeAlertType = type;
        _periodicAlertActive = true;
        _lastAlertTime = millis();

        switch (type) {
            case AlertType::GENERAL:
                _alertPeriod = BUZZER_ALERT_PERIOD;
                break;
            case AlertType::PROBLEM:
                _alertPeriod = BUZZER_PROBLEM_PERIOD;
                break;
        }

        _alertTimer.changePeriod(_alertPeriod);
        _alertTimer.reset();

        if (!_alertTimer.isActive()) {
            _alertTimer.start();
        }

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

    void update() {
        // Check for tone duration completion
        if (_toneActive) {
            uint32_t elapsed = millis() - _toneStartTime;
            if (elapsed >= _toneDuration) {
                handleToneComplete();
            }
        }

        // Handle periodic alert triggers from timer callback
        if (_triggerNextAlert && _periodicAlertActive) {
            _triggerNextAlert = false;
            triggerPeriodicAlertTone();
        }
    }

    void playSuccessMelody() {
        playPatternInternal(SUCCESS_MELODY, SUCCESS_MELODY_LENGTH);
    }

    void playErrorMelody() {
        playPatternInternal(ERROR_MELODY, ERROR_MELODY_LENGTH);
    }

} // namespace BuzzerController
