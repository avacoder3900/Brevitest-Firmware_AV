/**************************************************************************/
/*!
    @file     TCS34725.cpp
    @author   KTOWN (Adafruit Industries)
    @license  BSD (see license.txt)

    Driver for the TCS34725 digital color sensors.

    Adafruit invests time and resources providing this open source code,
    please support Adafruit and open-source hardware by purchasing
    products from Adafruit!

    @section  HISTORY

    v1.0 - First release
*/
/**************************************************************************/

#include <stdlib.h>
#include "TCS34725.h"

#define THIS_WIRE (_wire_number == 1 ? Wire1 : Wire)
#define THAT_WIRE (_wire_number == 1 ? Wire : Wire1)

/*========================================================================*/
/*                          PRIVATE FUNCTIONS                             */
/*========================================================================*/

/**************************************************************************/
/*!
    @brief  Writes a register and an 8 bit value over I2C
*/
/**************************************************************************/
void TCS34725::write8 (uint8_t reg, uint8_t value)
{
    uint8_t result = 0xFF, bytes_sent;
    int tries = 0;

    while (result != 0 && ++tries < 6) {
        THIS_WIRE.beginTransmission(TCS34725_ADDRESS);
        THIS_WIRE.write(TCS34725_COMMAND_BIT | reg);
        /*Serial.printlnf("Command %d, sent %d", TCS34725_COMMAND_BIT | reg, bytes_sent);*/
        bytes_sent = THIS_WIRE.write(value);
        /*Serial.printlnf("Data %d, sent %d", value, bytes_sent);*/
        result = THIS_WIRE.endTransmission();
        if (result != 0) {
            Serial.printlnf("Bad transmission, %d, try = %d", result, tries );
        }
    }
}

/**************************************************************************/
/*!
    @brief  Reads an 8 bit value over I2C
*/
/**************************************************************************/
uint8_t TCS34725::read8(uint8_t reg)
{
  THIS_WIRE.beginTransmission(TCS34725_ADDRESS);
  THIS_WIRE.write(TCS34725_COMMAND_BIT | reg);
  THIS_WIRE.endTransmission();

  THIS_WIRE.requestFrom(TCS34725_ADDRESS, 1);
  return THIS_WIRE.read();
}

/**************************************************************************/
/*!
    @brief  Reads a 16 bit values over I2C
*/
/**************************************************************************/
uint16_t TCS34725::read16(uint8_t reg)
{
  uint16_t x; uint16_t t;

  THIS_WIRE.beginTransmission(TCS34725_ADDRESS);
  THIS_WIRE.write(TCS34725_COMMAND_BIT | reg);
  THIS_WIRE.endTransmission();

  THIS_WIRE.requestFrom(TCS34725_ADDRESS, 2);
  t = THIS_WIRE.read();
  x = THIS_WIRE.read();
  x <<= 8;
  x |= t;
  return x;
}

/**************************************************************************/
/*!
    Enables the device
*/
/**************************************************************************/
void TCS34725::enable(void)
{
    if (!_is_enabled) {
        /*Serial.printlnf("Enabling sensor %d", _wire_number);*/
        write8(TCS34725_ENABLE, TCS34725_ENABLE_PON);
        delay(5);
        write8(TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN);
        _is_enabled = true;
    }
}

/**************************************************************************/
/*!
    Disables the device (putting it in lower power sleep mode)
*/
/**************************************************************************/
void TCS34725::disable(void)
{
  /* Turn the device off to save power */
  /*uint8_t reg = 0;*/
  /*reg = read8(TCS34725_ENABLE);*/
  /*write8(TCS34725_ENABLE, reg & ~(TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN));*/
  if (_is_enabled) {
      write8(TCS34725_ENABLE, 0x00);
      _is_enabled = false;
  }
}

/**************************************************************************/
/*!
    Enables the device
*/
/**************************************************************************/
boolean TCS34725::isEnabled(void)
{
  return _is_enabled;
}

/*========================================================================*/
/*                            CONSTRUCTORS                                */
/*========================================================================*/

/**************************************************************************/
/*!
    Constructor
*/
/**************************************************************************/
TCS34725::TCS34725(uint8_t sensor_number)
{
  _tcs34725IntegrationTime = TCS34725_INTEGRATIONTIME_154MS;
  _tcs34725Gain = TCS34725_GAIN_4X;
  _wire_number = sensor_number;
  _is_enabled = false;
}

