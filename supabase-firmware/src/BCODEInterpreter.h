/**
 * @file BCODEInterpreter.h
 * @brief BCODE instruction interpreter for Brevitest test execution
 * @author Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file provides the BCODE interpreter that executes test instructions.
 * BCODE is a custom instruction set that controls the test workflow including
 * stage movement, spectrophotometer readings, timing, and hardware control.
 *
 * BCODE Format:
 *   Instructions are delimited by '|' (ITEM_DELIM)
 *   Command and arguments separated by ':' (ATTR_DELIM)
 *   Multiple arguments separated by ',' (ARG_DELIM)
 *   Example: "0:|1:1000|2:5000,300|99:"
 *
 * User Stories Implemented:
 *   - TEST-002: BCODE interpreter with all opcodes
 *   - TEST-007: BCODE specification (documented in header)
 */

#ifndef BCODEINTERPRETER_H
#define BCODEINTERPRETER_H

#include "Particle.h"
#include "DataTypes.h"

//==============================================================================
// BCODE OPCODE DEFINITIONS
//==============================================================================

/**
 * @brief BCODE instruction opcodes
 * @details These opcodes control test execution. The opcode is the first
 *          number in each instruction, followed by optional parameters.
 */
enum class BCODEOpcode : uint8_t {
    // Test lifecycle
    START_TEST = 0,         ///< Start test - must be first instruction
    DELAY = 1,              ///< Delay(milliseconds)
    END_TEST = 99,          ///< End test - signals test completion

    // Stage control
    MOVE_MICRONS = 2,       ///< Move stage (microns, step_delay_us)
    OSCILLATE = 3,          ///< Oscillate stage (microns, step_delay_us, cycles)

    // Spectrophotometer settings
    SET_SENSOR_PARAMS = 10, ///< Set sensor params (gain, step, integration_time)
    BASELINE_SCANS = 11,    ///< Take baseline scans (num_scans)
    TEST_SCANS = 14,        ///< Take test scans (num_scans)
    SENSOR_READING = 15,    ///< Single reading (channel, gain, step, integration)
    CONTINUOUS_SCANS = 16,  ///< Continuous scans (baseline, start_pos, distance, step_delay)

    // Control flow
    REPEAT_BEGIN = 20,      ///< Start repeat block (iterations)
    REPEAT_END = 21,        ///< End repeat block

    // Invalid/unknown
    INVALID = 255           ///< Invalid opcode marker
};

//==============================================================================
// BCODE INSTRUCTION STRUCTURE
//==============================================================================

/**
 * @brief Parsed BCODE instruction with parameters
 */
struct BCODEInstruction {
    BCODEOpcode opcode;     ///< Instruction opcode
    int32_t params[4];      ///< Up to 4 parameters
    uint8_t param_count;    ///< Number of parameters parsed
    bool valid;             ///< Whether instruction was parsed successfully

    /** @brief Default constructor */
    BCODEInstruction() : opcode(BCODEOpcode::INVALID), param_count(0), valid(false) {
        for (int i = 0; i < 4; i++) params[i] = 0;
    }
};

//==============================================================================
// INTERPRETER STATE
//==============================================================================

/**
 * @brief BCODE interpreter execution state
 */
enum class InterpreterState : uint8_t {
    IDLE = 0,           ///< Not executing - no BCODE loaded
    READY = 1,          ///< BCODE loaded, ready to execute
    RUNNING = 2,        ///< Currently executing instructions
    PAUSED = 3,         ///< Execution paused (e.g., during delay)
    COMPLETED = 4,      ///< Execution completed successfully
    ERROR = 5,          ///< Execution stopped due to error
    CANCELLED = 6       ///< Execution cancelled externally
};

//==============================================================================
// CALLBACK FUNCTION TYPES
//==============================================================================

/**
 * @brief Callback for delay operations
 * @details Called periodically during delay to allow heater control and
 *          cartridge detection. Return false to cancel execution.
 */
typedef bool (*DelayLoopCallback)(void);

/**
 * @brief Callback for stage movement
 * @param microns Distance to move in microns (positive = forward)
 * @param step_delay_us Delay between steps in microseconds
 * @return true if movement successful
 */
typedef bool (*StageMoveCallback)(int32_t microns, uint16_t step_delay_us);

/**
 * @brief Callback for stage oscillation
 * @param microns Amplitude in microns
 * @param step_delay_us Delay between steps
 * @param cycles Number of oscillation cycles
 * @param loop_callback Function to call between oscillations
 * @return true if oscillation successful
 */
