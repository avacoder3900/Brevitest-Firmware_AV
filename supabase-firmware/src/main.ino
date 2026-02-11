/**
 * @file main.ino
 * @brief Main entry point for Brevitest Supabase Firmware
 * @version 200
 * @date February 2026
 *
 * This firmware controls the Brevitest Acuity GEN2 diagnostic device with
 * Supabase cloud backend integration, replacing the legacy Particle/CouchDB architecture.
 *
 * Target Platform: Particle B-Series SoM (NRF52840) on Acuity GEN2 Main Board R5
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
// FIRMWARE VERSION (references DataTypes.h FIRMWARE_VERSION = 200)
//==============================================================================

#define FIRMWARE_VERSION_CODE FIRMWARE_VERSION
#define FIRMWARE_VERSION_STRING "200.0.0-supabase"

//==============================================================================
// LEGACY-COMPATIBLE CONSTANTS
//==============================================================================

// Detector debouncing
#define DETECTOR_DEBOUNCE_DELAY 10              // 10ms debounce for cartridge detector

// Heater
#define HEATER_READY_TEMP_DELTA 10              // 1.0°C delta (in 10x format = 10)
#define HEATER_READY_DEBOUNCE_DELAY 5000        // 5 second debounce for heater ready

// Buzzer
#define BUZZER_FREQUENCY 600
#define BUZZER_DURATION 1000
#define BUZZER_INSERT_FREQUENCY 620
#define BUZZER_INSERT_DURATION 200
#define BUZZER_ALERT_FREQUENCY 850
#define BUZZER_ALERT_DURATION 500
#define BUZZER_PROBLEM_FREQUENCY 620
#define BUZZER_PROBLEM_DURATION 100

// Stage positions
#define STAGE_MICRONS_TO_TEST_START_POSITION 7860

// Motor speeds
#define MOTOR_FAST_STEP_DELAY 290
#define MOTOR_SLOW_STEP_DELAY 600

// Validation retry
#define VALIDATION_TIMEOUT_MS 45000             // 45 second timeout
#define VALIDATION_MAX_RETRIES 3                // 3 retry attempts
#define VALIDATION_RETRY_BACKOFF_BASE 5000      // 5 second base backoff

// Upload/reset timeouts
#define UPLOAD_TIMEOUT_MS 30000                 // 30 second timeout
#define RESET_TIMEOUT_MS 30000                  // 30 second timeout

// Recently tested cooldown
#define RECENT_TEST_COOLDOWN_MS 30000           // 30 second cooldown

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

// Storage manager
StorageManager storageManager;

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

//==============================================================================
// HARDWARE LOOP STATE (Legacy-compatible)
//==============================================================================

// Heater debouncing - 5000ms debounce to prevent false heater ready signals
bool heater_ready = false;
bool previous_heater_ready = false;
bool heater_debouncing_in_progress = false;
unsigned long heater_debounce_time = 0;

// Detector debouncing - 10ms debounce to prevent false triggers
volatile bool detector_changed = false;
bool detector_debouncing = false;
unsigned long detector_debouncing_time = 0;

// Recently tested barcode cooldown
char last_tested_barcode[BARCODE_UUID_LENGTH + 1];
unsigned long last_tested_timestamp = 0;

// Pending barcode (for cartridge rejection during heating)
char pending_barcode_uuid[BARCODE_UUID_LENGTH + 1];
bool pending_barcode_available = false;

// Buzzer triggers
bool start_alert_buzzer = false;
bool start_problem_buzzer = false;

// Validation retry tracking
int validation_retry_count = 0;
unsigned long validation_retry_delay_until = 0;

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

// Legacy-compatible hardware loop functions
bool heater_debounced();
void set_device_indicators();
void hardware_loop();

//==============================================================================
// INTERRUPT HANDLER
//==============================================================================

volatile bool cartridgeStateChanged = false;

void cartridgeInterruptHandler() {
    cartridgeStateChanged = true;
}

//==============================================================================
// PARTICLE CLOUD VARIABLES
//==============================================================================

// Particle cloud variables (legacy compatibility)
int cloud_temperature = 0;
char cloud_magnet_validation[1024] = "";
char cloud_device_status[256] = "";

//==============================================================================
// PARTICLE CLOUD FUNCTIONS (Forward declarations)
//==============================================================================

int cloudFunction_runTest(String command);
int cloudFunction_resetDevice(String command);
int cloudFunction_setHeaterTarget(String command);
int cloudFunction_getStatus(String command);

//==============================================================================
// SETUP (ALPHA-016)
//==============================================================================

void setup() {
    // Initialize serial and wait for connection up to 15s (legacy: 15s wait)
    Serial.begin(115200);
    unsigned long serialWaitStart = millis();
    while (!Serial && (millis() - serialWaitStart) < 15000) {
        delay(100);
    }

    Log.info("===========================================");
    Log.info("Brevitest Supabase Firmware v%s", FIRMWARE_VERSION_STRING);
    Log.info("===========================================");

    // Start in INITIALIZING mode
    deviceState.init();
    deviceState.setMode(DeviceMode::INITIALIZING);

    // Initialize HAL
    Log.info("Initializing HAL...");
    if (!HAL::init()) {
        Log.error("HAL initialization failed!");
        deviceState.setMode(DeviceMode::ERROR_STATE);
        return;
    }

    // Register state change callback
    Log.info("Initializing state machine...");
    deviceState.registerStateChangeCallback(onStateChange);

    // Register Particle cloud variables (ALPHA-016 requirement)
    Log.info("Registering Particle cloud variables...");
    Particle.variable("temperature", cloud_temperature);
    Particle.variable("magnet_validation", cloud_magnet_validation);
    Particle.variable("device_status", cloud_device_status);

    // Register Particle cloud functions (ALPHA-016 requirement)
    Log.info("Registering Particle cloud functions...");
    Particle.function("runTest", cloudFunction_runTest);
    Particle.function("resetDevice", cloudFunction_resetDevice);
    Particle.function("setHeaterTarget", cloudFunction_setHeaterTarget);
    Particle.function("getStatus", cloudFunction_getStatus);

    // Initialize storage manager (GAMMA-007 through GAMMA-013)
    Log.info("Initializing storage manager...");
    if (!storageManager.init()) {
        Log.error("Storage manager initialization failed!");
        // Continue anyway - storage is not critical for basic operation
    } else {
        // GAMMA-013: Handle interrupted test recovery
        // Check EEPROM for running_test_uuid on startup
        if (storageManager.hasRecoveryData()) {
            Log.warn("Detected interrupted test - attempting recovery");
            BrevitestTestRecord recoveryRecord;
            if (storageManager.handleInterruptedTestRecovery(&recoveryRecord)) {
                Log.info("Interrupted test saved to cache: %s", recoveryRecord.cartridge_id);
            }
        }
    }

    // Initialize motor controller
    Log.info("Initializing motor controller...");
    if (!motorController.init()) {
        Log.warn("Motor controller initialization failed");
    } else {
        // Home the stage at startup so motor is at known position
        Log.info("Homing stage...");
        motorController.home(true);  // sleepAfter=true to save power
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
// MAIN LOOP (ALPHA-012)
//==============================================================================

/**
 * @brief Main loop - state machine driven operation
 *
 * LEGACY BEHAVIOR (brevitest-firmware.ino:5444-5843):
 * - ALWAYS calls process_serial_port() and hardware_loop() every iteration
 * - State-driven behavior for all 11 states
 * - Validation retry logic with exponential backoff
 * - Barcode scanning only when heater_debounced()
 */
