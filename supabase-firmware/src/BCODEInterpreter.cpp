/**
 * @file BCODEInterpreter.cpp
 * @brief Implementation of BCODE instruction interpreter
 * @author Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * User Stories Implemented:
 *   - TEST-002: BCODE interpreter with all opcodes
 */

#include "BCODEInterpreter.h"

//==============================================================================
// CONSTANTS
//==============================================================================

/** @brief Delay unit for chunked delays (allows heater control loop) */
static const uint16_t BCODE_DELAY_UNIT_MS = 1000;

//==============================================================================
// CONSTRUCTOR AND LIFECYCLE
//==============================================================================

BCODEInterpreter::BCODEInterpreter() {
    reset();
}

void BCODEInterpreter::reset() {
    _bcode[0] = '\0';
    _bcode_length = 0;
    _instruction_pointer = 0;
    _instruction_count = 0;
    _state = InterpreterState::IDLE;
    _last_error = ErrorCode::SUCCESS;
    _current_instruction = BCODEInstruction();
    _repeat_depth = 0;

    for (uint8_t i = 0; i < MAX_REPEAT_DEPTH; i++) {
        _repeat_stack[i].start_index = 0;
        _repeat_stack[i].iterations_remaining = 0;
        _repeat_stack[i].active = false;
    }

    // Reset callbacks
    _delay_callback = nullptr;
    _stage_move_callback = nullptr;
    _stage_oscillate_callback = nullptr;
    _spectro_config_callback = nullptr;
    _spectro_scan_callback = nullptr;
    _spectro_single_callback = nullptr;
    _spectro_continuous_callback = nullptr;
}

//==============================================================================
// BCODE LOADING
//==============================================================================

bool BCODEInterpreter::loadBCODE(const char* bcode, uint16_t length) {
    if (bcode == nullptr || length == 0) {
        setError(ErrorCode::ERR_INVALID_PARAMETER);
        return false;
    }

    if (length >= BCODE_CAPACITY) {
        setError(ErrorCode::ERR_BCODE_INVALID);
        return false;
    }

    // Copy BCODE to internal buffer
    memcpy(_bcode, bcode, length);
    _bcode[length] = '\0';
    _bcode_length = length;

    // Reset execution state
    _instruction_pointer = 0;
    _instruction_count = 0;
    _repeat_depth = 0;
    _last_error = ErrorCode::SUCCESS;
    _state = InterpreterState::READY;

    Log.info("BCODEInterpreter: Loaded %u bytes of BCODE", length);
    return true;
}

bool BCODEInterpreter::isBCODELoaded() const {
    return _bcode_length > 0 && (_state == InterpreterState::READY ||
                                  _state == InterpreterState::RUNNING ||
                                  _state == InterpreterState::PAUSED);
}

uint16_t BCODEInterpreter::getBCODELength() const {
    return _bcode_length;
}

//==============================================================================
// EXECUTION CONTROL
//==============================================================================

bool BCODEInterpreter::startExecution() {
    if (_state != InterpreterState::READY) {
        if (_bcode_length == 0) {
            setError(ErrorCode::ERR_BCODE_INVALID);
            return false;
        }
        // Reset for re-execution
        _instruction_pointer = 0;
        _instruction_count = 0;
        _repeat_depth = 0;
    }

    _state = InterpreterState::RUNNING;
    _last_error = ErrorCode::SUCCESS;
    Log.info("BCODEInterpreter: Starting execution");
    return true;
}

bool BCODEInterpreter::executeNextInstruction() {
    if (_state != InterpreterState::RUNNING) {
        return false;
    }

    // Check if we've reached the end
    if (_instruction_pointer >= _bcode_length ||
        _bcode[_instruction_pointer] == '\0') {
        _state = InterpreterState::COMPLETED;
        Log.info("BCODEInterpreter: Execution completed (%u instructions)", _instruction_count);
        return false;
    }

    // Check for end delimiter
    if (_bcode[_instruction_pointer] == END_DELIM) {
        _state = InterpreterState::COMPLETED;
        Log.info("BCODEInterpreter: End delimiter reached");
        return false;
    }

    // Parse next instruction
    _current_instruction = parseInstruction();

    if (!_current_instruction.valid) {
        setError(ErrorCode::ERR_BCODE_INVALID);
        return false;
    }

    // Execute instruction
    bool success = executeInstruction(_current_instruction);
    _instruction_count++;

    if (!success && _state == InterpreterState::RUNNING) {
        // Error occurred but wasn't already handled
        if (_last_error == ErrorCode::SUCCESS) {
            setError(ErrorCode::ERR_TEST_FAILED);
        }
        return false;
    }

    return _state == InterpreterState::RUNNING;
}

