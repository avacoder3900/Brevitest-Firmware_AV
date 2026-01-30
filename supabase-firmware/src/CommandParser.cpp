/**
 * @file CommandParser.cpp
 * @brief Implementation of serial command parser for Brevitest firmware
 * @author Agent GAMMA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * User Stories Implemented:
 *   - SER-001: CommandParser class implementation
 */

#include "CommandParser.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

//==============================================================================
// GLOBAL INSTANCE
//==============================================================================

CommandParser serialParser;

CommandParser& getCommandParser() {
    return serialParser;
}

//==============================================================================
// PARSED COMMAND IMPLEMENTATION
//==============================================================================

ParsedCommand::ParsedCommand()
    : commandId(-1)
    , paramCount(0)
    , valid(false)
{
    for (int i = 0; i < MAX_COMMAND_PARAMS; i++) {
        params[i] = 0;
    }
}

void ParsedCommand::clear() {
    commandId = -1;
    paramCount = 0;
    valid = false;
    for (int i = 0; i < MAX_COMMAND_PARAMS; i++) {
        params[i] = 0;
    }
}

int32_t ParsedCommand::getParam(uint8_t index, int32_t defaultValue) const {
    if (index < paramCount) {
        return params[index];
    }
    return defaultValue;
}

bool ParsedCommand::hasParam(uint8_t index) const {
    return index < paramCount;
}

//==============================================================================
// COMMAND PARSER CONSTRUCTOR
//==============================================================================

CommandParser::CommandParser()
    : _initialized(false)
    , _baudRate(SERIAL_DEFAULT_BAUD)
    , _echoEnabled(false)
    , _loggingEnabled(true)
    , _bufferIndex(0)
    , _handler(nullptr)
    , _commandCount(0)
    , _errorCount(0)
    , _overflowCount(0)
{
    _buffer[0] = '\0';
}

//==============================================================================
// INITIALIZATION
//==============================================================================

bool CommandParser::init(uint32_t baudRate) {
    if (_initialized && baudRate == _baudRate) {
        return true; // Already initialized with same baud rate
    }

    _baudRate = baudRate;

    // Initialize serial port
    Serial.begin(_baudRate);

    // Clear buffer
    clearBuffer();

    // Reset statistics
    resetStats();

    _initialized = true;

    if (_loggingEnabled) {
        Log.info("CommandParser initialized at %lu baud", (unsigned long)_baudRate);
    }

    return true;
}

bool CommandParser::isInitialized() const {
    return _initialized;
}

//==============================================================================
// CONFIGURATION
//==============================================================================

void CommandParser::setCommandHandler(CommandHandler handler) {
    _handler = handler;
}

void CommandParser::setEcho(bool enable) {
    _echoEnabled = enable;
}

bool CommandParser::isEchoEnabled() const {
    return _echoEnabled;
}

void CommandParser::setLogging(bool enable) {
    _loggingEnabled = enable;
}

bool CommandParser::isLoggingEnabled() const {
    return _loggingEnabled;
}

//==============================================================================
// MAIN PROCESSING
//==============================================================================

bool CommandParser::process() {
    if (!_initialized) {
        return false;
    }

    bool commandProcessed = false;

    // Process all available characters
    while (Serial.available() > 0) {
        char c = Serial.read();

        // Echo if enabled
        if (_echoEnabled) {
            Serial.write(c);
        }

        // Handle carriage return (ignore, just wait for newline)
        if (c == CARRIAGE_RETURN) {
            continue;
        }

        // Handle newline - command is complete
        if (c == COMMAND_TERMINATOR) {
            // Null-terminate the buffer
            _buffer[_bufferIndex] = '\0';

            // Process the complete command
            if (_bufferIndex > 0) {
                processLine(_buffer);
                commandProcessed = true;
            }

            // Reset buffer for next command
            clearBuffer();
            continue;
        }

        // Handle backspace
        if (c == '\b' || c == 127) { // Backspace or DEL
            if (_bufferIndex > 0) {
                _bufferIndex--;
                if (_echoEnabled) {
                    Serial.print("\b \b"); // Erase character on terminal
                }
            }
            continue;
        }

        // Add character to buffer
        if (_bufferIndex < CMD_PARSER_BUFFER_SIZE - 1) {
            _buffer[_bufferIndex++] = c;
        } else {
            // Buffer overflow
            _overflowCount++;
            if (_loggingEnabled) {
                Log.warn("CommandParser: buffer overflow, clearing");
            }
            clearBuffer();
        }
    }

    return commandProcessed;
}

bool CommandParser::hasPendingInput() const {
    return _bufferIndex > 0;
}

uint16_t CommandParser::getBufferUsage() const {
    return _bufferIndex;
}

