/**
 * @file Spectrophotometer.h
 * @brief High-level spectrophotometer controller for Brevitest device
 * @author Agent ZETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This module provides high-level control of the spectrophotometer subsystem,
 * including the AS7341 11-channel spectral sensor and PCA9536 channel multiplexer.
 *
 * Features:
 * - Multi-channel spectrophotometer control (A, B, C)
 * - Single and continuous reading modes
 * - Baseline calibration with dark current compensation
 * - Non-blocking operation with callback support
 * - Integration with HAL for I2C communication
 *
 * User Stories Implemented:
 *   - SPEC-002: Spectrophotometer class
 *   - SPEC-003: PCA9536 channel multiplexing
 *   - SPEC-004: Single reading capture
 *   - SPEC-005: Continuous reading mode
 *   - SPEC-006: Calibration functions
 *
 * Hardware:
 * - AS7341 spectral sensor (I2C 0x39)
 * - PCA9536 I/O expander for channel selection (I2C 0x41)
 * - Three optical channels: A, B, C
 */

#ifndef SPECTROPHOTOMETER_H
#define SPECTROPHOTOMETER_H

#include "Particle.h"
#include "drivers/AS7341.h"
#include "DataTypes.h"
#include "HardwareConfig.h"

//==============================================================================
// CONSTANTS
//==============================================================================

/** @brief Maximum number of readings that can be stored */
constexpr uint16_t SPECTRO_BUFFER_SIZE = SPECTRO_MAX_READINGS;

/** @brief Number of channels on PCA9536 mux */
constexpr uint8_t SPECTRO_NUM_CHANNELS = 3;

/** @brief Valid channel identifiers */
constexpr char SPECTRO_CHANNEL_A = 'A';
constexpr char SPECTRO_CHANNEL_B = 'B';
constexpr char SPECTRO_CHANNEL_C = 'C';

//==============================================================================
// ERROR CODES
//==============================================================================

/**
 * @brief Spectrophotometer-specific error codes
 */
enum class SpectroError : int8_t {
    SUCCESS = 0,              ///< Operation successful
    ERR_NOT_INITIALIZED = -1, ///< Module not initialized
    ERR_I2C_FAILURE = -2,     ///< I2C communication failure
    ERR_SENSOR_FAULT = -3,    ///< AS7341 sensor fault
    ERR_MUX_FAULT = -4,       ///< PCA9536 mux fault
    ERR_INVALID_CHANNEL = -5, ///< Invalid channel specified
    ERR_TIMEOUT = -6,         ///< Operation timed out
    ERR_BUFFER_FULL = -7,     ///< Reading buffer is full
    ERR_NO_BASELINE = -8      ///< No baseline captured
};

//==============================================================================
// CALLBACK TYPES
//==============================================================================

/**
 * @brief Callback function type for continuous reading mode
 * @param reading Pointer to the captured reading
 * @param index Reading index in the sequence
 */
typedef void (*SpectroReadingCallback)(const BrevitestSpectrophotometerReading* reading, uint16_t index);

//==============================================================================
// BASELINE DATA STRUCTURE
//==============================================================================

/**
 * @brief Baseline calibration data for a single channel
 */
struct SpectroBaselineData {
    bool valid;           ///< true if baseline has been captured
    uint16_t f1;          ///< F1 baseline value
    uint16_t f2;          ///< F2 baseline value
    uint16_t f3;          ///< F3 baseline value
    uint16_t f4;          ///< F4 baseline value
    uint16_t f5;          ///< F5 baseline value
    uint16_t f6;          ///< F6 baseline value
    uint16_t f7;          ///< F7 baseline value
    uint16_t f8;          ///< F8 baseline value
    uint16_t clear;       ///< Clear channel baseline
    uint16_t nir;         ///< NIR channel baseline
    uint32_t timestamp;   ///< When baseline was captured

    SpectroBaselineData() : valid(false), f1(0), f2(0), f3(0), f4(0),
                            f5(0), f6(0), f7(0), f8(0), clear(0), nir(0),
                            timestamp(0) {}
};