typedef bool (*StageOscillateCallback)(int32_t microns, uint16_t step_delay_us,
                                        uint16_t cycles, DelayLoopCallback loop_callback);

/**
 * @brief Callback for spectrophotometer configuration
 * @param gain AGAIN value
 * @param astep ASTEP value
 * @param atime ATIME value
 * @return true if configuration successful
 */
typedef bool (*SpectroConfigCallback)(uint8_t gain, uint16_t astep, uint8_t atime);

/**
 * @brief Callback for taking spectrophotometer readings
 * @param num_scans Number of scans to take
 * @param is_baseline true for baseline scans, false for test scans
 * @return Number of readings taken
 */
typedef uint16_t (*SpectroScanCallback)(uint16_t num_scans, bool is_baseline);

/**
 * @brief Callback for single spectrophotometer reading
 * @param channel Channel ('A', 'B', 'C', or 0 for all)
 * @param gain AGAIN value
 * @param astep ASTEP value
 * @param atime ATIME value
 * @return true if reading successful
 */
typedef bool (*SpectroSingleCallback)(char channel, uint8_t gain, uint16_t astep, uint8_t atime);

/**
 * @brief Callback for continuous spectrophotometer scanning
 * @param is_baseline true for baseline, false for test
 * @param start_position Starting stage position
 * @param distance Scan distance
 * @param step_delay_us Step delay
 * @return Number of readings taken
 */
typedef uint16_t (*SpectroContinuousCallback)(bool is_baseline, int32_t start_position,
                                               int32_t distance, uint16_t step_delay_us);

//==============================================================================
// BCODE INTERPRETER CLASS
//==============================================================================

/**
 * @class BCODEInterpreter
 * @brief BCODE instruction interpreter
 *
 * Parses and executes BCODE test instructions. The interpreter maintains
 * execution state and supports nested repeat blocks.
 *
 * Usage:
 *   BCODEInterpreter interpreter;
 *   interpreter.loadBCODE(bcode_string, length);
 *   interpreter.setCallbacks(...);
 *   while (interpreter.getState() == InterpreterState::RUNNING) {
 *       interpreter.executeNextInstruction();
 *   }
 */
class BCODEInterpreter {
public:
    //==========================================================================
    // LIFECYCLE
    //==========================================================================

    /**
     * @brief Constructor
     */
    BCODEInterpreter();

    /**
     * @brief Reset interpreter to initial state
     */
    void reset();

    //==========================================================================
    // BCODE LOADING
    //==========================================================================

    /**
     * @brief Load BCODE string for execution
     * @param bcode Pointer to BCODE string
     * @param length Length of BCODE string
     * @return true if BCODE loaded successfully
     */
    bool loadBCODE(const char* bcode, uint16_t length);

    /**
     * @brief Check if BCODE is loaded and valid
     * @return true if BCODE is ready for execution
     */
    bool isBCODELoaded() const;

    /**
     * @brief Get the loaded BCODE length
     * @return Length of loaded BCODE, or 0 if not loaded
     */
    uint16_t getBCODELength() const;

    //==========================================================================
    // EXECUTION
    //==========================================================================

    /**
     * @brief Start BCODE execution from beginning
     * @return true if execution started successfully
     */
    bool startExecution();

    /**
     * @brief Execute the next instruction
     * @return true if instruction executed successfully, false on error/completion
     */
    bool executeNextInstruction();

    /**
     * @brief Cancel execution
     * @details Sets state to CANCELLED and stops execution
     */
    void cancelExecution();

    /**
     * @brief Pause execution
     */
    void pauseExecution();

    /**
     * @brief Resume paused execution
     */
    void resumeExecution();

    //==========================================================================
    // STATE QUERIES
    //==========================================================================

    /**
     * @brief Get current interpreter state
     * @return Current InterpreterState
     */
    InterpreterState getState() const;

    /**
     * @brief Get current instruction pointer position
     * @return Position in BCODE string
     */
    uint16_t getInstructionPointer() const;

    /**
     * @brief Get current instruction number (for progress tracking)
     * @return Number of instructions executed
     */
    uint16_t getInstructionCount() const;

    /**
     * @brief Get last error code
     * @return ErrorCode of last error, or SUCCESS if no error
     */
    ErrorCode getLastError() const;

    /**
     * @brief Get human-readable error message
     * @return Error message string
     */
    const char* getErrorMessage() const;

