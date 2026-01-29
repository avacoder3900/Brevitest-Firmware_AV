/**
 * @file TestRunner.h
 * @brief Test execution controller for Brevitest device
 * @author Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file provides the TestRunner class that orchestrates complete test
 * execution including BCODE interpretation, data collection, and result
 * generation. It coordinates all hardware modules during test runs.
 *
 * Test Workflow:
 *   1. startTest() - Initialize test with assay and cartridge
 *   2. update() - Called each loop to drive BCODE execution
 *   3. Test completes or is cancelled
 *   4. getTestRecord() - Retrieve completed test data
 *
 * User Stories Implemented:
 *   - TEST-001: TestRunner class with all lifecycle methods
 *   - TEST-003: Test data collection (spectrophotometer readings)
 *   - TEST-004: Test record generation
 *   - TEST-005: Cartridge removal detection
 *   - TEST-006: Stress test mode
 */

#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include "Particle.h"
#include "DataTypes.h"
#include "BCODEInterpreter.h"

//==============================================================================
// FORWARD DECLARATIONS (for callback integration)
//==============================================================================

// These would be provided by other modules
class MotorController;
class HeaterController;
class SpectrophotometerDriver;
class BuzzerController;

//==============================================================================
// TEST RUNNER CONFIGURATION
//==============================================================================

/**
 * @brief Test runner configuration structure
 */
struct TestRunnerConfig {
    uint16_t heater_target_temp;    ///< Target temperature (10x Celsius)
    uint16_t heater_ready_delta;    ///< Temperature delta for "ready" (10x)
    uint32_t validation_timeout_ms; ///< Cloud validation timeout
    uint8_t max_validation_retries; ///< Max validation retry attempts
    bool require_heater_ready;      ///< Whether to wait for heater

    /** @brief Default configuration */
    TestRunnerConfig() :
        heater_target_temp(450),        // 45.0°C
        heater_ready_delta(10),         // 1.0°C
        validation_timeout_ms(45000),   // 45 seconds
        max_validation_retries(3),
        require_heater_ready(true)
    {}
};

//==============================================================================
// TEST RUNNER STATE
//==============================================================================

/**
 * @brief Test runner operational states
 */
enum class TestRunnerState : uint8_t {
    IDLE = 0,               ///< Ready to start a test
    INITIALIZING = 1,       ///< Setting up hardware for test
    EXECUTING = 2,          ///< BCODE execution in progress
    COMPLETING = 3,         ///< Test finishing, generating record
    COMPLETED = 4,          ///< Test complete, record ready
    CANCELLED = 5,          ///< Test cancelled (cartridge removed)
    ERROR = 6               ///< Test failed with error
};

//==============================================================================
// STRESS TEST CONFIGURATION
//==============================================================================

/**
 * @brief Stress test configuration
 */
struct StressTestConfig {
    uint16_t total_cycles;          ///< Total cycles to run
    uint16_t led_power_start;       ///< Starting LED power (0-255)
    uint16_t led_power_end;         ///< Ending LED power (0-255)
    uint16_t led_power_step;        ///< LED power increment
    bool vary_led_power;            ///< Whether to vary LED power
    uint32_t cycle_delay_ms;        ///< Delay between cycles

    /** @brief Default stress test config */
    StressTestConfig() :
        total_cycles(100),
        led_power_start(115),
        led_power_end(115),
        led_power_step(0),
        vary_led_power(false),
        cycle_delay_ms(1000)
    {}
};

/**
 * @brief Stress test status
 */
struct StressTestStatus {
    uint32_t cycles_completed;      ///< Cycles completed this session
    uint32_t lifetime_cycles;       ///< Total lifetime cycles
    uint32_t total_readings;        ///< Total readings taken
    uint16_t current_led_power;     ///< Current LED power setting
    bool running;                   ///< Whether stress test is active

    StressTestStatus() :
        cycles_completed(0),
        lifetime_cycles(0),
        total_readings(0),
        current_led_power(115),
        running(false)
    {}
};

//==============================================================================
// CALLBACK TYPES FOR HARDWARE INTEGRATION
//==============================================================================

/**
 * @brief Callback to get current temperature
 * @return Temperature in 10x Celsius
 */
typedef int16_t (*GetTemperatureCallback)(void);

/**
 * @brief Callback to check heater ready status
 * @return true if heater is at target temperature
 */
typedef bool (*IsHeaterReadyCallback)(void);

/**
 * @brief Callback to control heater power
 * @param power PID-calculated power value
 */
typedef void (*SetHeaterPowerCallback)(uint8_t power);

/**
 * @brief Callback to check cartridge presence
 * @return true if cartridge is inserted
 */
typedef bool (*IsCartridgeInsertedCallback)(void);

/**
 * @brief Callback to get current stage position
 * @return Stage position in microns
 */
typedef int32_t (*GetStagePositionCallback)(void);