void CommandParser::clearBuffer() {
    _bufferIndex = 0;
    _buffer[0] = '\0';
}

//==============================================================================
// LINE PROCESSING
//==============================================================================

void CommandParser::processLine(const char* line) {
    // Log the received command
    if (_loggingEnabled) {
        Log.info("CMD: %s", line);
    }

    // Parse the command
    ParsedCommand cmd;
    if (!parseString(line, cmd)) {
        _errorCount++;
        respondError(-1, "Parse error");
        return;
    }

    _commandCount++;

    // Dispatch to handler
    if (_handler != nullptr) {
        int result = _handler(cmd);
        if (result < 0 && _loggingEnabled) {
            Log.warn("Command %d returned error: %d", cmd.commandId, result);
        }
    } else {
        if (_loggingEnabled) {
            Log.warn("No command handler registered");
        }
        respondError(-2, "No handler");
    }
}

//==============================================================================
// PARSING UTILITIES
//==============================================================================

bool CommandParser::parseString(const char* input, ParsedCommand& cmd) {
    cmd.clear();

    if (input == nullptr || input[0] == '\0') {
        return false;
    }

    int index = 0;

    // Skip leading whitespace
    index = skipWhitespace(input, index);

    if (input[index] == '\0') {
        return false; // Empty command
    }

    // Parse command ID
    int32_t cmdId;
    index = parseNextParam(input, index, &cmdId, -1);

    if (cmdId < 0 || cmdId > 9999) {
        return false; // Invalid command ID
    }

    cmd.commandId = (int16_t)cmdId;
    cmd.valid = true;

    // Parse optional parameters
    while (index >= 0 && cmd.paramCount < MAX_COMMAND_PARAMS) {
        index = skipWhitespace(input, index);

        if (input[index] == '\0') {
            break; // End of string
        }

        int32_t param;
        int newIndex = parseNextParam(input, index, &param, 0);

        if (newIndex == index) {
            break; // No more parameters
        }

        cmd.params[cmd.paramCount++] = param;
        index = newIndex;
    }

    return true;
}

int CommandParser::parseNextParam(const char* str, int startIndex, int32_t* outValue, int32_t defaultValue) {
    if (str == nullptr || outValue == nullptr || startIndex < 0) {
        if (outValue) *outValue = defaultValue;
        return -1;
    }

    // Skip whitespace
    int index = skipWhitespace(str, startIndex);

    if (str[index] == '\0') {
        *outValue = defaultValue;
        return -1; // End of string
    }

    // Check for numeric start
    if (!isDigitOrSign(str[index])) {
        *outValue = defaultValue;
        return index; // Not a number, return same position
    }

    // Parse the number
    bool negative = false;
    if (str[index] == '-') {
        negative = true;
        index++;
    } else if (str[index] == '+') {
        index++;
    }

    // Must have at least one digit after sign
    if (str[index] < '0' || str[index] > '9') {
        *outValue = defaultValue;
        return startIndex;
    }

    int32_t value = 0;
    while (str[index] >= '0' && str[index] <= '9') {
        value = value * 10 + (str[index] - '0');
        index++;
    }

    *outValue = negative ? -value : value;
    return index;
}

int CommandParser::skipWhitespace(const char* str, int index) {
    if (str == nullptr || index < 0) {
        return -1;
    }

    while (str[index] == ' ' || str[index] == '\t') {
        index++;
    }

    return index;
}

bool CommandParser::isDigitOrSign(char c) {
    return (c >= '0' && c <= '9') || c == '-' || c == '+';
}

//==============================================================================
// OUTPUT UTILITIES
//==============================================================================

void CommandParser::respond(const char* format, ...) {
    if (format == nullptr) {
        return;
    }

    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    Serial.println(buffer);
}

void CommandParser::respondOK(int result) {
    Serial.printlnf("OK:%d", result);
}

void CommandParser::respondError(int errorCode, const char* message) {
    if (message != nullptr) {
        Serial.printlnf("ERR:%d:%s", errorCode, message);
    } else {
        Serial.printlnf("ERR:%d", errorCode);
    }
}

void CommandParser::printHelp(const char* message) {
    if (message != nullptr) {
        Serial.println(message);
    }
}

//==============================================================================
// STATISTICS
//==============================================================================

uint32_t CommandParser::getCommandCount() const {
    return _commandCount;
}

uint32_t CommandParser::getErrorCount() const {
    return _errorCount;
}

uint32_t CommandParser::getOverflowCount() const {
    return _overflowCount;
}

void CommandParser::resetStats() {
    _commandCount = 0;
    _errorCount = 0;
    _overflowCount = 0;
}
