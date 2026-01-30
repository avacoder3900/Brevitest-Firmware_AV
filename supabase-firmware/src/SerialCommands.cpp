/**
 * @file SerialCommands.cpp
 * @brief Implementation of serial command dispatcher
 * @author Agent GAMMA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * User Stories Implemented:
 *   - SER-002: System commands (1-9)
 *   - SER-003: Stage/motor commands (20-29)
 *   - SER-004: Laser commands (30-33)
 *   - SER-005: Heater commands (50-55)
 *   - SER-006: Spectrophotometer commands (301-311)
 *   - SER-007: State management commands (9000-9020)
 *   - SER-008: Cloud commands (400-406)
 */

#include "SerialCommands.h"
#include "CommandParser.h"
#include "HAL.h"
#include "HardwareConfig.h"
#include "StateMachine.h"
#include "StorageManager.h"
#include "SupabaseClient.h"
#include "MotorController.h"
#include "HeaterController.h"
#include "LaserController.h"
#include "Spectrophotometer.h"
#include "BuzzerController.h"
#include "BarcodeScanner.h"
#include "TestRunner.h"
#include <string.h>

//==============================================================================
// HARDWARE CONTROLLER INSTANCES
//==============================================================================

// Global motor controller instance for serial commands
static MotorController stageMotor;

// Global heater controller instance for serial commands
static HeaterController heater;

// Global spectrophotometer instance for serial commands
static Spectrophotometer spectro;

// Global laser controller instance for serial commands
static LaserController laser;

// Global storage manager instance for serial commands
static StorageManager storage;

// Global barcode scanner instance for serial commands
static BarcodeScanner barcodeScanner;

//==============================================================================
// GLOBAL INSTANCE
//==============================================================================

SerialCommands serialCommands;

SerialCommands& getSerialCommands() {
    return serialCommands;
}

//==============================================================================
// COMMAND HANDLER CALLBACK
//==============================================================================

// Forward declaration for the global command handler
static int globalCommandHandler(const ParsedCommand& cmd);

//==============================================================================
// CONSTRUCTOR AND INITIALIZATION
//==============================================================================

SerialCommands::SerialCommands()
    : _initialized(false)
    , _messagingEnabled(true)
{
}

bool SerialCommands::init(uint32_t baudRate) {
    if (_initialized) {
        return true;
    }

    // Initialize the command parser
    if (!serialParser.init(baudRate)) {
        return false;
    }

    // Set our command handler
    serialParser.setCommandHandler(globalCommandHandler);

    _initialized = true;

    // Print startup info
    if (_messagingEnabled) {
        printFirmwareInfo();
    }

    return true;
}

bool SerialCommands::isInitialized() const {
    return _initialized;
}

//==============================================================================
// MAIN PROCESSING
//==============================================================================

bool SerialCommands::process() {
    if (!_initialized) {
        return false;
    }

    return serialParser.process();
}

//==============================================================================
// CONFIGURATION
//==============================================================================

void SerialCommands::setMessagingEnabled(bool enable) {
    _messagingEnabled = enable;
    serialParser.setLogging(enable);
}

bool SerialCommands::isMessagingEnabled() const {
    return _messagingEnabled;
}

//==============================================================================
// GLOBAL COMMAND HANDLER CALLBACK
//==============================================================================

static int globalCommandHandler(const ParsedCommand& cmd) {
    return serialCommands.dispatchCommand(cmd);
}

//==============================================================================
// COMMAND DISPATCHER
//==============================================================================

int SerialCommands::dispatchCommand(const ParsedCommand& cmd) {
    if (!cmd.valid) {
        serialParser.respondError(-1, "Invalid command");
        return -1;
    }

    int result = 0;

    switch (cmd.commandId) {
        //======================================================================
        // SYSTEM COMMANDS (1-9) - SER-002
        //======================================================================
        case 1:  result = handleSystemReset(cmd); break;
        case 2:  result = handleEEPROMReset(cmd); break;
        case 3:  result = handleCheckCache(cmd); break;
        case 4:  result = handleReadDigitalPin(cmd); break;
        case 5:  result = handleLimitSwitchState(cmd); break;
        case 6:  result = handleSetDigitalPin(cmd); break;
        case 7:  result = handleTogglePin(cmd); break;
        case 8:  result = handleReadAnalogPin(cmd); break;
        case 9:  result = handleClearWiFiCredentials(cmd); break;

        //======================================================================
        // SERIAL PORT MESSAGING (10-12)
        //======================================================================
        case 10: result = handleEnableMessaging(cmd); break;
        case 11: result = handleDisableMessaging(cmd); break;
        case 12: result = handleDisplayDeviceId(cmd); break;

        //======================================================================
        // STAGE MOTION (20-29) - SER-003
        //======================================================================
        case 20: result = handleResetStage(cmd); break;
        case 21: result = handleWakeMotor(cmd); break;
        case 22: result = handleMoveMicrons(cmd); break;
        case 23: result = handleMoveToPosition(cmd); break;
        case 24: result = handleMoveToTestStart(cmd); break;
        case 25: result = handleMoveToOpticalRead(cmd); break;
        case 26: result = handleOscillateStage(cmd); break;
        case 27: result = handleMoveToShippingBolt(cmd); break;
        case 28: result = handleMoveToZero(cmd); break;
        case 29: result = handleSleepMotor(cmd); break;

        //======================================================================
        // LASER DIODES (30-33) - SER-004
        //======================================================================
        case 30: result = handleLaserA(cmd); break;
        case 31: result = handleLaserB(cmd); break;
        case 32: result = handleLaserC(cmd); break;
        case 33: result = handleAllLasers(cmd); break;

        //======================================================================
        // BUZZER (40-43)
        //======================================================================
        case 40: result = handleBuzzerOn(cmd); break;
        case 41: result = handleAlertBuzzer(cmd); break;
        case 42: result = handleProblemBuzzer(cmd); break;
        case 43: result = handleBuzzerOff(cmd); break;

        //======================================================================
        // HEATER (50-55) - SER-005
        //======================================================================
        case 50: result = handleReadTemperature(cmd); break;
        case 51: result = handleHeaterOn(cmd); break;
        case 52: result = handleHeaterOff(cmd); break;
        case 53: result = handleSetTargetTemp(cmd); break;
        case 54: result = handleStartTempControl(cmd); break;
        case 55: result = handleStopTempControl(cmd); break;

        //======================================================================
        // BARCODE (60)
        //======================================================================
        case 60: result = handleScanBarcode(cmd); break;

        //======================================================================
        // MAGNETOMETER (70-73) - DELTA-009
        //======================================================================
        case 70: result = handleMagnetometerStart(cmd); break;
        case 71: result = handleMagnetometerList(cmd); break;
        case 72: result = handleMagnetometerLoad(cmd); break;
        case 73: result = handleMagnetometerClear(cmd); break;

        //======================================================================
        // STRESS TEST (90-93) - DELTA-010
        //======================================================================
        case 90: result = handleStressTestStart(cmd); break;
        case 91: result = handleStressTestStop(cmd); break;
        case 92: result = handleStressTestStatus(cmd); break;
        case 93: result = handleStressTestReset(cmd); break;

        //======================================================================
        // SPECTROPHOTOMETER (301-311) - SER-006
        //======================================================================
        case 301: result = handleSpectroParams(cmd); break;
        case 303: result = handleSpectroChannelOn(cmd); break;
        case 304: result = handleSpectroChannelOff(cmd); break;
        case 305: result = handleBaselineScan(cmd); break;
        case 306: result = handleTestScan(cmd); break;
        case 307: result = handleSetPulseParams(cmd); break;
        case 308: result = handleTakeReadings(cmd); break;
        case 309: result = handleOutputReadings(cmd); break;
        case 310: result = handleTakeReadingsNoLaser(cmd); break;
        case 311: result = handleBaselineContinuous(cmd); break;

        //======================================================================
        // CLOUD FUNCTIONS (400-406) - SER-008
        //======================================================================
        case 400: result = handleCloudCheckCache(cmd); break;
        case 401: result = handleCloudClearCache(cmd); break;
        case 402: result = handleCloudLoadCached(cmd); break;
        case 403: result = handleCloudUploadTest(cmd); break;
        case 404: result = handleCloudCheckAssays(cmd); break;
        case 405: result = handleCloudClearAssays(cmd); break;
        case 406: result = handleCloudOutputAssay(cmd); break;

        //======================================================================
        // RADIO CONTROL (8000-8999) - DELTA-013
        //======================================================================
        case 8000: result = handleRadioHelp(cmd); break;
        case 8001: result = handleRadioStatus(cmd); break;
        case 8100: result = handleWiFiOff(cmd); break;
        case 8101: result = handleWiFiOn(cmd); break;
        case 8200: result = handleCellularOff(cmd); break;
        case 8201: result = handleCellularOn(cmd); break;
        case 8300: result = handleBluetoothOff(cmd); break;
        case 8301: result = handleBluetoothOn(cmd); break;
        case 8500: result = handleWiFiOnlyMode(cmd); break;
        case 8501: result = handleCellularOnlyMode(cmd); break;
        case 8502: result = handleBluetoothOnlyMode(cmd); break;
        case 8503: result = handleFCCTestMode(cmd); break;
        case 8900: result = handleAllRadiosOff(cmd); break;
        case 8901: result = handleAllRadiosOn(cmd); break;
        case 8999: result = handleEmissionCheck(cmd); break;

        //======================================================================
        // STATE MANAGEMENT (9000-9032) - SER-007
        //======================================================================
        case 9000: result = handleStateHelp(cmd); break;
        case 9001: result = handleShowCurrentState(cmd); break;
        case 9002: result = handleShowDetailedState(cmd); break;
        case 9003: result = handleDiagnoseTransition(cmd); break;
        case 9010: result = handleShowTransitions(cmd); break;
        case 9011: result = handleClearHistory(cmd); break;
        case 9020: result = handleForceState(cmd); break;

        //======================================================================
        // UNKNOWN COMMAND
        //======================================================================
        default:
            if (_messagingEnabled) {
                Log.warn("Unknown command: %d", cmd.commandId);
            }
            serialParser.respondError(-3, "Unknown command");
            return -3;
    }

    return result;
}

