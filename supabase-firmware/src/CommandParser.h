/**
 * @file CommandParser.h
 * @brief Serial command parser for Brevitest firmware
 * @author Agent GAMMA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module provides serial command parsing for debugging, testing, and device control.
 * Commands are numeric IDs followed by optional space-separated parameters.
 *
 * Command Format:
 *   <command_id> [param1] [param2] [param3] [param4] [param5]\n
 *
 * Examples:
 *   "1\n"           - Command 1, no parameters
 *   "22 500 100\n"  - Command 22 with params 500 and 100
 *   "301 1 2 3\n"   - Command 301 with params 1, 2, and 3
 *
 * User Stories Implemented:
 *   - SER-001: CommandParser class
 *
 * Features:
 * - Configurable baud rate (default 115200)
 * - Parses numeric command IDs (0-9999)
 * - Supports up to 5 optional integer parameters
 * - Newline-terminated commands
 * - 128-byte command buffer with overflow protection
 * - Non-blocking operation
 */

#ifndef COMMANDPARSER_H
#define COMMANDPARSER_H

#include "Particle.h"
#include <stdint.h>

//==============================================================================
// CONFIGURATION CONSTANTS
//==============================================================================

/** @brief Default serial baud rate */
constexpr uint32_t SERIAL_DEFAULT_BAUD = 115200;

/** @brief Maximum command buffer size (bytes) */
constexpr uint16_t CMD_PARSER_BUFFER_SIZE = 128;

/** @brief Maximum number of parameters per command */
constexpr uint8_t MAX_COMMAND_PARAMS = 5;

/** @brief Command terminator character */
constexpr char COMMAND_TERMINATOR = '\n';

/** @brief Carriage return (stripped) */
constexpr char CARRIAGE_RETURN = '\r';

/** @brief Parameter separator character */
constexpr char PARAM_SEPARATOR = ' ';

//==============================================================================
// PARSED COMMAND STRUCTURE
//==============================================================================

/**
 * @brief Structure containing a parsed command
 * @details Holds the command ID and up to 5 integer parameters
 */
struct ParsedCommand {
    int16_t commandId;                      ///< Command ID (0-9999), -1 if invalid
    int32_t params[MAX_COMMAND_PARAMS];     ///< Parameter values
    uint8_t paramCount;                     ///< Number of valid parameters
    bool valid;                             ///< true if command parsed successfully

    /**
     * @brief Default constructor - initializes to invalid state
     */
    ParsedCommand();

    /**
     * @brief Clear/reset the parsed command
     */
    void clear();

    /**
     * @brief Get parameter with default value fallback
     * @param index Parameter index (0-4)
     * @param defaultValue Value to return if parameter not present
     * @return Parameter value or default
     */
    int32_t getParam(uint8_t index, int32_t defaultValue = 0) const;

    /**
     * @brief Check if a specific parameter was provided
     * @param index Parameter index (0-4)
     * @return true if parameter exists
     */
    bool hasParam(uint8_t index) const;
};

//==============================================================================
// COMMAND CALLBACK TYPE
//==============================================================================

/**
 * @brief Callback function type for command execution
 * @param cmd The parsed command to execute
 * @return Result code (0 = success, negative = error)
 */
typedef int (*CommandHandler)(const ParsedCommand& cmd);

//==============================================================================
// COMMAND PARSER CLASS
//==============================================================================

/**
 * @brief Serial command parser and dispatcher
 * @details Handles serial input buffering, command parsing, and dispatch.
 *          Call process() in the main loop to handle incoming serial data.
 *
 * Usage:
 * @code
 *   CommandParser parser;
 *   parser.init(115200);
 *   parser.setCommandHandler(myCommandHandler);
 *
 *   void loop() {
 *       parser.process();  // Call every loop iteration
 *   }
 *
 *   int myCommandHandler(const ParsedCommand& cmd) {
 *       switch(cmd.commandId) {
 *           case 1: // Handle command 1
 *               break;
 *       }
 *       return 0;
 *   }
 * @endcode
 */
class CommandParser {
public:
    //==========================================================================
    // LIFECYCLE
    //==========================================================================

    /**
     * @brief Constructor
     */
    CommandParser();

    /**
     * @brief Initialize the serial interface
     * @param baudRate Serial baud rate (default 115200)
     * @return true on success
     */
    bool init(uint32_t baudRate = SERIAL_DEFAULT_BAUD);

    /**
     * @brief Check if parser is initialized
     * @return true if init() has been called successfully
     */
    bool isInitialized() const;

    //==========================================================================
    // CONFIGURATION
    //==========================================================================

    /**
     * @brief Set the command handler callback
     * @param handler Function to call when a complete command is received
     */
    void setCommandHandler(CommandHandler handler);