void BCODEInterpreter::cancelExecution() {
    if (_state == InterpreterState::RUNNING || _state == InterpreterState::PAUSED) {
        _state = InterpreterState::CANCELLED;
        _last_error = ErrorCode::ERR_TEST_CANCELLED;
        Log.warn("BCODEInterpreter: Execution cancelled");
    }
}

void BCODEInterpreter::pauseExecution() {
    if (_state == InterpreterState::RUNNING) {
        _state = InterpreterState::PAUSED;
        Log.info("BCODEInterpreter: Execution paused");
    }
}

void BCODEInterpreter::resumeExecution() {
    if (_state == InterpreterState::PAUSED) {
        _state = InterpreterState::RUNNING;
        Log.info("BCODEInterpreter: Execution resumed");
    }
}

//==============================================================================
// STATE QUERIES
//==============================================================================

InterpreterState BCODEInterpreter::getState() const {
    return _state;
}

uint16_t BCODEInterpreter::getInstructionPointer() const {
    return _instruction_pointer;
}

uint16_t BCODEInterpreter::getInstructionCount() const {
    return _instruction_count;
}

ErrorCode BCODEInterpreter::getLastError() const {
    return _last_error;
}

const char* BCODEInterpreter::getErrorMessage() const {
    return errorCodeToString(_last_error);
}

const BCODEInstruction& BCODEInterpreter::getCurrentInstruction() const {
    return _current_instruction;
}

//==============================================================================
// CALLBACK REGISTRATION
//==============================================================================

void BCODEInterpreter::setDelayLoopCallback(DelayLoopCallback callback) {
    _delay_callback = callback;
}

void BCODEInterpreter::setStageMoveCallback(StageMoveCallback callback) {
    _stage_move_callback = callback;
}

void BCODEInterpreter::setStageOscillateCallback(StageOscillateCallback callback) {
    _stage_oscillate_callback = callback;
}

void BCODEInterpreter::setSpectroConfigCallback(SpectroConfigCallback callback) {
    _spectro_config_callback = callback;
}

void BCODEInterpreter::setSpectroScanCallback(SpectroScanCallback callback) {
    _spectro_scan_callback = callback;
}

void BCODEInterpreter::setSpectroSingleCallback(SpectroSingleCallback callback) {
    _spectro_single_callback = callback;
}

void BCODEInterpreter::setSpectroContinuousCallback(SpectroContinuousCallback callback) {
    _spectro_continuous_callback = callback;
}

//==============================================================================
// TOKEN PARSING
//==============================================================================

uint16_t BCODEInterpreter::getToken(uint16_t index, int32_t* token) {
    if (_state == InterpreterState::CANCELLED) {
        return index;
    }

    // End of string - return current position
    if (_bcode[index] == ITEM_DELIM || _bcode[index] == '\0') {
        return index;
    }

    // Command has no arguments - skip delimiter
    if (_bcode[index] == ATTR_DELIM) {
        return index + 1;
    }

    // Extract numeric token
    uint16_t i = index;
    while (i < _bcode_length) {
        char c = _bcode[i];

        if (c == ATTR_DELIM) {
            *token = extractInt(_bcode, index, i - index);
            return i;  // Return position at delimiter
        }

        if (c == ARG_DELIM) {
            *token = extractInt(_bcode, index, i - index);
            return i + 1;  // Skip past comma
        }

        if (c == ITEM_DELIM || c == '\0') {
            *token = extractInt(_bcode, index, i - index);
            return i;
        }

        i++;
    }

    return i;
}