//==============================================================================
// SPECTROPHOTOMETER CLASS
//==============================================================================

/**
 * @class Spectrophotometer
 * @brief High-level controller for spectrophotometer subsystem
 *
 * This class provides a complete interface for controlling the spectrophotometer
 * hardware, including:
 * - Initialization and configuration
 * - Channel multiplexer control
 * - Single shot and continuous reading modes
 * - Baseline calibration
 *
 * Usage:
 * @code
 *   Spectrophotometer spectro;
 *
 *   // Initialize
 *   if (spectro.init()) {
 *       // Configure
 *       spectro.setIntegrationTime(49, 999);  // ATIME=49, ASTEP=999
 *       spectro.setGain(7);                    // 64x gain
 *
 *       // Capture baseline
 *       spectro.captureBaseline('A');
 *
 *       // Take reading
 *       BrevitestSpectrophotometerReading reading;
 *       if (spectro.readChannel('A', &reading)) {
 *           // Process reading...
 *       }
 *   }
 * @endcode
 */
class Spectrophotometer {
public:
    /**
     * @brief Default constructor
     */
    Spectrophotometer();

    /**
     * @brief Destructor
     */
    ~Spectrophotometer();

    //==========================================================================
    // INITIALIZATION (SPEC-002)
    //==========================================================================

    /**
     * @brief Initialize the spectrophotometer subsystem
     * @return true if initialization successful
     *
     * Initializes:
     * - AS7341 spectral sensor with default configuration
     * - PCA9536 channel multiplexer
     * - Internal state and buffers
     */
    bool init();

    /**
     * @brief Check if spectrophotometer is initialized and ready
     * @return true if ready for operation
     */
    bool isReady() const;

    /**
     * @brief Check if AS7341 sensor is responding
     * @return true if sensor is connected
     */
    bool isSensorConnected();

    /**
     * @brief Get the last error that occurred
     * @return SpectroError code
     */
    SpectroError getLastError() const { return lastError_; }

    /**
     * @brief Get error description string
     * @param error Error code
     * @return Human-readable error description
     */
    static const char* errorToString(SpectroError error);

    //==========================================================================
    // CONFIGURATION (SPEC-002)
    //==========================================================================

    /**
     * @brief Set integration time parameters
     * @param atime ATIME value (0-255)
     * @param astep ASTEP value (0-65535)
     * @return true if configuration successful
     *
     * Integration time = (ATIME + 1) * (ASTEP + 1) * 2.78us
     * Default: ATIME=49, ASTEP=999 gives ~139ms integration time
     */
    bool setIntegrationTime(uint8_t atime, uint16_t astep);

    /**
     * @brief Set gain value
     * @param again Gain setting (0-10)
     * @return true if configuration successful
     *
     * Gain values: 0=0.5x, 1=1x, 2=2x, 3=4x, 4=8x, 5=16x,
     *              6=32x, 7=64x, 8=128x, 9=256x, 10=512x
     */
    bool setGain(uint8_t again);

    /**
     * @brief Get current ATIME setting
     * @return ATIME value
     */
    uint8_t getAtime() const { return atime_; }

    /**
     * @brief Get current ASTEP setting
     * @return ASTEP value
     */
    uint16_t getAstep() const { return astep_; }

    /**
     * @brief Get current gain setting
     * @return AGAIN value
     */
    uint8_t getGain() const { return again_; }

    //==========================================================================
    // CHANNEL MULTIPLEXING (SPEC-003)
    //==========================================================================

    /**
     * @brief Select spectrophotometer channel via PCA9536 mux
     * @param channel Channel to select ('A', 'B', 'C', or 0 for all off)
     * @return true if selection successful
     */
    bool selectChannel(char channel);

    /**
     * @brief Get currently selected channel
     * @return Current channel ('A', 'B', 'C') or 0 if none
     */
    char getSelectedChannel() const { return selectedChannel_; }

