/**
 * @file Spectrophotometer.cpp
 * @brief High-level spectrophotometer controller implementation
 * @author Agent ZETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * Implementation of the Spectrophotometer class for controlling the AS7341
 * spectral sensor and PCA9536 channel multiplexer.
 *
 * User Stories Implemented:
 *   - SPEC-002: Spectrophotometer class
 *   - SPEC-003: PCA9536 channel multiplexing
 *   - SPEC-004: Single reading capture
 *   - SPEC-005: Continuous reading mode
 *   - SPEC-006: Calibration functions
 */

#include "Spectrophotometer.h"
#include "HAL.h"
#include "MotorController.h"
#include "LaserController.h"
#include "HeaterController.h"

//==============================================================================
// CONSTRUCTOR / DESTRUCTOR
//==============================================================================

Spectrophotometer::Spectrophotometer()
    : sensor_(&Wire)
    , initialized_(false)
    , sensorReady_(false)
    , muxReady_(false)
    , lastError_(SpectroError::SUCCESS)
    , atime_(SPECTRO_ATIME_DEFAULT)
    , astep_(SPECTRO_ASTEP_DEFAULT)
    , again_(SPECTRO_AGAIN_DEFAULT)
    , selectedChannel_(0)
    , readingNumber_(0)
    , testStartTime_(0)
    , continuousModeActive_(false)
    , continuousChannel_(0)
    , readingCallback_(nullptr)
    , lastReadingTime_(0)
    , readingCount_(0)
    , subtractBaseline_(false)
{
    // Initialize baseline data
    for (uint8_t i = 0; i < SPECTRO_NUM_CHANNELS; i++) {
        baseline_[i] = SpectroBaselineData();
    }
}

Spectrophotometer::~Spectrophotometer() {
    // Ensure continuous mode is stopped
    stopContinuousMode();

    // Turn off all channels
    allChannelsOff();
}

//==============================================================================
// INITIALIZATION (SPEC-002)
//==============================================================================

bool Spectrophotometer::init() {
    Log.info("Spectrophotometer: Initializing...");

    // Initialize AS7341 sensor
    int8_t result = sensor_.begin(AS7341::MeasurementMode::SPM);
    if (result != AS7341::ERR_OK) {
        Log.error("Spectrophotometer: AS7341 initialization failed (error %d)", result);
        lastError_ = SpectroError::ERR_SENSOR_FAULT;
        return false;
    }

    // Verify sensor ID
    uint8_t id = sensor_.readID();
    if (id != AS7341::DEVICE_ID) {
        Log.error("Spectrophotometer: AS7341 ID mismatch (expected 0x%02X, got 0x%02X)",
                  AS7341::DEVICE_ID, id);
        lastError_ = SpectroError::ERR_SENSOR_FAULT;
        return false;
    }

    sensorReady_ = true;
    Log.info("Spectrophotometer: AS7341 detected (ID: 0x%02X)", id);

    // Initialize PCA9536 multiplexer
    if (!initMux()) {
        Log.error("Spectrophotometer: PCA9536 mux initialization failed");
        lastError_ = SpectroError::ERR_MUX_FAULT;
        return false;
    }
    muxReady_ = true;
    Log.info("Spectrophotometer: PCA9536 mux initialized");

    // Configure default settings
    sensor_.setAtime(atime_);
    sensor_.setAstep(astep_);
    sensor_.setGain(again_);

    // Turn off all channels initially
    allChannelsOff();

    // Clear reading buffer
    clearReadingBuffer();

    initialized_ = true;
    lastError_ = SpectroError::SUCCESS;

    Log.info("Spectrophotometer: Initialization complete (ATIME=%d, ASTEP=%d, AGAIN=%d)",
             atime_, astep_, again_);

    return true;
}

bool Spectrophotometer::isReady() const {
    return initialized_ && sensorReady_ && muxReady_;
}

bool Spectrophotometer::isSensorConnected() {
    return sensor_.isConnected();
}

