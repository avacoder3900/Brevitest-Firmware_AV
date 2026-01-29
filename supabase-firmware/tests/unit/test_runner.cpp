/**
 * @file test_runner.cpp
 * @brief Unit tests for TestRunner and BCODEInterpreter
 * @author Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * User Stories Implemented:
 *   - TEST-008: Test engine unit tests
 */

#include "TestRunner.h"
#include "BCODEInterpreter.h"
#include <cassert>
#include <cstring>

//==============================================================================
// TEST FRAMEWORK (Simple assertions)
//==============================================================================

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) do { \
    tests_run++; \
    if (condition) { \
        tests_passed++; \
        Serial.print("[PASS] "); \
    } else { \
        tests_failed++; \
        Serial.print("[FAIL] "); \
    } \
    Serial.println(message); \
} while(0)

#define TEST_ASSERT_EQUAL(expected, actual, message) do { \
    tests_run++; \
    if ((expected) == (actual)) { \
        tests_passed++; \
        Serial.print("[PASS] "); \
    } else { \
        tests_failed++; \
        Serial.print("[FAIL] "); \
        Serial.print("Expected: "); Serial.print(expected); \
        Serial.print(", Got: "); Serial.print(actual); \
        Serial.print(" - "); \
    } \
    Serial.println(message); \
} while(0)

//==============================================================================
// MOCK CALLBACKS
//==============================================================================

static bool mock_cartridge_inserted = true;
static int16_t mock_temperature = 450;  // 45.0°C
static bool mock_heater_ready = true;
static int32_t mock_stage_position = 0;
static uint8_t mock_heater_power = 0;

static int mock_move_calls = 0;
static int mock_oscillate_calls = 0;
static int mock_scan_calls = 0;

bool mockIsCartridgeInserted() {
    return mock_cartridge_inserted;
}

int16_t mockGetTemperature() {
    return mock_temperature;
}

bool mockIsHeaterReady() {
    return mock_heater_ready;
}

void mockSetHeaterPower(uint8_t power) {
    mock_heater_power = power;
}

int32_t mockGetStagePosition() {
    return mock_stage_position;
}

bool mockDelayLoop() {
    return mock_cartridge_inserted;
}

bool mockStageMove(int32_t microns, uint16_t step_delay) {
    mock_move_calls++;
    mock_stage_position += microns;
    return true;
}

bool mockStageOscillate(int32_t microns, uint16_t step_delay, uint16_t cycles, DelayLoopCallback cb) {
    mock_oscillate_calls++;
    return true;
}

bool mockSpectroConfig(uint8_t gain, uint16_t astep, uint8_t atime) {
    return true;
}

uint16_t mockSpectroScan(uint16_t num_scans, bool is_baseline) {
    mock_scan_calls++;
    return num_scans * 3;  // 3 readings per scan
}

bool mockSpectroSingle(char channel, uint8_t gain, uint16_t astep, uint8_t atime) {
    return true;
}

void resetMocks() {
    mock_cartridge_inserted = true;
    mock_temperature = 450;
    mock_heater_ready = true;
    mock_stage_position = 0;
    mock_heater_power = 0;
    mock_move_calls = 0;
    mock_oscillate_calls = 0;
    mock_scan_calls = 0;
}

//==============================================================================
// BCODE INTERPRETER TESTS
//==============================================================================

void test_bcode_load() {
    Serial.println("\n=== BCODE Load Tests ===");
    BCODEInterpreter interpreter;

    // Test 1: Load valid BCODE
    const char* bcode = "0:|1:1000|99:";
    TEST_ASSERT(interpreter.loadBCODE(bcode, strlen(bcode)),
                "Load valid BCODE");

    // Test 2: Check state after load
    TEST_ASSERT_EQUAL(static_cast<int>(InterpreterState::READY),
                      static_cast<int>(interpreter.getState()),
                      "State is READY after load");

    // Test 3: Check BCODE is loaded
    TEST_ASSERT(interpreter.isBCODELoaded(), "isBCODELoaded returns true");

    // Test 4: Check length
    TEST_ASSERT_EQUAL(strlen(bcode), interpreter.getBCODELength(),
                      "BCODE length matches");

    // Test 5: Load null BCODE
    TEST_ASSERT(!interpreter.loadBCODE(nullptr, 0), "Reject null BCODE");

    // Test 6: Load empty BCODE
    TEST_ASSERT(!interpreter.loadBCODE("", 0), "Reject empty BCODE");
}