void loop() {
    // === ALWAYS CALL THESE EVERY ITERATION ===
    processSerialCommands();

    // Check if cartridge state changed (from interrupt)
    if (cartridgeStateChanged) {
        cartridgeStateChanged = false;
        detector_changed = true;  // Signal hardware_loop to start debouncing
    }

    hardware_loop();  // CRITICAL: Must be called every iteration

    // Update buzzer (for non-blocking patterns)
    BuzzerController::update();

    // === STATE MACHINE PROCESSING ===
    switch (deviceState.getCurrentMode()) {
        case DeviceMode::IDLE:
            // Waiting for cartridge insertion
            // Check for cached test results to upload
            break;

        case DeviceMode::INITIALIZING:
            // One-time initialization already done in setup()
            deviceState.setMode(DeviceMode::IDLE);
            break;

        case DeviceMode::HEATING:
            // Check if heater is ready (with debouncing)
            if (heater_debounced()) {
                // Heater ready - check for pending barcode
                if (pending_barcode_available && deviceState.hasCartridge()) {
                    // Re-scan cartridge that was inserted during heating
                    Log.info("Heater ready with pending barcode - re-scanning");
                    pending_barcode_available = false;
                    deviceState.setMode(DeviceMode::BARCODE_SCANNING);
                    barcodeScanner.triggerScan();
                } else if (deviceState.hasCartridge()) {
                    // Cartridge present, heater ready - start scanning
                    Log.info("Heater ready, starting barcode scan");
                    deviceState.setMode(DeviceMode::BARCODE_SCANNING);
                    barcodeScanner.triggerScan();
                } else {
                    // No cartridge - go to idle
                    deviceState.setMode(DeviceMode::IDLE);
                }
            }
            break;

        case DeviceMode::BARCODE_SCANNING:
            // Only process barcode if heater is debounced
            if (!heater_debounced()) {
                // Heater not ready - store pending barcode and wait
                if (barcodeScanner.getScanState() == ScanState::SUCCESS) {
                    const char* barcode = barcodeScanner.getLastBarcode();
                    strncpy(pending_barcode_uuid, barcode, BARCODE_UUID_LENGTH);
                    pending_barcode_uuid[BARCODE_UUID_LENGTH] = '\0';
                    pending_barcode_available = true;
                    Log.info("Barcode read during heating: %s - waiting for heater", barcode);
                    deviceState.setMode(DeviceMode::HEATING);
                }
            } else {
                // Heater ready - process barcode normally
                if (barcodeScanner.getScanState() == ScanState::SUCCESS) {
                    const char* barcode = barcodeScanner.getLastBarcode();
                    strncpy(currentCartridgeId, barcode, BARCODE_UUID_LENGTH);
                    currentCartridgeId[BARCODE_UUID_LENGTH] = '\0';

                    // Check recently tested cooldown
                    if (last_tested_barcode[0] != '\0' &&
                        strcmp(barcode, last_tested_barcode) == 0 &&
                        (millis() - last_tested_timestamp) < RECENT_TEST_COOLDOWN_MS) {
                        Log.info("Same barcode tested recently - waiting for cooldown");
                        break;
                    }

                    Log.info("Barcode read: %s", currentCartridgeId);
                    deviceState.setCurrentCartridgeId(currentCartridgeId);
                    deviceState.setCartridgeState(CartridgeState::BARCODE_READ);

                    deviceState.setMode(DeviceMode::VALIDATING_CARTRIDGE);
                    deviceState.startCloudOperation();
                    startCartridgeValidation();
                }
            }
            break;

        case DeviceMode::VALIDATING_CARTRIDGE:
            // === VALIDATION RETRY LOGIC WITH EXPONENTIAL BACKOFF ===
            // Check for validation timeout
            if (deviceState.isCloudOperationTimeout(VALIDATION_TIMEOUT_MS)) {
                if (validation_retry_count < VALIDATION_MAX_RETRIES) {
                    validation_retry_count++;
                    unsigned long backoff = VALIDATION_RETRY_BACKOFF_BASE * validation_retry_count;
                    validation_retry_delay_until = millis() + backoff;
                    deviceState.endCloudOperation();
                    Log.warn("Validation timeout, retry %d/%d in %lu ms",
                             validation_retry_count, VALIDATION_MAX_RETRIES, backoff);
                } else {
                    // Max retries exceeded
                    Log.error("Validation failed after %d retries", VALIDATION_MAX_RETRIES);
                    deviceState.setCartridgeState(CartridgeState::INVALID);
                    deviceState.endCloudOperation();
                    deviceState.setError(ErrorCode::ERR_CLOUD_TIMEOUT, "Validation timeout");
                    validation_retry_count = 0;
                }
            }

            // Check if we should retry
            if (!deviceState.isCloudOperationPending() &&
                validation_retry_delay_until > 0 &&
                millis() > validation_retry_delay_until) {
                validation_retry_delay_until = 0;
                if (deviceState.hasCartridge()) {
                    Log.info("Retrying validation...");
                    deviceState.startCloudOperation();
                    startCartridgeValidation();
                } else {
                    // Cartridge removed during retry wait
                    validation_retry_count = 0;
                    deviceState.setMode(DeviceMode::IDLE);
                }
            }
            break;

        case DeviceMode::RUNNING_TEST:
            // Update test runner
            testRunner.update();

            // Check for test completion
            if (testRunner.getState() == TestRunnerState::COMPLETED) {
                Log.info("Test completed, uploading results");
                // Record for cooldown
                strncpy(last_tested_barcode, currentCartridgeId, BARCODE_UUID_LENGTH);
                last_tested_barcode[BARCODE_UUID_LENGTH] = '\0';
                last_tested_timestamp = millis();

                deviceState.setTestState(TestState::COMPLETED);
                deviceState.setMode(DeviceMode::UPLOADING_RESULTS);
                deviceState.startCloudOperation();
                startResultUpload();
            } else if (testRunner.getState() == TestRunnerState::CANCELLED ||
                       testRunner.getState() == TestRunnerState::ERROR) {
                Log.warn("Test ended with state: %s",
                         testRunnerStateToString(testRunner.getState()));
                deviceState.setTestState(TestState::CANCELLED);
                deviceState.setMode(DeviceMode::IDLE);
            }
            break;

        case DeviceMode::UPLOADING_RESULTS:
            // Check for upload timeout
            if (deviceState.isCloudOperationTimeout(UPLOAD_TIMEOUT_MS)) {
                Log.error("Upload timeout - caching results");
                deviceState.endCloudOperation();
                // TODO: Cache test results to /cache directory
                deviceState.setMode(DeviceMode::IDLE);
            }
            break;

        case DeviceMode::RESETTING_CARTRIDGE:
            // Check for reset timeout
            if (deviceState.isCloudOperationTimeout(RESET_TIMEOUT_MS)) {
                Log.error("Reset timeout");
                deviceState.endCloudOperation();
                deviceState.setMode(DeviceMode::IDLE);
            }
            break;

        case DeviceMode::STRESS_TESTING:
            // Stress test mode
            if (testRunner.isStressTestRunning()) {
                testRunner.executeStressTestCycle();
            }
            break;

        case DeviceMode::ERROR_STATE:
            // Error indicator handled by set_device_indicators()
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

// Note: Cartridge insertion/removal is now handled by hardware_loop() with proper debouncing.
// These functions are kept for backward compatibility with any external calls.

void handleCartridgeInserted() {
    // Now handled in hardware_loop() with proper debouncing
    // This is called from the interrupt handler, which sets detector_changed = true
}

void handleCartridgeRemoved() {
    // Now handled in hardware_loop() with proper debouncing
    // This is called from the interrupt handler, which sets detector_changed = true
}

//==============================================================================
// CLOUD OPERATIONS
//==============================================================================

// Async callback for cartridge validation
void onValidationCallback(const ValidateCartridgeResponse& response, void* context);
// Async callback for assay loading
void onAssayLoadCallback(const LoadAssayResponse& response, void* context);
// Async callback for test upload
void onUploadCallback(const UploadTestResponse& response, void* context);

void startCartridgeValidation() {
    Log.info("Starting cartridge validation for: %s", currentCartridgeId);

    deviceState.startCloudOperation();

    // Async validation via SupabaseClient - returns immediately, callback fires on completion
    if (!cloudClient.validateCartridgeAsync(currentCartridgeId, onValidationCallback, nullptr)) {
        Log.error("Failed to initiate cartridge validation");
        deviceState.endCloudOperation();
        deviceState.setMode(DeviceMode::IDLE);
        BuzzerController::playErrorMelody();
    }
}

void onValidationCallback(const ValidateCartridgeResponse& response, void* context) {
    deviceState.endCloudOperation();

    if (!response.isSuccess() || !response.isValid) {
        Log.error("Cartridge validation failed: %s", response.errorMessage);
        deviceState.setMode(DeviceMode::IDLE);
        BuzzerController::playErrorMelody();
        return;
    }

    Log.info("Cartridge validated, assay: %s", response.assayId);

    // Load assay from cloud (async)
    deviceState.startCloudOperation();
    if (!cloudClient.loadAssayAsync(response.assayId, onAssayLoadCallback, nullptr)) {
        Log.error("Failed to initiate assay load");
        deviceState.endCloudOperation();
        deviceState.setMode(DeviceMode::IDLE);
        BuzzerController::playErrorMelody();
    }
}

void onAssayLoadCallback(const LoadAssayResponse& response, void* context) {
    deviceState.endCloudOperation();

    if (!response.isSuccess()) {
        Log.error("Assay load failed: %s", response.errorMessage);
        deviceState.setMode(DeviceMode::IDLE);
        BuzzerController::playErrorMelody();
        return;
    }

    Log.info("Assay loaded: %s, BCODE length: %u", response.assayId, response.bcodeLength);

    // Populate assay structure
    strncpy(currentAssay.id, response.assayId, ASSAY_UUID_LENGTH);
    currentAssay.id[ASSAY_UUID_LENGTH] = '\0';
    currentAssay.duration = response.duration;
    currentAssay.BCODE_length = response.bcodeLength;
    strncpy(currentAssay.BCODE, response.bcode, BCODE_CAPACITY - 1);
    currentAssay.BCODE[BCODE_CAPACITY - 1] = '\0';

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

    deviceState.startCloudOperation();

    // Async upload via SupabaseClient - returns immediately
    if (!cloudClient.uploadTestAsync(record, onUploadCallback, nullptr)) {
        Log.error("Failed to initiate test upload");
        deviceState.endCloudOperation();

        // Cache for offline retry
        storageManager.cacheTestData(record);
        Log.info("Test record cached for later upload");

        BuzzerController::startPeriodicAlert(AlertType::GENERAL);
        deviceState.setMode(DeviceMode::IDLE);
    }
}

void onUploadCallback(const UploadTestResponse& response, void* context) {
    deviceState.endCloudOperation();

    if (response.isSuccess() && response.acknowledged) {
        Log.info("Test results uploaded successfully");
        BuzzerController::playSuccessMelody();
    } else {
        Log.error("Test upload failed: %s", response.errorMessage);
        BuzzerController::playErrorMelody();

        // Cache for offline retry
        BrevitestTestRecord* record = testRunner.getTestRecord();
        storageManager.cacheTestData(record);
        Log.info("Test record cached for later upload");
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
// LEGACY-COMPATIBLE HARDWARE LOOP FUNCTIONS
//==============================================================================

/**
 * @brief Heater ready debouncing (ALPHA-009)
 *
 * LEGACY BEHAVIOR (brevitest-firmware.ino:4878-4900):
 * - Returns false while debouncing is in progress
 * - Starts debounce timer when heater_ready state changes
 * - Uses HEATER_READY_DEBOUNCE_DELAY (5000ms) for debounce period
 * - Only returns true heater_ready after debounce completes
 *
 * CRITICAL: This prevents false heater ready signals that could damage cartridges.
 */
bool heater_debounced() {
    if (heater_debouncing_in_progress) {
        if (millis() > heater_debounce_time) {
            heater_debouncing_in_progress = false;
            heater_debounce_time = 0;
            return heater_ready;
        } else {
            return false;
        }
    } else if (previous_heater_ready != heater_ready) {
        heater_debouncing_in_progress = true;
        heater_debounce_time = millis() + HEATER_READY_DEBOUNCE_DELAY;
        return false;
    }
    return heater_ready;
}

/**
 * @brief Set device LED and buzzer indicators based on state (ALPHA-010)
 *
 * LED BEHAVIOR:
 * - STRESS_TESTING, RUNNING_TEST, BARCODE_SCANNING, VALIDATING_*: Red solid (don't touch)
 * - ERROR_STATE with invalid cartridge: Green blink + problem buzzer
 * - ERROR_STATE without invalid cartridge: Red solid
 * - HEATING (with or without cartridge): Red solid (waiting for temperature)
 * - IDLE with cartridge + UPLOADED test: Green blink + alert buzzer
 * - IDLE with cartridge but no UPLOADED test: Green blink, no buzzer
 * - IDLE without cartridge + heater ready: Green solid (ready for cartridge)
 * - IDLE without cartridge + heater not ready: Red solid (heating up)
 * - UPLOADING_RESULTS: Red solid, alert buzzer if cartridge present
 * - INITIALIZING: Red solid
 */
void set_device_indicators() {
    switch (deviceState.getCurrentMode()) {
        case DeviceMode::STRESS_TESTING:
        case DeviceMode::RUNNING_TEST:
        case DeviceMode::BARCODE_SCANNING:
        case DeviceMode::VALIDATING_CARTRIDGE:
        case DeviceMode::VALIDATING_MAGNETOMETER:
            // Active operation - don't touch (red solid)
            LEDController::setPattern(LEDColors::RED, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL);
            break;

        case DeviceMode::ERROR_STATE:
            if (deviceState.isCartridgeInvalid()) {
                // Invalid cartridge - remove it (green blink + problem buzzer)
                LEDController::setPattern(LEDColors::GREEN, BrvtLEDPattern::BLINK, BrvtLEDSpeed::NORMAL);
                start_problem_buzzer = true;
            } else {
                // Other error - don't touch (red solid)
                LEDController::setPattern(LEDColors::RED, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL);
            }
            break;

        case DeviceMode::HEATING:
            // Heating up - red solid (with or without cartridge)
            LEDController::setPattern(LEDColors::RED, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL);
            BuzzerController::stopPeriodicAlert();
            break;

        case DeviceMode::IDLE:
            if (deviceState.hasCartridge()) {
                // Cartridge present but not processed - remove it (green blink)
                LEDController::setPattern(LEDColors::GREEN, BrvtLEDPattern::BLINK, BrvtLEDSpeed::NORMAL);

                // Activate buzzer ONLY if test was just completed
                if (deviceState.getTestState() == TestState::UPLOADED) {
                    start_alert_buzzer = true;
                } else {
                    BuzzerController::stopPeriodicAlert();
                }
            } else if (heater_debounced()) {
                // Heater ready and no cartridge - ready for insertion (green solid)
                LEDController::setPattern(LEDColors::GREEN, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL);
                BuzzerController::stopPeriodicAlert();
            } else {
                // Heater not ready - heating up (red solid)
                LEDController::setPattern(LEDColors::RED, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL);
                BuzzerController::stopPeriodicAlert();
            }
            break;

        case DeviceMode::UPLOADING_RESULTS:
            // Uploading - don't touch (red solid)
            LEDController::setPattern(LEDColors::RED, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL);
            // If test is uploaded and cartridge still present, activate buzzer
            if (deviceState.getTestState() == TestState::UPLOADED && deviceState.hasCartridge()) {
                start_alert_buzzer = true;
            } else {
                BuzzerController::stopPeriodicAlert();
            }
            break;

        case DeviceMode::RESETTING_CARTRIDGE:
            // Resetting - don't touch (red solid)
            LEDController::setPattern(LEDColors::RED, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL);
            break;

        case DeviceMode::INITIALIZING:
            // Starting up - don't touch (red solid)
            LEDController::setPattern(LEDColors::RED, BrvtLEDPattern::SOLID, BrvtLEDSpeed::NORMAL);
            break;
    }

    LEDController::update();
}

/**
 * @brief Main hardware monitoring loop (ALPHA-011)
 *
 * LEGACY BEHAVIOR (brevitest-firmware.ino:5254-5416):
 * MUST be called every loop iteration. Handles:
 * 1. Heater temperature monitoring and heater_ready update
 * 2. Cartridge detector debouncing (10ms)
 * 3. Cartridge insertion handling (reset stage, move to test start, scan barcode)
 * 4. Cartridge removal handling (reset stage, clear state, reset to idle)
 * 5. Call set_device_indicators() for LED/buzzer
 * 6. Run PID temperature control
 * 7. Process buzzer triggers
 *
 * CRITICAL: This is the heart of the firmware. Missing this is why it doesn't work.
 */
void hardware_loop() {
    // === HEATER TEMPERATURE MONITORING ===
    previous_heater_ready = heater_ready;
    int16_t currentTemp = heaterController.getCurrentTemperature();
    int16_t targetTemp = heaterController.getTargetTemperature();
    int16_t temp_delta = targetTemp - currentTemp;
    // Check that temperature is within range AND positive (temp must be below target)
    heater_ready = (temp_delta >= 0 && temp_delta < HEATER_READY_TEMP_DELTA);

    // === CARTRIDGE DETECTION DEBOUNCING ===
    if (detector_debouncing) {
        if (millis() > detector_debouncing_time) {
            // Debouncing period complete - process the state change
            detector_debouncing_time = 0;
            detector_debouncing = false;
            detector_changed = false;
            bool new_detector_state = HAL::readCartridgeSwitch();

            Log.info("%s detected", new_detector_state ? "Insertion" : "Removal");

            if (new_detector_state) {
                // === CARTRIDGE INSERTED ===
                motorController.home();
                motorController.moveToPosition(STAGE_MICRONS_TO_TEST_START_POSITION, MOTOR_FAST_STEP_DELAY);
                motorController.disable();

                // === PREVENT RE-SCANNING AFTER TEST COMPLETION ===
                bool skip_barcode_scan = false;

                if (deviceState.getCurrentMode() == DeviceMode::IDLE &&
                    deviceState.getCartridgeState() == CartridgeState::DETECTED &&
                    deviceState.getCurrentCartridgeId()[0] == '\0') {

                    // Check if test was just uploaded
                    if (deviceState.getTestState() == TestState::UPLOADED) {
                        skip_barcode_scan = true;
                        Log.info("Cartridge still inserted after test - skipping barcode scan");
                    }
                    // Check if this barcode was recently tested
                    else if (last_tested_barcode[0] != '\0' &&
                             last_tested_timestamp > 0 &&
                             (millis() - last_tested_timestamp) < RECENT_TEST_COOLDOWN_MS) {
                        skip_barcode_scan = false;  // Allow scan, check after barcode is read
                    }
                }

                if (!skip_barcode_scan) {
                    deviceState.setCartridgeState(CartridgeState::DETECTED);

                    // Check if heater is ready
                    if (heater_ready && deviceState.canTransitionTo(DeviceMode::BARCODE_SCANNING)) {
                        // Heater ready - start barcode scanning immediately
                        deviceState.setMode(DeviceMode::BARCODE_SCANNING);
                        barcodeScanner.triggerScan();
                        BuzzerController::playTone(BUZZER_INSERT_FREQUENCY, BUZZER_INSERT_DURATION);
                        Log.info("Heater ready - starting barcode scan");
                    } else {
                        // Heater not ready - transition to HEATING and wait
                        deviceState.setMode(DeviceMode::HEATING);
                        Log.info("Cartridge inserted - waiting for heater to reach temperature");
                    }
                }
            } else {
                // === CARTRIDGE REMOVED ===
                motorController.home();
                BuzzerController::stopPeriodicAlert();
                deviceState.clearCurrentCartridgeId();

                // === CLEANUP VALIDATION RETRY TRACKING ===
                validation_retry_count = 0;
                validation_retry_delay_until = 0;
                if (deviceState.isCloudOperationPending()) {
                    deviceState.endCloudOperation();
                }

                // === CLEAR RECENTLY TESTED BARCODE TRACKING ===
                last_tested_barcode[0] = '\0';
                last_tested_timestamp = 0;

                // Clear pending barcode
                pending_barcode_uuid[0] = '\0';
                pending_barcode_available = false;

                deviceState.resetToIdle();
            }
        }
    } else if (detector_changed) {
        // Start debouncing period to avoid false triggers
        detector_debouncing = true;
        detector_debouncing_time = millis() + DETECTOR_DEBOUNCE_DELAY;
    }

    // === UPDATE DEVICE INDICATORS ===
    set_device_indicators();

    // === TEMPERATURE CONTROL ===
    heaterController.update();

    // === BUZZER MANAGEMENT ===
    if (start_problem_buzzer) {
        start_problem_buzzer = false;
        BuzzerController::playTone(BUZZER_PROBLEM_FREQUENCY, BUZZER_PROBLEM_DURATION);
    } else if (start_alert_buzzer) {
        start_alert_buzzer = false;
        BuzzerController::playTone(BUZZER_ALERT_FREQUENCY, BUZZER_ALERT_DURATION);
    }
}

// Note: LED status is now handled by set_device_indicators() called from hardware_loop()

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
    // Update cloud variable (ALPHA-016)
    cloud_temperature = heaterController.getCurrentTemperature();

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

//==============================================================================
// PARTICLE CLOUD FUNCTIONS (ALPHA-016)
//==============================================================================

/**
 * @brief Run test cloud function
 * @param command Cartridge UUID to test
 * @return 1 on success, -1 on error
 */
int cloudFunction_runTest(String command) {
    if (command.length() == 0) {
        return -1;
    }

    if (deviceState.getCurrentMode() != DeviceMode::IDLE) {
        Log.warn("Cannot start test - device not idle");
        return -1;
    }

    Log.info("Cloud function: runTest(%s)", command.c_str());
    // TODO: Implement cloud-initiated test
    return 1;
}

/**
 * @brief Reset device cloud function
 * @param command Reset type (optional)
 * @return 1 on success
 */
int cloudFunction_resetDevice(String command) {
    Log.info("Cloud function: resetDevice(%s)", command.c_str());
    deviceState.resetToIdle();
    testRunner.stopTest();
    motorController.home();
    laserController.disableAllLasers();
    return 1;
}

/**
 * @brief Set heater target temperature cloud function
 * @param command Temperature in 10x format (e.g., "450" = 45.0°C)
 * @return 1 on success, -1 on error
 */
int cloudFunction_setHeaterTarget(String command) {
    int temp = command.toInt();
    if (temp < 200 || temp > 600) {  // 20.0°C to 60.0°C range
        Log.warn("Invalid heater target: %d", temp);
        return -1;
    }

    Log.info("Cloud function: setHeaterTarget(%d)", temp);
    heaterController.setTargetTemperature(temp);
    return 1;
}

/**
 * @brief Get device status cloud function
 * @param command Unused
 * @return 1 always
 */
int cloudFunction_getStatus(String command) {
    // Update cloud variables for retrieval
    cloud_temperature = heaterController.getCurrentTemperature();

    snprintf(cloud_device_status, sizeof(cloud_device_status),
             "{\"mode\":\"%s\",\"temp\":%d,\"heater\":\"%s\",\"cartridge\":\"%s\"}",
             deviceModeToString(deviceState.getCurrentMode()),
             heaterController.getCurrentTemperature(),
             heaterController.isReady() ? "ready" : "heating",
             HAL::readCartridgeSwitch() ? "inserted" : "none");

    Log.info("Cloud function: getStatus() -> %s", cloud_device_status);
    return 1;
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
