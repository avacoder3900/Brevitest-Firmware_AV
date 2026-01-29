/**
 * @file TestRunner.cpp
 * @brief Implementation of test execution controller
 * @author Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * User Stories Implemented:
 *   - TEST-001: TestRunner class with all lifecycle methods
 *   - TEST-003: Test data collection (spectrophotometer readings)
 *   - TEST-004: Test record generation
 *   - TEST-005: Cartridge removal detection
 *   - TEST-006: Stress test mode
 */

#include "TestRunner.h"

//==============================================================================
// SINGLETON INSTANCE
//==============================================================================

static TestRunner* _instance = nullptr;

TestRunner& getTestRunner() {
    if (_instance == nullptr) {
        static TestRunner instance;
        _instance = &instance;
    }
    return *_instance;
}

//==============================================================================
// STATIC CALLBACK WRAPPER
//==============================================================================

/**
 * @brief Static callback for BCODE delay loop
 * @details Routes to singleton instance for heater control and cartridge check
 */
static bool staticBcodeDelayLoop() {
    TestRunner& runner = getTestRunner();

    // Check cartridge presence
    if (!runner.checkCartridge()) {
        return false;  // Cancel execution
    }

    // Run heater control
    runner.updateHeater();

    return true;
}

//==============================================================================
// CONSTRUCTOR AND LIFECYCLE
//==============================================================================

TestRunner::TestRunner() :
    _initialized(false),
    _state(TestRunnerState::IDLE),
    _test_state(TestState::NOT_STARTED),
    _last_error(ErrorCode::SUCCESS),
    _test_start_time(0),
    _cartridge_removed(false),
    _record_finalized(false),
    _spectro_gain(SPECTRO_AGAIN_DEFAULT),
    _spectro_astep(SPECTRO_ASTEP_DEFAULT),
    _spectro_atime(SPECTRO_ATIME_DEFAULT),
    _get_temp_callback(nullptr),
    _is_heater_ready_callback(nullptr),
    _set_heater_power_callback(nullptr),
    _is_cartridge_inserted_callback(nullptr),
    _get_stage_position_callback(nullptr),
    _state_change_callback(nullptr)
{
    _instance = this;
}

bool TestRunner::init() {
    if (_initialized) {
        return true;
    }

    Log.info("TestRunner: Initializing");

    // Reset state
    _state = TestRunnerState::IDLE;
    _test_state = TestState::NOT_STARTED;
    _last_error = ErrorCode::SUCCESS;
    _cartridge_removed = false;
    _record_finalized = false;

    // Initialize test record with defaults
    initTestRecord(&_test_record);

    // Configure BCODE interpreter callbacks
    _interpreter.setDelayLoopCallback(staticBcodeDelayLoop);

    _initialized = true;
    Log.info("TestRunner: Initialization complete");
    return true;
}

void TestRunner::shutdown() {
    if (_state == TestRunnerState::EXECUTING) {
        stopTest();
    }

    _initialized = false;
    _state = TestRunnerState::IDLE;
    Log.info("TestRunner: Shutdown complete");
}

void TestRunner::update() {
    if (!_initialized) {
        return;
    }

    switch (_state) {
        case TestRunnerState::IDLE:
            // Nothing to do
            break;

        case TestRunnerState::INITIALIZING:
            // Check if heater is ready (if required)
            if (_config.require_heater_ready && _is_heater_ready_callback) {
                if (_is_heater_ready_callback()) {
                    Log.info("TestRunner: Heater ready, starting execution");
                    setState(TestRunnerState::EXECUTING);
                    _interpreter.startExecution();
                }
            } else {
                // No heater check required
                setState(TestRunnerState::EXECUTING);
                _interpreter.startExecution();
            }
            break;

        case TestRunnerState::EXECUTING:
            // Check cartridge
            if (!checkCartridge()) {
                onCartridgeRemoved();
                return;
            }

            // Run heater control
            updateHeater();

            // Execute next BCODE instruction
            if (_interpreter.getState() == InterpreterState::RUNNING) {
                _interpreter.executeNextInstruction();
            }

            // Check if execution completed
            if (_interpreter.getState() == InterpreterState::COMPLETED) {
                Log.info("TestRunner: BCODE execution completed");
                _test_state = TestState::COMPLETED;
                setState(TestRunnerState::COMPLETING);
            } else if (_interpreter.getState() == InterpreterState::ERROR) {
                _last_error = _interpreter.getLastError();
                _test_state = TestState::CANCELLED;
                setState(TestRunnerState::ERROR);
            } else if (_interpreter.getState() == InterpreterState::CANCELLED) {
                _test_state = TestState::CANCELLED;
                setState(TestRunnerState::CANCELLED);
            }
            break;

        case TestRunnerState::COMPLETING:
            // Finalize test record
            if (finalizeRecord()) {
                _test_state = TestState::UPLOAD_PENDING;
                setState(TestRunnerState::COMPLETED);
            } else {
                setState(TestRunnerState::ERROR);
            }
            break;

        case TestRunnerState::COMPLETED:
        case TestRunnerState::CANCELLED:
        case TestRunnerState::ERROR:
            // Terminal states - nothing to do
            break;
    }
}