void test_bcode_parse_opcodes() {
    Serial.println("\n=== BCODE Opcode Parsing Tests ===");
    BCODEInterpreter interpreter;

    // Test START_TEST (opcode 0)
    const char* start_test = "0:|99:";
    interpreter.loadBCODE(start_test, strlen(start_test));
    interpreter.startExecution();
    interpreter.executeNextInstruction();
    const BCODEInstruction& inst = interpreter.getCurrentInstruction();
    TEST_ASSERT_EQUAL(static_cast<int>(BCODEOpcode::START_TEST),
                      static_cast<int>(inst.opcode),
                      "Parse START_TEST opcode");

    // Test DELAY (opcode 1) with parameter
    const char* delay_cmd = "0:|1:500|99:";
    interpreter.reset();
    interpreter.loadBCODE(delay_cmd, strlen(delay_cmd));
    interpreter.setDelayLoopCallback(mockDelayLoop);
    interpreter.startExecution();
    interpreter.executeNextInstruction();  // START_TEST
    interpreter.executeNextInstruction();  // DELAY
    const BCODEInstruction& delay_inst = interpreter.getCurrentInstruction();
    TEST_ASSERT_EQUAL(static_cast<int>(BCODEOpcode::DELAY),
                      static_cast<int>(delay_inst.opcode),
                      "Parse DELAY opcode");
    TEST_ASSERT_EQUAL(500, delay_inst.params[0], "Parse DELAY parameter");

    // Test MOVE_MICRONS (opcode 2) with two parameters
    const char* move_cmd = "0:|2:1000,300|99:";
    interpreter.reset();
    interpreter.loadBCODE(move_cmd, strlen(move_cmd));
    interpreter.setStageMoveCallback(mockStageMove);
    interpreter.startExecution();
    interpreter.executeNextInstruction();  // START_TEST
    interpreter.executeNextInstruction();  // MOVE
    const BCODEInstruction& move_inst = interpreter.getCurrentInstruction();
    TEST_ASSERT_EQUAL(static_cast<int>(BCODEOpcode::MOVE_MICRONS),
                      static_cast<int>(move_inst.opcode),
                      "Parse MOVE_MICRONS opcode");
    TEST_ASSERT_EQUAL(1000, move_inst.params[0], "Parse MOVE microns parameter");
    TEST_ASSERT_EQUAL(300, move_inst.params[1], "Parse MOVE delay parameter");
}

void test_bcode_execution() {
    Serial.println("\n=== BCODE Execution Tests ===");
    resetMocks();
    BCODEInterpreter interpreter;

    // Test simple BCODE execution
    const char* bcode = "0:|2:5000,300|2:-5000,300|99:";
    interpreter.loadBCODE(bcode, strlen(bcode));
    interpreter.setDelayLoopCallback(mockDelayLoop);
    interpreter.setStageMoveCallback(mockStageMove);
    interpreter.startExecution();

    // Execute all instructions
    int count = 0;
    while (interpreter.getState() == InterpreterState::RUNNING && count < 10) {
        interpreter.executeNextInstruction();
        count++;
    }

    TEST_ASSERT_EQUAL(static_cast<int>(InterpreterState::COMPLETED),
                      static_cast<int>(interpreter.getState()),
                      "Execution completes successfully");
    TEST_ASSERT_EQUAL(2, mock_move_calls, "Correct number of move calls");
    TEST_ASSERT_EQUAL(0, mock_stage_position,
                      "Stage returns to original position");
}