    /**
     * @brief Enable or disable echo of received characters
     * @param enable true to echo characters back to serial
     */
    void setEcho(bool enable);

    /**
     * @brief Get echo setting
     * @return true if echo is enabled
     */
    bool isEchoEnabled() const;

    /**
     * @brief Enable or disable command logging
     * @param enable true to log received commands
     */
    void setLogging(bool enable);

    /**
     * @brief Get logging setting
     * @return true if logging is enabled
     */
    bool isLoggingEnabled() const;

    //==========================================================================
    // MAIN PROCESSING
    //==========================================================================

    /**
     * @brief Process any available serial input
     * @details Call this function in the main loop. It reads available
     *          serial data and processes complete commands.
     * @return true if a command was processed this call
     */
    bool process();

    /**
     * @brief Check if there is a pending command in the buffer
     * @return true if partial command is being received
     */
    bool hasPendingInput() const;

    /**
     * @brief Get current buffer usage
     * @return Number of characters in buffer
     */
    uint16_t getBufferUsage() const;

    /**
     * @brief Clear the input buffer
     */
    void clearBuffer();

    //==========================================================================
    // PARSING UTILITIES (can be used independently)
    //==========================================================================

    /**
     * @brief Parse a command string into a ParsedCommand structure
     * @param input Command string (null-terminated)
     * @param cmd Output parsed command structure
     * @return true if parsing succeeded
     */
    static bool parseString(const char* input, ParsedCommand& cmd);

    /**
     * @brief Parse next integer parameter from string
     * @param str Input string
     * @param startIndex Starting position in string
     * @param outValue Pointer to store parsed value
     * @param defaultValue Value to use if parsing fails
     * @return Index after parsed value, or -1 if end of string
     */
    static int parseNextParam(const char* str, int startIndex, int32_t* outValue, int32_t defaultValue);

    //==========================================================================
    // OUTPUT UTILITIES
    //==========================================================================

    /**
     * @brief Print a response to serial
     * @param format Printf-style format string
     * @param ... Format arguments
     */
    void respond(const char* format, ...);

    /**
     * @brief Print an OK response with optional result
     * @param result Optional result value
     */
    void respondOK(int result = 0);

    /**
     * @brief Print an error response
     * @param errorCode Error code
     * @param message Error message (optional)
     */
    void respondError(int errorCode, const char* message = nullptr);

    /**
     * @brief Print a help message
     * @param message Help text to display
     */
    void printHelp(const char* message);

    //==========================================================================
    // STATISTICS
    //==========================================================================

    /**
     * @brief Get count of commands processed
     * @return Number of commands successfully parsed
     */
    uint32_t getCommandCount() const;

    /**
     * @brief Get count of parsing errors
     * @return Number of parse failures
     */
    uint32_t getErrorCount() const;

    /**
     * @brief Get count of buffer overflows
     * @return Number of times buffer overflowed
     */
    uint32_t getOverflowCount() const;

    /**
     * @brief Reset all statistics
     */
    void resetStats();

private:
    //==========================================================================
    // PRIVATE MEMBERS
    //==========================================================================

    bool _initialized;                      ///< Initialization flag
    uint32_t _baudRate;                     ///< Configured baud rate
    bool _echoEnabled;                      ///< Echo characters back to serial
    bool _loggingEnabled;                   ///< Log received commands

    char _buffer[CMD_PARSER_BUFFER_SIZE];       ///< Input buffer
    uint16_t _bufferIndex;                  ///< Current write position in buffer

    CommandHandler _handler;                ///< Command handler callback

    // Statistics
    uint32_t _commandCount;                 ///< Commands processed
    uint32_t _errorCount;                   ///< Parse errors
    uint32_t _overflowCount;                ///< Buffer overflows

    //==========================================================================
    // PRIVATE METHODS
    //==========================================================================

    /**
     * @brief Process a complete command line
     * @param line Null-terminated command string
     */
    void processLine(const char* line);

    /**
     * @brief Skip whitespace in string
     * @param str String to scan
     * @param index Starting index
     * @return Index of first non-whitespace character
     */
    static int skipWhitespace(const char* str, int index);

    /**
     * @brief Check if character is a digit
     * @param c Character to check
     * @return true if digit (0-9) or minus sign
     */
    static bool isDigitOrSign(char c);
};

//==============================================================================
// GLOBAL INSTANCE
//==============================================================================

/**
 * @brief Global command parser instance
 * @details Single instance for use throughout firmware
 */
extern CommandParser serialParser;

/**
 * @brief Get reference to the global command parser
 * @return Reference to global CommandParser instance
 */
CommandParser& getCommandParser();

#endif // COMMANDPARSER_H
