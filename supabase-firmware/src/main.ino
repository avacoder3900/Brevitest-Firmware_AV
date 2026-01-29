/**
 * @file main.ino
 * @brief Main entry point for Brevitest Supabase Firmware
 * @version 57
 * @date January 2026
 *
 * This firmware controls the Brevitest diagnostic device with Supabase cloud
 * backend integration, replacing the legacy Particle/CouchDB architecture.
 *
 * Target Platform: Particle Boron (NRF52840)
 * DeviceOS: 6.3.3
 */

#include "Particle.h"

// System configuration
SYSTEM_MODE(AUTOMATIC);
SYSTEM_THREAD(ENABLED);

// Module includes
#include "DataTypes.h"
#include "HardwareConfig.h"
#include "HAL.h"
#include "StateMachine.h"
#include "StorageManager.h"
#include "SupabaseClient.h"
#include "MotorController.h"
#include "HeaterController.h"
#include "LaserController.h"
#include "Spectrophotometer.h"
#include "BarcodeScanner.h"
#include "BuzzerController.h"
#include "LEDController.h"
#include "TestRunner.h"
#include "BCODEInterpreter.h"

//==============================================================================
// FIRMWARE VERSION
//==============================================================================

#define FIRMWARE_VERSION_CODE 57
#define FIRMWARE_VERSION_STRING "57.0.0-supabase"

//==============================================================================
// GLOBAL STATE
//==============================================================================

// Device state machine - declared extern in StateMachine.h, defined in StateMachine.cpp

// Hardware controllers
MotorController motorController;
HeaterController heaterController;
LaserController laserController;
Spectrophotometer spectrophotometer;
BarcodeScanner barcodeScanner;
// Note: BuzzerController is a namespace, not a class - use BuzzerController:: functions directly
// Note: LEDController is also a namespace - use LEDController:: functions directly

// Cloud client
SupabaseClient cloudClient;

// Test runner (singleton)
TestRunner& testRunner = getTestRunner();

// Current test data
BrevitestAssay currentAssay;
char currentCartridgeId[BARCODE_UUID_LENGTH + 1];

// Timing
unsigned long lastHeaterUpdate = 0;
unsigned long lastStatusUpdate = 0;
const unsigned long HEATER_UPDATE_INTERVAL = 100;  // 100ms
const unsigned long STATUS_UPDATE_INTERVAL = 5000; // 5 seconds

// Serial command buffer
char serialBuffer[64];
int serialBufferIndex = 0;

// Logging
SerialLogHandler logHandler(LOG_LEVEL_INFO);

//==============================================================================
// FORWARD DECLARATIONS
//==============================================================================

void processSerialCommands();
void handleCartridgeInserted();
void handleCartridgeRemoved();
void onStateChange(DeviceMode oldMode, DeviceMode newMode);
void runHeaterControl();
void updateStatusLED();

//==============================================================================
// INTERRUPT HANDLER
//==============================================================================

volatile bool cartridgeStateChanged = false;

void cartridgeInterruptHandler() {
    cartridgeStateChanged = true;
}

//==============================================================================
// SETUP
//==============================================================================

