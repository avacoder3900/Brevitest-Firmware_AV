/*!
 *@file DFRobot_AS7341.cpp
 *@brief Define the basic structure of class DFRobot_AS7341, the implementation of the basic methods
 *@copyright   Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 *@license     The MIT license (MIT)
 *@author [fengli](li.feng@dfrobot.com)
 *@version  V1.0
 *@date  2020-07-16
 *@url https://github.com/DFRobot/DFRobot_AS7341
 */

#include "DFRobot_AS7341.h"

DFRobot_AS7341::DFRobot_AS7341(TwoWire *pWire)
{
    _pWire = pWire;
    _address = 0x39;
}
int DFRobot_AS7341::begin(eMode_t mode)
{
    _pWire->beginTransmission(_address);
    if (_pWire->endTransmission() != 0)
    {
        DBG("");
        DBG("bus data access error");
        DBG("");
        return ERR_DATA_BUS;
    }
    enableAS7341(true);
    measureMode = mode;
    return ERR_OK;
}

uint8_t DFRobot_AS7341::readID()
{
    uint8_t id;
    if (readReg(REG_AS7341_ID, &id, 1) == 0)
    {
        DBG("id read error");
        return 0;
    }
    else
    {
        return id;
    }
}

void DFRobot_AS7341::enableAS7341(bool on)
{
    uint8_t data;
    readReg(REG_AS7341_ENABLE, &data, 1);
    if (on == true)
    {
        data = data | 1;
    }
    else
    {
        data = data & (~1);
    }
    writeReg(REG_AS7341_ENABLE, &data, 1);
    delay(10);
}

void DFRobot_AS7341::enableSpectralMeasure(bool on)
{
    uint8_t data;
    readReg(REG_AS7341_ENABLE, &data, 1);
    if (on == true)
    {
        data = data | (1 << 1);
    }
    else
    {
        data = data & (~(1 << 1));
    }
    writeReg(REG_AS7341_ENABLE, &data, 1);
}

void DFRobot_AS7341::enableSMUX(bool on)
{
    uint8_t data;
    readReg(REG_AS7341_ENABLE, &data, 1);
    if (on == true)
    {
        data = data | (1 << 4);
    }
    else
    {
        data = data & (~(1 << 4));
    }
    writeReg(REG_AS7341_ENABLE, &data, 1);
}

void DFRobot_AS7341::config(eMode_t mode)
{
    uint8_t data;
    setBank(1);
    readReg(REG_AS7341_CONFIG, &data, 1);
    switch (mode)
    {
    case eSpm:
    {
        data = (data & (~3)) | eSpm;
    };
    break;
    case eSyns:
    {
        data = (data & (~3)) | eSyns;
    };
    break;
    case eSynd:
    {
        data = (data & (~3)) | eSynd;
    };
    break;
    default:
        break;
    }
    writeReg(REG_AS7341_CONFIG, &data, 1);
    setBank(0);
}

void DFRobot_AS7341::F1F4_Clear_NIR()
{
    writeReg(0x00, 0x30);
    writeReg(0x01, 0x01);
    writeReg(0x02, 0x00);
    writeReg(0x03, 0x00);
    writeReg(0x04, 0x00);
    writeReg(0x05, 0x42);
    writeReg(0x06, 0x00);
    writeReg(0x07, 0x00);
    writeReg(0x08, 0x50);
    writeReg(0x09, 0x00);
    writeReg(0x0A, 0x00);
    writeReg(0x0B, 0x00);
    writeReg(0x0C, 0x20);
    writeReg(0x0D, 0x04);
    writeReg(0x0E, 0x00);
    writeReg(0x0F, 0x30);
    writeReg(0x10, 0x01);
    writeReg(0x11, 0x50);
    writeReg(0x12, 0x00);
    writeReg(0x13, 0x06);
}

void DFRobot_AS7341::F5F8_Clear_NIR()
{
    writeReg(byte(0x00), byte(0x00));
    writeReg(byte(0x01), byte(0x00));
    writeReg(byte(0x02), byte(0x00));
    writeReg(byte(0x03), byte(0x40));
    writeReg(byte(0x04), byte(0x02));
    writeReg(byte(0x05), byte(0x00));
    writeReg(byte(0x06), byte(0x10));
    writeReg(byte(0x07), byte(0x03));
    writeReg(byte(0x08), byte(0x50));
    writeReg(byte(0x09), byte(0x10));
    writeReg(byte(0x0A), byte(0x03));
    writeReg(byte(0x0B), byte(0x00));
    writeReg(byte(0x0C), byte(0x00));
    writeReg(byte(0x0D), byte(0x00));
    writeReg(byte(0x0E), byte(0x24));
    writeReg(byte(0x0F), byte(0x00));
    writeReg(byte(0x10), byte(0x00));
    writeReg(byte(0x11), byte(0x50));
    writeReg(byte(0x12), byte(0x00));
    writeReg(byte(0x13), byte(0x06));
}