bool TestRunner::isReady() const {
    return _initialized && _state == TestRunnerState::IDLE;
}

String TestRunner::getStatus() const {
    String status = "TestRunner: ";
    status += testRunnerStateToString(_state);

    if (_state == TestRunnerState::EXECUTING) {
        status += " (";
        status += String(getTestProgress());
        status += "%)";
    }

    return status;
}

//==============================================================================
// TEST CONTROL
//==============================================================================

bool TestRunner::startTest(const BrevitestAssay* assay, const char* cartridge_id) {
    if (!_initialized) {
        setError(ErrorCode::ERR_INVALID_STATE);
        return false;
    }

    if (_state != TestRunnerState::IDLE) {
        Log.warn("TestRunner: Cannot start test - not idle (state=%d)",
                 static_cast<int>(_state));
        setError(ErrorCode::ERR_INVALID_STATE);
        return false;
    }

    if (assay == nullptr || cartridge_id == nullptr) {
        setError(ErrorCode::ERR_INVALID_PARAMETER);
        return false;
    }

    if (assay->BCODE_length == 0 || assay->BCODE[0] == '\0') {
        Log.error("TestRunner: Invalid or empty BCODE");
        setError(ErrorCode::ERR_BCODE_INVALID);
        return false;
    }

    Log.info("TestRunner: Starting test - Assay: %s, Cartridge: %s",
             assay->id, cartridge_id);

    // Initialize test record
    initializeTestRecord(assay, cartridge_id);

    // Load BCODE into interpreter
    if (!_interpreter.loadBCODE(assay->BCODE, assay->BCODE_length)) {
        Log.error("TestRunner: Failed to load BCODE");
        setError(ErrorCode::ERR_BCODE_INVALID);
        return false;
    }

    // Reset state
    _test_start_time = millis();
    _test_record.start_time = Time.now();
    _cartridge_removed = false;
    _record_finalized = false;
    _test_state = TestState::RUNNING;
    _last_error = ErrorCode::SUCCESS;

    // Transition to initializing (wait for heater if needed)
    setState(TestRunnerState::INITIALIZING);

    return true;
}

void TestRunner::stopTest() {
    if (_state == TestRunnerState::IDLE) {
        return;
    }

    Log.info("TestRunner: Stopping test");

    // Cancel BCODE execution
    _interpreter.cancelExecution();

    // Save partial data
    _test_record.duration = (millis() - _test_start_time) / 1000;

    // Set appropriate state
    if (_cartridge_removed) {
        _test_state = TestState::CANCELLED;
        setState(TestRunnerState::CANCELLED);
    } else {
        _test_state = TestState::CANCELLED;
        setState(TestRunnerState::CANCELLED);
    }
}

TestState TestRunner::getTestState() const {
    return _test_state;
}

TestRunnerState TestRunner::getState() const {
    return _state;
}

uint8_t TestRunner::getTestProgress() const {
    if (_state != TestRunnerState::EXECUTING) {
        if (_state == TestRunnerState::COMPLETED) return 100;
        return 0;
    }

    // Calculate progress based on BCODE instruction pointer
    uint16_t length = _interpreter.getBCODELength();
    if (length == 0) return 0;

    uint16_t position = _interpreter.getInstructionPointer();
    return static_cast<uint8_t>((position * 100) / length);
}

uint32_t TestRunner::getElapsedTime() const {
    if (_test_start_time == 0) return 0;
    return millis() - _test_start_time;
}

//==============================================================================
// TEST DATA
//==============================================================================

BrevitestTestRecord* TestRunner::getTestRecord() {
    return &_test_record;
}

const BrevitestTestRecord* TestRunner::getTestRecord() const {
    return &_test_record;
}

uint16_t TestRunner::getReadingCount() const {
    return _test_record.number_of_readings;
}

bool TestRunner::isRecordComplete() const {
    return _record_finalized &&
           (_state == TestRunnerState::COMPLETED ||
            _state == TestRunnerState::CANCELLED);
}

