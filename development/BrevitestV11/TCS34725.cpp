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
    uint8_t result = 0xFF, bytes_sent = 0;
    int tries = 0;

    while (result != 0 && bytes_sent != 1 && ++tries < 6) {
        THIS_WIRE.beginTransmission(TCS34725_ADDRESS);
        bytes_sent = THIS_WIRE.write(TCS34725_COMMAND_BIT | reg);
        if (bytes_sent != 1) {
            Serial.printlnf("Bad write command, try: %d, bytes sent: %d", tries, bytes_sent);
            delay(50);
        }
        else {
            bytes_sent = THIS_WIRE.write(value);
            if (bytes_sent != 1) {
                Serial.printlnf("Bad write value, try: %d, bytes sent: %d", tries, bytes_sent);
                delay(50);
            }
            else {
                result = THIS_WIRE.endTransmission();
                if (result != 0) {
                    Serial.printlnf("Bad write endTransmission, %d, try: %d", result, tries);
                    delay(50);
                }
            }
        }
    }
}

/**************************************************************************/
/*!
    @brief  Request a register read over I2C
*/
/**************************************************************************/

boolean TCS34725::requestRead(uint8_t reg, uint8_t number_of_bytes) {
    uint8_t result = 0xFF, bytes_sent, byte_read;
    int tries = 0;

    while (result != 0 && ++tries < 6) {
      THIS_WIRE.beginTransmission(TCS34725_ADDRESS);
      bytes_sent = THIS_WIRE.write(TCS34725_COMMAND_BIT | reg);
      result = THIS_WIRE.endTransmission();

      if (result != 0 || bytes_sent != 1) {
          Serial.printlnf("Bad requestRead write, try: %d, result: %d, bytes_sent: %d", tries, result, bytes_sent);
          delay(50);
      }
    }

    if (result == 0) {
        tries = 0;
        while (result != number_of_bytes && ++tries < 6) {
            result = THIS_WIRE.requestFrom(TCS34725_ADDRESS, 1);
            if (result != number_of_bytes) {
                Serial.printlnf("Bad readRequest requestFrom, %d, try: %d, result: %d", tries, result);
                delay(10);
            }
        }
        if (result == number_of_bytes) {
            tries = 0;
            result = THIS_WIRE.available();
            while (result != number_of_bytes && ++tries < 6) {
                Serial.printlnf("Waiting for readRequest response, %d, try: %d, result: %d", tries, result);
                delay(10);
                result = THIS_WIRE.available();
            }
            if (result == 1) {
                return true;
            }
        }
    }

    return false;
}

/**************************************************************************/
/*!
    @brief  Reads an 8 bit value over I2C
*/
/**************************************************************************/

uint8_t TCS34725::read8(uint8_t reg)
{
    if (requestRead(reg, 1) {
        return THIS_WIRE.read();
    }
    else {
        return 0xFF;
    }
}

/**************************************************************************/
/*!
    @brief  Reads a 16 bit values over I2C
*/
/**************************************************************************/
uint16_t TCS34725::read16(uint8_t reg)
{
  uint16_t x; uint16_t t;

  if (requestRead(reg, 2) {
      t = THIS_WIRE.read();
      x = THIS_WIRE.read();
      return (x << 8) | t;
  }
  else {
      return 0xFFFF;
  }
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

/**************************************************************************/
/*!
    Initializes I2C and configures the sensor (call this function before
    doing anything else)
*/
/**************************************************************************/
boolean TCS34725::begin(tcs34725IntegrationTime_t it, tcs34725Gain_t gain)
{
    int tries = 0;
    uint8_t id = 0;

    while ((id != 0x44) && (id != 0x10))
    {
        if (THAT_WIRE.isEnabled()) {
            THAT_WIRE.end();
        }

        if (THIS_WIRE.isEnabled()) {
            THIS_WIRE.end();
            delay(5);
        }

        THIS_WIRE.begin();
        delay(5);

        /* Make sure we're actually connected */
        id = read8(TCS34725_ID);
        if ((id != 0x44) && (id != 0x10)) {
            if (++tries > 5) {
                return false;
            }
            Serial.printlnf("Not connected to sensor, try: %d, retrying...", tries);
        }

    }

    setIntegrationTime(it);
    setGain(gain);

    /* Note: by default, the device is in power down mode on bootup */
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
    if (ready) {
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