void DFRobot_AS7341::FDConfig()
{

    writeReg(byte(0x00), byte(0x00));
    writeReg(byte(0x01), byte(0x00));
    writeReg(byte(0x02), byte(0x00));
    writeReg(byte(0x03), byte(0x00));
    writeReg(byte(0x04), byte(0x00));
    writeReg(byte(0x05), byte(0x00));
    writeReg(byte(0x06), byte(0x00));
    writeReg(byte(0x07), byte(0x00));
    writeReg(byte(0x08), byte(0x00));
    writeReg(byte(0x09), byte(0x00));
    writeReg(byte(0x0A), byte(0x00));
    writeReg(byte(0x0B), byte(0x00));
    writeReg(byte(0x0C), byte(0x00));
    writeReg(byte(0x0D), byte(0x00));
    writeReg(byte(0x0E), byte(0x00));
    writeReg(byte(0x0F), byte(0x00));
    writeReg(byte(0x10), byte(0x00));
    writeReg(byte(0x11), byte(0x00));
    writeReg(byte(0x12), byte(0x00));
    writeReg(byte(0x13), byte(0x60));
}

void DFRobot_AS7341::startMeasure(eChChoose_t mode)
{
    uint8_t data = 0;

    readReg(REG_AS7341_CFG_0, &data, 1);
    data = data & (~(1 << 4));
    writeReg(REG_AS7341_CFG_0, &data, 1);

    enableSpectralMeasure(false);
    writeReg(0xAF, 0x10);
    if (mode == eF1F4ClearNIR)
        F1F4_Clear_NIR();
    else if (mode == eF5F8ClearNIR)
        F5F8_Clear_NIR();
    enableSMUX(true);
    config(eSpm);
    enableSpectralMeasure(true);
}

bool DFRobot_AS7341::measureComplete()
{
    uint8_t status;
    readReg(REG_AS7341_STATUS_2, &status, 1);
    return ((status & (1 << 6)));
}

uint16_t DFRobot_AS7341::getChannelData(uint8_t channel)
{
    uint8_t data[2];
    uint16_t channelData = 0x0000;
    readReg(REG_AS7341_CH0_DATA_L + channel * 2, data, 1);
    readReg(REG_AS7341_CH0_DATA_H + channel * 2, data + 1, 1);
    channelData = data[1];
    channelData = (channelData << 8) | data[0];
    // delay(5);
    return channelData;
}

DFRobot_AS7341::sModeOneData_t DFRobot_AS7341::readSpectralDataOne()
{
    sModeOneData_t data;
    data.ADF1 = getChannelData(0);
    data.ADF2 = getChannelData(1);
    data.ADF3 = getChannelData(2);
    data.ADF4 = getChannelData(3);
    data.ADCLEAR = getChannelData(4);
    data.ADNIR = getChannelData(5);
    return data;
}

DFRobot_AS7341::sModeTwoData_t DFRobot_AS7341::readSpectralDataTwo()
{
    sModeTwoData_t data;
    data.ADF5 = getChannelData(0);
    data.ADF6 = getChannelData(1);
    data.ADF7 = getChannelData(2);
    data.ADF8 = getChannelData(3);
    data.ADCLEAR = getChannelData(4);
    data.ADNIR = getChannelData(5);
    return data;
}

void DFRobot_AS7341::setGpio(bool connect)
{
    uint8_t data;
    readReg(REG_AS7341_CPIO, &data, 1);
    if (connect == true)
    {
        data = data | (1 << 0);
    }
    else
    {
        data = data & (~(1 << 0));
    }
    writeReg(REG_AS7341_CPIO, &data, 1);
}

void DFRobot_AS7341::setGpioMode(uint8_t mode)
{
    uint8_t data;

    readReg(REG_AS7341_GPIO_2, &data, 1);

    if (mode == INPUT)
    {
        data = data | (1 << 2);
    }

    if (mode == OUTPUT)
    {
        data = data & (~(1 << 2));
    }
    writeReg(REG_AS7341_GPIO_2, &data, 1);
}