void test_bcode_repeat() {
    Serial.println("\n=== BCODE Repeat Block Tests ===");
    resetMocks();
    BCODEInterpreter interpreter;

    // Test repeat block: move 1000 microns 3 times
    const char* bcode = "0:|20:3|2:1000,300|21:|99:";
    interpreter.loadBCODE(bcode, strlen(bcode));
    interpreter.setDelayLoopCallback(mockDelayLoop);
    interpreter.setStageMoveCallback(mockStageMove);
    interpreter.startExecution();

    int count = 0;
    while (interpreter.getState() == InterpreterState::RUNNING && count < 20) {
        interpreter.executeNextInstruction();
        count++;
    }

    TEST_ASSERT_EQUAL(static_cast<int>(InterpreterState::COMPLETED),
                      static_cast<int>(interpreter.getState()),
                      "Repeat execution completes");
    TEST_ASSERT_EQUAL(3, mock_move_calls, "Repeat executes correct iterations");
    TEST_ASSERT_EQUAL(3000, mock_stage_position,
                      "Stage position after 3 moves");
}

void test_bcode_cancellation() {
    Serial.println("\n=== BCODE Cancellation Tests ===");
    resetMocks();
    BCODEInterpreter interpreter;

    const char* bcode = "0:|1:5000|2:1000,300|99:";
    interpreter.loadBCODE(bcode, strlen(bcode));
    interpreter.setDelayLoopCallback(mockDelayLoop);
    interpreter.startExecution();

    // Execute first instruction
    interpreter.executeNextInstruction();

    // Cancel execution
    interpreter.cancelExecution();

    TEST_ASSERT_EQUAL(static_cast<int>(InterpreterState::CANCELLED),
                      static_cast<int>(interpreter.getState()),
                      "State is CANCELLED after cancelExecution()");
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::ERR_TEST_CANCELLED),
                      static_cast<int>(interpreter.getLastError()),
                      "Error code is TEST_CANCELLED");
}

void test_bcode_cartridge_removal() {
    Serial.println("\n=== BCODE Cartridge Removal Tests ===");
    resetMocks();
    BCODEInterpreter interpreter;

    const char* bcode = "0:|1:100|2:1000,300|99:";
    interpreter.loadBCODE(bcode, strlen(bcode));
    interpreter.setDelayLoopCallback(mockDelayLoop);
    interpreter.startExecution();

    // Execute START_TEST
    interpreter.executeNextInstruction();

    // Simulate cartridge removal
    mock_cartridge_inserted = false;

    // Try to execute DELAY - should cancel via callback
    interpreter.executeNextInstruction();

    TEST_ASSERT_EQUAL(static_cast<int>(InterpreterState::CANCELLED),
                      static_cast<int>(interpreter.getState()),
                      "Execution cancelled on cartridge removal");
}

//==============================================================================
// TEST RUNNER TESTS
//==============================================================================

void test_runner_init() {
    Serial.println("\n=== TestRunner Initialization Tests ===");
    TestRunner runner;

    // Test initialization
    TEST_ASSERT(runner.init(), "TestRunner initializes successfully");
    TEST_ASSERT(runner.isReady(), "TestRunner is ready after init");
    TEST_ASSERT_EQUAL(static_cast<int>(TestRunnerState::IDLE),
                      static_cast<int>(runner.getState()),
                      "Initial state is IDLE");
}