bool TestRunner::finalizeRecord() {
    if (_record_finalized) {
        return true;
    }

    Log.info("TestRunner: Finalizing test record");

    // Set duration
    _test_record.duration = (millis() - _test_start_time) / 1000;

    // Calculate checksum
    _test_record.checksum = calculateTestRecordChecksum(&_test_record);

    _record_finalized = true;

    Log.info("TestRunner: Record finalized - %u readings, %u sec duration, checksum=0x%08lX",
             _test_record.number_of_readings,
             _test_record.duration,
             _test_record.checksum);

    return true;
}

//==============================================================================
// SPECTROPHOTOMETER DATA COLLECTION
//==============================================================================

bool TestRunner::addReading(const BrevitestSpectrophotometerReading* reading) {
    if (reading == nullptr) {
        return false;
    }

    if (_test_record.number_of_readings >= SPECTRO_MAX_READINGS) {
        Log.warn("TestRunner: Maximum readings reached (%u)", SPECTRO_MAX_READINGS);
        return false;
    }

    // Copy reading into test record
    memcpy(&_test_record.reading[_test_record.number_of_readings],
           reading, sizeof(BrevitestSpectrophotometerReading));
    _test_record.number_of_readings++;

    return true;
}

void TestRunner::getSpectroSettings(uint8_t* gain, uint16_t* astep, uint8_t* atime) const {
    if (gain) *gain = _spectro_gain;
    if (astep) *astep = _spectro_astep;
    if (atime) *atime = _spectro_atime;
}

void TestRunner::setSpectroSettings(uint8_t gain, uint16_t astep, uint8_t atime) {
    _spectro_gain = gain;
    _spectro_astep = astep;
    _spectro_atime = atime;

    // Update test record
    _test_record.again = gain;
    _test_record.astep = astep;
    _test_record.atime = atime;

    Log.trace("TestRunner: Spectro settings - gain=%u, astep=%u, atime=%u",
              gain, astep, atime);
}

void TestRunner::addBaselineScans(uint16_t count) {
    _test_record.baseline_scans += count;
}

void TestRunner::addTestScans(uint16_t count) {
    _test_record.test_scans += count;
}

//==============================================================================
// CARTRIDGE HANDLING
//==============================================================================

void TestRunner::onCartridgeRemoved() {
    if (_cartridge_removed) {
        return;  // Already handled
    }

    Log.warn("TestRunner: Cartridge removed during test!");
    _cartridge_removed = true;

    // Cancel BCODE execution
    _interpreter.cancelExecution();

    // Save partial data
    _test_record.duration = (millis() - _test_start_time) / 1000;

    // Update state
    _test_state = TestState::CANCELLED;
    setState(TestRunnerState::CANCELLED);
}

bool TestRunner::wasCartridgeRemoved() const {
    return _cartridge_removed;
}

bool TestRunner::checkCartridge() {
    if (_is_cartridge_inserted_callback) {
        return _is_cartridge_inserted_callback();
    }
    // If no callback registered, assume cartridge is present
    return true;
}

//==============================================================================
// STRESS TEST MODE
//==============================================================================

bool TestRunner::startStressTest(const StressTestConfig& config) {
    if (_state != TestRunnerState::IDLE) {
        Log.warn("TestRunner: Cannot start stress test - not idle");
        return false;
    }

    Log.info("TestRunner: Starting stress test - %u cycles", config.total_cycles);

    _stress_config = config;
    _stress_status.cycles_completed = 0;
    _stress_status.total_readings = 0;
    _stress_status.current_led_power = config.led_power_start;
    _stress_status.running = true;

    return true;
}

void TestRunner::stopStressTest() {
    if (!_stress_status.running) {
        return;
    }

    Log.info("TestRunner: Stopping stress test after %lu cycles",
             _stress_status.cycles_completed);

    _stress_status.running = false;
    setState(TestRunnerState::IDLE);
}

bool TestRunner::isStressTestRunning() const {
    return _stress_status.running;
}

StressTestStatus TestRunner::getStressTestStatus() const {
    return _stress_status;
}

