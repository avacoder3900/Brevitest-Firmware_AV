/**
 * @file SerialCommands.h
 * @brief Serial command dispatcher for Brevitest firmware
 * @author Agent GAMMA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module implements the serial command interface for debugging, testing,
 * and device control. It handles 100+ commands across multiple categories.
 *
 * Command Categories:
 *   - System Commands (1-9): Reset, EEPROM, pin control
 *   - Serial Port Messaging (10-12): Enable/disable messaging, device ID
 *   - Stage Motion (20-29): Motor and stage control
 *   - Laser Diodes (30-33): Laser control
 *   - Buzzer (40-43): Audio feedback
 *   - Heater (50-55): Temperature control
 *   - Barcode Scanner (60): Barcode reading
 *   - Spectrophotometer (301-311): Optical sensor control
 *   - Cloud Functions (400-406): Cloud operations
 *   - Communication (8000-8999): Radio control
 *   - State Management (9000-9032): Device state control
 *
 * User Stories Implemented:
 *   - SER-002: System commands (1-9)
 *   - SER-003: Stage/motor commands (20-29)
 *   - SER-004: Laser commands (30-33)
 *   - SER-005: Heater commands (50-55)
 *   - SER-006: Spectrophotometer commands (301-311)
 *   - SER-007: State management commands (9000-9021)
 *   - SER-008: Cloud commands (400-406)
 */

#ifndef SERIALCOMMANDS_H
#define SERIALCOMMANDS_H

#include "Particle.h"
#include "CommandParser.h"
#include <stdint.h>

//==============================================================================
// FIRMWARE VERSION INFO
//==============================================================================
// Note: FIRMWARE_VERSION and DATA_FORMAT_VERSION are defined in DataTypes.h

/** @brief Firmware version string */
#define FIRMWARE_VERSION_STRING "1.0.0"

/** @brief Firmware version code (for cloud reporting) */
#define FIRMWARE_VERSION_CODE 100

/** @brief Build date (auto-generated) */
#define FIRMWARE_BUILD_DATE __DATE__

//==============================================================================
// COMMAND CATEGORIES
//==============================================================================

/**
 * @brief Command category enumeration
 * @note Prefixed with CAT_ to avoid conflicts with Particle SDK macros (e.g., BLE)
 */
enum class CommandCategory {
    CAT_SYSTEM,           ///< System commands (1-9)
    CAT_SERIAL_PORT,      ///< Serial port messaging (10-12)
    CAT_STAGE_MOTION,     ///< Stage/motor control (20-29)
    CAT_LASER,            ///< Laser control (30-33)
    CAT_BUZZER,           ///< Buzzer control (40-43)
    CAT_HEATER,           ///< Heater/temperature (50-55)
    CAT_BARCODE,          ///< Barcode scanner (60)
    CAT_MAGNETOMETER,     ///< Magnetometer (70-73)
    CAT_STRESS_TEST,      ///< Stress testing (90-93)
    CAT_BLE,              ///< Bluetooth LE (200)
    CAT_SPECTRO,          ///< Spectrophotometer (301-311)
    CAT_CLOUD,            ///< Cloud functions (400-406)
    CAT_COMMUNICATION,    ///< Radio control (8000-8999)
    CAT_STATE,            ///< State management (9000-9032)
    CAT_UNKNOWN           ///< Unknown category
};

//==============================================================================
// SERIAL COMMANDS CLASS
//==============================================================================

/**
 * @brief Serial command dispatcher
 * @details Handles all serial commands by delegating to appropriate hardware
 *          controllers. Uses the CommandParser for input processing.
 *
 * Usage:
 * @code
 *   SerialCommands commands;
 *   commands.init();
 *
 *   void loop() {
 *       commands.process();  // Call every loop iteration
 *   }
 * @endcode
 */
class SerialCommands {
public:
    //==========================================================================
    // LIFECYCLE
    //==========================================================================

    /**
     * @brief Constructor
     */
    SerialCommands();

    /**
     * @brief Initialize the serial command system
     * @param baudRate Serial baud rate (default 115200)
     * @return true on success
     */
    bool init(uint32_t baudRate = SERIAL_DEFAULT_BAUD);

    /**
     * @brief Check if command system is initialized
     * @return true if initialized
     */
    bool isInitialized() const;

    //==========================================================================
    // MAIN PROCESSING
    //==========================================================================

    /**
     * @brief Process any pending serial commands
     * @details Call this in the main loop
     * @return true if a command was processed
     */
    bool process();

