/*
 * SoftI2CMaster.cpp -- Multi-instance software I2C Master library
 *
 *
 * 2010-12 Tod E. Kurt, http://todbot.com/blog/
 *
 * This code takes some tricks from:
 *  http://codinglab.blogspot.com/2008/10/i2c-on-avr-using-bit-banging.html
 *
 *
 *  Modifications 2015 by Leo Linbeck III
 *
 */

 #if defined(PLATFORM_ID)  //Only defined if a Particle device
   #include "application.h"
 /*#if (PLATFORM_ID == 0)  // Core
   #define pinLO(_pin) (PIN_MAP[_pin].gpio_peripheral->BRR = PIN_MAP[_pin].gpio_pin)
   #define pinHI(_pin) (PIN_MAP[_pin].gpio_peripheral->BSRR = PIN_MAP[_pin].gpio_pin)
 #elif (PLATFORM_ID == 6) // Photon
   static STM32_Pin_Info* PIN_MAP2 = HAL_Pin_Map(); // Pointer required for highest access speed
   #define pinLO(_pin) (PIN_MAP2[_pin].gpio_peripheral->BSRRH = PIN_MAP2[_pin].gpio_pin)
   #define pinHI(_pin) (PIN_MAP2[_pin].gpio_peripheral->BSRRL = PIN_MAP2[_pin].gpio_pin)
 #else
   #error "*** PLATFORM_ID not supported by this library. PLATFORM should be Core or Photon ***"
 #endif*/
 #endif

#include "SoftI2CMaster.h"

#include <string.h>

#define  i2cbitdelay 500

#define  I2C_ACK  0
#define  I2C_NAK  1

//
// Constructor
//
SoftI2CMaster::SoftI2CMaster()
{
}

SoftI2CMaster::SoftI2CMaster(uint8_t sdaPin, uint8_t sclPin)
{
    setPins(sdaPin, sclPin, true);
    i2c_init();
}

//
SoftI2CMaster::SoftI2CMaster(uint8_t sdaPin, uint8_t sclPin, uint8_t pullups)
{
    setPins(sdaPin, sclPin, pullups);
    i2c_init();
}

//
// Turn Arduino pin numbers into PORTx, DDRx, and PINx
//
void SoftI2CMaster::setPins(uint8_t sdaPin, uint8_t sclPin, uint8_t pullups)
{
    usePullups = pullups;

    _sdaPin = sdaPin;
    _sclPin = sclPin;
}

//
//
//

void SoftI2CMaster::beginWriteTransmission(uint8_t address, uint8_t reg)
{
  i2c_start();

  i2c_write((address<<1) | 0); // clr read bit
  i2c_write(reg);
}

void SoftI2CMaster::beginWriteTransmission(int address, uint8_t reg)
{
    beginWriteTransmission((uint8_t) address, reg);
}

void SoftI2CMaster::beginReadTransmission(uint8_t address, uint8_t reg)
{
  i2c_start();

  i2c_write((address<<1) | 0); // clr read bit
  i2c_write(reg);

  i2c_start();
  i2c_write((address<<1) | 1); // set read bit
}

void SoftI2CMaster::beginReadTransmission(int address,  uint8_t reg)
{
    beginReadTransmission( (uint8_t) address, reg);
}

//
//
//
void SoftI2CMaster::endTransmission(void)
{
  i2c_stop();
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
void SoftI2CMaster::write(uint8_t data)
{
    i2c_write(data);
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
void SoftI2CMaster::write(uint8_t* data, uint8_t quantity)
{
    for(uint8_t i = 0; i < quantity; ++i){
        write(data[i]);
    }
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
void SoftI2CMaster::write(char* data)
{
    write((uint8_t*)data, strlen(data));
}

// must be called in:
// slave tx event callback
// or after beginTransmission(address)
void SoftI2CMaster::write(int data)
{
    return write((uint8_t)data);
}

uint8_t SoftI2CMaster::read(uint8_t ack)
{
  return i2c_read(ack);
}

//
uint8_t SoftI2CMaster::read()
{
    return i2c_read(true);
}

//
uint8_t SoftI2CMaster::readLast()
{
    return i2c_read(false);
}

//------ PRIVATE METHODS --------------------------------------------


//
// Inits bitbanging port, must be called before using the functions below
//
void SoftI2CMaster::i2c_init(void)
{
  pinMode(_sdaPin, OUTPUT);
  pinMode(_sclPin, OUTPUT);
  pinSetFast(_sdaPin);
  pinSetFast(_sclPin);
  delayMicroseconds(i2cbitdelay);
}

// Send a START Condition
//
void SoftI2CMaster::i2c_start(void)
{
    // set both to high at the same time
    //I2C_DDR &=~ (_BV( I2C_SDA ) | _BV( I2C_SCL ));
    //*_sclDirReg &=~ (_sdaBitMask | _sclBitMask);

  pinSetFast(_sdaPin);
  delayMicroseconds(i2cbitdelay);

  pinSetFast(_sclPin);
  delayMicroseconds(i2cbitdelay);

  pinResetFast(_sdaPin);
  delayMicroseconds(i2cbitdelay);

  pinResetFast(_sclPin);
  delayMicroseconds(i2cbitdelay);
}

// Send a STOP Condition
//
void SoftI2CMaster::i2c_stop(void)
{
  pinResetFast(_sdaPin);
  delayMicroseconds(i2cbitdelay);

  pinSetFast(_sclPin);
  delayMicroseconds(i2cbitdelay);

  pinSetFast(_sdaPin);
  delayMicroseconds(i2cbitdelay);
}

// write a byte to the I2C slave device
//
void SoftI2CMaster::i2c_writebit( uint8_t c )
{
  if ( c > 0 ) {
      pinSetFast(_sdaPin);
  } else {
      pinResetFast(_sdaPin);
  }
  delayMicroseconds(i2cbitdelay);

  pinSetFast(_sclPin);
  delayMicroseconds(i2cbitdelay);

  pinResetFast(_sclPin);
  delayMicroseconds(i2cbitdelay);
}

void SoftI2CMaster::i2c_write( uint8_t c )
{
  for ( uint8_t i = 0; i < 8; i++) {
      i2c_writebit( c & 128 );
      c <<= 1;
  }

  pinSetFast(_sdaPin);
  pinMode(_sdaPin, INPUT);
  i2c_readbit();
  pinMode(_sdaPin, OUTPUT);
}

// read a bit from the I2C slave device
//
uint8_t SoftI2CMaster::i2c_readbit(void)
{
  uint8_t c;
  bool stretch;

  do {
    pinSetFast(_sclPin);
    pinMode(_sclPin, INPUT);
    stretch = pinReadFast(_sclPin) == LOW;
    pinMode(_sclPin, OUTPUT);
  } while(stretch);
  delayMicroseconds(i2cbitdelay);

  c = pinReadFast(_sdaPin);
  delayMicroseconds(i2cbitdelay);

  pinResetFast(_sclPin);
  delayMicroseconds(i2cbitdelay);

  return (c == LOW ? 0 : 1);
}

uint8_t SoftI2CMaster::i2c_read(uint8_t ack)
{
  uint8_t i, res = 0;

  pinSetFast(_sdaPin);
  pinMode(_sdaPin, INPUT);
  for (i = 0; i < 8; i++) {
      res <<= 1;
      res |= i2c_readbit();
  }
  pinMode(_sdaPin, OUTPUT);

  if (ack) {
      i2c_writebit(I2C_ACK);
  }
  else {
      i2c_writebit(I2C_NAK);
  }

  delayMicroseconds(i2cbitdelay);

  return res;
}