void setup() {
    // Initialize serial
    Serial.begin(115200);
    delay(2000);  // Wait for serial connection

    Log.info("===========================================");
    Log.info("Brevitest Supabase Firmware v%s", FIRMWARE_VERSION_STRING);
    Log.info("===========================================");

    // Initialize HAL
    Log.info("Initializing HAL...");
    if (!HAL::init()) {
        Log.error("HAL initialization failed!");
        deviceState.setMode(DeviceMode::ERROR_STATE);
        return;
    }

    // Initialize state machine
    Log.info("Initializing state machine...");
    deviceState.init();
    deviceState.registerStateChangeCallback(onStateChange);

    // Initialize storage
    Log.info("Initializing storage...");
    // StorageManager::init();

    // Initialize motor controller
    Log.info("Initializing motor controller...");
    if (!motorController.init()) {
        Log.warn("Motor controller initialization failed");
    }

    // Initialize heater controller
    Log.info("Initializing heater controller...");
    if (!heaterController.init()) {
        Log.warn("Heater controller initialization failed");
    }
    heaterController.setTargetTemperature(450);  // 45.0°C
    heaterController.enable();

    // Initialize laser controller
    Log.info("Initializing laser controller...");
    if (!laserController.init()) {
        Log.warn("Laser controller initialization failed");
    }

    // Initialize spectrophotometer
    Log.info("Initializing spectrophotometer...");
    if (!spectrophotometer.init()) {
        Log.warn("Spectrophotometer initialization failed");
    }

    // Initialize barcode scanner
    Log.info("Initializing barcode scanner...");
    if (!barcodeScanner.init()) {
        Log.warn("Barcode scanner initialization failed");
    }

    // Initialize buzzer
    Log.info("Initializing buzzer...");
    if (!BuzzerController::init()) {
        Log.warn("Buzzer initialization failed");
    }

    // Initialize LED controller
    Log.info("Initializing LED controller...");
    if (!LEDController::init()) {
        Log.warn("LED controller initialization failed");
    }

    // Initialize cloud client
    Log.info("Initializing Supabase client...");
    cloudClient.init();
    // cloudClient.setEndpoint("https://your-project.supabase.co/functions/v1");
    // cloudClient.setApiKey("your-api-key");

    // Initialize test runner
    Log.info("Initializing test runner...");
    testRunner.init();
    setupTestRunnerCallbacks();

    // Setup cartridge detection interrupt
    attachInterrupt(PIN_CARTRIDGE_DETECT, cartridgeInterruptHandler, CHANGE);

    // Set initial state
    deviceState.setMode(DeviceMode::IDLE);

    // Startup beep
    BuzzerController::playTone(BUZZER_FREQUENCY, 200);

    Log.info("Initialization complete. Device ready.");
    Log.info("===========================================");
}

//==============================================================================
// MAIN LOOP
//==============================================================================

void loop() {
    // Process serial commands
    processSerialCommands();

    // Check cartridge state change
    if (cartridgeStateChanged) {
        cartridgeStateChanged = false;
        delay(50);  // Debounce

        if (HAL::readCartridgeSwitch()) {
            handleCartridgeInserted();
        } else {
            handleCartridgeRemoved();
        }
    }

    // Run heater control loop
    if (millis() - lastHeaterUpdate >= HEATER_UPDATE_INTERVAL) {
        lastHeaterUpdate = millis();
        runHeaterControl();
    }

    // Update buzzer (for non-blocking patterns)
    BuzzerController::update();

    // Update LED status
    updateStatusLED();

    // State machine processing
    switch (deviceState.getCurrentMode()) {
        case DeviceMode::IDLE:
            // Waiting for cartridge insertion
            break;

        case DeviceMode::INITIALIZING:
            // One-time initialization already done in setup()
            deviceState.setMode(DeviceMode::IDLE);
            break;

        case DeviceMode::HEATING:
            // Check if heater is ready
            if (heaterController.isReady()) {
                Log.info("Heater ready, starting barcode scan");
                deviceState.setMode(DeviceMode::BARCODE_SCANNING);
                barcodeScanner.triggerScan();
            }
            break;

        case DeviceMode::BARCODE_SCANNING:
            // Check for barcode result
            if (barcodeScanner.getScanState() == ScanState::SUCCESS) {
                const char* barcode = barcodeScanner.getLastBarcode();
                strncpy(currentCartridgeId, barcode, BARCODE_UUID_LENGTH);
                currentCartridgeId[BARCODE_UUID_LENGTH] = '\0';

                Log.info("Barcode read: %s", currentCartridgeId);
                BuzzerController::playTone(BUZZER_INSERT_FREQUENCY, BUZZER_INSERT_DURATION);

                deviceState.setMode(DeviceMode::VALIDATING_CARTRIDGE);
                startCartridgeValidation();
            }
            break;

        case DeviceMode::VALIDATING_CARTRIDGE:
            // Cloud validation handled asynchronously
            // Check for validation result in cloud callbacks
            break;

        case DeviceMode::RUNNING_TEST:
            // Update test runner
            testRunner.update();

            // Check for test completion
            if (testRunner.getState() == TestRunnerState::COMPLETED) {
                Log.info("Test completed, uploading results");
                deviceState.setMode(DeviceMode::UPLOADING_RESULTS);
                startResultUpload();
            } else if (testRunner.getState() == TestRunnerState::CANCELLED ||
                       testRunner.getState() == TestRunnerState::ERROR) {
                Log.warn("Test ended with state: %s",
                         testRunnerStateToString(testRunner.getState()));
                deviceState.setMode(DeviceMode::IDLE);
                BuzzerController::playErrorMelody();
            }
            break;

        case DeviceMode::UPLOADING_RESULTS:
            // Upload handled asynchronously
            break;

        case DeviceMode::STRESS_TESTING:
            // Stress test mode
            if (testRunner.isStressTestRunning()) {
                testRunner.executeStressTestCycle();
            }
            break;

        case DeviceMode::ERROR_STATE:
            // Error indicator
            break;

        default:
            break;
    }

    // Periodic status logging
    if (millis() - lastStatusUpdate >= STATUS_UPDATE_INTERVAL) {
        lastStatusUpdate = millis();
        logStatus();
    }
}