void test_runner_start_test() {
    Serial.println("\n=== TestRunner Start Test Tests ===");
    resetMocks();
    TestRunner runner;
    runner.init();

    // Configure callbacks
    runner.setIsCartridgeInsertedCallback(mockIsCartridgeInserted);
    runner.setIsHeaterReadyCallback(mockIsHeaterReady);

    // Create assay
    BrevitestAssay assay;
    strcpy(assay.id, "TEST001");
    strcpy(assay.BCODE, "0:|1:100|99:");
    assay.BCODE_length = strlen(assay.BCODE);
    assay.duration = 10;

    // Start test
    TEST_ASSERT(runner.startTest(&assay, "CART-UUID-12345678901234567890"),
                "Start test succeeds");
    TEST_ASSERT_EQUAL(static_cast<int>(TestRunnerState::INITIALIZING),
                      static_cast<int>(runner.getState()),
                      "State is INITIALIZING after start");

    // Try to start another test
    TEST_ASSERT(!runner.startTest(&assay, "CART-UUID-ANOTHER"),
                "Cannot start test when not idle");
}

void test_runner_test_record() {
    Serial.println("\n=== TestRunner Test Record Tests ===");
    TestRunner runner;
    runner.init();

    // Configure callbacks
    runner.setIsCartridgeInsertedCallback(mockIsCartridgeInserted);
    runner.setIsHeaterReadyCallback(mockIsHeaterReady);

    // Create and run a minimal test
    BrevitestAssay assay;
    strcpy(assay.id, "ASSAY001");
    strcpy(assay.BCODE, "0:|99:");
    assay.BCODE_length = strlen(assay.BCODE);

    const char* cartridge_id = "CART-0000-1111-2222-3333-444455556666";
    runner.startTest(&assay, cartridge_id);

    // Get test record
    BrevitestTestRecord* record = runner.getTestRecord();
    TEST_ASSERT(record != nullptr, "Test record is not null");
    TEST_ASSERT(strcmp(record->assay_id, "ASSAY001") == 0,
                "Assay ID copied correctly");
    TEST_ASSERT(strncmp(record->cartridge_id, "CART-0000", 9) == 0,
                "Cartridge ID copied correctly");
}

void test_runner_add_reading() {
    Serial.println("\n=== TestRunner Add Reading Tests ===");
    TestRunner runner;
    runner.init();

    // Create a reading
    BrevitestSpectrophotometerReading reading;
    initSpectrophotometerReading(&reading);
    reading.number = 0;
    reading.channel = 'A';
    reading.f1 = 1000;
    reading.f2 = 2000;
    reading.msec = 12345;

    // Add reading
    TEST_ASSERT(runner.addReading(&reading), "Add first reading");
    TEST_ASSERT_EQUAL(1, runner.getReadingCount(), "Reading count is 1");

    // Add more readings
    for (int i = 1; i < 10; i++) {
        reading.number = i;
        reading.msec = i * 1000;
        TEST_ASSERT(runner.addReading(&reading), "Add reading");
    }
    TEST_ASSERT_EQUAL(10, runner.getReadingCount(), "Reading count is 10");

    // Verify data in record
    const BrevitestTestRecord* record = runner.getTestRecord();
    TEST_ASSERT_EQUAL(1000, record->reading[0].f1, "First reading F1 correct");
    TEST_ASSERT_EQUAL(12345, record->reading[0].msec, "First reading msec correct");
}

void test_runner_spectro_settings() {
    Serial.println("\n=== TestRunner Spectro Settings Tests ===");
    TestRunner runner;
    runner.init();

    // Set settings
    runner.setSpectroSettings(7, 999, 49);

    uint8_t gain;
    uint16_t astep;
    uint8_t atime;
    runner.getSpectroSettings(&gain, &astep, &atime);

    TEST_ASSERT_EQUAL(7, gain, "Gain setting correct");
    TEST_ASSERT_EQUAL(999, astep, "ASTEP setting correct");
    TEST_ASSERT_EQUAL(49, atime, "ATIME setting correct");

    // Verify test record updated
    const BrevitestTestRecord* record = runner.getTestRecord();
    TEST_ASSERT_EQUAL(7, record->again, "Record AGAIN updated");
    TEST_ASSERT_EQUAL(999, record->astep, "Record ASTEP updated");
    TEST_ASSERT_EQUAL(49, record->atime, "Record ATIME updated");
}

