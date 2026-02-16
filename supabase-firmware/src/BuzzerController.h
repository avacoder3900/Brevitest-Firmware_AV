/**
 * @file BuzzerController.h
 * @brief Buzzer/piezo controller for audio feedback
 *
 * Non-blocking buzzer control with predefined alerts, periodic alert modes,
 * and melody playback. All operations use timer callbacks for duration control.
 */

#ifndef BUZZERCONTROLLER_H
#define BUZZERCONTROLLER_H

#include "Particle.h"
#include "HardwareConfig.h"

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
// BUZZER CONTROLLER NAMESPACE
// ============================================================================

namespace BuzzerController {

    // Initialization
    bool init();
    bool isInitialized();

    // Update (call in loop)
    void update();

    // Tone playback
    void playTone(uint16_t frequency, uint16_t duration);
    void stopTone();

    // Melodies
    void playSuccessMelody();
    void playErrorMelody();

    // Periodic alerts
    void startPeriodicAlert(AlertType type);
    void stopPeriodicAlert();

} // namespace BuzzerController

#endif // BUZZERCONTROLLER_H