const char* Spectrophotometer::errorToString(SpectroError error) {
    switch (error) {
        case SpectroError::SUCCESS:           return "Success";
        case SpectroError::ERR_NOT_INITIALIZED: return "Not initialized";
        case SpectroError::ERR_I2C_FAILURE:   return "I2C communication failure";
        case SpectroError::ERR_SENSOR_FAULT:  return "AS7341 sensor fault";
        case SpectroError::ERR_MUX_FAULT:     return "PCA9536 mux fault";
        case SpectroError::ERR_INVALID_CHANNEL: return "Invalid channel";
        case SpectroError::ERR_TIMEOUT:       return "Operation timeout";
        case SpectroError::ERR_BUFFER_FULL:   return "Buffer full";
        case SpectroError::ERR_NO_BASELINE:   return "No baseline captured";
        default:                              return "Unknown error";
    }
}

//==============================================================================
// CONFIGURATION (SPEC-002)
//==============================================================================

bool Spectrophotometer::setIntegrationTime(uint8_t atime, uint16_t astep) {
    if (!isReady()) {
        lastError_ = SpectroError::ERR_NOT_INITIALIZED;
        return false;
    }

    sensor_.setAtime(atime);
    sensor_.setAstep(astep);

    atime_ = atime;
    astep_ = astep;

    Log.trace("Spectrophotometer: Integration time set (ATIME=%d, ASTEP=%d)", atime, astep);
    return true;
}

bool Spectrophotometer::setGain(uint8_t again) {
    if (!isReady()) {
        lastError_ = SpectroError::ERR_NOT_INITIALIZED;
        return false;
    }

    if (again > 10) {
        again = 10;  // Clamp to maximum
    }

    sensor_.setGain(again);
    again_ = again;

    Log.trace("Spectrophotometer: Gain set to %d", again);
    return true;
}

//==============================================================================
// CHANNEL MULTIPLEXING (SPEC-003)
//==============================================================================

bool Spectrophotometer::initMux() {
    // Use HAL function to initialize the mux
    return HAL::initSpectroMux();
}

bool Spectrophotometer::selectChannel(char channel) {
    if (!isReady()) {
        lastError_ = SpectroError::ERR_NOT_INITIALIZED;
        return false;
    }

    // Validate channel
    if (channel != SPECTRO_CHANNEL_A &&
        channel != SPECTRO_CHANNEL_B &&
        channel != SPECTRO_CHANNEL_C &&
        channel != 0) {
        lastError_ = SpectroError::ERR_INVALID_CHANNEL;
        return false;
    }

    // Use HAL to select channel
    if (!HAL::selectSpectroChannel(channel)) {
        lastError_ = SpectroError::ERR_MUX_FAULT;
        return false;
    }

    selectedChannel_ = channel;
    lastError_ = SpectroError::SUCCESS;

    Log.trace("Spectrophotometer: Selected channel %c", channel ? channel : '0');
    return true;
}

bool Spectrophotometer::allChannelsOff() {
    return selectChannel(0);
}

bool Spectrophotometer::verifyChannelSelection(char channel) {
    // Read back from PCA9536 output register
    uint8_t readback = 0;
    if (!HAL::i2cReadRegister(I2C_ADDR_SPECTRO_MUX, SPECTRO_MUX_OUTPUT_CMD, &readback)) {
        return false;
    }

    uint8_t expected = channelToMuxValue(channel);
    return (readback == expected);
}

uint8_t Spectrophotometer::channelToMuxValue(char channel) {
    switch (channel) {
        case SPECTRO_CHANNEL_A: return SPECTRO_MUX_CHANNEL_A;
        case SPECTRO_CHANNEL_B: return SPECTRO_MUX_CHANNEL_B;
        case SPECTRO_CHANNEL_C: return SPECTRO_MUX_CHANNEL_C;
        default:                return SPECTRO_MUX_ALL_OFF;
    }
}