/*========================================================================*/
/*                           PUBLIC FUNCTIONS                             */
/*========================================================================*/

/**************************************************************************/
/*!
    Initializes I2C and configures the sensor (call this function before
    doing anything else)
*/
/**************************************************************************/
boolean TCS34725::begin(tcs34725IntegrationTime_t it, tcs34725Gain_t gain)
{
    /*if (THAT_WIRE.isEnabled()) {
        THAT_WIRE.end();
    }

    if (THIS_WIRE.isEnabled()) {
        THIS_WIRE.end();
        delay(5);
    }
*/
    THIS_WIRE.begin();
    delay(5);

    /* Make sure we're actually connected */
    uint8_t x = read8(TCS34725_ID);
    if ((x != 0x44) && (x != 0x10))
    {
        Serial.println("Not connected to sensor");
      return false;
    }

    /* Note: by default, the device is in power down mode on bootup */
    enable();

    setIntegrationTime(it);
    setGain(gain);

  return true;
}

boolean TCS34725::begin(void)
{
    if (THAT_WIRE.isEnabled()) {
        THAT_WIRE.end();
    }

    if (THIS_WIRE.isEnabled()) {
        THIS_WIRE.end();
        delay(5);
    }

    THIS_WIRE.begin();

    /* Make sure we're actually connected */
    uint8_t x = read8(TCS34725_ID);
    if ((x != 0x44) && (x != 0x10))
    {
        Serial.println("Not connected to sensor");
      return false;
    }

    /* Note: by default, the device is in power down mode on bootup */
    enable();

    setIntegrationTime(_tcs34725IntegrationTime);
    setGain(_tcs34725Gain);

  return true;
}

/**************************************************************************/
/*!
    Releases the I2C bus (call this function after
    doing everything else)
*/
/**************************************************************************/
boolean TCS34725::end(void)
{
    disable();
    THIS_WIRE.end();
    if (_wire_number == 1) {
        pinMode(C4, INPUT);
        pinMode(C5, INPUT);
    }
    else {
        pinMode(D0, INPUT);
        pinMode(D1, INPUT);
    }
    return true;
}

/**************************************************************************/
/*!
    Sets the integration time for the TC34725
*/
/**************************************************************************/
void TCS34725::setIntegrationTime(tcs34725IntegrationTime_t it)
{
    if (it == _tcs34725IntegrationTime) {
        return;
    }
  /* Update the timing register */
  write8(TCS34725_ATIME, it);

  /* Update value placeholders */
  _tcs34725IntegrationTime = it;
}

/**************************************************************************/
/*!
    Adjusts the gain on the TCS34725 (adjusts the sensitivity to light)
*/
/**************************************************************************/
void TCS34725::setGain(tcs34725Gain_t gain)
{
    if (gain == _tcs34725Gain) {
        return;
    }
  /* Update the timing register */
  write8(TCS34725_CONTROL, gain);

  /* Update value placeholders */
  _tcs34725Gain = gain;
}

/**************************************************************************/
/*!
    @brief  Reads the raw red, green, blue and clear channel values
*/
/**************************************************************************/
void TCS34725::getRawData (tcs34725IntegrationTime_t it, tcs34725Gain_t gain, uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c)
{
    int tries = 50;
    bool ready = false;
    uint8_t state;

    begin(it, gain);

    while (!ready && --tries > 0) {
        state = read8(TCS34725_STATUS);
        /*Serial.printlnf("Sensor state: %d, try %d", state, 10-tries);*/
        ready = (state & 0x01) == 1;
        if (!ready) {
            /*Serial.print(".");*/
            delay(20);
        }
    }
    if (tries > 0) {
        /*Serial.println("Successful sensor read");*/
        *c = read16(TCS34725_CDATAL);
        *r = read16(TCS34725_RDATAL);
        *g = read16(TCS34725_GDATAL);
        *b = read16(TCS34725_BDATAL);
    }
    else {
        Serial.println("Unsuccessful sensor read");
        *c = *r = *g = *b = 0xFFFF;
    }

    end();
}
