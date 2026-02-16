/**
 * @file AS7341.h
 * @brief AS7341 11-channel spectral sensor driver
 * @author Agent ZETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * This file provides a low-level driver for the AS7341 11-channel visible light
 * spectral sensor from ams/Osram. The sensor provides 8 wavelength channels (F1-F8),
 * plus Clear and NIR channels.
 *
 * Features:
 * - Configurable integration time (ATIME, ASTEP)
 * - Configurable gain (AGAIN: 0.5x to 512x)
 * - SMUX configuration for channel mapping
 * - Spectral measurement mode (SPM)
 * - LED control for on-chip LED
 * - Flicker detection
 *
 * Channel wavelengths:
 *   F1: 415nm (violet)    F5: 555nm (yellow-green)
 *   F2: 445nm (blue)      F6: 590nm (orange)
 *   F3: 480nm (cyan)      F7: 630nm (red)
 *   F4: 515nm (green)     F8: 680nm (deep red)
 *   Clear: unfiltered     NIR: near-infrared
 *
 * User Stories Implemented:
 *   - SPEC-001: Port AS7341 driver from legacy firmware
 *
 * @note Based on DFRobot_AS7341 legacy driver
 * @see https://ams.com/as7341 for datasheet
 */

#ifndef AS7341_H
#define AS7341_H

#include "Particle.h"

