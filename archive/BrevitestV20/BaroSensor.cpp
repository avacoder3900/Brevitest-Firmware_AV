/* BaroSensor Arduino library, for Freetronics BARO module (MS5637-02BA03)
 * http://www.freetronics.com/baro
 *
 * Copyright (C)2014 Freetronics Pty Ltd. Licensed under GNU GPLv3 as described in the LICENSE file.
 *
 * Written by Angus Gratton (angus at freetronics dot com)
 */

#include "BaroSensor.h"
#include "application.h"

/* i2c address of module */
#define BARO_ADDR 0x76

/* delay to wait for sampling to complete, on each OSR level */
const uint8_t SamplingDelayMs[6] PROGMEM = {
  2,
  4,
  6,
  10,
  18,
  34
};

/* module commands */
#define CMD_RESET 0x1E
#define CMD_PROM_READ(offs) (0xA0+(offs<<1)) /* Offset 0-7 */
#define CMD_START_D1(oversample_level) (0x40 + 2*(int)oversample_level)
#define CMD_START_D2(oversample_level) (0x50 + 2*(int)oversample_level)
#define CMD_READ_ADC 0x00

BaroSensorClass BaroSensor;

void BaroSensorClass::begin()
{
  Wire1.begin();
  Wire1.beginTransmission(BARO_ADDR);
  Wire1.write(CMD_RESET);
  err = Wire1.endTransmission();
  if(err) return;

  uint16_t prom[7];
  for(int i = 0; i < 7; i++) {
    Wire1.beginTransmission(BARO_ADDR);
    Wire1.write(CMD_PROM_READ(i));
    err = Wire1.endTransmission(false);
    if(err)
      return;
    int req = Wire1.requestFrom(BARO_ADDR, 2);
    if(req != 2) {
      err = ERR_BAD_READLEN;
      Serial.printlnf("Barometric pressure sensor error, %d", err);
      return;
    }
    prom[i] = ((uint16_t)Wire1.read()) << 8;
    prom[i] |= Wire1.read();
  }

  // TODO verify CRC4 in top 4 bits of prom[0] (follows AN520 but not directly...)

  c1 = prom[1];
  c2 = prom[2];
  c3 = prom[3];
  c4 = prom[4];
  c5 = prom[5];
  c6 = prom[6];
  initialised = true;
}

int BaroSensorClass::getTemperature(TempUnit scale, BaroOversampleLevel level)
{
  int result;
  if(getTempAndPressure(&result, NULL, scale, level))
    return result;
  else
    return -1;
}

int BaroSensorClass::getPressure(BaroOversampleLevel level)
{
  int result;
  if(getTempAndPressure(NULL, &result, CELSIUS, level))
    return result;
  else
    return -1;
}

bool BaroSensorClass::getTempAndPressure(int *temperature, int *pressure, TempUnit tempScale, BaroOversampleLevel level)
{
    if (err || !initialised) {
        return false;
    }

    int64_t d2 = takeReading(CMD_START_D2(level), level);
    if (d2 == 0) {
        return false;
    }

    int64_t dT = d2 - ((int64_t) c5 << 8);
    *temperature = (2000 + ((dT * (int64_t) c6) >> 23)) / 10;
    if (tempScale == FAHRENHEIT) {
      *temperature = *temperature * 9 / 5 + 32;
    }

    if(pressure != NULL) {
        int64_t d1 = takeReading(CMD_START_D1(level), level);
        if (d1 == 0) {
            return false;
        }

        int64_t OFF  = ((int64_t) c2 << 17) + ((dT * (int64_t) c4) >> 6);
        int64_t SENS = ((int64_t) c1 << 16) + ((dT * (int64_t) c3) >> 7);
        *pressure = (((((d1 * SENS) >> 21) - OFF) >> 15) + 2000) * 2953 / 1000000;
    }

    return true;
}

int BaroSensorClass::takeReading(uint8_t trigger_cmd, BaroOversampleLevel oversample_level)
{
  Wire1.beginTransmission(BARO_ADDR);
  Wire1.write(trigger_cmd);
  err = Wire1.endTransmission();

  if(err)
    return 0;
  uint8_t sampling_delay = pgm_read_byte(SamplingDelayMs + (int)oversample_level);
  delay(sampling_delay);

  Wire1.beginTransmission(BARO_ADDR);
  Wire1.write(CMD_READ_ADC);
  err = Wire1.endTransmission(false);
  if(err)
    return 0;
  int req = Wire1.requestFrom(BARO_ADDR, 3);
  if(req != 3)
    req = Wire1.requestFrom(BARO_ADDR, 3); // Sometimes first read fails...?
  if(req != 3) {
    err = ERR_BAD_READLEN;
    return 0;
  }
  uint32_t result = (uint32_t)Wire1.read() << 16;
  result |= (uint32_t)Wire1.read() << 8;
  result |= Wire1.read();
  return result;
}

void BaroSensorClass::dumpDebugOutput()
{
  Serial.print(F("C1 = 0x"));
  Serial.println(c1, HEX);
  Serial.print(F("C2 = 0x"));
  Serial.println(c2, HEX);
  Serial.print(F("C3 = 0x"));
  Serial.println(c3, HEX);
  Serial.print(F("C4 = 0x"));
  Serial.println(c4, HEX);
  Serial.print(F("C5 = 0x"));
  Serial.println(c5, HEX);
  Serial.print(F("C6 = 0x"));
  Serial.println(c6, HEX);
  Serial.print(F("d1 first = 0x"));
  Serial.println(takeReading(CMD_START_D1(OSR_8192), OSR_8192));
  Serial.print(F("d2 first = 0x"));
  Serial.println(takeReading(CMD_START_D2(OSR_8192), OSR_8192));
  Serial.print(F("d1 second = 0x"));
  Serial.println(takeReading(CMD_START_D1(OSR_8192), OSR_8192));
  Serial.print(F("d2 second = 0x"));
  Serial.println(takeReading(CMD_START_D2(OSR_8192), OSR_8192));
  Serial.print(F("d1 third = 0x"));
  Serial.println(takeReading(CMD_START_D1(OSR_8192), OSR_8192));
  Serial.print(F("d2 third = 0x"));
  Serial.println(takeReading(CMD_START_D2(OSR_8192), OSR_8192));
  int temp, pressure;
  bool res = getTempAndPressure(&temp, &pressure);
  Serial.print(F("result (fourth) = "));
  Serial.println(res ? F("OK") : F("ERR"));
  Serial.print(F("Temp (fourth) = "));
  Serial.println(temp);
  Serial.print(F("Pressure (fourth) = "));
  Serial.println(pressure);
  Serial.print(F("Error (fourth) = "));
  Serial.println(err);
}
