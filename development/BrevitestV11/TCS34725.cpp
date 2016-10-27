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
void TCS34725::write8 (uint8_t reg, uint32_t value)
{
  THIS_WIRE.beginTransmission(TCS34725_ADDRESS);
  THIS_WIRE.write(TCS34725_COMMAND_BIT | reg);
  THIS_WIRE.write(value & 0xFF);
  THIS_WIRE.endTransmission();
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
  write8(TCS34725_ENABLE, TCS34725_ENABLE_PON);
  delay(3);
  write8(TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN);
  _is_enabled = true;
}

/**************************************************************************/
/*!
    Disables the device (putting it in lower power sleep mode)
*/
/**************************************************************************/
void TCS34725::disable(void)
{
  /* Turn the device off to save power */
  uint8_t reg = 0;
  reg = read8(TCS34725_ENABLE);
  write8(TCS34725_ENABLE, reg & ~(TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN));
  _is_enabled = false;
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
  _tcs34725Gain = TCS34725_GAIN_1X;
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
      return false;
    }

    /* Note: by default, the device is in power down mode on bootup */
    setIntegrationTime(it);
    setGain(gain);

    enable();

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
      return false;
    }

    /* Note: by default, the device is in power down mode on bootup */
    setIntegrationTime(_tcs34725IntegrationTime);
    setGain(_tcs34725Gain);

    enable();

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
void TCS34725::getRawData (uint16_t *r, uint16_t *g, uint16_t *b, uint16_t *c)
{
  *c = read16(TCS34725_CDATAL);
  *r = read16(TCS34725_RDATAL);
  *g = read16(TCS34725_GDATAL);
  *b = read16(TCS34725_BDATAL);

  /* Set a delay for the integration time */
  switch (_tcs34725IntegrationTime)
  {
    case TCS34725_INTEGRATIONTIME_2_4MS:
      delay(3);
      break;
    case TCS34725_INTEGRATIONTIME_24MS:
      delay(24);
      break;
    case TCS34725_INTEGRATIONTIME_50MS:
      delay(50);
      break;
    case TCS34725_INTEGRATIONTIME_101MS:
      delay(101);
      break;
    case TCS34725_INTEGRATIONTIME_154MS:
      delay(154);
      break;
    case TCS34725_INTEGRATIONTIME_700MS:
      delay(700);
      break;
  }
}

void TCS34725::setInterrupt(boolean i) {
  uint8_t r = read8(TCS34725_ENABLE);
  if (i) {
    r |= TCS34725_ENABLE_AIEN;
  } else {
    r &= ~TCS34725_ENABLE_AIEN;
  }
  write8(TCS34725_ENABLE, r);
}

void TCS34725::clearInterrupt(void) {
  THIS_WIRE.beginTransmission(TCS34725_ADDRESS);
  THIS_WIRE.write(TCS34725_COMMAND_BIT | 0x66);
  THIS_WIRE.endTransmission();
}


void TCS34725::setIntLimits(uint16_t low, uint16_t high) {
   write8(0x04, low & 0xFF);
   write8(0x05, low >> 8);
   write8(0x06, high & 0xFF);
   write8(0x07, high >> 8);
}