char Spectrophotometer::indexToChannel(uint8_t index) {
    switch (index) {
        case 0: return SPECTRO_CHANNEL_A;
        case 1: return SPECTRO_CHANNEL_B;
        case 2: return SPECTRO_CHANNEL_C;
        default: return 0;
    }
}

int8_t Spectrophotometer::channelToIndex(char channel) {
    switch (channel) {
        case SPECTRO_CHANNEL_A: return 0;
        case SPECTRO_CHANNEL_B: return 1;
        case SPECTRO_CHANNEL_C: return 2;
        default: return -1;
    }
}

//==============================================================================
// SINGLE READING CAPTURE (SPEC-004)
//==============================================================================

bool Spectrophotometer::readAllChannels(BrevitestSpectrophotometerReading* reading,
                                         uint16_t position,
                                         uint16_t temperature,
                                         uint16_t laserOutput) {
    if (!isReady()) {
        lastError_ = SpectroError::ERR_NOT_INITIALIZED;
        return false;
    }

    if (reading == nullptr) {
        lastError_ = SpectroError::ERR_SENSOR_FAULT;
        return false;
    }

    // Read all channels from AS7341 (this does two SMUX configurations internally)
    AS7341::FullSpectralData data = sensor_.readAllChannels(SPECTRO_TIMEOUT);

    if (sensor_.getLastError() != AS7341::ERR_OK) {
        lastError_ = SpectroError::ERR_SENSOR_FAULT;
        return false;
    }

    // Populate the reading structure
    reading->number = readingNumber_++;
    reading->channel = selectedChannel_;
    reading->position = position;
    reading->temperature = temperature;
    reading->laser_output = laserOutput;
    reading->msec = millis() - testStartTime_;

    reading->f1 = data.f1;
    reading->f2 = data.f2;
    reading->f3 = data.f3;
    reading->f4 = data.f4;
    reading->f5 = data.f5;
    reading->f6 = data.f6;
    reading->f7 = data.f7;
    reading->f8 = data.f8;
    reading->clear = data.clear;
    reading->nir = data.nir;

    // Apply baseline subtraction if enabled
    if (subtractBaseline_ && selectedChannel_ != 0) {
        applyBaselineSubtraction(reading, selectedChannel_);
    }

    lastError_ = SpectroError::SUCCESS;
    return true;
}

bool Spectrophotometer::readChannel(char channel,
                                     BrevitestSpectrophotometerReading* reading,
                                     uint16_t position,
                                     uint16_t temperature,
                                     uint16_t laserOutput) {
    // Select the channel
    if (!selectChannel(channel)) {
        return false;
    }

    // Small delay for channel to stabilize
    delay(2);

    // Read all channels
    return readAllChannels(reading, position, temperature, laserOutput);
}

void Spectrophotometer::setTestStartTime(uint32_t startTime) {
    testStartTime_ = startTime;
}

//==============================================================================
// CONTINUOUS READING MODE (SPEC-005)
//==============================================================================

bool Spectrophotometer::startContinuousMode(char channel, SpectroReadingCallback callback) {
    if (!isReady()) {
        lastError_ = SpectroError::ERR_NOT_INITIALIZED;
        return false;
    }

    // Validate channel
    if (channel != SPECTRO_CHANNEL_A &&
        channel != SPECTRO_CHANNEL_B &&
        channel != SPECTRO_CHANNEL_C) {
        lastError_ = SpectroError::ERR_INVALID_CHANNEL;
        return false;
    }

    // Select the channel
    if (!selectChannel(channel)) {
        return false;
    }

    continuousModeActive_ = true;
    continuousChannel_ = channel;
    readingCallback_ = callback;
    lastReadingTime_ = millis();

    Log.info("Spectrophotometer: Continuous mode started on channel %c", channel);
    return true;
}