/**
 * @brief Callback for state change notification
 * @param old_state Previous state
 * @param new_state New state
 */
typedef void (*StateChangeCallback)(TestRunnerState old_state, TestRunnerState new_state);

//==============================================================================
// TEST RUNNER CLASS
//==============================================================================

/**
 * @class TestRunner
 * @brief Main test execution controller
 *
 * Manages the complete test lifecycle from initialization through result
 * generation. Coordinates BCODE interpretation with hardware control.
 *
 * Usage:
 *   TestRunner runner;
 *   runner.init();
 *   runner.setCallbacks(...);
 *   runner.startTest(assay, cartridge_id);
 *   while (runner.getState() == TestRunnerState::EXECUTING) {
 *       runner.update();
 *   }
 *   BrevitestTestRecord* record = runner.getTestRecord();
 */
class TestRunner {
public:
    //==========================================================================
    // LIFECYCLE (TEST-001)
    //==========================================================================

    /**
     * @brief Constructor
     */
    TestRunner();

    /**
     * @brief Initialize test runner
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Shutdown test runner
     */
    void shutdown();

    /**
     * @brief Update function - call every loop iteration
     * @details Drives BCODE execution and monitors test state
     */
    void update();

    /**
     * @brief Check if test runner is ready
     * @return true if ready to start a test
     */
    bool isReady() const;

    /**
     * @brief Get human-readable status string
     * @return Status string
     */
    String getStatus() const;

    //==========================================================================
    // TEST CONTROL (TEST-001)
    //==========================================================================

    /**
     * @brief Start a test with given assay and cartridge
     * @param assay Pointer to assay with BCODE
     * @param cartridge_id Cartridge UUID string
     * @return true if test started successfully
     */
    bool startTest(const BrevitestAssay* assay, const char* cartridge_id);

    /**
     * @brief Stop the current test
     * @details Gracefully stops execution and saves partial data
     */
    void stopTest();

    /**
     * @brief Get current test state
     * @return Current TestState
     */
    TestState getTestState() const;

    /**
     * @brief Get test runner state
     * @return Current TestRunnerState
     */
    TestRunnerState getState() const;

    /**
     * @brief Get test progress percentage
     * @return Progress 0-100
     */
    uint8_t getTestProgress() const;

    /**
     * @brief Get elapsed test time
     * @return Elapsed time in milliseconds
     */
    uint32_t getElapsedTime() const;

    //==========================================================================
    // TEST DATA (TEST-003, TEST-004)
    //==========================================================================

    /**
     * @brief Get pointer to test record
     * @return Pointer to current test record
     */
    BrevitestTestRecord* getTestRecord();

    /**
     * @brief Get const pointer to test record
     * @return Const pointer to current test record
     */
    const BrevitestTestRecord* getTestRecord() const;

    /**
     * @brief Get number of readings collected
     * @return Number of spectrophotometer readings
     */
    uint16_t getReadingCount() const;

    /**
     * @brief Check if test record is complete and valid
     * @return true if record is ready for upload
     */
    bool isRecordComplete() const;

    /**
     * @brief Finalize test record (calculate checksum, etc.)
     * @return true if finalization successful
     */
    bool finalizeRecord();

    //==========================================================================
    // SPECTROPHOTOMETER DATA COLLECTION (TEST-003)
    //==========================================================================

    /**
     * @brief Add a spectrophotometer reading to the test record
     * @param reading Pointer to reading data
     * @return true if reading added successfully
     */
    bool addReading(const BrevitestSpectrophotometerReading* reading);

    /**
     * @brief Get current spectrophotometer settings
     * @param gain Output AGAIN value
     * @param astep Output ASTEP value
     * @param atime Output ATIME value
     */
    void getSpectroSettings(uint8_t* gain, uint16_t* astep, uint8_t* atime) const;

    /**
     * @brief Set spectrophotometer settings
     * @param gain AGAIN value
     * @param astep ASTEP value
     * @param atime ATIME value
     */
    void setSpectroSettings(uint8_t gain, uint16_t astep, uint8_t atime);

    /**
     * @brief Increment baseline scan count
     * @param count Number of scans to add
     */
    void addBaselineScans(uint16_t count);

    /**
     * @brief Increment test scan count
     * @param count Number of scans to add
     */
    void addTestScans(uint16_t count);

    //==========================================================================
    // CARTRIDGE HANDLING (TEST-005)
    //==========================================================================

    /**
     * @brief Handle cartridge removal event
     * @details Called when cartridge removal is detected during test
     */
    void onCartridgeRemoved();

    /**
     * @brief Check if test was cancelled due to cartridge removal
     * @return true if cancelled
     */
    bool wasCartridgeRemoved() const;

    //==========================================================================
    // STRESS TEST MODE (TEST-006)
    //==========================================================================

    /**
     * @brief Start stress test mode
     * @param config Stress test configuration
     * @return true if stress test started
     */
    bool startStressTest(const StressTestConfig& config);