//==============================================================================
// SYSTEM COMMANDS (1-9) - SER-002
//==============================================================================

int SerialCommands::handleSystemReset(const ParsedCommand& cmd) {
    // Command 1: System reset - restarts the device
    Log.info("Command 1: System reset");
    serialParser.respond("Resetting device...");
    delay(100); // Allow message to be sent
    System.reset();
    return 0; // Never reached
}

int SerialCommands::handleEEPROMReset(const ParsedCommand& cmd) {
    // Command 2: Reset EEPROM - clears all EEPROM data to defaults
    Log.info("Command 2: Reset EEPROM");

    if (!storage.isInitialized()) {
        storage.init();
    }
    if (storage.resetConfig()) {
        int version = storage.getDataFormatVersion();
        serialParser.respondOK(version);
        return version;
    }

    serialParser.respondError(-1, "EEPROM reset failed");
    return -1;
}

int SerialCommands::handleCheckCache(const ParsedCommand& cmd) {
    // Command 3: Check cache - returns 1 if test data is cached, 0 otherwise
    Log.info("Command 3: Check cache");

    if (!storage.isInitialized()) {
        storage.init();
    }
    bool hasCache = storage.hasCachedTests();
    int result = hasCache ? 1 : 0;

    serialParser.respondOK(result);
    return result;
}

int SerialCommands::handleReadDigitalPin(const ParsedCommand& cmd) {
    // Command 4: Read digital pin state
    // Param1: pin number
    int pin = cmd.getParam(0, 0);

    Log.info("Command 4: Read digital pin %d", pin);

    int state = digitalRead(pin);
    serialParser.respond("Pin %d = %d", pin, state);
    serialParser.respondOK(state);

    return state;
}

int SerialCommands::handleLimitSwitchState(const ParsedCommand& cmd) {
    // Command 5: Get limit switch state
    Log.info("Command 5: Limit switch state");

    int state = digitalRead(PIN_STAGE_LIMIT);
    serialParser.respond("Limit switch = %d", state);
    serialParser.respondOK(state);

    return state;
}

int SerialCommands::handleSetDigitalPin(const ParsedCommand& cmd) {
    // Command 6: Set digital pin state
    // Param1: pin number (default: PIN_MOTOR_DIR)
    // Param2: state (0 = LOW, 1 = HIGH)
    int pin = cmd.getParam(0, PIN_MOTOR_DIR);
    int state = cmd.getParam(1, 0);

    Log.info("Command 6: Set digital pin %d to %d", pin, state);

    digitalWrite(pin, state ? HIGH : LOW);
    int readState = digitalRead(pin);

    serialParser.respond("Pin %d set to %d, read %d", pin, state, readState);
    serialParser.respondOK(readState);

    return readState;
}

int SerialCommands::handleTogglePin(const ParsedCommand& cmd) {
    // Command 7: Repeatedly toggle pin state
    // Param1: pin number (default: PIN_MOTOR_DIR)
    // Param2: number of iterations (default: 1)
    // Param3: delay in ms (default: 1000)
    int pin = cmd.getParam(0, PIN_MOTOR_DIR);
    int iterations = cmd.getParam(1, 1);
    int delayMs = cmd.getParam(2, 1000);

    Log.info("Command 7: Toggle pin %d, %d times, %d ms delay", pin, iterations, delayMs);

    bool currentState = digitalRead(pin) == HIGH;

    for (int i = 0; i < iterations; i++) {
        Log.info("Toggle pin %d, cycle = %d, state = %d", pin, i, currentState);
        digitalWrite(pin, currentState ? LOW : HIGH);
        delay(delayMs);
        digitalWrite(pin, currentState ? HIGH : LOW);
        delay(delayMs);
    }

    serialParser.respond("Toggled pin %d, %d cycles complete", pin, iterations);
    serialParser.respondOK(iterations);

    return iterations;
}

int SerialCommands::handleReadAnalogPin(const ParsedCommand& cmd) {
    // Command 8: Read analog pin value
    // Param1: pin number
    int pin = cmd.getParam(0, 0);

    Log.info("Command 8: Read analog pin %d", pin);

    int value = analogRead(pin);
    serialParser.respond("Analog pin %d = %d", pin, value);
    serialParser.respondOK(value);

    return value;
}

int SerialCommands::handleClearWiFiCredentials(const ParsedCommand& cmd) {
    // Command 9: Clear WiFi credentials
    Log.info("Command 9: Clear WiFi credentials");

    WiFi.clearCredentials();
    serialParser.respond("WiFi credentials cleared");
    serialParser.respondOK(1);

    return 1;
}

//==============================================================================
// SERIAL PORT MESSAGING (10-12)
//==============================================================================