namespace AS7341 {

//==============================================================================
// I2C ADDRESS
//==============================================================================

/** @brief Default I2C address for AS7341 (cannot be changed) */
constexpr uint8_t I2C_ADDRESS = 0x39;

//==============================================================================
// REGISTER DEFINITIONS
//==============================================================================

// Configuration registers (accessible in both banks)
constexpr uint8_t REG_CONFIG      = 0x70;   // Configuration register
constexpr uint8_t REG_STAT        = 0x71;   // Status register
constexpr uint8_t REG_EDGE        = 0x72;   // Edge detection register
constexpr uint8_t REG_CPIO        = 0x73;   // GPIO/Interrupt configuration
constexpr uint8_t REG_LED         = 0x74;   // LED control register

// Enable and timing registers
constexpr uint8_t REG_ENABLE      = 0x80;   // Enable register (power, measurements)
constexpr uint8_t REG_ATIME       = 0x81;   // Integration time setting
constexpr uint8_t REG_WTIME       = 0x83;   // Wait time between measurements

// Spectral threshold registers
constexpr uint8_t REG_SP_TH_L_LSB = 0x84;   // Spectral threshold low, LSB
constexpr uint8_t REG_SP_TH_L_MSB = 0x85;   // Spectral threshold low, MSB
constexpr uint8_t REG_SP_TH_H_LSB = 0x86;   // Spectral threshold high, LSB
constexpr uint8_t REG_SP_TH_H_MSB = 0x87;   // Spectral threshold high, MSB

// Device ID registers
constexpr uint8_t REG_AUXID       = 0x90;   // Auxiliary ID
constexpr uint8_t REG_REVID       = 0x91;   // Revision ID
constexpr uint8_t REG_ID          = 0x92;   // Device ID (should be 0x24)

// Status registers
constexpr uint8_t REG_STATUS_1    = 0x93;   // Status register 1
constexpr uint8_t REG_ASTATUS     = 0x94;   // Auto-gain status

// Channel data registers (6 ADC channels available at a time via SMUX)
constexpr uint8_t REG_CH0_DATA_L  = 0x95;   // Channel 0 data low byte
constexpr uint8_t REG_CH0_DATA_H  = 0x96;   // Channel 0 data high byte
constexpr uint8_t REG_CH1_DATA_L  = 0x97;   // Channel 1 data low byte
constexpr uint8_t REG_CH1_DATA_H  = 0x98;   // Channel 1 data high byte
constexpr uint8_t REG_CH2_DATA_L  = 0x99;   // Channel 2 data low byte
constexpr uint8_t REG_CH2_DATA_H  = 0x9A;   // Channel 2 data high byte
constexpr uint8_t REG_CH3_DATA_L  = 0x9B;   // Channel 3 data low byte
constexpr uint8_t REG_CH3_DATA_H  = 0x9C;   // Channel 3 data high byte
constexpr uint8_t REG_CH4_DATA_L  = 0x9D;   // Channel 4 data low byte
constexpr uint8_t REG_CH4_DATA_H  = 0x9E;   // Channel 4 data high byte
constexpr uint8_t REG_CH5_DATA_L  = 0x9F;   // Channel 5 data low byte
constexpr uint8_t REG_CH5_DATA_H  = 0xA0;   // Channel 5 data high byte

// Additional status registers
constexpr uint8_t REG_STATUS_2    = 0xA3;   // Status register 2 (measurement complete)
constexpr uint8_t REG_STATUS_3    = 0xA4;   // Status register 3
constexpr uint8_t REG_STATUS_5    = 0xA6;   // Status register 5
constexpr uint8_t REG_STATUS_6    = 0xA7;   // Status register 6

// Configuration registers
constexpr uint8_t REG_CFG_0       = 0xA9;   // Configuration 0 (register bank select)
constexpr uint8_t REG_CFG_1       = 0xAA;   // Configuration 1 (AGAIN)
constexpr uint8_t REG_CFG_3       = 0xAC;   // Configuration 3
constexpr uint8_t REG_CFG_6       = 0xAF;   // Configuration 6 (SMUX command)
constexpr uint8_t REG_CFG_8       = 0xB1;   // Configuration 8
constexpr uint8_t REG_CFG_9       = 0xB2;   // Configuration 9
constexpr uint8_t REG_CFG_10      = 0xB3;   // Configuration 10
constexpr uint8_t REG_CFG_12      = 0xB5;   // Configuration 12

// Persistence and GPIO registers
constexpr uint8_t REG_PERS        = 0xBD;   // Interrupt persistence
constexpr uint8_t REG_GPIO_2      = 0xBE;   // GPIO configuration 2

// Integration step registers
constexpr uint8_t REG_ASTEP_L     = 0xCA;   // ASTEP low byte
constexpr uint8_t REG_ASTEP_H     = 0xCB;   // ASTEP high byte

// Auto-gain and flicker detection registers
constexpr uint8_t REG_AGC_GAIN_MAX = 0xCF;  // AGC maximum gain
constexpr uint8_t REG_AZ_CONFIG   = 0xD6;   // Auto-zero configuration
constexpr uint8_t REG_FD_TIME_1   = 0xD8;   // Flicker detection time 1
constexpr uint8_t REG_TIME_2      = 0xDA;   // Time configuration 2
constexpr uint8_t REG_CFG0        = 0xD7;   // Configuration 0 (alternate)
constexpr uint8_t REG_STATUS      = 0xDB;   // Flicker detection status
constexpr uint8_t REG_INTENAB     = 0xF9;   // Interrupt enable
constexpr uint8_t REG_CONTROL     = 0xFA;   // Control register

// FIFO registers
constexpr uint8_t REG_FIFO_MAP    = 0xFC;   // FIFO mapping
constexpr uint8_t REG_FIFO_LVL    = 0xFD;   // FIFO level
constexpr uint8_t REG_FDATA_L     = 0xFE;   // FIFO data low byte
constexpr uint8_t REG_FDATA_H     = 0xFF;   // FIFO data high byte

//==============================================================================
// ENABLE REGISTER BITS
//==============================================================================

constexpr uint8_t ENABLE_PON      = 0x01;   // Power ON
constexpr uint8_t ENABLE_SP_EN    = 0x02;   // Spectral measurement enable
constexpr uint8_t ENABLE_WEN      = 0x08;   // Wait timer enable
constexpr uint8_t ENABLE_SMUXEN   = 0x10;   // SMUX enable
constexpr uint8_t ENABLE_FDEN     = 0x40;   // Flicker detection enable

//==============================================================================
// STATUS REGISTER BITS
//==============================================================================

constexpr uint8_t STATUS2_AVALID  = 0x40;   // Spectral data valid

//==============================================================================
// DEVICE ID
//==============================================================================

constexpr uint8_t DEVICE_ID       = 0x24;   // Expected device ID value

//==============================================================================
// ERROR CODES
//==============================================================================

constexpr int8_t ERR_OK           = 0;      // Operation successful
constexpr int8_t ERR_DATA_BUS     = -1;     // I2C communication error
constexpr int8_t ERR_IC_VERSION   = -2;     // Device ID mismatch

//==============================================================================
// ENUMERATIONS
//==============================================================================

/**
 * @brief Measurement modes for the AS7341
 */
enum class MeasurementMode : uint8_t {
    SPM  = 0,   // Spectral measurement (single reading)
    SYNS = 1,   // Synchronous start
    SYND = 3    // Synchronous end
};

/**
 * @brief Channel mapping modes for SMUX configuration
 * @details The AS7341 has 6 ADC channels but 10 photodiodes. SMUX configuration
 *          maps 6 photodiodes to the 6 ADC channels at a time. To read all 10
 *          channels, two measurements are needed with different SMUX configurations.
 */
enum class ChannelMapping : uint8_t {
    F1F4_CLEAR_NIR = 0,   // Map F1, F2, F3, F4, Clear, NIR to ADC channels
    F5F8_CLEAR_NIR = 1    // Map F5, F6, F7, F8, Clear, NIR to ADC channels
};

/**
 * @brief Individual channel identifiers
 */
enum class Channel : uint8_t {
    F1    = 0,    // 415nm violet
    F2    = 1,    // 445nm blue
    F3    = 2,    // 480nm cyan
    F4    = 3,    // 515nm green
    F5    = 4,    // 555nm yellow-green
    F6    = 5,    // 590nm orange
    F7    = 6,    // 630nm red
    F8    = 7,    // 680nm deep red
    CLEAR = 8,    // Unfiltered
    NIR   = 9     // Near-infrared
};

/**
 * @brief Gain settings for AGAIN register
 * @details Gain values: 0=0.5x, 1=1x, 2=2x, 3=4x, 4=8x, 5=16x,
 *                       6=32x, 7=64x, 8=128x, 9=256x, 10=512x
 */
enum class Gain : uint8_t {
    GAIN_0_5X  = 0,
    GAIN_1X    = 1,
    GAIN_2X    = 2,
    GAIN_4X    = 3,
    GAIN_8X    = 4,
    GAIN_16X   = 5,
    GAIN_32X   = 6,
    GAIN_64X   = 7,
    GAIN_128X  = 8,
    GAIN_256X  = 9,
    GAIN_512X  = 10
};

//==============================================================================
// DATA STRUCTURES
//==============================================================================

/**
 * @brief Data from F1-F4, Clear, NIR channels (first SMUX configuration)
 */
struct ModeOneData {
    uint16_t f1;      // F1 channel (415nm)
    uint16_t f2;      // F2 channel (445nm)
    uint16_t f3;      // F3 channel (480nm)
    uint16_t f4;      // F4 channel (515nm)
    uint16_t clear;   // Clear channel
    uint16_t nir;     // NIR channel
};

/**
 * @brief Data from F5-F8, Clear, NIR channels (second SMUX configuration)
 */
struct ModeTwoData {
    uint16_t f5;      // F5 channel (555nm)
    uint16_t f6;      // F6 channel (590nm)
    uint16_t f7;      // F7 channel (630nm)
    uint16_t f8;      // F8 channel (680nm)
    uint16_t clear;   // Clear channel
    uint16_t nir;     // NIR channel
};

/**
 * @brief Complete spectral data from all 10 channels
 */
struct FullSpectralData {
    uint16_t f1;      // F1 channel (415nm)
    uint16_t f2;      // F2 channel (445nm)
    uint16_t f3;      // F3 channel (480nm)
    uint16_t f4;      // F4 channel (515nm)
    uint16_t f5;      // F5 channel (555nm)
    uint16_t f6;      // F6 channel (590nm)
    uint16_t f7;      // F7 channel (630nm)
    uint16_t f8;      // F8 channel (680nm)
    uint16_t clear;   // Clear channel (average of both readings)
    uint16_t nir;     // NIR channel (average of both readings)
};

//==============================================================================
// AS7341 DRIVER CLASS
//==============================================================================

/**
 * @class Driver
 * @brief Low-level driver for AS7341 spectral sensor
 *
 * This class provides low-level access to the AS7341 sensor including:
 * - Initialization and configuration
 * - Integration time and gain settings
 * - SMUX configuration for channel mapping
 * - Spectral measurement triggering
 * - Channel data reading
 * - LED control
 *
 * Usage:
 * @code
 *   AS7341::Driver sensor;
 *   if (sensor.begin() == AS7341::ERR_OK) {
 *       sensor.setAtime(49);
 *       sensor.setAstep(999);
 *       sensor.setGain(AS7341::Gain::GAIN_64X);
 *
 *       sensor.startMeasure(AS7341::ChannelMapping::F1F4_CLEAR_NIR);
 *       while (!sensor.measureComplete()) { delay(1); }
 *       AS7341::ModeOneData data1 = sensor.readSpectralDataOne();
 *
 *       sensor.startMeasure(AS7341::ChannelMapping::F5F8_CLEAR_NIR);
 *       while (!sensor.measureComplete()) { delay(1); }
 *       AS7341::ModeTwoData data2 = sensor.readSpectralDataTwo();
 *   }
 * @endcode
 */
class Driver {
public:
    /**
     * @brief Constructor
     * @param pWire I2C bus interface (default: Wire)
     */
    explicit Driver(TwoWire* pWire = &Wire);