void test_runner_stress_test() {
    Serial.println("\n=== TestRunner Stress Test Tests ===");
    TestRunner runner;
    runner.init();

    // Configure stress test
    StressTestConfig config;
    config.total_cycles = 5;
    config.vary_led_power = true;
    config.led_power_start = 100;
    config.led_power_end = 200;

    TEST_ASSERT(runner.startStressTest(config), "Start stress test");
    TEST_ASSERT(runner.isStressTestRunning(), "Stress test is running");

    // Execute cycles
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT(runner.executeStressTestCycle(), "Execute stress cycle");
    }

    // Check completion
    StressTestStatus status = runner.getStressTestStatus();
    TEST_ASSERT_EQUAL(5, status.cycles_completed, "Completed 5 cycles");

    // Next cycle should return false (complete)
    TEST_ASSERT(!runner.executeStressTestCycle(), "Cycle returns false when complete");
    TEST_ASSERT(!runner.isStressTestRunning(), "Stress test stopped");
}

//==============================================================================
// DATA TYPES TESTS
//==============================================================================

void test_data_types_sizes() {
    Serial.println("\n=== Data Structure Size Tests ===");

    TEST_ASSERT_EQUAL(32, sizeof(BrevitestSpectrophotometerReading),
                      "SpectrophotometerReading is 32 bytes");
    TEST_ASSERT_EQUAL(9668, sizeof(BrevitestTestRecord),
                      "TestRecord is 9668 bytes");
}

void test_checksum() {
    Serial.println("\n=== Checksum Tests ===");
    TestRunner runner;
    runner.init();

    // Add some data
    BrevitestSpectrophotometerReading reading;
    initSpectrophotometerReading(&reading);
    for (int i = 0; i < 5; i++) {
        reading.number = i;
        reading.f1 = 1000 + i;
        runner.addReading(&reading);
    }

    // Finalize record
    TEST_ASSERT(runner.finalizeRecord(), "Finalize record");

    // Verify checksum
    const BrevitestTestRecord* record = runner.getTestRecord();
    TEST_ASSERT(record->checksum != 0, "Checksum is non-zero");
    TEST_ASSERT(verifyTestRecordChecksum(record), "Checksum verifies correctly");
}

//==============================================================================
// MAIN TEST RUNNER
//==============================================================================

void run_all_tests() {
    tests_run = 0;
    tests_passed = 0;
    tests_failed = 0;

    Serial.println("========================================");
    Serial.println("  BREVITEST TEST ENGINE UNIT TESTS");
    Serial.println("========================================");

    // BCODE Interpreter Tests
    test_bcode_load();
    test_bcode_parse_opcodes();
    test_bcode_execution();
    test_bcode_repeat();
    test_bcode_cancellation();
    test_bcode_cartridge_removal();

    // TestRunner Tests
    test_runner_init();
    test_runner_start_test();
    test_runner_test_record();
    test_runner_add_reading();
    test_runner_spectro_settings();
    test_runner_stress_test();

    // Data Types Tests
    test_data_types_sizes();
    test_checksum();

    // Print summary
    Serial.println("\n========================================");
    Serial.println("  TEST SUMMARY");
    Serial.println("========================================");
    Serial.print("Tests Run:    "); Serial.println(tests_run);
    Serial.print("Tests Passed: "); Serial.println(tests_passed);
    Serial.print("Tests Failed: "); Serial.println(tests_failed);
    Serial.println("========================================");

    if (tests_failed == 0) {
        Serial.println("ALL TESTS PASSED!");
    } else {
        Serial.println("SOME TESTS FAILED!");
    }
}

// For Particle device integration
void setup() {
    Serial.begin(115200);
    delay(3000);  // Wait for serial connection
    run_all_tests();
}

void loop() {
    // Nothing to do
    delay(10000);
}