//==============================================================================
// TEST RUNNER CALLBACKS
//==============================================================================

void setupTestRunnerCallbacks() {
    // Temperature callback
    testRunner.setGetTemperatureCallback([]() -> int16_t {
        return heaterController.getCurrentTemperature();
    });

    // Heater ready callback
    testRunner.setIsHeaterReadyCallback([]() -> bool {
        return heaterController.isReady();
    });

    // Heater power callback
    testRunner.setSetHeaterPowerCallback([](uint8_t power) {
        heaterController.setDefaultPower(power);
    });

    // Cartridge detection callback
    testRunner.setIsCartridgeInsertedCallback([]() -> bool {
        return HAL::readCartridgeSwitch();
    });

    // Stage position callback
    testRunner.setGetStagePositionCallback([]() -> int32_t {
        return motorController.getCurrentPosition();
    });

    // BCODE interpreter callbacks
    BCODEInterpreter& interp = testRunner.getInterpreter();

    // Delay loop (heater control during delays)
    interp.setDelayLoopCallback([]() -> bool {
        runHeaterControl();
        BuzzerController::update();
        return HAL::readCartridgeSwitch();  // Return false if cartridge removed
    });

    // Stage move
    interp.setStageMoveCallback([](int32_t microns, uint16_t stepDelay) -> bool {
        return motorController.moveRelative(microns, stepDelay) == MotorResult::SUCCESS;
    });

    // Stage oscillate
    interp.setStageOscillateCallback([](int32_t microns, uint16_t stepDelay,
                                        uint16_t cycles, DelayLoopCallback cb) -> bool {
        for (uint16_t i = 0; i < cycles; i++) {
            motorController.moveRelative(microns, stepDelay);
            if (cb && !cb()) return false;
            motorController.moveRelative(-microns, stepDelay);
            if (cb && !cb()) return false;
        }
        return true;
    });

    // Spectrophotometer config
    interp.setSpectroConfigCallback([](uint8_t gain, uint16_t astep, uint8_t atime) -> bool {
        spectrophotometer.setGain(gain);
        spectrophotometer.setIntegrationTime(astep, atime);
        testRunner.setSpectroSettings(gain, astep, atime);
        return true;
    });

    // Spectrophotometer scan
    interp.setSpectroScanCallback([](uint16_t numScans, bool isBaseline) -> uint16_t {
        uint16_t readingsTaken = 0;
        int32_t originalPosition = motorController.getCurrentPosition();

        for (uint16_t scan = 0; scan < numScans; scan++) {
            // Take readings on all 3 channels
            for (char channel = 'A'; channel <= 'C'; channel++) {
                laserController.enableLaser(channel);
                delay(50);  // Laser warmup

                BrevitestSpectrophotometerReading reading;
                initSpectrophotometerReading(&reading);

                reading.number = testRunner.getReadingCount();
                reading.channel = channel;
                reading.position = motorController.getCurrentPosition();
                reading.temperature = heaterController.getCurrentTemperature();
                reading.msec = testRunner.getElapsedTime();

                // Read AS7341 channels
                spectrophotometer.readChannel(channel, &reading,
                    reading.position, reading.temperature, 0);

                laserController.disableLaser(channel);

                testRunner.addReading(&reading);
                readingsTaken++;
            }
        }

        if (isBaseline) {
            testRunner.addBaselineScans(numScans);
        } else {
            testRunner.addTestScans(numScans);
        }

        // Return to original position
        motorController.moveToPosition(originalPosition, MOTOR_SLOW_STEP_DELAY);

        return readingsTaken;
    });
}

//==============================================================================
// CARTRIDGE HANDLING
//==============================================================================

void handleCartridgeInserted() {
    if (deviceState.getCurrentMode() != DeviceMode::IDLE) {
        Log.warn("Cartridge inserted but device not idle");
        return;
    }

    Log.info("Cartridge inserted");
    BuzzerController::cartridgeInsert();

    // Start heating
    deviceState.setMode(DeviceMode::HEATING);
}