int32_t BCODEInterpreter::extractInt(const char* str, uint16_t start, uint16_t length) {
    if (length == 0) return 0;

    int32_t result = 0;
    bool negative = false;
    uint16_t i = start;

    // Check for negative
    if (str[i] == '-') {
        negative = true;
        i++;
        length--;
    }

    // Parse digits
    while (length > 0 && i < _bcode_length) {
        char c = str[i];
        if (c >= '0' && c <= '9') {
            result = result * 10 + (c - '0');
        }
        i++;
        length--;
    }

    return negative ? -result : result;
}

//==============================================================================
// INSTRUCTION PARSING
//==============================================================================

BCODEInstruction BCODEInterpreter::parseInstruction() {
    BCODEInstruction instruction;
    int32_t cmd = 0;

    // Get opcode
    uint16_t index = getToken(_instruction_pointer, &cmd);

    // Validate opcode range
    if (cmd < 0 || cmd > 255) {
        Log.error("BCODEInterpreter: Invalid opcode %ld", cmd);
        return instruction;
    }

    instruction.opcode = static_cast<BCODEOpcode>(cmd);
    instruction.param_count = 0;

    // Skip past attribute delimiter between opcode and parameters
    if (index < _bcode_length && _bcode[index] == ATTR_DELIM) {
        index++;
    }

    // Parse parameters (up to 4)
    while (instruction.param_count < 4 &&
           index < _bcode_length &&
           _bcode[index] != ITEM_DELIM &&
           _bcode[index] != '\0') {

        int32_t param = 0;
        index = getToken(index, &param);
        instruction.params[instruction.param_count++] = param;
    }

    // Skip past item delimiter
    if (index < _bcode_length && _bcode[index] == ATTR_DELIM) {
        index++;
    }
    if (index < _bcode_length && _bcode[index] == ITEM_DELIM) {
        index++;
    }

    _instruction_pointer = index;
    instruction.valid = true;

    return instruction;
}

//==============================================================================
// INSTRUCTION EXECUTION
//==============================================================================