bool TestRunner::executeStressTestCycle() {
    if (!_stress_status.running) {
        return false;
    }

    // Check if we've completed all cycles
    if (_stress_status.cycles_completed >= _stress_config.total_cycles) {
        Log.info("TestRunner: Stress test complete - %lu cycles",
                 _stress_status.cycles_completed);
        _stress_status.running = false;
        return false;
    }

    // Update LED power if varying
    if (_stress_config.vary_led_power) {
        uint16_t power_range = _stress_config.led_power_end - _stress_config.led_power_start;
        if (_stress_config.total_cycles > 1 && power_range > 0) {
            _stress_status.current_led_power = _stress_config.led_power_start +
                (power_range * _stress_status.cycles_completed) / (_stress_config.total_cycles - 1);
        }
    }

    // Increment cycle counter
    _stress_status.cycles_completed++;
    _stress_status.lifetime_cycles++;

    Log.trace("TestRunner: Stress test cycle %lu/%u, LED power=%u",
              _stress_status.cycles_completed,
              _stress_config.total_cycles,
              _stress_status.current_led_power);

    return true;
}

//==============================================================================
// CONFIGURATION
//==============================================================================

void TestRunner::setConfig(const TestRunnerConfig& config) {
    _config = config;
}

const TestRunnerConfig& TestRunner::getConfig() const {
    return _config;
}

//==============================================================================
// CALLBACK REGISTRATION
//==============================================================================

void TestRunner::setGetTemperatureCallback(GetTemperatureCallback callback) {
    _get_temp_callback = callback;
}

void TestRunner::setIsHeaterReadyCallback(IsHeaterReadyCallback callback) {
    _is_heater_ready_callback = callback;
}

void TestRunner::setSetHeaterPowerCallback(SetHeaterPowerCallback callback) {
    _set_heater_power_callback = callback;
}

void TestRunner::setIsCartridgeInsertedCallback(IsCartridgeInsertedCallback callback) {
    _is_cartridge_inserted_callback = callback;
}

void TestRunner::setGetStagePositionCallback(GetStagePositionCallback callback) {
    _get_stage_position_callback = callback;
}

void TestRunner::setStateChangeCallback(StateChangeCallback callback) {
    _state_change_callback = callback;
}

//==============================================================================
// BCODE INTERPRETER ACCESS
//==============================================================================

BCODEInterpreter& TestRunner::getInterpreter() {
    return _interpreter;
}

const BCODEInterpreter& TestRunner::getInterpreter() const {
    return _interpreter;
}

//==============================================================================
// ERROR HANDLING
//==============================================================================

ErrorCode TestRunner::getLastError() const {
    return _last_error;
}

const char* TestRunner::getErrorMessage() const {
    return errorCodeToString(_last_error);
}

//==============================================================================
// INTERNAL METHODS
//==============================================================================

void TestRunner::setState(TestRunnerState new_state) {
    if (_state == new_state) {
        return;
    }

    TestRunnerState old_state = _state;
    _state = new_state;

    Log.info("TestRunner: State %s -> %s",
             testRunnerStateToString(old_state),
             testRunnerStateToString(new_state));

    if (_state_change_callback) {
        _state_change_callback(old_state, new_state);
    }
}

void TestRunner::initializeTestRecord(const BrevitestAssay* assay, const char* cartridge_id) {
    // Clear existing record
    initTestRecord(&_test_record);

    // Copy IDs
    strncpy(_test_record.cartridge_id, cartridge_id, BARCODE_UUID_LENGTH);
    _test_record.cartridge_id[BARCODE_UUID_LENGTH] = '\0';

    strncpy(_test_record.assay_id, assay->id, ASSAY_UUID_LENGTH);
    _test_record.assay_id[ASSAY_UUID_LENGTH] = '\0';

    // Set initial spectrophotometer settings
    _test_record.astep = _spectro_astep;
    _test_record.atime = _spectro_atime;
    _test_record.again = _spectro_gain;

    Log.trace("TestRunner: Test record initialized");
}

void TestRunner::updateHeater() {
    // This would integrate with heater controller
    // For now, just call the callback if set
    if (_set_heater_power_callback && _get_temp_callback) {
        // In a real implementation, this would run PID control
        // For now we just call the callback with a placeholder
    }
}

void TestRunner::setError(ErrorCode error) {
    _last_error = error;
    _state = TestRunnerState::ERROR;
    Log.error("TestRunner: Error %d - %s",
              static_cast<int>(error), errorCodeToString(error));
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

const char* testRunnerStateToString(TestRunnerState state) {
    switch (state) {
        case TestRunnerState::IDLE:         return "IDLE";
        case TestRunnerState::INITIALIZING: return "INITIALIZING";
        case TestRunnerState::EXECUTING:    return "EXECUTING";
        case TestRunnerState::COMPLETING:   return "COMPLETING";
        case TestRunnerState::COMPLETED:    return "COMPLETED";
        case TestRunnerState::CANCELLED:    return "CANCELLED";
        case TestRunnerState::ERROR:        return "ERROR";
        default:                            return "UNKNOWN";
    }
}