    /**
     * @brief Initialize the sensor
     * @param mode Measurement mode (default: SPM)
     * @return ERR_OK on success, error code otherwise
     */
    int8_t begin(MeasurementMode mode = MeasurementMode::SPM);

    /**
     * @brief Read the device ID
     * @return Device ID (should be 0x24 for AS7341)
     */
    uint8_t readID();

    /**
     * @brief Check if device is connected and responding
     * @return true if device is present and ID matches
     */
    bool isConnected();

    //--------------------------------------------------------------------------
    // Configuration
    //--------------------------------------------------------------------------

    /**
     * @brief Set ATIME (integration time multiplier)
     * @param value ATIME value (0-255)
     * @details Integration time = (ATIME + 1) * (ASTEP + 1) * 2.78us
     */
    void setAtime(uint8_t value);

    /**
     * @brief Set ASTEP (integration step time)
     * @param value ASTEP value (0-65535)
     * @details Integration time = (ATIME + 1) * (ASTEP + 1) * 2.78us
     */
    void setAstep(uint16_t value);

    /**
     * @brief Set gain (AGAIN)
     * @param value Gain setting (0-10, corresponding to 0.5x to 512x)
     */
    void setGain(uint8_t value);

    /**
     * @brief Set gain using enum
     * @param gain Gain enumeration value
     */
    void setGain(Gain gain);