void DFRobot_AS7341::setBank(uint8_t addr)
{
    uint8_t data = 0;
    readReg(REG_AS7341_CFG_0, &data, 1);
    if (addr == 1)
    {

        data = data | (1 << 4);
    }

    if (addr == 0)
    {

        data = data & (~(1 << 4));
    }
    writeReg(REG_AS7341_CFG_0, &data, 1);
}

void DFRobot_AS7341::setInt(bool connect)
{
    uint8_t data;
    readReg(REG_AS7341_CPIO, &data, 1);
    if (connect == true)
    {
        data = data | (1 << 1);
    }
    else
    {
        data = data & (~(1 << 1));
    }
    writeReg(REG_AS7341_CPIO, &data, 1);
}

void DFRobot_AS7341::endSleep()
{
    uint8_t data;
    readReg(REG_AS7341_INTENAB, &data, 1);
    data = data | (1 << 3);
    writeReg(REG_AS7341_INTENAB, &data, 1);
}

void DFRobot_AS7341::clearFIFO()
{
    uint8_t data;
    readReg(REG_AS7341_CONTROL, &data, 1);
    data = data | (1 << 0);
    data = data & (~(1 << 0));
    writeReg(REG_AS7341_CONTROL, &data, 1);
}

void DFRobot_AS7341::setAtime(uint8_t value)
{
    writeReg(REG_AS7341_ATIME, &value, 1);
}

void DFRobot_AS7341::setAGAIN(uint8_t value)
{
    if (value > 10)
        value = 10;
    writeReg(REG_AS7341_CFG_1, &value, 1);
}

void DFRobot_AS7341::setAstep(uint16_t value)
{
    uint8_t highValue, lowValue;
    lowValue = value & 0x00ff;
    highValue = value >> 8;
    writeReg(REG_AS7341_ASTEP_L, &lowValue, 1);

    writeReg(REG_AS7341_ASTEP_H, &highValue, 1);
}

float DFRobot_AS7341::getIntegrationTime()
{

    uint8_t data;
    uint8_t astepData[2] = {0};
    uint16_t astep;
    readReg(REG_AS7341_ATIME, &data, 1);
    readReg(REG_AS7341_ASTEP_L, astepData, 1);
    readReg(REG_AS7341_ASTEP_L, astepData + 1, 1);
    astep = astepData[1];
    astep = (astep << 8) | astepData[0];
    if (data == 0)
    {
    }
    else if (data > 0 && data < 255)
    {
    }
    else if (data == 255)
    {
    }
    return 0;
}

void DFRobot_AS7341::setWtime(uint8_t value)
{
    writeReg(REG_AS7341_WTIME, &value, 1);
}
float DFRobot_AS7341::getWtime()
{
    float value;
    uint8_t data;
    readReg(REG_AS7341_WTIME, &data, 1);
    if (data == 0)
    {
        value = 2.78;
    }
    else if (data == 1)
    {
        value = 5.56;
    }
    else if (data > 1 && data < 255)
    {
        value = 2.78 * (data + 1);
    }
    else if (data == 255)
    {
        value = 711;
    }
    return value;
}

void DFRobot_AS7341::writeReg(uint8_t reg, uint8_t data)
{
    writeReg(reg, &data, 1);
}
void DFRobot_AS7341::writeReg(uint8_t reg, void *pBuf, size_t size)
{
    if (pBuf == NULL)
    {
        DBG("pBuf ERROR!! : null pointer");
    }
    uint8_t *_pBuf = (uint8_t *)pBuf;
    _pWire->beginTransmission(_address);
    _pWire->write(reg);
    for (uint16_t i = 0; i < size; i++)
    {
        _pWire->write(_pBuf[i]);
    }
    _pWire->endTransmission();
    // delay(1);
}
uint8_t DFRobot_AS7341::readReg(uint8_t reg)
{

    uint8_t data;
    readReg(reg, &data, 1);
    return data;
}
uint8_t DFRobot_AS7341::readReg(uint8_t reg, void *pBuf, size_t size)
{
    if (pBuf == NULL)
    {
        DBG("pBuf ERROR!! : null pointer");
    }
    uint8_t *_pBuf = (uint8_t *)pBuf;
    _pWire->beginTransmission(_address);
    _pWire->write(reg);
    if (_pWire->endTransmission() != 0)
    {
        return 0;
    }
    delayMicroseconds(10);
    _pWire->requestFrom(_address, size);
    for (uint16_t i = 0; i < size; i++)
    {
        _pBuf[i] = _pWire->read();
    }
    return size;
}