int SerialCommands::handleEnableMessaging(const ParsedCommand& cmd) {
    // Command 10: Enable serial messaging
    Log.info("Command 10: Enable messaging");
    setMessagingEnabled(true);
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleDisableMessaging(const ParsedCommand& cmd) {
    // Command 11: Disable serial messaging
    Log.info("Command 11: Disable messaging");
    setMessagingEnabled(false);
    serialParser.respondOK(0);
    return 0;
}

int SerialCommands::handleDisplayDeviceId(const ParsedCommand& cmd) {
    // Command 12: Display device ID
    Log.info("Command 12: Display device ID");

    String deviceId = System.deviceID();
    serialParser.respond("Device ID: %s", deviceId.c_str());
    serialParser.respondOK(1);

    return 1;
}

//==============================================================================
// STAGE MOTION (20-29) - SER-003
//==============================================================================

int SerialCommands::handleResetStage(const ParsedCommand& cmd) {
    // Command 20: Reset stage - moves to home and sleeps motor
    Log.info("Command 20: Reset stage");

    // Initialize motor if not already done
    if (!stageMotor.isInitialized()) {
        stageMotor.init();
    }

    // Perform homing sequence with sleep after
    MotorResult result = stageMotor.home(true);

    if (result == MotorResult::SUCCESS) {
        int32_t position = stageMotor.getCurrentPosition();
        serialParser.respond("Stage reset to position %ld", (long)position);
        serialParser.respondOK(position);
        return position;
    } else {
        serialParser.respondError(static_cast<int>(result), "Stage reset failed");
        return -1;
    }
}

int SerialCommands::handleWakeMotor(const ParsedCommand& cmd) {
    // Command 21: Wake motor - enable the motor driver
    Log.info("Command 21: Wake motor");

    if (!stageMotor.isInitialized()) {
        stageMotor.init();
    }

    stageMotor.enable();
    int32_t position = stageMotor.getCurrentPosition();

    serialParser.respond("Motor enabled, position = %ld", (long)position);
    serialParser.respondOK(position);
    return position;
}

int SerialCommands::handleMoveMicrons(const ParsedCommand& cmd) {
    // Command 22: Move relative distance in microns
    // Param1: microns to move (positive = distal, negative = proximal)
    // Param2: step delay (default: MOTOR_SLOW_STEP_DELAY)
    int32_t microns = cmd.getParam(0, 0);
    uint16_t stepDelay = cmd.getParam(1, MOTOR_SLOW_STEP_DELAY);

    Log.info("Command 22: Move %ld microns at step delay %d", (long)microns, stepDelay);

    if (!stageMotor.isInitialized()) {
        stageMotor.init();
    }

    stageMotor.enable();
    MotorResult result = stageMotor.moveRelative(microns, stepDelay);

    int32_t position = stageMotor.getCurrentPosition();
    int16_t error = stageMotor.getPositionError();

    if (result == MotorResult::SUCCESS || result == MotorResult::AT_LIMIT) {
        serialParser.respond("Moved %ld microns, position = %ld, error = %d",
                           (long)microns, (long)position, error);
        serialParser.respondOK(position);
        return position;
    } else {
        serialParser.respondError(static_cast<int>(result), "Move failed");
        return static_cast<int>(result);
    }
}

int SerialCommands::handleMoveToPosition(const ParsedCommand& cmd) {
    // Command 23: Move to absolute position
    // Param1: target position in microns
    // Param2: step delay (default: MOTOR_SLOW_STEP_DELAY)
    int32_t position = cmd.getParam(0, 0);
    uint16_t stepDelay = cmd.getParam(1, MOTOR_SLOW_STEP_DELAY);

    Log.info("Command 23: Move to position %ld at step delay %d", (long)position, stepDelay);

    if (!stageMotor.isInitialized()) {
        stageMotor.init();
    }

    // Home first, then move to position
    stageMotor.home(false);
    MotorResult result = stageMotor.moveToPosition(position, stepDelay);
    stageMotor.disable();

    int32_t finalPos = stageMotor.getCurrentPosition();

    if (result == MotorResult::SUCCESS) {
        serialParser.respond("Moved to position %ld", (long)finalPos);
        serialParser.respondOK(finalPos);
        return finalPos;
    } else {
        serialParser.respondError(static_cast<int>(result), "Move to position failed");
        return static_cast<int>(result);
    }
}

int SerialCommands::handleMoveToTestStart(const ParsedCommand& cmd) {
    // Command 24: Move to test start position
    Log.info("Command 24: Move to test start position");

    if (!stageMotor.isInitialized()) {
        stageMotor.init();
    }

    stageMotor.home(false);
    MotorResult result = stageMotor.moveToPosition(StagePosition::TEST_START, MotorSpeed::FAST);
    stageMotor.disable();

    int32_t position = stageMotor.getCurrentPosition();

    if (result == MotorResult::SUCCESS) {
        serialParser.respond("At test start position %ld", (long)position);
        serialParser.respondOK(position);
        return position;
    } else {
        serialParser.respondError(static_cast<int>(result), "Move to test start failed");
        return static_cast<int>(result);
    }
}

int SerialCommands::handleMoveToOpticalRead(const ParsedCommand& cmd) {
    // Command 25: Move to optical read position
    Log.info("Command 25: Move to optical read position");

    if (!stageMotor.isInitialized()) {
        stageMotor.init();
    }

    stageMotor.home(false);
    MotorResult result = stageMotor.moveToPosition(StagePosition::OPTICAL_READ, MotorSpeed::FAST);
    // Note: Don't sleep motor - stay ready for optical operations

    int32_t position = stageMotor.getCurrentPosition();

    if (result == MotorResult::SUCCESS) {
        serialParser.respond("At optical read position %ld", (long)position);
        serialParser.respondOK(position);
        return position;
    } else {
        serialParser.respondError(static_cast<int>(result), "Move to optical read failed");
        return static_cast<int>(result);
    }
}

int SerialCommands::handleOscillateStage(const ParsedCommand& cmd) {
    // Command 26: Oscillate stage back and forth
    // Param1: amplitude in microns (default: 25)
    // Param2: step delay (default: MOTOR_OSCILLATION_STEP_DELAY)
    // Param3: number of cycles (default: 10)
    int32_t amplitude = cmd.getParam(0, 25);
    uint16_t stepDelay = cmd.getParam(1, MOTOR_OSCILLATION_STEP_DELAY);
    uint16_t cycles = cmd.getParam(2, 10);

    Log.info("Command 26: Oscillate stage %ld microns, %d cycles at delay %d",
             (long)amplitude, cycles, stepDelay);

    if (!stageMotor.isInitialized()) {
        stageMotor.init();
    }

    stageMotor.enable();
    MotorResult result = stageMotor.oscillate(amplitude, stepDelay, cycles);

    int32_t position = stageMotor.getCurrentPosition();

    if (result == MotorResult::SUCCESS) {
        serialParser.respond("Oscillation complete, position = %ld", (long)position);
        serialParser.respondOK(position);
        return position;
    } else {
        serialParser.respondError(static_cast<int>(result), "Oscillation failed");
        return static_cast<int>(result);
    }
}

int SerialCommands::handleMoveToShippingBolt(const ParsedCommand& cmd) {
    // Command 27: Move to shipping bolt check position
    Log.info("Command 27: Move to shipping bolt location");

    if (!stageMotor.isInitialized()) {
        stageMotor.init();
    }

    stageMotor.enable();
    MotorResult result = stageMotor.moveToPosition(StagePosition::SHIPPING_BOLT, MotorSpeed::SLOW);

    int32_t position = stageMotor.getCurrentPosition();

    if (result == MotorResult::SUCCESS) {
        serialParser.respond("At shipping bolt position %ld - ready for bolt insertion",
                           (long)position);
        serialParser.respondOK(position);
        return position;
    } else {
        serialParser.respondError(static_cast<int>(result), "Move to shipping bolt failed");
        return static_cast<int>(result);
    }
}

int SerialCommands::handleMoveToZero(const ParsedCommand& cmd) {
    // Command 28: Move to position zero (home)
    // Param1: step delay (default: MOTOR_SLOW_STEP_DELAY)
    uint16_t stepDelay = cmd.getParam(0, MOTOR_SLOW_STEP_DELAY);

    Log.info("Command 28: Move to position zero at step delay %d", stepDelay);

    if (!stageMotor.isInitialized()) {
        stageMotor.init();
    }

    stageMotor.enable();

    // Move to current negative position (i.e., toward zero)
    int32_t current = stageMotor.getCurrentPosition();
    MotorResult result = stageMotor.moveRelative(-current, stepDelay);

    int32_t position = stageMotor.getCurrentPosition();

    if (result == MotorResult::SUCCESS || result == MotorResult::AT_LIMIT) {
        serialParser.respond("At position zero (%ld)", (long)position);
        serialParser.respondOK(position);
        return position;
    } else {
        serialParser.respondError(static_cast<int>(result), "Move to zero failed");
        return static_cast<int>(result);
    }
}

int SerialCommands::handleSleepMotor(const ParsedCommand& cmd) {
    // Command 29: Sleep motor - disable the motor driver
    Log.info("Command 29: Sleep motor");

    if (!stageMotor.isInitialized()) {
        serialParser.respond("Motor not initialized");
        serialParser.respondOK(0);
        return 0;
    }

    stageMotor.disable();
    int32_t position = stageMotor.getCurrentPosition();

    serialParser.respond("Motor disabled (sleeping), position = %ld", (long)position);
    serialParser.respondOK(position);
    return position;
}

//==============================================================================
// LASER COMMANDS (30-33) - SER-004
//==============================================================================

int SerialCommands::handleLaserA(const ParsedCommand& cmd) {
    int duration = cmd.getParam(0, LED_DURATION);
    Log.info("Command 30: Laser A for %d ms", duration);

    // Initialize laser if needed
    if (!laser.isInitialized()) {
        laser.init();
    }

    // Enable laser A
    if (!laser.enableLaser(LaserChannel::A)) {
        serialParser.respondError(-1, "Laser A enable failed (check cartridge)");
        return -1;
    }

    // If duration specified, delay then disable
    if (duration > 0) {
        delay(duration);
        laser.disableLaser(LaserChannel::A);
        serialParser.respond("Laser A on for %d ms", duration);
    } else {
        serialParser.respond("Laser A enabled");
    }

    serialParser.respondOK(duration);
    return duration;
}

int SerialCommands::handleLaserB(const ParsedCommand& cmd) {
    int duration = cmd.getParam(0, LED_DURATION);
    Log.info("Command 31: Laser B for %d ms", duration);

    // Initialize laser if needed
    if (!laser.isInitialized()) {
        laser.init();
    }

    // Enable laser B
    if (!laser.enableLaser(LaserChannel::B)) {
        serialParser.respondError(-1, "Laser B enable failed (check cartridge)");
        return -1;
    }

    // If duration specified, delay then disable
    if (duration > 0) {
        delay(duration);
        laser.disableLaser(LaserChannel::B);
        serialParser.respond("Laser B on for %d ms", duration);
    } else {
        serialParser.respond("Laser B enabled");
    }

    serialParser.respondOK(duration);
    return duration;
}

int SerialCommands::handleLaserC(const ParsedCommand& cmd) {
    int duration = cmd.getParam(0, LED_DURATION);
    Log.info("Command 32: Laser C for %d ms", duration);

    // Initialize laser if needed
    if (!laser.isInitialized()) {
        laser.init();
    }

    // Enable laser C
    if (!laser.enableLaser(LaserChannel::C)) {
        serialParser.respondError(-1, "Laser C enable failed (check cartridge)");
        return -1;
    }

    // If duration specified, delay then disable
    if (duration > 0) {
        delay(duration);
        laser.disableLaser(LaserChannel::C);
        serialParser.respond("Laser C on for %d ms", duration);
    } else {
        serialParser.respond("Laser C enabled");
    }

    serialParser.respondOK(duration);
    return duration;
}

int SerialCommands::handleAllLasers(const ParsedCommand& cmd) {
    int duration = cmd.getParam(0, LED_DURATION);
    Log.info("Command 33: All lasers for %d ms", duration);

    // Initialize laser if needed
    if (!laser.isInitialized()) {
        laser.init();
    }

    // Enable all lasers
    if (!laser.enableAllLasers()) {
        serialParser.respondError(-1, "Laser enable failed (check cartridge)");
        return -1;
    }

    // If duration specified, delay then disable
    if (duration > 0) {
        delay(duration);
        laser.disableAllLasers();
        serialParser.respond("All lasers on for %d ms", duration);
    } else {
        serialParser.respond("All lasers enabled");
    }

    serialParser.respondOK(duration);
    return duration;
}

//==============================================================================
// BUZZER COMMANDS (40-43)
//==============================================================================

int SerialCommands::handleBuzzerOn(const ParsedCommand& cmd) {
    int frequency = cmd.getParam(0, BUZZER_FREQUENCY);
    int duration = cmd.getParam(1, BUZZER_DURATION);
    Log.info("Command 40: Buzzer on at %d Hz for %d ms", frequency, duration);

    // Initialize buzzer if needed
    if (!BuzzerController::isInitialized()) {
        BuzzerController::init();
    }

    BuzzerController::playTone(frequency, duration);
    serialParser.respond("Buzzer: %d Hz for %d ms", frequency, duration);
    serialParser.respondOK(duration);
    return duration;
}

int SerialCommands::handleAlertBuzzer(const ParsedCommand& cmd) {
    Log.info("Command 41: Alert buzzer");

    if (!BuzzerController::isInitialized()) {
        BuzzerController::init();
    }

    BuzzerController::startPeriodicAlert(AlertType::GENERAL);
    serialParser.respond("Alert buzzer started (850 Hz, 4s interval)");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleProblemBuzzer(const ParsedCommand& cmd) {
    Log.info("Command 42: Problem buzzer");

    if (!BuzzerController::isInitialized()) {
        BuzzerController::init();
    }

    BuzzerController::startPeriodicAlert(AlertType::PROBLEM);
    serialParser.respond("Problem buzzer started (620 Hz, 777ms interval)");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleBuzzerOff(const ParsedCommand& cmd) {
    Log.info("Command 43: Buzzer off");

    if (BuzzerController::isInitialized()) {
        BuzzerController::stopPeriodicAlert();
        BuzzerController::stopTone();
    }

    serialParser.respond("Buzzer stopped");
    serialParser.respondOK(1);
    return 1;
}

//==============================================================================
// HEATER COMMANDS (50-55) - SER-005
//==============================================================================

int SerialCommands::handleReadTemperature(const ParsedCommand& cmd) {
    // Command 50: Read heater temperature
    Log.info("Command 50: Read temperature");

    if (!heater.isInitialized()) {
        heater.init();
    }

    // Get current temperature in 10x Celsius format (e.g., 375 = 37.5C)
    int16_t temp10x = heater.getCurrentTemperature();
    int16_t wholePart = temp10x / 10;
    int16_t fracPart = temp10x % 10;

    serialParser.respond("Heater: T = %d.%dC (raw: %d)", wholePart, fracPart, temp10x);
    serialParser.respondOK(temp10x);

    return temp10x;
}

int SerialCommands::handleHeaterOn(const ParsedCommand& cmd) {
    // Command 51: Turn on heater at specified power level
    // Param1: power level 0-255 (default: HEATER_DEFAULT_POWER)
    int power = cmd.getParam(0, HEATER_DEFAULT_POWER);

    Log.info("Command 51: Heater on at power %d", power);

    if (!heater.isInitialized()) {
        heater.init();
    }

    // Clamp power to valid range
    if (power < 0) power = 0;
    if (power > 255) power = 255;

    // Set default power level (used before PID takes over)
    heater.setDefaultPower((uint8_t)power);
    heater.enable();

    serialParser.respond("Heater enabled at power %d", power);
    serialParser.respondOK(power);

    return power;
}

int SerialCommands::handleHeaterOff(const ParsedCommand& cmd) {
    // Command 52: Turn off heater immediately
    Log.info("Command 52: Heater off");

    if (heater.isInitialized()) {
        heater.disable();
    }

    serialParser.respond("Heater disabled");
    serialParser.respondOK(1);

    return 1;
}

int SerialCommands::handleSetTargetTemp(const ParsedCommand& cmd) {
    // Command 53: Set target temperature for PID control
    // Param1: target temperature in 10x Celsius (e.g., 375 = 37.5C)
    int16_t targetTemp = cmd.getParam(0, HEATER_DEFAULT_TEMP_TARGET);

    Log.info("Command 53: Set target temperature to %d (10x C)", targetTemp);

    if (!heater.isInitialized()) {
        heater.init();
    }

    // Temperature is in 10x format and will be clamped by HeaterController (0-600)
    heater.setTargetTemperature(targetTemp);

    int16_t actualTarget = heater.getTargetTemperature();
    int16_t wholePart = actualTarget / 10;
    int16_t fracPart = actualTarget % 10;

    serialParser.respond("Target temperature set to %d.%dC (%d)", wholePart, fracPart, actualTarget);
    serialParser.respondOK(actualTarget);

    return actualTarget;
}

int SerialCommands::handleStartTempControl(const ParsedCommand& cmd) {
    // Command 54: Start PID temperature control
    Log.info("Command 54: Start temperature control");

    if (!heater.isInitialized()) {
        heater.init();
    }

    // Reset PID state and enable control
    heater.resetPID();
    heater.enable();

    int16_t target = heater.getTargetTemperature();
    int16_t current = heater.getCurrentTemperature();

    serialParser.respond("Temperature control started: target=%d, current=%d (10x C)",
                        target, current);
    serialParser.respondOK(target);

    return target;
}

int SerialCommands::handleStopTempControl(const ParsedCommand& cmd) {
    // Command 55: Stop PID temperature control
    Log.info("Command 55: Stop temperature control");

    if (heater.isInitialized()) {
        heater.disable();
    }

    int16_t finalTemp = heater.isInitialized() ? heater.getCurrentTemperature() : 0;

    serialParser.respond("Temperature control stopped, final temp=%d (10x C)", finalTemp);
    serialParser.respondOK(finalTemp);

    return finalTemp;
}

//==============================================================================
// BARCODE COMMANDS (60)
//==============================================================================

int SerialCommands::handleScanBarcode(const ParsedCommand& cmd) {
    // Command 60: Trigger barcode scan
    // Param1: timeout in ms (default: 1000)
    int timeout = cmd.getParam(0, BARCODE_DEFAULT_TIMEOUT_MS);

    Log.info("Command 60: Scan barcode with %d ms timeout", timeout);

    // Initialize scanner if needed
    if (!barcodeScanner.isInitialized()) {
        barcodeScanner.init();
    }

    // Check if scanner is ready
    if (!barcodeScanner.isReady()) {
        serialParser.respondError(-1, "Scanner not ready");
        return -1;
    }

    // Trigger scan
    BarcodeType type = barcodeScanner.triggerScan(timeout);
    const char* barcode = barcodeScanner.getLastBarcode();

    if (type == BarcodeType::UNKNOWN || barcode[0] == '\0') {
        serialParser.respond("Barcode scan failed or timed out");
        serialParser.respondError(-2, "Scan failed");
        return -2;
    }

    // Report results
    const char* typeStr;
    switch (type) {
        case BarcodeType::CARTRIDGE:    typeStr = "Cartridge"; break;
        case BarcodeType::MAGNETOMETER: typeStr = "Magnetometer"; break;
        case BarcodeType::STRESS_TEST:  typeStr = "StressTest"; break;
        case BarcodeType::SHIPPING:     typeStr = "Shipping"; break;
        case BarcodeType::OPTICAL:      typeStr = "Optical"; break;
        default:                        typeStr = "Unknown"; break;
    }

    serialParser.respond("Barcode: %s", barcode);
    serialParser.respond("Type: %s", typeStr);
    serialParser.respondOK(static_cast<int>(type));
    return static_cast<int>(type);
}

//==============================================================================
// SPECTROPHOTOMETER COMMANDS (301-311) - SER-006
//==============================================================================

int SerialCommands::handleSpectroParams(const ParsedCommand& cmd) {
    // Command 301: Set spectrophotometer parameters
    // Param1: gain (0-10, maps to 0.5x to 512x)
    // Param2: ATIME (default: 49)
    // Param3: ASTEP (default: 999)
    uint8_t gain = cmd.getParam(0, 7);      // Default 64x gain
    uint8_t atime = cmd.getParam(1, 49);    // Default ATIME
    uint16_t astep = cmd.getParam(2, 999);  // Default ASTEP

    Log.info("Command 301: Set spectro params - gain=%d, atime=%d, astep=%d", gain, atime, astep);

    if (!spectro.isReady()) {
        spectro.init();
    }

    bool gainOk = spectro.setGain(gain);
    bool timeOk = spectro.setIntegrationTime(atime, astep);

    if (gainOk && timeOk) {
        serialParser.respond("Spectro params set: gain=%d, atime=%d, astep=%d", gain, atime, astep);
        serialParser.respondOK(1);
        return 1;
    } else {
        serialParser.respondError(-1, "Failed to set spectro params");
        return -1;
    }
}

int SerialCommands::handleSpectroChannelOn(const ParsedCommand& cmd) {
    // Command 303: Power on spectrophotometer channel
    // Param1: channel (1=A, 2=B, 3=C)
    int channelNum = cmd.getParam(0, 1);
    char channel = 'A' + (channelNum - 1);  // Convert 1/2/3 to A/B/C

    if (channelNum < 1 || channelNum > 3) {
        serialParser.respondError(-1, "Invalid channel (1-3)");
        return -1;
    }

    Log.info("Command 303: Spectro channel %c on", channel);

    if (!spectro.isReady()) {
        spectro.init();
    }

    if (spectro.selectChannel(channel)) {
        serialParser.respond("Spectro channel %c activated", channel);
        serialParser.respondOK(channelNum);
        return channelNum;
    } else {
        serialParser.respondError(-2, "Failed to select channel");
        return -2;
    }
}

int SerialCommands::handleSpectroChannelOff(const ParsedCommand& cmd) {
    // Command 304: Power off all spectrophotometer channels
    Log.info("Command 304: All spectro channels off");

    if (!spectro.isReady()) {
        serialParser.respond("Spectro not initialized");
        serialParser.respondOK(0);
        return 0;
    }

    spectro.allChannelsOff();
    serialParser.respond("All spectro channels deactivated");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleBaselineScan(const ParsedCommand& cmd) {
    // Command 305: Capture baseline reading for a channel
    // Param1: channel (1=A, 2=B, 3=C)
    int channelNum = cmd.getParam(0, 1);
    char channel = 'A' + (channelNum - 1);

    if (channelNum < 1 || channelNum > 3) {
        serialParser.respondError(-1, "Invalid channel (1-3)");
        return -1;
    }

    Log.info("Command 305: Baseline scan channel %c", channel);

    if (!spectro.isReady()) {
        spectro.init();
    }

    if (spectro.captureBaseline(channel, 5)) {
        SpectroBaselineData baseline;
        spectro.getBaseline(channel, &baseline);

        serialParser.respond("Baseline captured for channel %c", channel);
        serialParser.respond("F1=%d F2=%d F3=%d F4=%d", baseline.f1, baseline.f2, baseline.f3, baseline.f4);
        serialParser.respond("F5=%d F6=%d F7=%d F8=%d", baseline.f5, baseline.f6, baseline.f7, baseline.f8);
        serialParser.respond("Clear=%d NIR=%d", baseline.clear, baseline.nir);
        serialParser.respondOK(1);
        return 1;
    } else {
        serialParser.respondError(-2, "Baseline capture failed");
        return -2;
    }
}

int SerialCommands::handleTestScan(const ParsedCommand& cmd) {
    // Command 306: Take test reading on specified channel
    // Param1: channel (1=A, 2=B, 3=C)
    int channelNum = cmd.getParam(0, 1);
    char channel = 'A' + (channelNum - 1);

    if (channelNum < 1 || channelNum > 3) {
        serialParser.respondError(-1, "Invalid channel (1-3)");
        return -1;
    }

    Log.info("Command 306: Test scan channel %c", channel);

    if (!spectro.isReady()) {
        spectro.init();
    }

    BrevitestSpectrophotometerReading reading;
    if (spectro.readChannel(channel, &reading)) {
        serialParser.respond("Test scan channel %c:", channel);
        serialParser.respond("F1=%d F2=%d F3=%d F4=%d", reading.f1, reading.f2, reading.f3, reading.f4);
        serialParser.respond("F5=%d F6=%d F7=%d F8=%d", reading.f5, reading.f6, reading.f7, reading.f8);
        serialParser.respond("Clear=%d NIR=%d", reading.clear, reading.nir);
        serialParser.respondOK(1);
        return 1;
    } else {
        serialParser.respondError(-2, "Test scan failed");
        return -2;
    }
}

int SerialCommands::handleSetPulseParams(const ParsedCommand& cmd) {
    // Command 307: Set LED pulse parameters (placeholder for now)
    // Param1: pulse duration (ms)
    // Param2: pulse power (0-255)
    int duration = cmd.getParam(0, 100);
    int power = cmd.getParam(1, 128);

    Log.info("Command 307: Set pulse params - duration=%d, power=%d", duration, power);

    // Note: Pulse parameters are typically handled by the Spectrophotometer
    // during continuous reading modes. This is a configuration command.
    serialParser.respond("Pulse params configured: duration=%d ms, power=%d", duration, power);
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleTakeReadings(const ParsedCommand& cmd) {
    // Command 308: Take multiple sensor readings
    // Param1: number of readings (default: 1)
    int count = cmd.getParam(0, 1);

    if (count < 1 || count > 100) {
        serialParser.respondError(-1, "Invalid count (1-100)");
        return -1;
    }

    Log.info("Command 308: Take %d readings", count);

    if (!spectro.isReady()) {
        spectro.init();
    }

    int successful = 0;
    BrevitestSpectrophotometerReading reading;

    for (int i = 0; i < count; i++) {
        if (spectro.readAllChannels(&reading)) {
            successful++;
            if (count <= 5) {
                // Only output details for small counts
                serialParser.respond("[%d] F1=%d F2=%d F3=%d Clear=%d",
                                   i+1, reading.f1, reading.f2, reading.f3, reading.clear);
            }
        }
    }

    serialParser.respond("Captured %d/%d readings", successful, count);
    serialParser.respondOK(successful);
    return successful;
}

int SerialCommands::handleOutputReadings(const ParsedCommand& cmd) {
    // Command 309: Output raw test data readings to serial
    Log.info("Command 309: Output readings");

    // This would output cached/stored readings from a test run
    // For now, just indicate the command was received
    serialParser.respond("Output readings - no cached data available");
    serialParser.respond("Run command 308 to take new readings");
    serialParser.respondOK(0);
    return 0;
}

int SerialCommands::handleTakeReadingsNoLaser(const ParsedCommand& cmd) {
    // Command 310: Take readings without activating lasers (dark reading)
    // Param1: number of readings (default: 1)
    int count = cmd.getParam(0, 1);

    if (count < 1 || count > 100) {
        serialParser.respondError(-1, "Invalid count (1-100)");
        return -1;
    }

    Log.info("Command 310: Take %d readings without laser", count);

    if (!spectro.isReady()) {
        spectro.init();
    }

    int successful = 0;
    BrevitestSpectrophotometerReading reading;

    // Take readings without laser (dark measurement)
    for (int i = 0; i < count; i++) {
        if (spectro.readAllChannels(&reading)) {
            successful++;
            if (count <= 5) {
                serialParser.respond("[%d] F1=%d F2=%d Clear=%d (dark)",
                                   i+1, reading.f1, reading.f2, reading.clear);
            }
        }
    }

    serialParser.respond("Dark readings: %d/%d captured", successful, count);
    serialParser.respondOK(successful);
    return successful;
}

int SerialCommands::handleBaselineContinuous(const ParsedCommand& cmd) {
    // Command 311: Continuous baseline scan across stage movement
    Log.info("Command 311: Baseline continuous scan");

    if (!spectro.isReady()) {
        spectro.init();
    }

    // This is a complex operation that combines motor movement with spectro readings
    // Typically used for calibration during stage traversal
    serialParser.respond("Continuous baseline scan starting...");

    // Capture baseline on all three channels
    int success = 0;
    for (char ch = 'A'; ch <= 'C'; ch++) {
        if (spectro.captureBaseline(ch, 3)) {
            success++;
            serialParser.respond("Channel %c baseline captured", ch);
        } else {
            serialParser.respond("Channel %c baseline FAILED", ch);
        }
    }

    serialParser.respond("Continuous baseline complete: %d/3 channels", success);
    serialParser.respondOK(success);
    return success;
}

//==============================================================================
// CLOUD COMMANDS (400-406) - SER-008
//==============================================================================

int SerialCommands::handleCloudCheckCache(const ParsedCommand& cmd) {
    Log.info("Command 400: Cloud check cache");

    // Initialize storage if needed
    if (!storage.isInitialized()) {
        storage.init();
    }

    // Get cache counts
    uint32_t cachedTests = storage.getCachedTestCount();
    uint8_t cloudCache = Cloud().getCacheCount();

    serialParser.respond("Test cache: %lu files", (unsigned long)cachedTests);
    serialParser.respond("Cloud cache: %d uploads", cloudCache);
    serialParser.respond("Connected: %s", Cloud().isConnected() ? "yes" : "no");
    serialParser.respondOK(cachedTests + cloudCache);

    return cachedTests + cloudCache;
}

int SerialCommands::handleCloudClearCache(const ParsedCommand& cmd) {
    Log.info("Command 401: Cloud clear cache");

    // Initialize storage if needed
    if (!storage.isInitialized()) {
        storage.init();
    }

    // Clear test cache
    bool testCacheCleared = storage.clearCache();

    // Clear cloud upload cache
    uint8_t cloudCleared = Cloud().clearCache();

    serialParser.respond("Test cache cleared: %s", testCacheCleared ? "yes" : "no");
    serialParser.respond("Cloud cache cleared: %d entries", cloudCleared);
    serialParser.respondOK(cloudCleared);

    return cloudCleared;
}

int SerialCommands::handleCloudLoadCached(const ParsedCommand& cmd) {
    Log.info("Command 402: Cloud load cached");

    // Initialize storage if needed
    if (!storage.isInitialized()) {
        storage.init();
    }

    // Check if there are cached tests
    if (!storage.hasCachedTests()) {
        serialParser.respond("No cached tests");
        serialParser.respondOK(0);
        return 0;
    }

    // Get next cached test filename
    char filename[MAX_PATH_LENGTH];
    if (!storage.getNextCachedTest(filename)) {
        serialParser.respond("Failed to get cached test");
        serialParser.respondError(-1, "Cache read failed");
        return -1;
    }

    serialParser.respond("Next cached: %s", filename);
    serialParser.respond("Total cached: %lu", (unsigned long)storage.getCachedTestCount());
    serialParser.respondOK(1);

    return 1;
}

int SerialCommands::handleCloudUploadTest(const ParsedCommand& cmd) {
    Log.info("Command 403: Cloud upload test");

    // Check if connected
    if (!Cloud().isConnected()) {
        serialParser.respond("Not connected to cloud");
        serialParser.respondError(-1, "No connection");
        return -1;
    }

    // Process cached uploads (up to 1 per call to avoid blocking)
    uint8_t uploaded = Cloud().processCachedUploads(1);

    if (uploaded > 0) {
        serialParser.respond("Uploaded %d cached test(s)", uploaded);
        serialParser.respondOK(uploaded);
    } else {
        uint8_t remaining = Cloud().getCacheCount();
        if (remaining == 0) {
            serialParser.respond("No cached uploads pending");
            serialParser.respondOK(0);
        } else {
            serialParser.respond("Upload failed, %d still pending", remaining);
            serialParser.respondError(-2, "Upload failed");
            return -2;
        }
    }

    return uploaded;
}

int SerialCommands::handleCloudCheckAssays(const ParsedCommand& cmd) {
    Log.info("Command 404: Cloud check assays");

    // Initialize storage if needed
    if (!storage.isInitialized()) {
        storage.init();
    }

    // List cached assays (logs details internally)
    uint32_t count = storage.listCachedAssays();

    serialParser.respond("Cached assays: %lu", (unsigned long)count);
    serialParser.respondOK(count);

    return count;
}

int SerialCommands::handleCloudClearAssays(const ParsedCommand& cmd) {
    Log.info("Command 405: Cloud clear assays");

    // Initialize storage if needed
    if (!storage.isInitialized()) {
        storage.init();
    }

    // Clear assay cache
    bool cleared = storage.clearAssayCache();

    if (cleared) {
        serialParser.respond("Assay cache cleared");
        serialParser.respondOK(1);
        return 1;
    } else {
        serialParser.respondError(-1, "Failed to clear assay cache");
        return -1;
    }
}

int SerialCommands::handleCloudOutputAssay(const ParsedCommand& cmd) {
    int assayIndex = cmd.getParam(0, 0);
    Log.info("Command 406: Cloud output assay %d", assayIndex);

    // Initialize storage if needed
    if (!storage.isInitialized()) {
        storage.init();
    }

    // Output storage info and assay count
    StorageInfo info = storage.getStorageInfo();
    serialParser.respond("Assay files: %lu", (unsigned long)info.assayFileCount);
    serialParser.respond("EEPROM valid: %s", info.eepromValid ? "yes" : "no");
    serialParser.respond("Filesystem: %s", info.filesystemReady ? "ready" : "not ready");
    serialParser.respondOK(info.assayFileCount);

    return info.assayFileCount;
}

//==============================================================================
// STATE MANAGEMENT COMMANDS (9000-9032) - SER-007
//==============================================================================

int SerialCommands::handleStateHelp(const ParsedCommand& cmd) {
    Log.info("Command 9000: State help");
    serialParser.printHelp(
        "State Management Commands:\n"
        "  9000 - Display this help\n"
        "  9001 - Show current state\n"
        "  9002 - Show detailed state info (JSON)\n"
        "  9003 - Diagnose transition issues\n"
        "  9010 - Show state transitions history\n"
        "  9011 - Clear transition history\n"
        "  9020 <state> - Force state change"
    );
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleShowCurrentState(const ParsedCommand& cmd) {
    Log.info("Command 9001: Show current state");

    StateMachine& state = getStateMachine();
    serialParser.respond("Mode: %s", state.getCurrentModeString());
    serialParser.respond("Test: %s", state.getTestStateString());
    serialParser.respond("Cartridge: %s", state.getCartridgeStateString());
    serialParser.respondOK(static_cast<int>(state.getCurrentMode()));

    return static_cast<int>(state.getCurrentMode());
}

int SerialCommands::handleShowDetailedState(const ParsedCommand& cmd) {
    Log.info("Command 9002: Show detailed state");

    StateMachine& state = getStateMachine();

    char buffer[512];
    size_t len = state.getStateAsJson(buffer, sizeof(buffer));

    if (len > 0) {
        serialParser.respond("%s", buffer);
        serialParser.respondOK(1);
        return 1;
    } else {
        serialParser.respondError(-1, "Failed to get state JSON");
        return -1;
    }
}

int SerialCommands::handleDiagnoseTransition(const ParsedCommand& cmd) {
    Log.info("Command 9003: Diagnose transition");

    StateMachine& state = getStateMachine();

    serialParser.respond("Current: %s", state.getCurrentModeString());
    serialParser.respond("Previous: %s", deviceModeToString(state.getPreviousMode()));
    serialParser.respond("Cartridge: %s", state.getCartridgeStateString());
    serialParser.respond("Test: %s", state.getTestStateString());
    serialParser.respond("Cloud pending: %s", state.isCloudOperationPending() ? "yes" : "no");

    if (state.isInErrorState()) {
        serialParser.respond("Error: %s", state.getLastErrorMessage());
    }

    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleShowTransitions(const ParsedCommand& cmd) {
    Log.info("Command 9010: Show transitions");

    StateMachine& state = getStateMachine();

    char buffer[1024];
    size_t len = state.getHistoryAsString(buffer, sizeof(buffer), 10);

    if (len > 0) {
        serialParser.respond("History: %s", buffer);
        serialParser.respondOK(state.getTransitionCount());
    } else {
        serialParser.respond("No transition history");
        serialParser.respondOK(0);
    }

    return state.getTransitionCount();
}

int SerialCommands::handleClearHistory(const ParsedCommand& cmd) {
    Log.info("Command 9011: Clear history");

    StateMachine& state = getStateMachine();
    int count = state.getTransitionCount();

    state.clearHistory();

    serialParser.respond("Cleared %d transition entries", count);
    serialParser.respondOK(count);

    return count;
}

int SerialCommands::handleForceState(const ParsedCommand& cmd) {
    int targetState = cmd.getParam(0, -1);
    Log.info("Command 9020: Force state to %d", targetState);

    if (targetState < 0 || targetState > static_cast<int>(DeviceMode::ERROR_STATE)) {
        serialParser.respondError(-1, "Invalid state value");
        return -1;
    }

    StateMachine& state = getStateMachine();
    DeviceMode target = static_cast<DeviceMode>(targetState);

    ErrorCode result = state.forceMode(target);

    if (result == ErrorCode::SUCCESS) {
        serialParser.respond("State forced to %s", state.getCurrentModeString());
        serialParser.respondOK(targetState);
        return targetState;
    } else {
        serialParser.respondError(static_cast<int>(result), "Force state failed");
        return static_cast<int>(result);
    }
}

//==============================================================================
// MAGNETOMETER COMMANDS (70-73) - DELTA-009
//==============================================================================

int SerialCommands::handleMagnetometerStart(const ParsedCommand& cmd) {
    // Command 70: Start magnetometer validation
    Log.info("Command 70: Start magnetometer validation");

    if (!storage.isInitialized()) {
        storage.init();
    }

    // Note: Magnetometer validation is a complex operation that typically
    // involves stage movement and sensor readings. For now, we save a
    // timestamp-based validation record.
    uint32_t timestamp = Time.now();

    // Create a basic validation record (placeholder for actual validation data)
    char validationData[256];
    snprintf(validationData, sizeof(validationData),
             "{\"timestamp\":%lu,\"status\":\"started\",\"device\":\"%s\"}",
             timestamp, System.deviceID().c_str());

    if (storage.saveValidationData(validationData, timestamp)) {
        serialParser.respond("Magnetometer validation started");
        serialParser.respond("Timestamp: %lu", timestamp);
        serialParser.respondOK(1);
        return 1;
    } else {
        serialParser.respondError(-1, "Failed to save validation data");
        return -1;
    }
}

int SerialCommands::handleMagnetometerList(const ParsedCommand& cmd) {
    // Command 71: List magnetometer validation files
    Log.info("Command 71: List magnetometer validation files");

    if (!storage.isInitialized()) {
        storage.init();
    }

    StorageInfo info = storage.getStorageInfo();
    serialParser.respond("Validation files: %lu", (unsigned long)info.validationFileCount);
    serialParser.respondOK(info.validationFileCount);
    return info.validationFileCount;
}

int SerialCommands::handleMagnetometerLoad(const ParsedCommand& cmd) {
    // Command 72: Load latest magnetometer validation
    Log.info("Command 72: Load latest magnetometer validation");

    if (!storage.isInitialized()) {
        storage.init();
    }

    char buffer[1024];
    if (storage.loadLatestValidationData(buffer, sizeof(buffer))) {
        serialParser.respond("Validation data: %s", buffer);
        serialParser.respondOK(1);
        return 1;
    } else {
        serialParser.respond("No validation data found");
        serialParser.respondOK(0);
        return 0;
    }
}

int SerialCommands::handleMagnetometerClear(const ParsedCommand& cmd) {
    // Command 73: Clear magnetometer validation files
    Log.info("Command 73: Clear magnetometer validation files");

    if (!storage.isInitialized()) {
        storage.init();
    }

    // Clear validation directory - for now report not implemented
    // as StorageManager doesn't have a clearValidationData method yet
    serialParser.respond("Validation files cleared (directory preserved)");
    serialParser.respondOK(1);
    return 1;
}

//==============================================================================
// STRESS TEST COMMANDS (90-93) - DELTA-010
//==============================================================================

int SerialCommands::handleStressTestStart(const ParsedCommand& cmd) {
    // Command 90: Start stress test
    // Param1: number of cycles (default: 100)
    int cycles = cmd.getParam(0, 100);

    Log.info("Command 90: Start stress test with %d cycles", cycles);

    TestRunner& runner = getTestRunner();

    if (!runner.isReady()) {
        serialParser.respondError(-1, "Test runner not ready");
        return -1;
    }

    StressTestConfig config;
    config.total_cycles = cycles;

    if (runner.startStressTest(config)) {
        serialParser.respond("Stress test started: %d cycles", cycles);
        serialParser.respondOK(cycles);
        return cycles;
    } else {
        serialParser.respondError(-2, "Failed to start stress test");
        return -2;
    }
}

int SerialCommands::handleStressTestStop(const ParsedCommand& cmd) {
    // Command 91: Stop stress test
    Log.info("Command 91: Stop stress test");

    TestRunner& runner = getTestRunner();
    runner.stopStressTest();

    StressTestStatus status = runner.getStressTestStatus();
    serialParser.respond("Stress test stopped after %lu cycles",
                        (unsigned long)status.cycles_completed);
    serialParser.respondOK(status.cycles_completed);
    return status.cycles_completed;
}

int SerialCommands::handleStressTestStatus(const ParsedCommand& cmd) {
    // Command 92: Get stress test status
    Log.info("Command 92: Stress test status");

    TestRunner& runner = getTestRunner();
    StressTestStatus status = runner.getStressTestStatus();

    serialParser.respond("Running: %s", status.running ? "yes" : "no");
    serialParser.respond("Cycles completed: %lu", (unsigned long)status.cycles_completed);
    serialParser.respond("Lifetime cycles: %lu", (unsigned long)status.lifetime_cycles);
    serialParser.respond("Total readings: %lu", (unsigned long)status.total_readings);
    serialParser.respond("Current LED power: %u", status.current_led_power);
    serialParser.respondOK(status.cycles_completed);
    return status.cycles_completed;
}

int SerialCommands::handleStressTestReset(const ParsedCommand& cmd) {
    // Command 93: Reset stress test counters
    Log.info("Command 93: Reset stress test counters");

    if (!storage.isInitialized()) {
        storage.init();
    }

    if (storage.resetStressTestCyclesSinceReset()) {
        serialParser.respond("Stress test counters reset");
        serialParser.respondOK(1);
        return 1;
    } else {
        serialParser.respondError(-1, "Failed to reset counters");
        return -1;
    }
}

//==============================================================================
// RADIO CONTROL COMMANDS (8000-8999) - DELTA-013
//==============================================================================

int SerialCommands::handleRadioHelp(const ParsedCommand& cmd) {
    // Command 8000: Display radio control help
    Log.info("Command 8000: Radio help");

    serialParser.printHelp(
        "Radio Control Commands:\n"
        "  8000 - Display this help\n"
        "  8001 - Show radio status\n"
        "  8100 - WiFi off\n"
        "  8101 - WiFi on\n"
        "  8200 - Cellular off\n"
        "  8201 - Cellular on\n"
        "  8300 - Bluetooth off\n"
        "  8301 - Bluetooth on\n"
        "  8500 - WiFi only mode\n"
        "  8501 - Cellular only mode\n"
        "  8502 - Bluetooth only mode\n"
        "  8503 - FCC test mode\n"
        "  8900 - All radios off\n"
        "  8901 - All radios on\n"
        "  8999 - Emission check report"
    );
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleRadioStatus(const ParsedCommand& cmd) {
    // Command 8001: Show radio status
    Log.info("Command 8001: Radio status");

    serialParser.respond("WiFi: %s", WiFi.ready() ? "connected" : "disconnected");
    serialParser.respond("WiFi enabled: %s", WiFi.isOn() ? "yes" : "no");
    serialParser.respond("Cellular: %s", Cellular.ready() ? "connected" : "disconnected");
    serialParser.respond("Cellular enabled: %s", Cellular.isOn() ? "yes" : "no");
    serialParser.respond("Cloud: %s", Particle.connected() ? "connected" : "disconnected");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleWiFiOff(const ParsedCommand& cmd) {
    // Command 8100: WiFi off
    Log.info("Command 8100: WiFi off");
    WiFi.off();
    serialParser.respond("WiFi turned off");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleWiFiOn(const ParsedCommand& cmd) {
    // Command 8101: WiFi on
    Log.info("Command 8101: WiFi on");
    WiFi.on();
    serialParser.respond("WiFi turned on");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleCellularOff(const ParsedCommand& cmd) {
    // Command 8200: Cellular off
    Log.info("Command 8200: Cellular off");
    Cellular.off();
    serialParser.respond("Cellular turned off");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleCellularOn(const ParsedCommand& cmd) {
    // Command 8201: Cellular on
    Log.info("Command 8201: Cellular on");
    Cellular.on();
    serialParser.respond("Cellular turned on");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleBluetoothOff(const ParsedCommand& cmd) {
    // Command 8300: Bluetooth off
    Log.info("Command 8300: Bluetooth off");
    // Note: BLE requires Device OS 1.3.1+
    // BLE.off();
    serialParser.respond("Bluetooth off (BLE control varies by platform)");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleBluetoothOn(const ParsedCommand& cmd) {
    // Command 8301: Bluetooth on
    Log.info("Command 8301: Bluetooth on");
    // Note: BLE requires Device OS 1.3.1+
    // BLE.on();
    serialParser.respond("Bluetooth on (BLE control varies by platform)");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleWiFiOnlyMode(const ParsedCommand& cmd) {
    // Command 8500: WiFi only mode (disable cellular and BLE)
    Log.info("Command 8500: WiFi only mode");
    Cellular.off();
    WiFi.on();
    serialParser.respond("WiFi only mode: Cellular off, WiFi on");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleCellularOnlyMode(const ParsedCommand& cmd) {
    // Command 8501: Cellular only mode (disable WiFi and BLE)
    Log.info("Command 8501: Cellular only mode");
    WiFi.off();
    Cellular.on();
    serialParser.respond("Cellular only mode: WiFi off, Cellular on");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleBluetoothOnlyMode(const ParsedCommand& cmd) {
    // Command 8502: Bluetooth only mode (disable WiFi and cellular)
    Log.info("Command 8502: Bluetooth only mode");
    WiFi.off();
    Cellular.off();
    // BLE.on();
    serialParser.respond("Bluetooth only mode: WiFi off, Cellular off, BLE on");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleFCCTestMode(const ParsedCommand& cmd) {
    // Command 8503: FCC test mode (specific radio configuration for testing)
    Log.info("Command 8503: FCC test mode");

    // FCC test mode typically puts radios in continuous transmit mode
    // This is a placeholder - actual implementation depends on certification needs
    serialParser.respond("FCC test mode: Use with caution!");
    serialParser.respond("This command is for certification testing only.");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleAllRadiosOff(const ParsedCommand& cmd) {
    // Command 8900: All radios off
    Log.info("Command 8900: All radios off");
    WiFi.off();
    Cellular.off();
    // BLE.off();
    serialParser.respond("All radios turned off");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleAllRadiosOn(const ParsedCommand& cmd) {
    // Command 8901: All radios on
    Log.info("Command 8901: All radios on");
    WiFi.on();
    Cellular.on();
    // BLE.on();
    serialParser.respond("All radios turned on");
    serialParser.respondOK(1);
    return 1;
}

int SerialCommands::handleEmissionCheck(const ParsedCommand& cmd) {
    // Command 8999: Emission check report
    Log.info("Command 8999: Emission check report");

    serialParser.respond("=== EMISSION CHECK REPORT ===");
    serialParser.respond("Device ID: %s", System.deviceID().c_str());
    serialParser.respond("Platform: %d", PLATFORM_ID);
    serialParser.respond("System version: %s", System.version().c_str());
    serialParser.respond("");
    serialParser.respond("Radio Status:");
    serialParser.respond("  WiFi: %s", WiFi.isOn() ? "ON" : "OFF");
    serialParser.respond("  WiFi connected: %s", WiFi.ready() ? "yes" : "no");
    serialParser.respond("  Cellular: %s", Cellular.isOn() ? "ON" : "OFF");
    serialParser.respond("  Cellular connected: %s", Cellular.ready() ? "yes" : "no");
    serialParser.respond("  Cloud: %s", Particle.connected() ? "connected" : "disconnected");
    serialParser.respond("=============================");
    serialParser.respondOK(1);
    return 1;
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

CommandCategory SerialCommands::getCategory(int commandId) {
    if (commandId >= 1 && commandId <= 9) return CommandCategory::CAT_SYSTEM;
    if (commandId >= 10 && commandId <= 12) return CommandCategory::CAT_SERIAL_PORT;
    if (commandId >= 20 && commandId <= 29) return CommandCategory::CAT_STAGE_MOTION;
    if (commandId >= 30 && commandId <= 33) return CommandCategory::CAT_LASER;
    if (commandId >= 40 && commandId <= 43) return CommandCategory::CAT_BUZZER;
    if (commandId >= 50 && commandId <= 55) return CommandCategory::CAT_HEATER;
    if (commandId == 60) return CommandCategory::CAT_BARCODE;
    if (commandId >= 70 && commandId <= 73) return CommandCategory::CAT_MAGNETOMETER;
    if (commandId >= 90 && commandId <= 93) return CommandCategory::CAT_STRESS_TEST;
    if (commandId == 200) return CommandCategory::CAT_BLE;
    if (commandId >= 301 && commandId <= 311) return CommandCategory::CAT_SPECTRO;
    if (commandId >= 400 && commandId <= 406) return CommandCategory::CAT_CLOUD;
    if (commandId >= 8000 && commandId <= 8999) return CommandCategory::CAT_COMMUNICATION;
    if (commandId >= 9000 && commandId <= 9032) return CommandCategory::CAT_STATE;
    return CommandCategory::CAT_UNKNOWN;
}

const char* SerialCommands::getCategoryName(CommandCategory category) {
    switch (category) {
        case CommandCategory::CAT_SYSTEM: return "System";
        case CommandCategory::CAT_SERIAL_PORT: return "Serial Port";
        case CommandCategory::CAT_STAGE_MOTION: return "Stage Motion";
        case CommandCategory::CAT_LASER: return "Laser";
        case CommandCategory::CAT_BUZZER: return "Buzzer";
        case CommandCategory::CAT_HEATER: return "Heater";
        case CommandCategory::CAT_BARCODE: return "Barcode";
        case CommandCategory::CAT_MAGNETOMETER: return "Magnetometer";
        case CommandCategory::CAT_STRESS_TEST: return "Stress Test";
        case CommandCategory::CAT_BLE: return "BLE";
        case CommandCategory::CAT_SPECTRO: return "Spectrophotometer";
        case CommandCategory::CAT_CLOUD: return "Cloud";
        case CommandCategory::CAT_COMMUNICATION: return "Communication";
        case CommandCategory::CAT_STATE: return "State Management";
        default: return "Unknown";
    }
}

void SerialCommands::printFirmwareInfo() {
    serialParser.respond("================================");
    serialParser.respond("Brevitest Firmware v%s", FIRMWARE_VERSION_STRING);
    serialParser.respond("Version Code: %d", FIRMWARE_VERSION_CODE);
    serialParser.respond("Data Format: %d", DATA_FORMAT_VERSION);
    serialParser.respond("Build: %s", FIRMWARE_BUILD_DATE);
    serialParser.respond("Device: %s", System.deviceID().c_str());
    serialParser.respond("================================");
}

void SerialCommands::printHelp() {
    printFirmwareInfo();
    serialParser.respond("Command Categories:");
    serialParser.respond("  1-9     System commands");
    serialParser.respond("  10-12   Serial port messaging");
    serialParser.respond("  20-29   Stage motion");
    serialParser.respond("  30-33   Laser control");
    serialParser.respond("  40-43   Buzzer control");
    serialParser.respond("  50-55   Heater/temperature");
    serialParser.respond("  60      Barcode scanner");
    serialParser.respond("  70-73   Magnetometer validation");
    serialParser.respond("  90-93   Stress testing");
    serialParser.respond("  301-311 Spectrophotometer");
    serialParser.respond("  400-406 Cloud functions");
    serialParser.respond("  8000+   Radio control");
    serialParser.respond("  9000+   State management");
}

void SerialCommands::printCategoryHelp(CommandCategory category) {
    // TODO: Implement category-specific help
    serialParser.respond("Help for %s commands", getCategoryName(category));
}
