/**
 * @file AS7341.cpp
 * @brief AS7341 11-channel spectral sensor driver implementation
 * @author Agent ZETA - Supabase Firmware Rewrite Project
 * @date January 2026
 *
 * Implementation of the AS7341 driver. Based on DFRobot_AS7341 legacy driver
 * with improved organization and error handling.
 *
 * User Stories Implemented:
 *   - SPEC-001: Port AS7341 driver from legacy firmware
 */

#include "AS7341.h"

namespace AS7341 {

//==============================================================================
// CONSTRUCTOR
//==============================================================================

Driver::Driver(TwoWire* pWire)
    : pWire_(pWire)
    , address_(I2C_ADDRESS)
    , measureMode_(MeasurementMode::SPM)
    , lastError_(ERR_OK)
{
}

//==============================================================================
// INITIALIZATION
//==============================================================================

int8_t Driver::begin(MeasurementMode mode) {
    // Check I2C communication
    pWire_->beginTransmission(address_);
    if (pWire_->endTransmission() != 0) {
        lastError_ = ERR_DATA_BUS;
        return ERR_DATA_BUS;
    }

    // Enable power
    enablePower(true);
    measureMode_ = mode;
    lastError_ = ERR_OK;
    return ERR_OK;
}

uint8_t Driver::readID() {
    uint8_t id = 0;
    if (readReg(REG_ID, &id, 1) == 0) {
        return 0;
    }
    return id;
}

bool Driver::isConnected() {
    uint8_t id = readID();
    return (id == DEVICE_ID);
}

//==============================================================================
// CONFIGURATION
//==============================================================================

void Driver::setAtime(uint8_t value) {
    writeReg(REG_ATIME, &value, 1);
}

void Driver::setAstep(uint16_t value) {
    uint8_t lowValue = value & 0x00FF;
    uint8_t highValue = value >> 8;
    writeReg(REG_ASTEP_L, &lowValue, 1);
    writeReg(REG_ASTEP_H, &highValue, 1);
}

void Driver::setGain(uint8_t value) {
    if (value > 10) {
        value = 10;  // Clamp to maximum gain
    }
    writeReg(REG_CFG_1, &value, 1);
}

void Driver::setGain(Gain gain) {
    setGain(static_cast<uint8_t>(gain));
}


//==============================================================================
// MEASUREMENT CONTROL
//==============================================================================

void Driver::startMeasure(ChannelMapping mapping) {
    uint8_t data = 0;

    // Clear low power bit in CFG_0
    readReg(REG_CFG_0, &data, 1);
    data = data & (~(1 << 4));
    writeReg(REG_CFG_0, &data, 1);

    // Disable spectral measurement during SMUX configuration
    enableSpectralMeasure(false);

    // Set SMUX command
    writeReg(REG_CFG_6, uint8_t(0x10));

    // Configure SMUX for selected channel mapping
    if (mapping == ChannelMapping::F1F4_CLEAR_NIR) {
        F1F4_Clear_NIR();
    } else {
        F5F8_Clear_NIR();
    }

    // Enable SMUX
    enableSMUX(true);

    // Configure for SPM mode
    config(MeasurementMode::SPM);

    // Enable spectral measurement
    enableSpectralMeasure(true);
}

bool Driver::measureComplete() {
    uint8_t status = 0;
    readReg(REG_STATUS_2, &status, 1);
    return ((status & STATUS2_AVALID) != 0);
}

ModeOneData Driver::readSpectralDataOne() {
    ModeOneData data;
    data.f1    = getChannelData(0);
    data.f2    = getChannelData(1);
    data.f3    = getChannelData(2);
    data.f4    = getChannelData(3);
    data.clear = getChannelData(4);
    data.nir   = getChannelData(5);
    return data;
}

ModeTwoData Driver::readSpectralDataTwo() {
    ModeTwoData data;
    data.f5    = getChannelData(0);
    data.f6    = getChannelData(1);
    data.f7    = getChannelData(2);
    data.f8    = getChannelData(3);
    data.clear = getChannelData(4);
    data.nir   = getChannelData(5);
    return data;
}

FullSpectralData Driver::readAllChannels(uint16_t timeoutMs) {
    FullSpectralData data = {0};
    uint32_t startTime;

    // First measurement: F1-F4, Clear, NIR
    startMeasure(ChannelMapping::F1F4_CLEAR_NIR);
    startTime = millis();
    while (!measureComplete()) {
        if (millis() - startTime > timeoutMs) {
            lastError_ = ERR_DATA_BUS;
            return data;
        }
        delay(1);
    }
    ModeOneData data1 = readSpectralDataOne();

    // Second measurement: F5-F8, Clear, NIR
    startMeasure(ChannelMapping::F5F8_CLEAR_NIR);
    startTime = millis();
    while (!measureComplete()) {
        if (millis() - startTime > timeoutMs) {
            lastError_ = ERR_DATA_BUS;
            return data;
        }
        delay(1);
    }
    ModeTwoData data2 = readSpectralDataTwo();

    // Combine data
    data.f1 = data1.f1;
    data.f2 = data1.f2;
    data.f3 = data1.f3;
    data.f4 = data1.f4;
    data.f5 = data2.f5;
    data.f6 = data2.f6;
    data.f7 = data2.f7;
    data.f8 = data2.f8;
    // Average Clear and NIR from both measurements
    data.clear = (data1.clear + data2.clear) / 2;
    data.nir   = (data1.nir + data2.nir) / 2;

    return data;
}


//==============================================================================
// ENABLE/DISABLE FUNCTIONS
//==============================================================================

void Driver::enablePower(bool on) {
    uint8_t data = 0;
    readReg(REG_ENABLE, &data, 1);
    if (on) {
        data |= ENABLE_PON;
    } else {
        data &= ~ENABLE_PON;
    }
    writeReg(REG_ENABLE, &data, 1);
    delay(10);  // Allow power to stabilize
}

void Driver::enableSpectralMeasure(bool on) {
    uint8_t data = 0;
    readReg(REG_ENABLE, &data, 1);
    if (on) {
        data |= ENABLE_SP_EN;
    } else {
        data &= ~ENABLE_SP_EN;
    }
    writeReg(REG_ENABLE, &data, 1);
}

void Driver::enableSMUX(bool on) {
    uint8_t data = 0;
    readReg(REG_ENABLE, &data, 1);
    if (on) {
        data |= ENABLE_SMUXEN;
    } else {
        data &= ~ENABLE_SMUXEN;
    }
    writeReg(REG_ENABLE, &data, 1);
}

void Driver::enableWait(bool on) {
    uint8_t data = 0;
    readReg(REG_ENABLE, &data, 1);
    if (on) {
        data |= ENABLE_WEN;
    } else {
        data &= ~ENABLE_WEN;
    }
    writeReg(REG_ENABLE, &data, 1);
}

void Driver::enableFlickerDetection(bool on) {
    uint8_t data = 0;
    readReg(REG_ENABLE, &data, 1);
    if (on) {
        data |= ENABLE_FDEN;
    } else {
        data &= ~ENABLE_FDEN;
    }
    writeReg(REG_ENABLE, &data, 1);
}

//==============================================================================
// LED CONTROL
//==============================================================================

void Driver::enableLed(bool on) {
    setBank(1);
    uint8_t data = 0;
    readReg(REG_LED, &data, 1);
    if (on) {
        data |= (1 << 7);
    } else {
        data &= ~(1 << 7);
    }
    writeReg(REG_LED, &data, 1);
    setBank(0);
}

void Driver::setLedCurrent(uint8_t current) {
    if (current > 20) {
        current = 20;  // Clamp to maximum
    }
    setBank(1);
    uint8_t data = 0;
    readReg(REG_LED, &data, 1);
    data = (data & 0xC0) | (current & 0x3F);
    writeReg(REG_LED, &data, 1);
    setBank(0);
}

//==============================================================================
// GPIO CONTROL
//==============================================================================

void Driver::setGpio(bool connect) {
    uint8_t data = 0;
    readReg(REG_CPIO, &data, 1);
    if (connect) {
        data |= (1 << 0);
    } else {
        data &= ~(1 << 0);
    }
    writeReg(REG_CPIO, &data, 1);
}

void Driver::setInt(bool connect) {
    uint8_t data = 0;
    readReg(REG_CPIO, &data, 1);
    if (connect) {
        data |= (1 << 1);
    } else {
        data &= ~(1 << 1);
    }
    writeReg(REG_CPIO, &data, 1);
}

//==============================================================================
// INTERNAL CONFIGURATION
//==============================================================================

void Driver::config(MeasurementMode mode) {
    uint8_t data = 0;
    setBank(1);
    readReg(REG_CONFIG, &data, 1);

    switch (mode) {
        case MeasurementMode::SPM:
            data = (data & (~3)) | static_cast<uint8_t>(MeasurementMode::SPM);
            break;
        case MeasurementMode::SYNS:
            data = (data & (~3)) | static_cast<uint8_t>(MeasurementMode::SYNS);
            break;
        case MeasurementMode::SYND:
            data = (data & (~3)) | static_cast<uint8_t>(MeasurementMode::SYND);
            break;
    }

    writeReg(REG_CONFIG, &data, 1);
    setBank(0);
}

void Driver::setBank(uint8_t addr) {
    uint8_t data = 0;
    readReg(REG_CFG_0, &data, 1);
    if (addr == 1) {
        data |= (1 << 4);
    } else {
        data &= ~(1 << 4);
    }
    writeReg(REG_CFG_0, &data, 1);
}

void Driver::clearInterrupt() {
    uint8_t data = 0;
    readReg(REG_STATUS_1, &data, 1);
    // Write-1-to-clear: write back read value to clear active interrupt flags
    if (data) {
        writeReg(REG_STATUS_1, &data, 1);
    }
}

void Driver::clearFIFO() {
    uint8_t data = 0;
    readReg(REG_CONTROL, &data, 1);
    data |= (1 << 0);
    data &= ~(1 << 0);
    writeReg(REG_CONTROL, &data, 1);
}

void Driver::spectralAutozero() {
    uint8_t data = 0;
    readReg(REG_AZ_CONFIG, &data, 1);
    data |= (1 << 7);
    writeReg(REG_AZ_CONFIG, &data, 1);
}

void Driver::endSleep() {
    uint8_t data = 0;
    readReg(REG_INTENAB, &data, 1);
    data |= (1 << 3);
    writeReg(REG_INTENAB, &data, 1);
}

//==============================================================================
// INTERRUPT CONFIGURATION
//==============================================================================

void Driver::enableSysInt(bool on) {
    uint8_t data = 0;
    readReg(REG_INTENAB, &data, 1);
    if (on) {
        data |= (1 << 0);
    } else {
        data &= ~(1 << 0);
    }
    writeReg(REG_INTENAB, &data, 1);
}

void Driver::enableFIFOInt(bool on) {
    uint8_t data = 0;
    readReg(REG_INTENAB, &data, 1);
    if (on) {
        data |= (1 << 2);
    } else {
        data &= ~(1 << 2);
    }
    writeReg(REG_INTENAB, &data, 1);
}

void Driver::enableSpectralInt(bool on) {
    uint8_t data = 0;
    readReg(REG_INTENAB, &data, 1);
    if (on) {
        data |= (1 << 3);
    } else {
        data &= ~(1 << 3);
    }
    writeReg(REG_INTENAB, &data, 1);
}

void Driver::enableFlickerInt(bool on) {
    uint8_t data = 0;
    readReg(REG_INTENAB, &data, 1);
    if (on) {
        data |= (1 << 4);
    } else {
        data &= ~(1 << 4);
    }
    writeReg(REG_INTENAB, &data, 1);
}

void Driver::setThreshold(uint16_t lowTh, uint16_t highTh) {
    writeReg(REG_SP_TH_L_LSB, static_cast<uint8_t>(lowTh & 0xFF));
    writeReg(REG_SP_TH_L_MSB, static_cast<uint8_t>(lowTh >> 8));
    writeReg(REG_SP_TH_H_LSB, static_cast<uint8_t>(highTh & 0xFF));
    writeReg(REG_SP_TH_H_MSB, static_cast<uint8_t>(highTh >> 8));
}

uint16_t Driver::getLowThreshold() {
    uint8_t lsb = readReg(REG_SP_TH_L_LSB);
    uint8_t msb = readReg(REG_SP_TH_L_MSB);
    return (static_cast<uint16_t>(msb) << 8) | lsb;
}

uint16_t Driver::getHighThreshold() {
    uint8_t lsb = readReg(REG_SP_TH_H_LSB);
    uint8_t msb = readReg(REG_SP_TH_H_MSB);
    return (static_cast<uint16_t>(msb) << 8) | lsb;
}

void Driver::enableSpectralInterrupt(bool on) {
    enableSpectralInt(on);
}

void Driver::setIntChannel(uint8_t channel) {
    uint8_t data = 0;
    readReg(REG_CFG_12, &data, 1);
    data = (data & 0xF8) | (channel & 0x07);
    writeReg(REG_CFG_12, &data, 1);
}

void Driver::setAPERS(uint8_t num) {
    writeReg(REG_PERS, num);
}

uint8_t Driver::getIntSource() {
    return readReg(REG_STATUS_1);
}

bool Driver::interrupt() {
    uint8_t status = readReg(REG_STATUS_1);
    return (status & 0x08) != 0;
}

//==============================================================================
// SMUX CONFIGURATION
//==============================================================================

/**
 * @brief Configure SMUX for F1, F2, F3, F4, Clear, NIR channels
 * @details Maps photodiodes to ADC channels:
 *   ADC0 = F1, ADC1 = F2, ADC2 = F3, ADC3 = F4, ADC4 = Clear, ADC5 = NIR
 */
void Driver::F1F4_Clear_NIR() {
    writeReg(0x00, uint8_t(0x30));
    writeReg(0x01, uint8_t(0x01));
    writeReg(0x02, uint8_t(0x00));
    writeReg(0x03, uint8_t(0x00));
    writeReg(0x04, uint8_t(0x00));
    writeReg(0x05, uint8_t(0x42));
    writeReg(0x06, uint8_t(0x00));
    writeReg(0x07, uint8_t(0x00));
    writeReg(0x08, uint8_t(0x50));
    writeReg(0x09, uint8_t(0x00));
    writeReg(0x0A, uint8_t(0x00));
    writeReg(0x0B, uint8_t(0x00));
    writeReg(0x0C, uint8_t(0x20));
    writeReg(0x0D, uint8_t(0x04));
    writeReg(0x0E, uint8_t(0x00));
    writeReg(0x0F, uint8_t(0x30));
    writeReg(0x10, uint8_t(0x01));
    writeReg(0x11, uint8_t(0x50));
    writeReg(0x12, uint8_t(0x00));
    writeReg(0x13, uint8_t(0x06));
}

/**
 * @brief Configure SMUX for F5, F6, F7, F8, Clear, NIR channels
 * @details Maps photodiodes to ADC channels:
 *   ADC0 = F5, ADC1 = F6, ADC2 = F7, ADC3 = F8, ADC4 = Clear, ADC5 = NIR
 */
void Driver::F5F8_Clear_NIR() {
    writeReg(0x00, uint8_t(0x00));
    writeReg(0x01, uint8_t(0x00));
    writeReg(0x02, uint8_t(0x00));
    writeReg(0x03, uint8_t(0x40));
    writeReg(0x04, uint8_t(0x02));
    writeReg(0x05, uint8_t(0x00));
    writeReg(0x06, uint8_t(0x10));
    writeReg(0x07, uint8_t(0x03));
    writeReg(0x08, uint8_t(0x50));
    writeReg(0x09, uint8_t(0x10));
    writeReg(0x0A, uint8_t(0x03));
    writeReg(0x0B, uint8_t(0x00));
    writeReg(0x0C, uint8_t(0x00));
    writeReg(0x0D, uint8_t(0x00));
    writeReg(0x0E, uint8_t(0x24));
    writeReg(0x0F, uint8_t(0x00));
    writeReg(0x10, uint8_t(0x00));
    writeReg(0x11, uint8_t(0x50));
    writeReg(0x12, uint8_t(0x00));
    writeReg(0x13, uint8_t(0x06));
}

/**
 * @brief Configure SMUX for flicker detection
 */
void Driver::FDConfig() {
    writeReg(0x00, uint8_t(0x00));
    writeReg(0x01, uint8_t(0x00));
    writeReg(0x02, uint8_t(0x00));
    writeReg(0x03, uint8_t(0x00));
    writeReg(0x04, uint8_t(0x00));
    writeReg(0x05, uint8_t(0x00));
    writeReg(0x06, uint8_t(0x00));
    writeReg(0x07, uint8_t(0x00));
    writeReg(0x08, uint8_t(0x00));
    writeReg(0x09, uint8_t(0x00));
    writeReg(0x0A, uint8_t(0x00));
    writeReg(0x0B, uint8_t(0x00));
    writeReg(0x0C, uint8_t(0x00));
    writeReg(0x0D, uint8_t(0x00));
    writeReg(0x0E, uint8_t(0x00));
    writeReg(0x0F, uint8_t(0x00));
    writeReg(0x10, uint8_t(0x00));
    writeReg(0x11, uint8_t(0x00));
    writeReg(0x12, uint8_t(0x00));
    writeReg(0x13, uint8_t(0x60));
}

//==============================================================================
// CHANNEL DATA READING
//==============================================================================

uint16_t Driver::getChannelData(uint8_t channel) {
    uint8_t data[2] = {0};
    readReg(REG_CH0_DATA_L + channel * 2, data, 1);
    readReg(REG_CH0_DATA_H + channel * 2, data + 1, 1);
    return (static_cast<uint16_t>(data[1]) << 8) | data[0];
}


//==============================================================================
// DEBUG/STATUS
//==============================================================================

void Driver::printStatus(int index) {
    uint8_t status1 = readReg(REG_STATUS_1);
    uint8_t status2 = readReg(REG_STATUS_2);
    uint8_t status3 = readReg(REG_STATUS_3);
    uint8_t enable = readReg(REG_ENABLE);

    Log.info("AS7341 Status [%d]: EN=0x%02X, S1=0x%02X, S2=0x%02X, S3=0x%02X",
             index, enable, status1, status2, status3);
}

//==============================================================================
// LOW-LEVEL I2C FUNCTIONS
//==============================================================================

void Driver::writeReg(uint8_t reg, uint8_t data) {
    writeReg(reg, &data, 1);
}

void Driver::writeReg(uint8_t reg, void* pBuf, size_t size) {
    if (pBuf == nullptr) {
        return;
    }

    uint8_t* buffer = static_cast<uint8_t*>(pBuf);

    pWire_->beginTransmission(address_);
    pWire_->write(reg);
    for (size_t i = 0; i < size; i++) {
        pWire_->write(buffer[i]);
    }
    pWire_->endTransmission();
}

uint8_t Driver::readReg(uint8_t reg) {
    uint8_t data = 0;
    readReg(reg, &data, 1);
    return data;
}

uint8_t Driver::readReg(uint8_t reg, void* pBuf, size_t size) {
    if (pBuf == nullptr) {
        return 0;
    }

    uint8_t* buffer = static_cast<uint8_t*>(pBuf);

    pWire_->beginTransmission(address_);
    pWire_->write(reg);
    if (pWire_->endTransmission() != 0) {
        lastError_ = ERR_DATA_BUS;
        return 0;
    }

    delayMicroseconds(10);
    pWire_->requestFrom(address_, static_cast<uint8_t>(size));

    for (size_t i = 0; i < size; i++) {
        buffer[i] = pWire_->read();
    }

    return static_cast<uint8_t>(size);
}

} // namespace AS7341