void Spectrophotometer::stopContinuousMode() {
    if (continuousModeActive_) {
        continuousModeActive_ = false;
        continuousChannel_ = 0;
        readingCallback_ = nullptr;
        Log.info("Spectrophotometer: Continuous mode stopped (%d readings captured)", readingCount_);
    }
}

bool Spectrophotometer::processContinuousMode() {
    if (!continuousModeActive_) {
        return false;
    }

    // Check if buffer is full
    if (isBufferFull()) {
        lastError_ = SpectroError::ERR_BUFFER_FULL;
        return false;
    }

    // Take a reading
    BrevitestSpectrophotometerReading reading;
    if (!readAllChannels(&reading)) {
        return false;
    }

    // Store in buffer
    if (!storeReading(&reading)) {
        return false;
    }

    // Call callback if registered
    if (readingCallback_ != nullptr) {
        readingCallback_(&reading, readingCount_ - 1);
    }

    lastReadingTime_ = millis();
    return true;
}

bool Spectrophotometer::storeReading(const BrevitestSpectrophotometerReading* reading) {
    if (readingCount_ >= SPECTRO_BUFFER_SIZE) {
        lastError_ = SpectroError::ERR_BUFFER_FULL;
        return false;
    }

    readingBuffer_[readingCount_] = *reading;
    readingCount_++;
    return true;
}

void Spectrophotometer::clearReadingBuffer() {
    readingCount_ = 0;
    memset(readingBuffer_, 0, sizeof(readingBuffer_));
}

//==============================================================================
// CALIBRATION FUNCTIONS (SPEC-006)
//==============================================================================

bool Spectrophotometer::captureBaseline(char channel, uint8_t numSamples) {
    if (!isReady()) {
        lastError_ = SpectroError::ERR_NOT_INITIALIZED;
        return false;
    }

    int8_t index = channelToIndex(channel);
    if (index < 0) {
        lastError_ = SpectroError::ERR_INVALID_CHANNEL;
        return false;
    }

    // Select the channel
    if (!selectChannel(channel)) {
        return false;
    }

    // Accumulator for averaging
    uint32_t sumF1 = 0, sumF2 = 0, sumF3 = 0, sumF4 = 0;
    uint32_t sumF5 = 0, sumF6 = 0, sumF7 = 0, sumF8 = 0;
    uint32_t sumClear = 0, sumNir = 0;

    // Take multiple samples
    for (uint8_t i = 0; i < numSamples; i++) {
        AS7341::FullSpectralData data = sensor_.readAllChannels(SPECTRO_TIMEOUT);

        if (sensor_.getLastError() != AS7341::ERR_OK) {
            lastError_ = SpectroError::ERR_SENSOR_FAULT;
            return false;
        }

        sumF1 += data.f1;
        sumF2 += data.f2;
        sumF3 += data.f3;
        sumF4 += data.f4;
        sumF5 += data.f5;
        sumF6 += data.f6;
        sumF7 += data.f7;
        sumF8 += data.f8;
        sumClear += data.clear;
        sumNir += data.nir;
    }

    // Calculate averages
    baseline_[index].f1 = static_cast<uint16_t>(sumF1 / numSamples);
    baseline_[index].f2 = static_cast<uint16_t>(sumF2 / numSamples);
    baseline_[index].f3 = static_cast<uint16_t>(sumF3 / numSamples);
    baseline_[index].f4 = static_cast<uint16_t>(sumF4 / numSamples);
    baseline_[index].f5 = static_cast<uint16_t>(sumF5 / numSamples);
    baseline_[index].f6 = static_cast<uint16_t>(sumF6 / numSamples);
    baseline_[index].f7 = static_cast<uint16_t>(sumF7 / numSamples);
    baseline_[index].f8 = static_cast<uint16_t>(sumF8 / numSamples);
    baseline_[index].clear = static_cast<uint16_t>(sumClear / numSamples);
    baseline_[index].nir = static_cast<uint16_t>(sumNir / numSamples);
    baseline_[index].timestamp = millis();
    baseline_[index].valid = true;

    Log.info("Spectrophotometer: Baseline captured for channel %c "
             "(F1=%d, F2=%d, F3=%d, F4=%d, F5=%d, F6=%d, F7=%d, F8=%d, Clear=%d, NIR=%d)",
             channel,
             baseline_[index].f1, baseline_[index].f2,
             baseline_[index].f3, baseline_[index].f4,
             baseline_[index].f5, baseline_[index].f6,
             baseline_[index].f7, baseline_[index].f8,
             baseline_[index].clear, baseline_[index].nir);

    lastError_ = SpectroError::SUCCESS;
    return true;
}