    //==========================================================================
    // CONFIGURATION
    //==========================================================================

    /**
     * @brief Enable or disable serial messaging output
     * @param enable true to enable messaging
     */
    void setMessagingEnabled(bool enable);

    /**
     * @brief Get messaging enabled state
     * @return true if messaging is enabled
     */
    bool isMessagingEnabled() const;

    //==========================================================================
    // HELP FUNCTIONS
    //==========================================================================

    /**
     * @brief Print help for all commands
     */
    void printHelp();

    /**
     * @brief Print help for a specific category
     * @param category Command category
     */
    void printCategoryHelp(CommandCategory category);

    //==========================================================================
    // COMMAND HANDLERS - Called by the dispatcher
    //==========================================================================

    // System Commands (1-9) - SER-002
    int handleSystemReset(const ParsedCommand& cmd);           // 1
    int handleEEPROMReset(const ParsedCommand& cmd);           // 2
    int handleCheckCache(const ParsedCommand& cmd);            // 3
    int handleReadDigitalPin(const ParsedCommand& cmd);        // 4
    int handleLimitSwitchState(const ParsedCommand& cmd);      // 5
    int handleSetDigitalPin(const ParsedCommand& cmd);         // 6
    int handleTogglePin(const ParsedCommand& cmd);             // 7
    int handleReadAnalogPin(const ParsedCommand& cmd);         // 8
    int handleClearWiFiCredentials(const ParsedCommand& cmd);  // 9

    // Serial Port Commands (10-12)
    int handleEnableMessaging(const ParsedCommand& cmd);       // 10
    int handleDisableMessaging(const ParsedCommand& cmd);      // 11
    int handleDisplayDeviceId(const ParsedCommand& cmd);       // 12

    // Stage Motion Commands (20-29) - SER-003
    int handleResetStage(const ParsedCommand& cmd);            // 20
    int handleWakeMotor(const ParsedCommand& cmd);             // 21
    int handleMoveMicrons(const ParsedCommand& cmd);           // 22
    int handleMoveToPosition(const ParsedCommand& cmd);        // 23
    int handleMoveToTestStart(const ParsedCommand& cmd);       // 24
    int handleMoveToOpticalRead(const ParsedCommand& cmd);     // 25
    int handleOscillateStage(const ParsedCommand& cmd);        // 26
    int handleMoveToShippingBolt(const ParsedCommand& cmd);    // 27
    int handleMoveToZero(const ParsedCommand& cmd);            // 28
    int handleSleepMotor(const ParsedCommand& cmd);            // 29

    // Laser Commands (30-33) - SER-004
    int handleLaserA(const ParsedCommand& cmd);                // 30
    int handleLaserB(const ParsedCommand& cmd);                // 31
    int handleLaserC(const ParsedCommand& cmd);                // 32
    int handleAllLasers(const ParsedCommand& cmd);             // 33

    // Buzzer Commands (40-43)
    int handleBuzzerOn(const ParsedCommand& cmd);              // 40
    int handleAlertBuzzer(const ParsedCommand& cmd);           // 41
    int handleProblemBuzzer(const ParsedCommand& cmd);         // 42
    int handleBuzzerOff(const ParsedCommand& cmd);             // 43

    // Heater Commands (50-55) - SER-005
    int handleReadTemperature(const ParsedCommand& cmd);       // 50
    int handleHeaterOn(const ParsedCommand& cmd);              // 51
    int handleHeaterOff(const ParsedCommand& cmd);             // 52
    int handleSetTargetTemp(const ParsedCommand& cmd);         // 53
    int handleStartTempControl(const ParsedCommand& cmd);      // 54
    int handleStopTempControl(const ParsedCommand& cmd);       // 55

    // Barcode Commands (60)
    int handleScanBarcode(const ParsedCommand& cmd);           // 60

    // Magnetometer Commands (70-73) - DELTA-009
    int handleMagnetometerStart(const ParsedCommand& cmd);     // 70
    int handleMagnetometerList(const ParsedCommand& cmd);      // 71
    int handleMagnetometerLoad(const ParsedCommand& cmd);      // 72
    int handleMagnetometerClear(const ParsedCommand& cmd);     // 73

    // Stress Test Commands (90-93) - DELTA-010
    int handleStressTestStart(const ParsedCommand& cmd);       // 90
    int handleStressTestStop(const ParsedCommand& cmd);        // 91
    int handleStressTestStatus(const ParsedCommand& cmd);      // 92
    int handleStressTestReset(const ParsedCommand& cmd);       // 93