bool BCODEInterpreter::executeInstruction(const BCODEInstruction& instruction) {
    if (_state == InterpreterState::CANCELLED) {
        return false;
    }

    switch (instruction.opcode) {
        case BCODEOpcode::START_TEST:
            // Opcode 0: Start test
            Log.info("BCODE: Start test");
            delay(1000);  // Initial delay as per legacy
            return true;

        case BCODEOpcode::DELAY:
            // Opcode 1: Delay(milliseconds)
            if (instruction.param_count < 1) {
                setError(ErrorCode::ERR_BCODE_INVALID);
                return false;
            }
            Log.info("BCODE: Delay %ld ms", instruction.params[0]);
            return executeDelay(instruction.params[0]);

        case BCODEOpcode::MOVE_MICRONS:
            // Opcode 2: Move Microns(microns, step_delay_us)
            if (instruction.param_count < 2) {
                setError(ErrorCode::ERR_BCODE_INVALID);
                return false;
            }
            Log.info("BCODE: Move %ld microns, %ld us", instruction.params[0], instruction.params[1]);
            if (_stage_move_callback) {
                return _stage_move_callback(instruction.params[0],
                                            static_cast<uint16_t>(instruction.params[1]));
            }
            return true;  // No callback = skip

        case BCODEOpcode::OSCILLATE:
            // Opcode 3: Oscillate(microns, step_delay_us, cycles)
            if (instruction.param_count < 3) {
                setError(ErrorCode::ERR_BCODE_INVALID);
                return false;
            }
            Log.info("BCODE: Oscillate %ld microns, %ld us, %ld cycles",
                     instruction.params[0], instruction.params[1], instruction.params[2]);
            if (_stage_oscillate_callback) {
                return _stage_oscillate_callback(instruction.params[0],
                                                  static_cast<uint16_t>(instruction.params[1]),
                                                  static_cast<uint16_t>(instruction.params[2]),
                                                  _delay_callback);
            }
            return true;

        case BCODEOpcode::SET_SENSOR_PARAMS:
            // Opcode 10: Set sensor params(gain, step, integration)
            if (instruction.param_count < 3) {
                setError(ErrorCode::ERR_BCODE_INVALID);
                return false;
            }
            Log.info("BCODE: Set sensor params: gain=%ld, step=%ld, atime=%ld",
                     instruction.params[0], instruction.params[1], instruction.params[2]);
            if (_spectro_config_callback) {
                return _spectro_config_callback(static_cast<uint8_t>(instruction.params[0]),
                                                 static_cast<uint16_t>(instruction.params[1]),
                                                 static_cast<uint8_t>(instruction.params[2]));
            }
            return true;

        case BCODEOpcode::BASELINE_SCANS:
            // Opcode 11: Baseline scans(num_scans)
            if (instruction.param_count < 1) {
                setError(ErrorCode::ERR_BCODE_INVALID);
                return false;
            }
            Log.info("BCODE: Baseline scans: %ld", instruction.params[0]);
            if (_spectro_scan_callback) {
                _spectro_scan_callback(static_cast<uint16_t>(instruction.params[0]), true);
            }
            return true;

        case BCODEOpcode::TEST_SCANS:
            // Opcode 14: Test scans(num_scans)
            if (instruction.param_count < 1) {
                setError(ErrorCode::ERR_BCODE_INVALID);
                return false;
            }
            Log.info("BCODE: Test scans: %ld", instruction.params[0]);
            if (_spectro_scan_callback) {
                _spectro_scan_callback(static_cast<uint16_t>(instruction.params[0]), false);
            }
            return true;

        case BCODEOpcode::SENSOR_READING:
            // Opcode 15: Take sensor readings(channel, gain, step, integration)
            if (instruction.param_count < 4) {
                setError(ErrorCode::ERR_BCODE_INVALID);
                return false;
            }
            {
                char channel = instruction.params[0] == 0 ? '\0' :
                              (instruction.params[0] == 1 ? 'A' :
                              (instruction.params[0] == 2 ? 'B' : 'C'));
                Log.info("BCODE: Sensor reading: chan=%c, gain=%ld, step=%ld, atime=%ld",
                         channel ? channel : '0', instruction.params[1],
                         instruction.params[2], instruction.params[3]);
                if (_spectro_single_callback) {
                    return _spectro_single_callback(channel,
                                                     static_cast<uint8_t>(instruction.params[1]),
                                                     static_cast<uint16_t>(instruction.params[2]),
                                                     static_cast<uint8_t>(instruction.params[3]));
                }
            }
            return true;

        case BCODEOpcode::CONTINUOUS_SCANS:
            // Opcode 16: Continuous scans(baseline, start_pos, distance, step_delay)
            if (instruction.param_count < 4) {
                setError(ErrorCode::ERR_BCODE_INVALID);
                return false;
            }
            Log.info("BCODE: Continuous scans: baseline=%ld, pos=%ld, dist=%ld, delay=%ld",
                     instruction.params[0], instruction.params[1],
                     instruction.params[2], instruction.params[3]);
            if (_spectro_continuous_callback) {
                _spectro_continuous_callback(instruction.params[0] == 1,
                                              instruction.params[1],
                                              instruction.params[2],
                                              static_cast<uint16_t>(instruction.params[3]));
            }
            return true;

        case BCODEOpcode::REPEAT_BEGIN:
            // Opcode 20: Repeat begin(iterations)
            if (instruction.param_count < 1) {
                setError(ErrorCode::ERR_BCODE_INVALID);
                return false;
            }
            Log.info("BCODE: Repeat begin: %ld iterations", instruction.params[0]);
            if (_repeat_depth >= MAX_REPEAT_DEPTH) {
                Log.error("BCODE: Max repeat depth exceeded");
                setError(ErrorCode::ERR_BCODE_INVALID);
                return false;
            }
            _repeat_stack[_repeat_depth].start_index = _instruction_pointer;
            _repeat_stack[_repeat_depth].iterations_remaining = instruction.params[0];
            _repeat_stack[_repeat_depth].active = true;
            _repeat_depth++;
            return true;

        case BCODEOpcode::REPEAT_END:
            // Opcode 21: Repeat end
            Log.trace("BCODE: Repeat end");
            if (_repeat_depth == 0 || !_repeat_stack[_repeat_depth - 1].active) {
                Log.error("BCODE: Repeat end without matching begin");
                setError(ErrorCode::ERR_BCODE_INVALID);
                return false;
            }
            _repeat_stack[_repeat_depth - 1].iterations_remaining--;
            if (_repeat_stack[_repeat_depth - 1].iterations_remaining > 0) {
                // Jump back to start of repeat block
                _instruction_pointer = _repeat_stack[_repeat_depth - 1].start_index;
                Log.trace("BCODE: Repeat loop, %u remaining",
                         _repeat_stack[_repeat_depth - 1].iterations_remaining);
            } else {
                // Exit repeat block
                _repeat_stack[_repeat_depth - 1].active = false;
                _repeat_depth--;
                Log.trace("BCODE: Repeat complete");
            }
            return true;

        case BCODEOpcode::END_TEST:
            // Opcode 99: End test
            Log.info("BCODE: End test");
            _state = InterpreterState::COMPLETED;
            return true;

        default:
            Log.warn("BCODE: Unknown opcode %d", static_cast<int>(instruction.opcode));
            // Skip unknown opcodes rather than failing
            return true;
    }
}