bool Spectrophotometer::getBaseline(char channel, SpectroBaselineData* baseline) {
    int8_t index = channelToIndex(channel);
    if (index < 0) {
        lastError_ = SpectroError::ERR_INVALID_CHANNEL;
        return false;
    }

    if (!baseline_[index].valid) {
        lastError_ = SpectroError::ERR_NO_BASELINE;
        return false;
    }

    *baseline = baseline_[index];
    return true;
}

uint16_t Spectrophotometer::getBaselineValue(char opticalChannel, AS7341::Channel spectralChannel) {
    int8_t index = channelToIndex(opticalChannel);
    if (index < 0 || !baseline_[index].valid) {
        return 0;
    }

    const SpectroBaselineData& bl = baseline_[index];
    switch (spectralChannel) {
        case AS7341::Channel::F1:    return bl.f1;
        case AS7341::Channel::F2:    return bl.f2;
        case AS7341::Channel::F3:    return bl.f3;
        case AS7341::Channel::F4:    return bl.f4;
        case AS7341::Channel::F5:    return bl.f5;
        case AS7341::Channel::F6:    return bl.f6;
        case AS7341::Channel::F7:    return bl.f7;
        case AS7341::Channel::F8:    return bl.f8;
        case AS7341::Channel::CLEAR: return bl.clear;
        case AS7341::Channel::NIR:   return bl.nir;
        default:                     return 0;
    }
}

void Spectrophotometer::clearBaseline(char channel) {
    if (channel == 0) {
        // Clear all baselines
        for (uint8_t i = 0; i < SPECTRO_NUM_CHANNELS; i++) {
            baseline_[i] = SpectroBaselineData();
        }
        Log.info("Spectrophotometer: All baselines cleared");
    } else {
        int8_t index = channelToIndex(channel);
        if (index >= 0) {
            baseline_[index] = SpectroBaselineData();
            Log.info("Spectrophotometer: Baseline cleared for channel %c", channel);
        }
    }
}

bool Spectrophotometer::hasValidBaseline(char channel) const {
    int8_t index = channelToIndex(channel);
    if (index < 0) {
        return false;
    }
    return baseline_[index].valid;
}

void Spectrophotometer::applyBaselineSubtraction(BrevitestSpectrophotometerReading* reading, char channel) {
    int8_t index = channelToIndex(channel);
    if (index < 0 || !baseline_[index].valid) {
        return;
    }

    const SpectroBaselineData& bl = baseline_[index];

    // Subtract baseline with floor at 0
    reading->f1 = (reading->f1 > bl.f1) ? (reading->f1 - bl.f1) : 0;
    reading->f2 = (reading->f2 > bl.f2) ? (reading->f2 - bl.f2) : 0;
    reading->f3 = (reading->f3 > bl.f3) ? (reading->f3 - bl.f3) : 0;
    reading->f4 = (reading->f4 > bl.f4) ? (reading->f4 - bl.f4) : 0;
    reading->f5 = (reading->f5 > bl.f5) ? (reading->f5 - bl.f5) : 0;
    reading->f6 = (reading->f6 > bl.f6) ? (reading->f6 - bl.f6) : 0;
    reading->f7 = (reading->f7 > bl.f7) ? (reading->f7 - bl.f7) : 0;
    reading->f8 = (reading->f8 > bl.f8) ? (reading->f8 - bl.f8) : 0;
    reading->clear = (reading->clear > bl.clear) ? (reading->clear - bl.clear) : 0;
    reading->nir = (reading->nir > bl.nir) ? (reading->nir - bl.nir) : 0;
}