    /**
     * @brief Turn off all channels
     * @return true if successful
     */
    bool allChannelsOff();

    /**
     * @brief Verify channel selection by read-back
     * @param channel Expected channel
     * @return true if readback matches expected
     */
    bool verifyChannelSelection(char channel);

    //==========================================================================
    // SINGLE READING CAPTURE (SPEC-004)
    //==========================================================================

    /**
     * @brief Read all channels from currently selected spectrophotometer
     * @param reading Pointer to structure to populate
     * @param position Current stage position (microsteps)
     * @param temperature Current temperature (10x Celsius)
     * @param laserOutput Current laser power setting
     * @return true if reading successful
     */
    bool readAllChannels(BrevitestSpectrophotometerReading* reading,
                         uint16_t position = 0,
                         uint16_t temperature = 0,
                         uint16_t laserOutput = 0);

    /**
     * @brief Read a specific channel (selects channel, reads, returns)
     * @param channel Channel to read ('A', 'B', 'C')
     * @param reading Pointer to structure to populate
     * @param position Current stage position
     * @param temperature Current temperature
     * @param laserOutput Current laser power
     * @return true if reading successful
     */
    bool readChannel(char channel,
                     BrevitestSpectrophotometerReading* reading,
                     uint16_t position = 0,
                     uint16_t temperature = 0,
                     uint16_t laserOutput = 0);

    /**
     * @brief Set the test start timestamp for relative timing
     * @param startTime Timestamp in milliseconds
     */
    void setTestStartTime(uint32_t startTime);

    /**
     * @brief Get current reading number
     * @return Number of readings taken since reset
     */
    uint8_t getReadingNumber() const { return readingNumber_; }

    /**
     * @brief Reset reading number to zero
     */
    void resetReadingNumber() { readingNumber_ = 0; }

    //==========================================================================
    // CONTINUOUS READING MODE (SPEC-005)
    //==========================================================================

    /**
     * @brief Start continuous reading mode
     * @param channel Channel to read continuously
     * @param callback Function to call for each reading (optional)
     * @return true if started successfully
     */
    bool startContinuousMode(char channel, SpectroReadingCallback callback = nullptr);

    /**
     * @brief Stop continuous reading mode
     */
    void stopContinuousMode();

    /**
     * @brief Check if continuous mode is active
     * @return true if in continuous mode
     */
    bool isContinuousModeActive() const { return continuousModeActive_; }

    /**
     * @brief Process continuous mode (call from main loop)
     * @return true if a reading was captured
     *
     * Must be called regularly when continuous mode is active.
     * Returns true when a new reading is available.
     */
    bool processContinuousMode();

    /**
     * @brief Get reading buffer
     * @return Pointer to reading array
     */
    BrevitestSpectrophotometerReading* getReadingBuffer() { return readingBuffer_; }

    /**
     * @brief Get number of readings in buffer
     * @return Number of readings captured
     */
    uint16_t getReadingCount() const { return readingCount_; }

    /**
     * @brief Clear reading buffer
     */
    void clearReadingBuffer();

    /**
     * @brief Check if reading buffer is full
     * @return true if buffer has reached maximum capacity
     */
    bool isBufferFull() const { return readingCount_ >= SPECTRO_BUFFER_SIZE; }

    //==========================================================================
    // CALIBRATION FUNCTIONS (SPEC-006)
    //==========================================================================

    /**
     * @brief Capture baseline reading for a channel
     * @param channel Channel to calibrate ('A', 'B', 'C')
     * @param numSamples Number of samples to average (default: 5)
     * @return true if baseline captured successfully
     *
     * Should be called with laser off or in dark conditions
     * to capture dark current levels.
     */
    bool captureBaseline(char channel, uint8_t numSamples = 5);

    /**
     * @brief Get baseline reading for a channel
     * @param channel Channel identifier
     * @param baseline Pointer to structure to populate
     * @return true if baseline is available
     */
    bool getBaseline(char channel, SpectroBaselineData* baseline);

