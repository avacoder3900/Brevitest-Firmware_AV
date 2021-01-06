/***************************************************
  This is a library for our Adafruit 16-channel PWM & Servo driver

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/815

  These displays use I2C to communicate, 2 pins are required to
  interface. For Arduino UNOs, thats SCL -> Analog 5, SDA -> Analog 4

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include "Adafruit_PWMServoDriver.h"
#include "application.h"
#include "math.h"

Adafruit_PWMServoDriver::Adafruit_PWMServoDriver(uint8_t addr) {
  _i2caddr = addr;
}

void Adafruit_PWMServoDriver::begin(void) {
 Wire.begin();
 reset();
}


void Adafruit_PWMServoDriver::reset(void) {
 write8(PCA9685_MODE1, 0x0);
}

void Adafruit_PWMServoDriver::setPWMFreq(float freq) {
  Log.info("Attempting to set freq %f", freq);

  float prescaleval = 25000000;
  prescaleval /= 4096;
  prescaleval /= freq;
  prescaleval -= 1;
  Log.info("Estimated pre-scale: %f", prescaleval);
  uint8_t prescale = floor(prescaleval + 0.5);
  Log.info("Final pre-scale: %d", prescale);

  uint8_t oldmode = read8(PCA9685_MODE1);
  Log.info("Current mode %X", oldmode);
  uint8_t newmode = (oldmode&0x7F) | 0x10; // sleep
  write8(PCA9685_MODE1, newmode); // go to sleep
  write8(PCA9685_PRESCALE, prescale); // set the prescaler
  write8(PCA9685_MODE1, oldmode);
  delay(5);
  write8(PCA9685_MODE1, oldmode | 0xa1);  //  This sets the MODE1 register to turn on auto increment.
                                          // This is why the beginTransmission below was not working.
   Log.info("Mode now %X", read8(PCA9685_MODE1));
}

void Adafruit_PWMServoDriver::setPWM(uint8_t num, uint16_t on, uint16_t off) {
  Log.info("Setting PWM %d: %d -> %d", num, on, off);

  Wire.beginTransmission(_i2caddr);

  Wire.write(LED0_ON_L+4*num);
  Wire.write(on);
  Wire.write(on>>8);
  Wire.write(off);
  Wire.write(off>>8);

  Wire.endTransmission();
}

uint8_t Adafruit_PWMServoDriver::read8(uint8_t addr) {
  Wire.beginTransmission(_i2caddr);

  Wire.write(addr);

  Wire.endTransmission();

  Wire.requestFrom((uint8_t)_i2caddr, (uint8_t)1);

  return Wire.read();
}

void Adafruit_PWMServoDriver::write8(uint8_t addr, uint8_t d) {
  Wire.beginTransmission(_i2caddr);

  Wire.write(addr);
  Wire.write(d);

  Wire.endTransmission();
}