//==============================================================================
// LED CONTROL
//==============================================================================

void Spectrophotometer::enableLed(bool on) {
    if (isReady()) {
        sensor_.enableLed(on);
    }
}

void Spectrophotometer::setLedCurrent(uint8_t current) {
    if (isReady()) {
        sensor_.setLedCurrent(current);
    }
}

//==============================================================================
// DIAGNOSTICS
//==============================================================================

bool Spectrophotometer::selfTest() {
    Log.info("Spectrophotometer: Running self-test...");

    // Test 1: Verify sensor connection
    if (!sensor_.isConnected()) {
        Log.error("Spectrophotometer: Self-test FAILED - AS7341 not responding");
        return false;
    }
    Log.info("  AS7341 connection: OK");

    // Test 2: Verify mux by selecting each channel
    for (uint8_t i = 0; i < SPECTRO_NUM_CHANNELS; i++) {
        char ch = indexToChannel(i);
        if (!selectChannel(ch)) {
            Log.error("Spectrophotometer: Self-test FAILED - Cannot select channel %c", ch);
            return false;
        }
        if (!verifyChannelSelection(ch)) {
            Log.error("Spectrophotometer: Self-test FAILED - Channel %c readback mismatch", ch);
            return false;
        }
    }
    Log.info("  PCA9536 mux: OK");

    // Test 3: Take a test reading
    selectChannel(SPECTRO_CHANNEL_A);
    AS7341::FullSpectralData testData = sensor_.readAllChannels(SPECTRO_TIMEOUT);
    if (sensor_.getLastError() != AS7341::ERR_OK) {
        Log.error("Spectrophotometer: Self-test FAILED - Cannot read spectral data");
        return false;
    }
    Log.info("  Spectral reading: OK (F1=%d, F4=%d, F8=%d)", testData.f1, testData.f4, testData.f8);

    // Turn off channels after test
    allChannelsOff();

    Log.info("Spectrophotometer: Self-test PASSED");
    return true;
}

void Spectrophotometer::printStatus() {
    Log.info("=== Spectrophotometer Status ===");
    Log.info("  Initialized: %s", initialized_ ? "Yes" : "No");
    Log.info("  Sensor Ready: %s", sensorReady_ ? "Yes" : "No");
    Log.info("  Mux Ready: %s", muxReady_ ? "Yes" : "No");
    Log.info("  Selected Channel: %c", selectedChannel_ ? selectedChannel_ : '-');
    Log.info("  Configuration: ATIME=%d, ASTEP=%d, AGAIN=%d", atime_, astep_, again_);
    Log.info("  Continuous Mode: %s", continuousModeActive_ ? "Active" : "Inactive");
    Log.info("  Readings in buffer: %d/%d", readingCount_, SPECTRO_BUFFER_SIZE);
    Log.info("  Baseline subtraction: %s", subtractBaseline_ ? "Enabled" : "Disabled");
    Log.info("  Baselines valid: A=%s, B=%s, C=%s",
             baseline_[0].valid ? "Yes" : "No",
             baseline_[1].valid ? "Yes" : "No",
             baseline_[2].valid ? "Yes" : "No");
    Log.info("  Last error: %s", errorToString(lastError_));

    // Print sensor status
    if (sensorReady_) {
        sensor_.printStatus(0);
    }
}

//==============================================================================
// COORDINATED SCANNING (BETA-019)
//==============================================================================

uint32_t Spectrophotometer::calculateIntegrationTimeUs(uint8_t atime, uint16_t astep) {
    // Integration time = (ATIME + 1) * (ASTEP + 1) * 2.78us
    return ((uint32_t)(atime + 1) * (uint32_t)(astep + 1) * 278) / 100;
}