    //--------------------------------------------------------------------------
    // Measurement Control
    //--------------------------------------------------------------------------

    /**
     * @brief Start spectral measurement with specified channel mapping
     * @param mapping Channel mapping mode (F1-F4 or F5-F8)
     */
    void startMeasure(ChannelMapping mapping);

    /**
     * @brief Check if measurement is complete
     * @return true if spectral data is ready
     */
    bool measureComplete();

    /**
     * @brief Read channel data for F1-F4, Clear, NIR mapping
     * @return ModeOneData structure with channel values
     */
    ModeOneData readSpectralDataOne();

    /**
     * @brief Read channel data for F5-F8, Clear, NIR mapping
     * @return ModeTwoData structure with channel values
     */
    ModeTwoData readSpectralDataTwo();

    /**
     * @brief Read all 10 channels (performs two measurement cycles)
     * @param timeoutMs Timeout for each measurement in milliseconds
     * @return FullSpectralData structure with all channel values
     */
    FullSpectralData readAllChannels(uint16_t timeoutMs = 1000);


    //--------------------------------------------------------------------------
    // Enable/Disable Functions
    //--------------------------------------------------------------------------

    /**
     * @brief Enable or disable spectral measurement
     * @param on true to enable, false to disable
     */
    void enableSpectralMeasure(bool on);

    /**
     * @brief Enable or disable the sensor power
     * @param on true to power on, false to power off
     */
    void enablePower(bool on);

    //--------------------------------------------------------------------------
    // LED Control
    //--------------------------------------------------------------------------

    /**
     * @brief Enable or disable the on-chip LED
     * @param on true to enable, false to disable
     */
    void enableLed(bool on);

    /**
     * @brief Set LED drive current
     * @param current Current level (1-20 corresponds to 4mA-42mA in 2mA steps)
     */
    void setLedCurrent(uint8_t current);

    //--------------------------------------------------------------------------
    // Debug/Status
    //--------------------------------------------------------------------------

    /**
     * @brief Print status register values for debugging
     * @param index Index for log identification
     */
    void printStatus(int index);

    /**
     * @brief Get last error code
     * @return Last error that occurred
     */
    int8_t getLastError() const { return lastError_; }

private:
    //--------------------------------------------------------------------------
    // Internal Configuration Functions
    //--------------------------------------------------------------------------

    void config(MeasurementMode mode);
    void setBank(uint8_t addr);
    void enableSMUX(bool on);
    void enableWait(bool on);
    void enableFlickerDetection(bool on);
    void clearInterrupt();
    void clearFIFO();
    void spectralAutozero();
    void endSleep();

    void setGpio(bool connect);
    void setInt(bool connect);

    void enableSysInt(bool on);
    void enableFIFOInt(bool on);
    void enableSpectralInt(bool on);
    void enableFlickerInt(bool on);

    void setThreshold(uint16_t lowTh, uint16_t highTh);
    uint16_t getLowThreshold();
    uint16_t getHighThreshold();
    void enableSpectralInterrupt(bool on);
    void setIntChannel(uint8_t channel);
    void setAPERS(uint8_t num);
    uint8_t getIntSource();
    bool interrupt();

    //--------------------------------------------------------------------------
    // SMUX Configuration
    //--------------------------------------------------------------------------

    void F1F4_Clear_NIR();
    void F5F8_Clear_NIR();
    void FDConfig();

    //--------------------------------------------------------------------------
    // Low-Level I2C Functions
    //--------------------------------------------------------------------------

    uint16_t getChannelData(uint8_t channel);

    void writeReg(uint8_t reg, uint8_t data);
    void writeReg(uint8_t reg, void* pBuf, size_t size);
    uint8_t readReg(uint8_t reg);
    uint8_t readReg(uint8_t reg, void* pBuf, size_t size);

    //--------------------------------------------------------------------------
    // Member Variables
    //--------------------------------------------------------------------------

    TwoWire* pWire_;               // I2C bus interface
    uint8_t address_;              // I2C address (always 0x39)
    MeasurementMode measureMode_;  // Current measurement mode
    int8_t lastError_;             // Last error code
};

} // namespace AS7341

#endif // AS7341_H