void handleCartridgeRemoved() {
    Log.info("Cartridge removed");
    BuzzerController::cartridgeRemove();

    // If test was running, notify test runner
    if (deviceState.getCurrentMode() == DeviceMode::RUNNING_TEST) {
        testRunner.onCartridgeRemoved();
    }

    // Reset to idle
    deviceState.setMode(DeviceMode::IDLE);
    motorController.home();
    laserController.disableAllLasers();
}

//==============================================================================
// CLOUD OPERATIONS
//==============================================================================

void startCartridgeValidation() {
    Log.info("Starting cartridge validation for: %s", currentCartridgeId);

    // TODO: Implement async validation with SupabaseClient
    // For now, simulate successful validation

    // In production:
    // cloudClient.validateCartridge(currentCartridgeId, onValidationComplete);

    // Simulated success - load assay and start test
    delay(1000);
    onValidationComplete(true, "ASSAY001");
}

void onValidationComplete(bool success, const char* assayId) {
    if (!success) {
        Log.error("Cartridge validation failed");
        deviceState.setMode(DeviceMode::IDLE);
        BuzzerController::playErrorMelody();
        return;
    }

    Log.info("Cartridge validated, assay: %s", assayId);

    // Load assay (in production, download from cloud)
    strcpy(currentAssay.id, assayId);

    // Example BCODE for testing
    const char* testBcode = "0:|10:7,999,49|2:7860,300|11:5|1:5000|14:10|2:-7860,300|99:";
    strcpy(currentAssay.BCODE, testBcode);
    currentAssay.BCODE_length = strlen(testBcode);
    currentAssay.duration = 60;

    // Start test
    if (testRunner.startTest(&currentAssay, currentCartridgeId)) {
        deviceState.setMode(DeviceMode::RUNNING_TEST);
        Log.info("Test started");
    } else {
        Log.error("Failed to start test: %s", testRunner.getErrorMessage());
        deviceState.setMode(DeviceMode::IDLE);
        BuzzerController::playErrorMelody();
    }
}

void startResultUpload() {
    BrevitestTestRecord* record = testRunner.getTestRecord();

    Log.info("Uploading test record: %u readings, %u sec duration",
             record->number_of_readings, record->duration);

    // TODO: Implement async upload with SupabaseClient
    // cloudClient.uploadTest(record, onUploadComplete);

    // Simulated success
    delay(2000);
    onUploadComplete(true);
}

void onUploadComplete(bool success) {
    if (success) {
        Log.info("Test results uploaded successfully");
        BuzzerController::playSuccessMelody();
    } else {
        Log.error("Test upload failed");
        BuzzerController::playErrorMelody();
        // TODO: Cache for retry
    }

    // Alert user to remove cartridge
    BuzzerController::startPeriodicAlert(AlertType::GENERAL);
    deviceState.setMode(DeviceMode::IDLE);
}

//==============================================================================
// HEATER CONTROL
//==============================================================================

void runHeaterControl() {
    heaterController.update();
}

//==============================================================================
// LED STATUS
//==============================================================================

void updateStatusLED() {
    switch (deviceState.getCurrentMode()) {
        case DeviceMode::IDLE:
            LEDController::setPattern(LEDColors::GREEN, BrvtLEDPattern::FADE, BrvtLEDSpeed::SLOW);
            break;
        case DeviceMode::HEATING:
            LEDController::setPattern(LEDColors::ORANGE, BrvtLEDPattern::PULSE, BrvtLEDSpeed::NORMAL);
            break;
        case DeviceMode::RUNNING_TEST:
            LEDController::setPattern(LEDColors::CYAN, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL);
            break;
        case DeviceMode::UPLOADING_RESULTS:
            LEDController::setPattern(LEDColors::BLUE, BrvtLEDPattern::BLINK, BrvtLEDSpeed::NORMAL);
            break;
        case DeviceMode::ERROR_STATE:
            LEDController::setPattern(LEDColors::RED, BrvtLEDPattern::BLINK, BrvtLEDSpeed::FAST);
            break;
        default:
            LEDController::setPattern(LEDColors::OFF, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL);
            break;
    }
    LEDController::update();
}

//==============================================================================
// STATE CHANGE CALLBACK
//==============================================================================

void onStateChange(DeviceMode oldMode, DeviceMode newMode) {
    Log.info("State change: %s -> %s",
             deviceModeToString(oldMode),
             deviceModeToString(newMode));
}

//==============================================================================
// STATUS LOGGING
//==============================================================================