    /**
     * @brief Get baseline value for a specific spectral channel
     * @param opticalChannel Optical channel ('A', 'B', 'C')
     * @param spectralChannel Spectral channel (AS7341::Channel)
     * @return Baseline value, or 0 if not available
     */
    uint16_t getBaselineValue(char opticalChannel, AS7341::Channel spectralChannel);

    /**
     * @brief Clear baseline data for a channel
     * @param channel Channel to clear ('A', 'B', 'C', or 0 for all)
     */
    void clearBaseline(char channel = 0);

    /**
     * @brief Check if baseline is valid for a channel
     * @param channel Channel to check
     * @return true if valid baseline exists
     */
    bool hasValidBaseline(char channel) const;

    /**
     * @brief Enable/disable baseline subtraction
     * @param enable true to enable baseline subtraction
     */
    void setBaselineSubtraction(bool enable) { subtractBaseline_ = enable; }

    /**
     * @brief Check if baseline subtraction is enabled
     * @return true if baseline subtraction is active
     */
    bool isBaselineSubtractionEnabled() const { return subtractBaseline_; }

    //==========================================================================
    // LED CONTROL
    //==========================================================================

    /**
     * @brief Enable/disable AS7341 onboard LED
     * @param on true to enable LED
     */
    void enableLed(bool on);

    /**
     * @brief Set AS7341 LED current
     * @param current Current level (1-20 = 4mA-42mA)
     */
    void setLedCurrent(uint8_t current);

    //==========================================================================
    // DIAGNOSTICS
    //==========================================================================

    /**
     * @brief Run self-test diagnostics
     * @return true if all tests pass
     */
    bool selfTest();

    /**
     * @brief Print status to log
     */
    void printStatus();

private:
    //==========================================================================
    // PRIVATE HELPER FUNCTIONS
    //==========================================================================

    /**
     * @brief Initialize the PCA9536 channel multiplexer
     * @return true if successful
     */
    bool initMux();

    /**
     * @brief Convert channel character to mux value
     * @param channel Channel character ('A', 'B', 'C')
     * @return Mux register value, or 0 for invalid channel
     */
    uint8_t channelToMuxValue(char channel);

    /**
     * @brief Convert channel index to character
     * @param index Channel index (0, 1, 2)
     * @return Channel character ('A', 'B', 'C')
     */
    static char indexToChannel(uint8_t index);

    /**
     * @brief Convert channel character to index
     * @param channel Channel character
     * @return Channel index (0, 1, 2) or -1 for invalid
     */
    static int8_t channelToIndex(char channel);

    /**
     * @brief Apply baseline subtraction to a reading
     * @param reading Reading to modify
     * @param channel Channel for baseline lookup
     */
    void applyBaselineSubtraction(BrevitestSpectrophotometerReading* reading, char channel);

    /**
     * @brief Store reading in buffer
     * @param reading Reading to store
     * @return true if stored successfully
     */
    bool storeReading(const BrevitestSpectrophotometerReading* reading);

    //==========================================================================
    // MEMBER VARIABLES
    //==========================================================================

    // AS7341 driver instance
    AS7341::Driver sensor_;

    // State flags
    bool initialized_;
    bool sensorReady_;
    bool muxReady_;
    SpectroError lastError_;

    // Configuration
    uint8_t atime_;
    uint16_t astep_;
    uint8_t again_;

    // Channel state
    char selectedChannel_;

    // Reading tracking
    uint8_t readingNumber_;
    uint32_t testStartTime_;

    // Continuous mode
    bool continuousModeActive_;
    char continuousChannel_;
    SpectroReadingCallback readingCallback_;
    uint32_t lastReadingTime_;

    // Reading buffer
    BrevitestSpectrophotometerReading readingBuffer_[SPECTRO_BUFFER_SIZE];
    uint16_t readingCount_;

    // Baseline data (one per channel: A, B, C)
    SpectroBaselineData baseline_[SPECTRO_NUM_CHANNELS];
    bool subtractBaseline_;
};

#endif // SPECTROPHOTOMETER_H
