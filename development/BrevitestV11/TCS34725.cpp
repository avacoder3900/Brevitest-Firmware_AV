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
    write8(TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN | TCS34725_ENABLE_WEN);
    /*write8(TCS34725_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN);*/
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
    /*write8(TCS34725_ENABLE, 0x00);*/
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
boolean TCS34725::setIntegrationTime(tcs34725IntegrationTime_t it)
{
    int tries = 0;
    tcs34725IntegrationTime_t reg = (tcs34725IntegrationTime_t) read8(TCS34725_ATIME);
    if (reg == it) {
        return false;
    }

    /*Serial.printlnf("Changing integration time from %d to %d", reg, it);*/
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
        return false;
    }
    /* Update value placeholders */
    _tcs34725IntegrationTime = reg;
    return true;
}

/**************************************************************************/
/*!
    Adjusts the gain on the TCS34725 (adjusts the sensitivity to light)
*/
/**************************************************************************/
boolean TCS34725::setGain(tcs34725Gain_t gain)
{
    int tries = 0;
    tcs34725Gain_t reg = (tcs34725Gain_t) (read8(TCS34725_CONTROL) & 0x03);
    if (reg == gain) {
        return true;
    }

    /*Serial.printlnf("Changing gain from %d to %d", reg, gain);*/
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
        return false;
    }
    /* Update value placeholders */
    _tcs34725Gain = reg;
    return true;
}

/**************************************************************************/
/*!
    @brief  Reads the raw red, green, blue and clear channel values
*/
/**************************************************************************/
#define SENSOR_STABILITY_THRESHOLD 0
#define SENSOR_WAIT_MAXIMUM_CYCLES 50

int integerSqrt(int n) {
    int shift, nShifted, result, candidateResult;

    if (n < 0) {
        return -1;
    }

    shift = 2;
    nShifted = n >> shift;
    while ((nShifted != 0) && (nShifted != n)) {
        shift += 2;
        nShifted = n >> shift;
    }
    shift -= 2;

    result = 0;
    while (shift >= 0) {
        result <<= 1;
        candidateResult = result + 1;
        if ((candidateResult * candidateResult) <= (n >> shift)) {
            result = candidateResult;
        }
        shift -= 2;
    }

    return result;
}

int TCS34725::takeReading (BrevitestSensorRecord *reading, int sample_tries, int it_delay, bool debug)
{
    int tries;
    uint16_t clear, red, green, blue, lvalue;
    int sum_clear = 0, sum_red = 0, sum_green = 0, sum_blue = 0, sum_lvalue = 0;
    int max_clear = 0, max_red = 0, max_green = 0, max_blue = 0, max_lvalue = 0;
    int min_clear = 65535, min_red = 65535, min_green = 65535, min_blue = 65535, min_lvalue = 65535;
    int count, iterations, samples = 0;
    bool ready = false;
    uint8_t state;
    unsigned long duration;

    /*enable();*/
    iterations = abs(sample_tries);
    for (count = 0; count < iterations; count++) {
        duration = millis();
        ready = false;
        tries = 0;
        while (!ready && ++tries <= SENSOR_WAIT_MAXIMUM_CYCLES) {
            state = read8(TCS34725_STATUS);
            /*if (debug) Serial.printlnf("Sensor state: %d, try %d", state, tries);*/
            ready = (state & 0x01) == 1;
            if (!ready) {
                if (debug) Serial.print(".");
                delay(20);
            }
        }

        if (ready) {
            /*Serial.println("Successful sensor read");*/
            clear = read16(TCS34725_CDATAL);
            red = read16(TCS34725_RDATAL);
            green = read16(TCS34725_GDATAL);
            blue = read16(TCS34725_BDATAL);
            if (clear) {
                lvalue = (10000 * integerSqrt((red * red) + (blue * blue) + (green * green))) / clear;
            }
            else {
                lvalue = 0;
            }
            if (sample_tries < 0) Serial.printlnf("Sensor reading -> count: %d, C: %d, R: %d, G: %d, B: %d, L: %d", count, clear, red, green, blue, lvalue);
            sum_clear += clear;
            sum_red += red;
            sum_green += green;
            sum_blue += blue;
            sum_lvalue += lvalue;
            if (clear > max_clear) max_clear = clear;
            if (red > max_red) max_red = red;
            if (green > max_green) max_green = green;
            if (blue > max_blue) max_blue = blue;
            if (lvalue > max_lvalue) max_lvalue = lvalue;
            if (clear < min_clear) min_clear = clear;
            if (red < min_red) min_red = red;
            if (green < min_green) min_green = green;
            if (blue < min_blue) min_blue = blue;
            if (lvalue < min_lvalue) min_lvalue = lvalue;
            samples++;

        }
        else {
            duration = millis() - duration;
            if (debug) Serial.printlnf("Unsuccessful sensor read, count: %d, tries: %d, duration: %u, status: %d", count, tries, duration, state);
        }

        delay(it_delay);
    }

    /*disable();*/

    reading->time_ms = millis();
    reading->samples = samples;
    reading->clear = sum_clear / samples;
    reading->red = sum_red / samples;
    reading->green = sum_green / samples;
    reading->blue = sum_blue / samples;
    max_clear = (100 * (max_clear - min_clear)) / reading->clear;
    max_red = (100 * (max_red - min_red)) / reading->red;
    max_green = (100 * (max_green - min_green)) / reading->green;
    max_blue = (100 * (max_blue - min_blue)) / reading->blue;
    max_lvalue = (100 * (max_lvalue - min_lvalue)) / lvalue;
    if (debug) Serial.printlnf("%c %d %u (%d, %d) (%d, %d) (%d, %d) (%d, %d) (%d, %d)", \
        reading->channel, reading->samples, reading->time_ms, reading->clear, max_clear, reading->red, max_red, reading->green, max_green, reading->blue, max_blue, lvalue, max_lvalue);

    return sum_lvalue / samples;
}
/*int TCS34725::getRawData (BrevitestSensorRecord *reading, int stability, int it_delay)
{
    int tries, lvalue = 0, old_lvalue = 0;
    uint16_t clear, red, green, blue;
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
            if (stability) Serial.printlnf("Sensor state: %d, try %d", state, tries);
            ready = (state & 0x01) == 1;
            if (!ready) {
                if (debug) Serial.print(".");
                delay(20);
            }
            else {
                duration = millis() - duration;
            }
        }

        if (ready) {
            Serial.println("Successful sensor read");
            clear = read16(TCS34725_CDATAL);
            red = read16(TCS34725_RDATAL);
            green = read16(TCS34725_GDATAL);
            blue = read16(TCS34725_BDATAL);
            lvalue = integerSqrt((red * red) + (blue * blue) + (green * green));
            if (stability) Serial.printlnf("Sensor reading -> count: %d, C: %d, R: %d, G: %d, B: %d, L: %d", reading_count, clear, red, green, blue, lvalue);
            stable = abs(lvalue - old_lvalue) <= SENSOR_STABILITY_THRESHOLD;
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
                    Serial.printlnf("Sensor failed to stabilize, reading count: %d, new: %d, old: %d", reading_count, lvalue, old_lvalue);
                }
                old_lvalue = lvalue;
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

    return lvalue;
}*/