void logStatus() {
    Log.info("Status: mode=%s, temp=%d.%d°C, heater=%s, cartridge=%s",
             deviceModeToString(deviceState.getCurrentMode()),
             heaterController.getCurrentTemperature() / 10,
             heaterController.getCurrentTemperature() % 10,
             heaterController.isReady() ? "ready" : "heating",
             HAL::readCartridgeSwitch() ? "inserted" : "none");
}

//==============================================================================
// SERIAL COMMANDS
//==============================================================================

void processSerialCommands() {
    while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
            if (serialBufferIndex > 0) {
                serialBuffer[serialBufferIndex] = '\0';
                executeSerialCommand(serialBuffer);
                serialBufferIndex = 0;
            }
        } else if (serialBufferIndex < sizeof(serialBuffer) - 1) {
            serialBuffer[serialBufferIndex++] = c;
        }
    }
}

void executeSerialCommand(const char* cmd) {
    int cmdNum = atoi(cmd);

    Log.info("Serial command: %d", cmdNum);

    switch (cmdNum) {
        case 1:  // Reset to idle
            deviceState.setMode(DeviceMode::IDLE);
            testRunner.stopTest();
            motorController.home();
            laserController.disableAllLasers();
            Serial.println("Reset to IDLE");
            break;

        case 10:  // Print status
            Serial.printf("Mode: %s\n", deviceModeToString(deviceState.getCurrentMode()));
            Serial.printf("Temp: %d.%d C\n",
                         heaterController.getCurrentTemperature() / 10,
                         heaterController.getCurrentTemperature() % 10);
            Serial.printf("Stage: %ld microns\n", motorController.getCurrentPosition());
            Serial.printf("Cartridge: %s\n",
                         HAL::readCartridgeSwitch() ? "inserted" : "none");
            break;

        case 20:  // Home stage
            motorController.home();
            Serial.println("Stage homed");
            break;

        case 21:  // Move stage (21:5000 = move 5000 microns)
            {
                const char* param = strchr(cmd, ':');
                if (param) {
                    int microns = atoi(param + 1);
                    motorController.moveRelative(microns, MOTOR_FAST_STEP_DELAY);
                    Serial.printf("Moved %d microns\n", microns);
                }
            }
            break;

        case 30:  // Laser control (30:A:1 = laser A on)
            {
                const char* param = strchr(cmd, ':');
                if (param && strlen(param) >= 4) {
                    char channel = param[1];
                    bool on = param[3] == '1';
                    if (on) {
                        laserController.enableLaser(channel);
                    } else {
                        laserController.disableLaser(channel);
                    }
                    Serial.printf("Laser %c %s\n", channel, on ? "ON" : "OFF");
                }
            }
            break;

        case 40:  // Buzzer test
            BuzzerController::playTone(BUZZER_FREQUENCY, BUZZER_DURATION);
            Serial.println("Buzzer test");
            break;

        case 50:  // Read temperature
            Serial.printf("Temperature: %d.%d C\n",
                         heaterController.getCurrentTemperature() / 10,
                         heaterController.getCurrentTemperature() % 10);
            break;

        case 60:  // Trigger barcode scan
            barcodeScanner.triggerScan();
            Serial.println("Barcode scan triggered");
            break;

        case 70:  // Spectrophotometer reading
            {
                BrevitestSpectrophotometerReading reading;
                if (spectrophotometer.readAllChannels(&reading)) {
                    Serial.printf("Spectro: F1=%d F2=%d F3=%d F4=%d F5=%d F6=%d F7=%d F8=%d CLR=%d NIR=%d\n",
                                 reading.f1, reading.f2, reading.f3, reading.f4,
                                 reading.f5, reading.f6, reading.f7, reading.f8,
                                 reading.clear, reading.nir);
                } else {
                    Serial.println("Spectro read failed");
                }
            }
            break;

        case 90:  // Start stress test
            {
                StressTestConfig config;
                config.total_cycles = 10;
                testRunner.startStressTest(config);
                deviceState.setMode(DeviceMode::STRESS_TESTING);
                Serial.println("Stress test started");
            }
            break;

        case 91:  // Stop stress test
            testRunner.stopStressTest();
            deviceState.setMode(DeviceMode::IDLE);
            Serial.println("Stress test stopped");
            break;

        case 99:  // Firmware version
            Serial.printf("Firmware: %s\n", FIRMWARE_VERSION_STRING);
            Serial.printf("DeviceOS: %s\n", System.version().c_str());
            Serial.printf("Device ID: %s\n", System.deviceID().c_str());
            break;

        default:
            Serial.printf("Unknown command: %d\n", cmdNum);
            break;
    }
}
