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

#define CHANNEL (_wire_number == 1 ? Wire1 : Wire)

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
        CHANNEL.beginTransmission(TCS34725_ADDRESS);
        bytes_sent = CHANNEL.write(TCS34725_COMMAND_BIT | reg);
        if (bytes_sent != 1) {
            Serial.printlnf("Bad write command, try: %d, bytes sent: %d", tries, bytes_sent);
            delay(50);
        }
        else {
            bytes_sent = CHANNEL.write(value);
            if (bytes_sent != 1) {
                Serial.printlnf("Bad write value, try: %d, bytes sent: %d", tries, bytes_sent);
                delay(50);
            }
            else {
                result = CHANNEL.endTransmission();
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
    uint8_t result = 0xFF, bytes_sent;
    int tries = 0;

    while (result != 0 && ++tries < 6) {
      CHANNEL.beginTransmission(TCS34725_ADDRESS);
      bytes_sent = CHANNEL.write(TCS34725_COMMAND_BIT | reg);
      result = CHANNEL.endTransmission();

      if (result != 0 || bytes_sent != 1) {
          Serial.printlnf("Bad requestRead write, try: %d, result: %d, bytes_sent: %d", tries, result, bytes_sent);
          delay(50);
      }
    }

    if (result == 0) {
        tries = 0;
        while (result != number_of_bytes && ++tries < 6) {
            result = CHANNEL.requestFrom(TCS34725_ADDRESS, number_of_bytes);
            if (result != number_of_bytes) {
                Serial.printlnf("Bad readRequest requestFrom, bytes requested: %d, result: %d, try: %d", number_of_bytes, result, tries);
                while (CHANNEL.available()) {
                    Serial.print(CHANNEL.read());
                }
                Serial.println();
                delay(10);
            }
        }
        if (result == number_of_bytes) {
            tries = 0;
            result = CHANNEL.available();
            while (result != number_of_bytes && ++tries < 6) {
                Serial.printlnf("Waiting for readRequest response, bytes requested: %d, result: %d, try: %d", number_of_bytes, result, tries);
                delay(10);
                result = CHANNEL.available();
            }
            if (result == number_of_bytes) {
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
    if (requestRead(reg, 1)) {
        return CHANNEL.read();
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

  if (requestRead(reg, 2)) {
      t = CHANNEL.read();
      x = CHANNEL.read();
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
    write8(TCS34725_ENABLE, TCS34725_ENABLE_PON);
    delay(5);
    /*write8(TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN | TCS34725_ENABLE_WEN);*/
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
  /*uint8_t reg = 0;*/
  /*reg = read8(TCS34725_ENABLE);*/
  /*write8(TCS34725_ENABLE, reg & ~(TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN));*/
    write8(TCS34725_ENABLE, 0x00);
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
        /*CHANNEL.reset();
        delay(5);*/
        CHANNEL.begin();
        delay(5);

        /* Make sure we're actually connected */
        id = read8(TCS34725_ID);
        /*Serial.println("Reading sensor ID number");*/
        if ((id != 0x44) && (id != 0x10)) {
            if (++tries > 5) {
                CHANNEL.end();
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
    CHANNEL.end();
    pinMode(C4, INPUT);
    pinMode(C5, INPUT);
    pinMode(D0, INPUT);
    pinMode(D1, INPUT);
    return true;
}

/**************************************************************************/
/*!
    Sets the integration time for the TC34725
*/
/**************************************************************************/
void TCS34725::setIntegrationTime(tcs34725IntegrationTime_t it)
{
    int tries = 0;
    tcs34725IntegrationTime_t reg = (tcs34725IntegrationTime_t) read8(TCS34725_ATIME);
    if (reg == it) {
        return;
    }

    /* Update the timing register */
    while (reg != it && ++tries < 6) {
        write8(TCS34725_ATIME, it);
        reg = (tcs34725IntegrationTime_t) read8(TCS34725_ATIME);
        if (reg != it) {
            Serial.printlnf("Integration time not updated, retrying, old: %d, new: %d", reg, it);
        }
    }

    if (reg != it) {
        Serial.printlnf("Unable to update integration time, old: %d, new: %d", reg, it);
    }
    /* Update value placeholders */
    _tcs34725IntegrationTime = reg;
}

/**************************************************************************/
/*!
    Adjusts the gain on the TCS34725 (adjusts the sensitivity to light)
*/
/**************************************************************************/
void TCS34725::setGain(tcs34725Gain_t gain)
{
    int tries = 0;
    tcs34725Gain_t reg = (tcs34725Gain_t) (read8(TCS34725_CONTROL) & 0x03);
    if (reg == gain) {
        return;
    }

    /* Update the gain register */
    while (reg != gain && ++tries < 6) {
        write8(TCS34725_CONTROL, gain);
        reg = (tcs34725Gain_t) (read8(TCS34725_CONTROL) & 0x03);
        if (reg != gain) {
            Serial.printlnf("Gain not updated, retrying, old: %d, new: %d", reg, gain);
        }
    }

    if (reg != gain) {
        Serial.printlnf("Unable to update gain, old: %d, new: %d", reg, gain);
    }
    /* Update value placeholders */
    _tcs34725Gain = reg;
}

/**************************************************************************/
/*!
    @brief  Reads the raw red, green, blue and clear channel values
*/
/**************************************************************************/
#define SENSOR_STABILITY_THRESHOLD 0
#define SENSOR_WAIT_MAXIMUM_CYCLES 50

void TCS34725::getRawData (BrevitestSensorRecord *reading, int stability, int it_delay)
{
    int tries;
    uint16_t clear, red, green, blue;
    uint16_t old_clear = 0;
    uint16_t old_red = 0;
    uint16_t old_green = 0;
    uint16_t old_blue = 0;
    int reading_count = 0;
    bool ready = false;
    bool stable;
    uint8_t state;
    unsigned long duration;

    while (stability == 0 || ++reading_count < stability) {
        duration = millis();
        ready = false;
        tries = 0;
        while (!ready && ++tries <= SENSOR_WAIT_MAXIMUM_CYCLES) {
            state = read8(TCS34725_STATUS);
            /*if (stability) Serial.printlnf("Sensor state: %d, try %d", state, tries);*/
            ready = (state & 0x01) == 1;
            if (!ready) {
                /*if (debug) Serial.print(".");*/
                delay(20);
            }
            else {
                duration = millis() - duration;
            }
        }

        if (ready) {
            /*Serial.println("Successful sensor read");*/
            clear = read16(TCS34725_CDATAL);
            red = read16(TCS34725_RDATAL);
            green = read16(TCS34725_GDATAL);
            blue = read16(TCS34725_BDATAL);
            if (stability) Serial.printlnf("Sensor reading -> count: %d, C: %d, R: %d, G: %d, B: %d", reading_count, clear, red, green, blue);
            stable = abs(clear - old_clear) <= SENSOR_STABILITY_THRESHOLD &&
                        abs(red - old_red) <= SENSOR_STABILITY_THRESHOLD &&
                        abs(green - old_green) <= SENSOR_STABILITY_THRESHOLD &&
                        abs(blue - old_blue) <= SENSOR_STABILITY_THRESHOLD;
            if (stability == 0 || (reading_count > 1 && stable)) {
                reading->time_ms = millis();
                reading->samples = reading_count;
                reading->clear = clear;
                reading->red = red;
                reading->green = green;
                reading->blue = blue;
                break;
            }
            else {
                if (reading_count == (stability - 1)) {
                    Serial.printlnf("Sensor failed to stabilize, reading count: %d, new: %d, old: %d", reading_count, clear, old_clear);
                }
                old_clear = clear;
                old_red = red;
                old_green = green;
                old_blue = blue;
            }
            delay(it_delay);
        }
        else {
            Serial.printlnf("Unsuccessful sensor read, tries: %d, duration: %u, status: %d", tries, duration, state);
            reading->time_ms = millis();
            reading->samples = 0;
            reading->clear = reading->red = reading->green = reading->blue = 0;
        }
    }
}