    /**
     * @brief Get current/last parsed instruction
     * @return Reference to current instruction
     */
    const BCODEInstruction& getCurrentInstruction() const;

    //==========================================================================
    // CALLBACK REGISTRATION
    //==========================================================================

    /**
     * @brief Set the delay loop callback
     * @param callback Function to call during delays
     */
    void setDelayLoopCallback(DelayLoopCallback callback);

    /**
     * @brief Set the stage move callback
     * @param callback Function to call for stage movement
     */
    void setStageMoveCallback(StageMoveCallback callback);

    /**
     * @brief Set the stage oscillate callback
     * @param callback Function to call for stage oscillation
     */
    void setStageOscillateCallback(StageOscillateCallback callback);

    /**
     * @brief Set the spectrophotometer config callback
     * @param callback Function to call for spectro configuration
     */
    void setSpectroConfigCallback(SpectroConfigCallback callback);

    /**
     * @brief Set the spectrophotometer scan callback
     * @param callback Function to call for baseline/test scans
     */
    void setSpectroScanCallback(SpectroScanCallback callback);

    /**
     * @brief Set the spectrophotometer single reading callback
     * @param callback Function to call for single readings
     */
    void setSpectroSingleCallback(SpectroSingleCallback callback);

    /**
     * @brief Set the continuous scan callback
     * @param callback Function to call for continuous scans
     */
    void setSpectroContinuousCallback(SpectroContinuousCallback callback);

private:
    //==========================================================================
    // INTERNAL STATE
    //==========================================================================

    char _bcode[BCODE_CAPACITY];        ///< BCODE buffer
    uint16_t _bcode_length;             ///< Length of loaded BCODE
    uint16_t _instruction_pointer;      ///< Current position in BCODE
    uint16_t _instruction_count;        ///< Instructions executed
    InterpreterState _state;            ///< Current interpreter state
    ErrorCode _last_error;              ///< Last error code
    BCODEInstruction _current_instruction; ///< Current parsed instruction

    // Repeat block tracking (supports 4 levels of nesting)
    static const uint8_t MAX_REPEAT_DEPTH = 4;
    struct RepeatBlock {
        uint16_t start_index;           ///< Index after REPEAT_BEGIN
        uint16_t iterations_remaining;  ///< Iterations left
        bool active;                    ///< Whether block is active
    };
    RepeatBlock _repeat_stack[MAX_REPEAT_DEPTH];
    uint8_t _repeat_depth;              ///< Current repeat nesting depth

    //==========================================================================
    // CALLBACKS
    //==========================================================================

    DelayLoopCallback _delay_callback;
    StageMoveCallback _stage_move_callback;
    StageOscillateCallback _stage_oscillate_callback;
    SpectroConfigCallback _spectro_config_callback;
    SpectroScanCallback _spectro_scan_callback;
    SpectroSingleCallback _spectro_single_callback;
    SpectroContinuousCallback _spectro_continuous_callback;

    //==========================================================================
    // INTERNAL METHODS
    //==========================================================================

    /**
     * @brief Parse the next token from BCODE
     * @param index Starting index
     * @param token Output token value
     * @return New index after token
     */
    uint16_t getToken(uint16_t index, int32_t* token);

    /**
     * @brief Parse instruction at current position
     * @return Parsed instruction
     */
    BCODEInstruction parseInstruction();

    /**
     * @brief Execute a parsed instruction
     * @param instruction Instruction to execute
     * @return true if execution successful
     */
    bool executeInstruction(const BCODEInstruction& instruction);

    /**
     * @brief Execute delay with loop callback
     * @param milliseconds Delay duration
     * @return true if delay completed, false if cancelled
     */
    bool executeDelay(int32_t milliseconds);

    /**
     * @brief Set error state
     * @param error Error code to set
     */
    void setError(ErrorCode error);

    /**
     * @brief Extract integer from string at position
     * @param str String to parse
     * @param start Start index
     * @param length Length of number
     * @return Parsed integer value
     */
    int32_t extractInt(const char* str, uint16_t start, uint16_t length);
};

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

/**
 * @brief Convert BCODEOpcode to string
 * @param opcode Opcode value
 * @return String representation
 */
const char* bcodeOpcodeToString(BCODEOpcode opcode);

/**
 * @brief Convert InterpreterState to string
 * @param state State value
 * @return String representation
 */
const char* interpreterStateToString(InterpreterState state);

#endif // BCODEINTERPRETER_H