    /**
     * @brief Stop stress test mode
     */
    void stopStressTest();

    /**
     * @brief Check if stress test is running
     * @return true if in stress test mode
     */
    bool isStressTestRunning() const;

    /**
     * @brief Get stress test status
     * @return Current stress test status
     */
    StressTestStatus getStressTestStatus() const;

    /**
     * @brief Execute one stress test cycle
     * @return true if cycle completed successfully
     */
    bool executeStressTestCycle();

    //==========================================================================
    // CONFIGURATION
    //==========================================================================

    /**
     * @brief Set test runner configuration
     * @param config Configuration structure
     */
    void setConfig(const TestRunnerConfig& config);

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    const TestRunnerConfig& getConfig() const;

    //==========================================================================
    // CALLBACK REGISTRATION
    //==========================================================================

    /**
     * @brief Set temperature reading callback
     */
    void setGetTemperatureCallback(GetTemperatureCallback callback);

    /**
     * @brief Set heater ready callback
     */
    void setIsHeaterReadyCallback(IsHeaterReadyCallback callback);

    /**
     * @brief Set heater power callback
     */
    void setSetHeaterPowerCallback(SetHeaterPowerCallback callback);

    /**
     * @brief Set cartridge detection callback
     */
    void setIsCartridgeInsertedCallback(IsCartridgeInsertedCallback callback);

    /**
     * @brief Set stage position callback
     */
    void setGetStagePositionCallback(GetStagePositionCallback callback);

    /**
     * @brief Set state change callback
     */
    void setStateChangeCallback(StateChangeCallback callback);

    //==========================================================================
    // BCODE INTERPRETER ACCESS
    //==========================================================================

    /**
     * @brief Get reference to BCODE interpreter
     * @return Reference to internal interpreter
     */
    BCODEInterpreter& getInterpreter();

    /**
     * @brief Get const reference to BCODE interpreter
     * @return Const reference to internal interpreter
     */
    const BCODEInterpreter& getInterpreter() const;

    //==========================================================================
    // ERROR HANDLING
    //==========================================================================

    /**
     * @brief Get last error code
     * @return ErrorCode
     */
    ErrorCode getLastError() const;

    /**
     * @brief Get error message
     * @return Error message string
     */
    const char* getErrorMessage() const;

private:
    //==========================================================================
    // INTERNAL STATE
    //==========================================================================

    bool _initialized;                  ///< Whether init() was called
    TestRunnerState _state;             ///< Current runner state
    TestState _test_state;              ///< Current test state
    ErrorCode _last_error;              ///< Last error code

    BCODEInterpreter _interpreter;      ///< BCODE interpreter instance
    BrevitestTestRecord _test_record;   ///< Current test record
    TestRunnerConfig _config;           ///< Configuration

    uint32_t _test_start_time;          ///< Test start timestamp
    bool _cartridge_removed;            ///< Cartridge removal flag
    bool _record_finalized;             ///< Whether record is finalized

    // Spectrophotometer settings (used during test)
    uint8_t _spectro_gain;
    uint16_t _spectro_astep;
    uint8_t _spectro_atime;

    // Stress test state
    StressTestConfig _stress_config;
    StressTestStatus _stress_status;

    //==========================================================================
    // CALLBACKS
    //==========================================================================

    GetTemperatureCallback _get_temp_callback;
    IsHeaterReadyCallback _is_heater_ready_callback;
    SetHeaterPowerCallback _set_heater_power_callback;
    IsCartridgeInsertedCallback _is_cartridge_inserted_callback;
    GetStagePositionCallback _get_stage_position_callback;
    StateChangeCallback _state_change_callback;

    //==========================================================================
    // INTERNAL METHODS
    //==========================================================================

    /**
     * @brief Set test runner state with notification
     * @param new_state New state to set
     */
    void setState(TestRunnerState new_state);

    /**
     * @brief Initialize test record with assay/cartridge info
     * @param assay Assay data
     * @param cartridge_id Cartridge UUID
     */
    void initializeTestRecord(const BrevitestAssay* assay, const char* cartridge_id);

    /**
     * @brief BCODE delay loop callback
     * @return true to continue, false to cancel
     */
    static bool bcodeDelayLoop();

    /**
     * @brief Check for cartridge removal
     * @return true if cartridge still present
     */
    bool checkCartridge();

    /**
     * @brief Run heater control loop
     */
    void updateHeater();

    /**
     * @brief Set error state
     * @param error Error code
     */
    void setError(ErrorCode error);
};

//==============================================================================
// SINGLETON ACCESS (optional pattern)
//==============================================================================

/**
 * @brief Get global TestRunner instance
 * @return Reference to singleton TestRunner
 * @note For use in static callbacks
 */
TestRunner& getTestRunner();

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Convert TestRunnerState to string
 * @param state State value
 * @return String representation
 */
const char* testRunnerStateToString(TestRunnerState state);

#endif // TESTRUNNER_H