uint16_t Spectrophotometer::calculateStepDelayForIntegration(uint8_t atime, uint16_t astep) {
    // Calculate integration time in microseconds
    uint32_t integrationTimeUs = calculateIntegrationTimeUs(atime, astep);

    // Well length is SPECTRO_WELL_LENGTH (5000 microns)
    // Each step is MOTOR_MICRONS_PER_EIGHTH_STEP (25 microns)
    int32_t stepsNeeded = SPECTRO_WELL_LENGTH / MOTOR_MICRONS_PER_EIGHTH_STEP;  // 200 steps

    // Time per step in microseconds
    uint32_t timePerStepUs = integrationTimeUs / stepsNeeded;

    // Step delay is half the time per step (since step duration = 2 * step_delay)
    uint16_t calculatedDelay = (uint16_t)(timePerStepUs / 2);

    // Enforce minimum step delay
    if (calculatedDelay < MOTOR_MINIMUM_STEP_DELAY) {
        calculatedDelay = MOTOR_MINIMUM_STEP_DELAY;
    }

    return calculatedDelay;
}

bool Spectrophotometer::readingContinuous(BrevitestTestRecord* test,
                                           MotorController* motor,
                                           LaserController* laser,
                                           HeaterController* heater,
                                           bool baseline,
                                           int32_t startingPosition,
                                           int32_t distanceToScan,
                                           uint16_t stepDelayUs,
                                           bool log) {
    if (!isReady() || test == nullptr || motor == nullptr || laser == nullptr) {
        lastError_ = SpectroError::ERR_NOT_INITIALIZED;
        return false;
    }

    int32_t previousPosition = motor->getCurrentPosition();

    // Set baseline or test scan count
    if (baseline) {
        test->baseline_scans = 1;
    } else {
        test->test_scans = 1;
    }

    // Set test configuration
    test->atime = atime_;
    test->astep = astep_;
    test->again = again_;

    // Calculate integration time in microseconds
    uint32_t integrationTimeUs = calculateIntegrationTimeUs(atime_, astep_);

    // Calculate reading distance: distance stage travels during one reading
    // Each step duration = 2 * step_delay_us
    // Distance per step = MOTOR_MICRONS_PER_EIGHTH_STEP (25 microns)
    int32_t readingDistance = (int32_t)((integrationTimeUs / (2 * stepDelayUs)) * MOTOR_MICRONS_PER_EIGHTH_STEP);

    // Ensure minimum reading distance
    if (readingDistance < MOTOR_MICRONS_PER_EIGHTH_STEP) {
        readingDistance = MOTOR_MICRONS_PER_EIGHTH_STEP;
    }

    // Calculate number of segments needed to cover scan distance
    int32_t numSegments = (distanceToScan + readingDistance - 1) / readingDistance;

    // Turn off heater during readings for ADC stabilization
    if (heater != nullptr) {
        heater->disable();
        delayMicroseconds(HEATER_STABILIZATION_TIME_US);
    }

    Log.info("Spectro: readingContinuous - start=%ld, distance=%ld, readingDist=%ld, segments=%ld",
             startingPosition, distanceToScan, readingDistance, numSegments);

    // Channel identifiers
    const char channels[3] = {'A', 'B', 'C'};

    // Read each channel (A, B, C) sequentially
    for (int j = 0; j < 3; j++) {
        char channel = channels[j];

        // Select the spectrophotometer channel
        if (!selectChannel(channel)) {
            Log.error("Spectro: Failed to select channel %c", channel);
            continue;
        }

        // Allow channel to stabilize
        delay(2);

        // Turn on laser for this channel
        laser->enableLaser(channel);
        delay(10);  // 10ms warmup delay

        // For each segment
        for (int32_t seg = 0; seg < numSegments; seg++) {
            // Check buffer capacity
            if (test->number_of_readings >= SPECTRO_MAX_READINGS) {
                Log.warn("Spectro: Reading buffer full");
                break;
            }

            // Calculate segment start and end positions
            int32_t segmentStart = startingPosition + seg * readingDistance;
            int32_t segmentEnd = segmentStart + readingDistance;

            // Last segment may be shorter
            if (segmentEnd > startingPosition + distanceToScan) {
                segmentEnd = startingPosition + distanceToScan;
            }

            int32_t segmentLength = segmentEnd - segmentStart;

            // Move to segment start position
            motor->moveToPosition(segmentStart, MOTOR_FAST_STEP_DELAY);

            // Get reference to reading structure
            BrevitestSpectrophotometerReading* reading = &(test->reading[test->number_of_readings]);
            reading->number = test->number_of_readings;
            reading->channel = channel;
            reading->temperature = heater ? heater->getCurrentTemperature() : 0;
            reading->position = (uint16_t)segmentStart;
            reading->laser_output = 0;  // Could read photodetector here

            // Start F1F4ClearNIR measurement
            uint32_t startTime = millis();
            sensor_.startMeasure(AS7341::ChannelMapping::F1F4_CLEAR_NIR);

            // Move stage through segment while sensor integrates
            motor->moveRelative(segmentLength, stepDelayUs);

            // Wait for F1F4ClearNIR measurement to complete
            while (!sensor_.measureComplete() && (millis() - startTime) < SPECTRO_TIMEOUT) {
                delayMicroseconds(100);
            }

            if (sensor_.measureComplete()) {
                AS7341::ModeOneData data1 = sensor_.readSpectralDataOne();
                reading->f1 = data1.f1;
                reading->f2 = data1.f2;
                reading->f3 = data1.f3;
                reading->f4 = data1.f4;
                reading->clear = data1.clear;
                reading->nir = data1.nir;
            } else {
                Log.warn("Spectro: F1F4ClearNIR measurement timed out");
            }

            reading->msec = millis() - testStartTime_;

            // Move back to segment start for F5-F8 measurement
            motor->moveToPosition(segmentStart, MOTOR_FAST_STEP_DELAY);

            // Start F5F8ClearNIR measurement
            startTime = millis();
            sensor_.startMeasure(AS7341::ChannelMapping::F5F8_CLEAR_NIR);

            // Move stage through segment again while sensor integrates
            motor->moveRelative(segmentLength, stepDelayUs);

            // Wait for F5F8ClearNIR measurement to complete
            while (!sensor_.measureComplete() && (millis() - startTime) < SPECTRO_TIMEOUT) {
                delayMicroseconds(100);
            }

            if (sensor_.measureComplete()) {
                AS7341::ModeTwoData data2 = sensor_.readSpectralDataTwo();
                reading->f5 = data2.f5;
                reading->f6 = data2.f6;
                reading->f7 = data2.f7;
                reading->f8 = data2.f8;
            } else {
                Log.warn("Spectro: F5F8ClearNIR measurement timed out");
            }

            test->number_of_readings++;

            if (log) {
                Log.info("%d\t%c\t%d\t%d\t%lu\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
                         reading->number, reading->channel, reading->position,
                         reading->temperature, reading->msec,
                         reading->f1, reading->f2, reading->f3, reading->f4,
                         reading->f5, reading->f6, reading->f7, reading->f8,
                         reading->clear, reading->nir);
            }
        }

        // Turn off laser after all segments for this channel
        laser->disableLaser(channel);
    }

    // Turn off all spectrophotometers
    allChannelsOff();

    // Return stage to previous position
    motor->moveToPosition(previousPosition, MOTOR_FAST_STEP_DELAY);

    // Re-enable heater if it was on
    if (heater != nullptr && heater->getTargetTemperature() > 0) {
        heater->enable();
    }

    Log.info("Spectro: readingContinuous complete, %d readings captured", test->number_of_readings);
    return true;
}