bool BCODEInterpreter::executeDelay(int32_t milliseconds) {
    if (milliseconds <= 0) return true;

    uint32_t start_time = millis();
    int32_t remaining = milliseconds;

    // Execute delay in chunks to allow heater control
    while (remaining > 0 && _state == InterpreterState::RUNNING) {
        // Call delay callback for heater control and cartridge detection
        if (_delay_callback) {
            if (!_delay_callback()) {
                // Callback returned false - cancel execution
                _state = InterpreterState::CANCELLED;
                _last_error = ErrorCode::ERR_TEST_CANCELLED;
                return false;
            }
        }

        // Delay for one unit or remaining time
        uint32_t delay_time = (remaining > BCODE_DELAY_UNIT_MS) ?
                               BCODE_DELAY_UNIT_MS : remaining;
        delay(delay_time);
        remaining -= delay_time;

        // Check if cancelled during delay
        if (_state == InterpreterState::CANCELLED) {
            return false;
        }
    }

    return true;
}

void BCODEInterpreter::setError(ErrorCode error) {
    _last_error = error;
    _state = InterpreterState::ERROR;
    Log.error("BCODEInterpreter: Error %d - %s",
              static_cast<int>(error), errorCodeToString(error));
}

//==============================================================================
// UTILITY FUNCTIONS
//==============================================================================

const char* bcodeOpcodeToString(BCODEOpcode opcode) {
    switch (opcode) {
        case BCODEOpcode::START_TEST:       return "START_TEST";
        case BCODEOpcode::DELAY:            return "DELAY";
        case BCODEOpcode::END_TEST:         return "END_TEST";
        case BCODEOpcode::MOVE_MICRONS:     return "MOVE_MICRONS";
        case BCODEOpcode::OSCILLATE:        return "OSCILLATE";
        case BCODEOpcode::SET_SENSOR_PARAMS: return "SET_SENSOR_PARAMS";
        case BCODEOpcode::BASELINE_SCANS:   return "BASELINE_SCANS";
        case BCODEOpcode::TEST_SCANS:       return "TEST_SCANS";
        case BCODEOpcode::SENSOR_READING:   return "SENSOR_READING";
        case BCODEOpcode::CONTINUOUS_SCANS: return "CONTINUOUS_SCANS";
        case BCODEOpcode::REPEAT_BEGIN:     return "REPEAT_BEGIN";
        case BCODEOpcode::REPEAT_END:       return "REPEAT_END";
        case BCODEOpcode::INVALID:          return "INVALID";
        default:                            return "UNKNOWN";
    }
}

const char* interpreterStateToString(InterpreterState state) {
    switch (state) {
        case InterpreterState::IDLE:        return "IDLE";
        case InterpreterState::READY:       return "READY";
        case InterpreterState::RUNNING:     return "RUNNING";
        case InterpreterState::PAUSED:      return "PAUSED";
        case InterpreterState::COMPLETED:   return "COMPLETED";
        case InterpreterState::ERROR:       return "ERROR";
        case InterpreterState::CANCELLED:   return "CANCELLED";
        default:                            return "UNKNOWN";
    }
}