    // Spectrophotometer Commands (301-311) - SER-006
    int handleSpectroParams(const ParsedCommand& cmd);         // 301
    int handleSpectroChannelOn(const ParsedCommand& cmd);      // 303
    int handleSpectroChannelOff(const ParsedCommand& cmd);     // 304
    int handleBaselineScan(const ParsedCommand& cmd);          // 305
    int handleTestScan(const ParsedCommand& cmd);              // 306
    int handleSetPulseParams(const ParsedCommand& cmd);        // 307
    int handleTakeReadings(const ParsedCommand& cmd);          // 308
    int handleOutputReadings(const ParsedCommand& cmd);        // 309
    int handleTakeReadingsNoLaser(const ParsedCommand& cmd);   // 310
    int handleBaselineContinuous(const ParsedCommand& cmd);    // 311

    // Cloud Commands (400-406) - SER-008
    int handleCloudCheckCache(const ParsedCommand& cmd);       // 400
    int handleCloudClearCache(const ParsedCommand& cmd);       // 401
    int handleCloudLoadCached(const ParsedCommand& cmd);       // 402
    int handleCloudUploadTest(const ParsedCommand& cmd);       // 403
    int handleCloudCheckAssays(const ParsedCommand& cmd);      // 404
    int handleCloudClearAssays(const ParsedCommand& cmd);      // 405
    int handleCloudOutputAssay(const ParsedCommand& cmd);      // 406

    // Radio Control Commands (8000-8999) - DELTA-013
    int handleRadioHelp(const ParsedCommand& cmd);             // 8000
    int handleRadioStatus(const ParsedCommand& cmd);           // 8001
    int handleWiFiOff(const ParsedCommand& cmd);               // 8100
    int handleWiFiOn(const ParsedCommand& cmd);                // 8101
    int handleCellularOff(const ParsedCommand& cmd);           // 8200
    int handleCellularOn(const ParsedCommand& cmd);            // 8201
    int handleBluetoothOff(const ParsedCommand& cmd);          // 8300
    int handleBluetoothOn(const ParsedCommand& cmd);           // 8301
    int handleWiFiOnlyMode(const ParsedCommand& cmd);          // 8500
    int handleCellularOnlyMode(const ParsedCommand& cmd);      // 8501
    int handleBluetoothOnlyMode(const ParsedCommand& cmd);     // 8502
    int handleFCCTestMode(const ParsedCommand& cmd);           // 8503
    int handleAllRadiosOff(const ParsedCommand& cmd);          // 8900
    int handleAllRadiosOn(const ParsedCommand& cmd);           // 8901
    int handleEmissionCheck(const ParsedCommand& cmd);         // 8999

    // State Management Commands (9000-9032) - SER-007
    int handleStateHelp(const ParsedCommand& cmd);             // 9000
    int handleShowCurrentState(const ParsedCommand& cmd);      // 9001
    int handleShowDetailedState(const ParsedCommand& cmd);     // 9002
    int handleDiagnoseTransition(const ParsedCommand& cmd);    // 9003
    int handleShowTransitions(const ParsedCommand& cmd);       // 9010
    int handleClearHistory(const ParsedCommand& cmd);          // 9011
    int handleForceState(const ParsedCommand& cmd);            // 9020

    //==========================================================================
    // UTILITY FUNCTIONS
    //==========================================================================

    /**
     * @brief Get category for a command ID
     * @param commandId Command ID
     * @return Command category
     */
    static CommandCategory getCategory(int commandId);

    /**
     * @brief Get category name as string
     * @param category Command category
     * @return Category name string
     */
    static const char* getCategoryName(CommandCategory category);

    /**
     * @brief Main command dispatcher
     * @param cmd Parsed command
     * @return Result code
     */
    int dispatchCommand(const ParsedCommand& cmd);

private:
    //==========================================================================
    // PRIVATE MEMBERS
    //==========================================================================

    bool _initialized;           ///< Initialization flag
    bool _messagingEnabled;      ///< Serial messaging output enabled

    //==========================================================================
    // PRIVATE METHODS
    //==========================================================================

    /**
     * @brief Print firmware info to serial
     */
    void printFirmwareInfo();
};

//==============================================================================
// GLOBAL INSTANCE
//==============================================================================

/**
 * @brief Global serial commands instance
 */
extern SerialCommands serialCommands;

/**
 * @brief Get reference to global serial commands
 * @return Reference to SerialCommands instance
 */
SerialCommands& getSerialCommands();

#endif // SERIALCOMMANDS_H
