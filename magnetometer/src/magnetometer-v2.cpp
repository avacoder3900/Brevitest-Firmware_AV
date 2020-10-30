/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/leo3linbeck/github/brevitest-device/magnetometer/src/magnetometer-v2.ino"
/*
 *    Example source code for an Arduino to show
 *    how to communicate with an Allegro ALS31313
 *
 *    Written by K. Robert Bate, Allegro MicroSystems, LLC.
 *
 *    ALS31300Demo is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */
#include <Wire.h>
#include <math.h>

void getMagnetometerReading(int location);
void send_message(const char *message_type, const char *message);
void receive_message(const char *event, const char *data);
void initialize_BLE();
void setup();
void loop();
void readALS31300ADC(int busAddress, int location);
uint16_t write(int busAddress, uint8_t address, uint32_t value);
uint16_t read(int busAddress, uint8_t address, uint32_t& value);
long SignExtendBitfield(uint32_t data, int width);
#line 14 "/Users/leo3linbeck/github/brevitest-device/magnetometer/src/magnetometer-v2.ino"
SYSTEM_THREAD(ENABLED);

// Return values of endTransmission in the Wire library
#define kNOERROR 0
#define kDATATOOLONGERROR 1
#define kRECEIVEDNACKONADDRESSERROR 2
#define kRECEIVEDNACKONDATAERROR 3
#define kOTHERERROR 4

#define BUFFER_SIZE 512

SerialLogHandler logHandler;

// led blinking support
bool ledState = false;
int ledPin = D7;
unsigned long nextTime;

//deviceAddress: ninetysix=0x60;
const int FIRSTCHANNEL = 0x60; //sensor 96
const int LASTCHANNEL = 0x6E; //sensor 110
// SCL Pin = 19
// SDA Pin = 18

bool readMagnetometer = false;
bool ble_connected = false;

#define BLE_NOTIFY BleCharacteristicProperty::NOTIFY
BleAdvertisingData advertData;

BleUuid magnetometerService("4d2b2311-bb00-43e3-a284-5c73b737c369");

BleUuid well1uuid("b1c14499-8e1d-41b2-b1bc-c89faa88d62a");
BleUuid well2uuid("2216cfb5-38a7-46a3-9509-ad7f287a569a");
BleUuid well3uuid("2230e907-583b-4328-84a4-8f9023a681c1");
BleUuid well4uuid("8c58309c-7c2d-4805-b71f-8137eb4a01f8");
BleUuid well5uuid("b9dc1dd4-a0da-4328-8003-6c72a526a12b");

void getMagnetometerReading(int location) {
    for(int deviceAddress = FIRSTCHANNEL; deviceAddress <= LASTCHANNEL; deviceAddress++) //reads sensors 96 through 110 and puts a space after reading 110
    {
        readALS31300ADC(deviceAddress, location);
    }
    Serial.println();
    // Blink the LED
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
}
//
// setup
//
// Initializes the Wire library for I2C communications,
// Serial for displaying the results and error messages,
// the hardware and variables to blink the LED,
// and sets the ALS31300 into customer access mode.
//

void send_message(const char *message_type, const char *message) {

}

void receive_message(const char *event, const char *data) {
    // Serial.printlnf("Read: event = %s, data = %s", event, data);
    int location = atoi(data);
    getMagnetometerReading(location);
    send_message("magnetometer-confirm", "success");
}

void initialize_BLE() {
    byte idBuf[24];

    BleCharacteristic magnetometerWell1Characteristic("well_1", BLE_NOTIFY, well1uuid, magnetometerService);
    BleCharacteristic magnetometerWell2Characteristic("well_2", BLE_NOTIFY, well2uuid, magnetometerService);
    BleCharacteristic magnetometerWell3Characteristic("well_3", BLE_NOTIFY, well3uuid, magnetometerService);
    BleCharacteristic magnetometerWell4Characteristic("well_4", BLE_NOTIFY, well4uuid, magnetometerService);
    BleCharacteristic magnetometerWell5Characteristic("well_5", BLE_NOTIFY, well5uuid, magnetometerService);

    System.deviceID().getBytes(idBuf, 24);

    // add magnetometer service to advertising
    advertData.appendServiceUUID(magnetometerService);
    advertData.appendCustomData(idBuf, 24);
    advertData.appendLocalName("Magnetometer");

    // Continuously advertise when not connected
    BLE.addCharacteristic(magnetometerWell1Characteristic);
    BLE.addCharacteristic(magnetometerWell2Characteristic);
    BLE.addCharacteristic(magnetometerWell3Characteristic);
    BLE.addCharacteristic(magnetometerWell4Characteristic);
    BLE.addCharacteristic(magnetometerWell5Characteristic);
    BLE.advertise(&advertData);
}

void setup()
{
    // Initialize the I2C communication library
    Wire.begin();
    Wire.setClock(1000000);    // 1 MHz What the heck is this. (original CLOCK_SPEED_100KHZ) - CWL
    
    // Initialize the serial port
    // Serial.begin(115200);
    // If using a Arduino with USB built in, uncomment the next line,
    // this allows the errors in Setup to be seen
    // while (!Serial);


    // Setup hardware and variables for code which blinks the LED
    nextTime = millis();
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
   
    for(int deviceAddress = FIRSTCHANNEL; deviceAddress <= LASTCHANNEL; deviceAddress++)
    {
        // Enter customer access mode on the ALS31300
        uint16_t error = write(deviceAddress, 0x24, 0x2C413534);
        if (error != kNOERROR)
        {
            Log.error("Error while trying to enter customer access mode. error = %d", error);
        }
    }

    initialize_BLE();

    // Disconnect from cloud - only communication is via bluetooth
    // Particle.disconnect();
}

// loop
//
// Every half second, read the ADCs of the ALS31300 and display
// the values and toggle the state of the LED.
//
void loop()
{    
    // if (Serial.available())
    // {
    //     while (Serial.available()) Serial.read();
    //     readMagnetometer = !readMagnetometer;
    // }

    // if (readMagnetometer) {
    //     getMagnetometerReading(0);
    // }
    if (BLE.connected() ^ ble_connected) {
        ble_connected = BLE.connected();
        Log.info("Bluetooth %sconnected", ble_connected ? "" : "dis");
    }
}

//
// readALS31300ADC
// Read the X, Y, Z 12 bit values from Register 0x28 and 0x29
// eight times quickly using the full loop mode.
//
void readALS31300ADC(int busAddress, int location)
{
    uint32_t value0x27;
    
    // // Read the register the I2C loop mode is in
    uint16_t error = read(busAddress, 0x27, value0x27);
    if (error != kNOERROR)
    {
        Serial.print("Unable to read the ALS31300. error = ");
        Serial.println(error);
    }
    
    // I2C loop mode is in bits 2 and 3 so mask them out and set them to the full loop mode
    value0x27 = (value0x27 & 0xFFFFFFF3) | (0x2 << 2);
    
    // // Write the new values to the register the I2C loop mode is in
    error = write(busAddress, 0x27, value0x27);
    if (error != kNOERROR)
    {
        Serial.print("Unable to read the ALS31300. error = ");
        Serial.println(error);
    }
    
    // Write the address that is going to be read from the ALS31300
    Wire.beginTransmission(busAddress);
    Wire.write(0x28);
    error = Wire.endTransmission(false);
    
    // The ALS31300 accepted the address
    if (error == kNOERROR)
    {
        int x;
        int y;
        int z;
        int t;
        // Eight times is arbitrary, there is no limit. What is being demonstrated
        // is that once the address is set to 0x28, the first four bytes read will be from 0x28
        // and the next four will be from 0x29 after that it starts all over at 0x28
        // until the register address is changed or the loop mode is changed.

        for (int count = 0; count < 1; ++count)
        {
            // Start the read and request 8 bytes which is the contents of register 0x28 and 0x29
            Wire.requestFrom(busAddress, 8);
            
            // Read the first 4 bytes which are the contents of register 0x28
            x = Wire.read() << 4;
            y = Wire.read() << 4;
            z = Wire.read() << 4;
            t = (Wire.read() & 0x3F) << 6; // not sure if first 4 bytes for temp - CWL
                    
            // Read the next 4 bytes which are the contents of register 0x29
            Wire.read();    // Upper byte not used
            x |= Wire.read() & 0x0F;
            byte d = Wire.read();
            y |= (d >> 4) & 0x0F;
            z |= d & 0x0F;
            t |= Wire.read() & 0x3F;    // temp - CWL
            //t |= (d >> 5) & 0x0F;    // temp - CWL
            
            // Sign extend the 12th bit for x, y and z.
            x = SignExtendBitfield((uint32_t)x, 12);
            y = SignExtendBitfield((uint32_t)y, 12);
            z = SignExtendBitfield((uint32_t)z, 12);
            t = SignExtendBitfield((uint32_t)t, 12);
            
            // Display the values of x, y and z
            float mx = (float)x / 1.0;
            float my = (float)y / 1.0;
            float mz = (float)z / 0.25;
            
           // float mag = sqrt(mx * mx + my * my + mz * mz);
           // float temp = (float)t /8; //temperature slope = 8 LSB/deg C
            float temp = ((((float)t+2000)/8+25)); //temperature slope = 8 LSB/deg C ***Nick says use 22 instead of 25 because 0xb0 = 22 in decimal
            Log.info("%lu\t%d\t%d\t%.1f\t%.1f\t%.1f\t%.1f", millis(), busAddress, location, temp, mx, my, mz); //output sensor address, magnet vector in gauss, temperature in deg C
            //Serial.printlnf("%d: %.1f, %.1f, %.1f, %.1f", busAddress, mx, my, mz, temp); //output sensor address, magnet vector in gauss, temperature in deg C
        }
   
    }
    else
    {
        Serial.printlnf("%d: error = %d", busAddress, error);
    }
}

// write
//
// Using I2C, write 32 bit data to an address to the device at the bus address
//
uint16_t write(int busAddress, uint8_t address, uint32_t value)
{
    // Write the address that is to be written to the device
    // and then the 4 bytes of data, MSB first
    Wire.beginTransmission(busAddress);
    Wire.write(address);
    Wire.write((byte)(value >> 24));
    Wire.write((byte)(value >> 16));
    Wire.write((byte)(value >> 8));
    Wire.write((byte)(value));
    return Wire.endTransmission();
}


//
// read
//
// Using I2C, read 32 bits of data from the address on the device at the bus address
//
uint16_t read(int busAddress, uint8_t address, uint32_t& value)
{
    // Write the address that is to be read to the device
    Wire.beginTransmission(busAddress);
    Wire.write(address);
    int error = Wire.endTransmission(false);
    // if the device accepted the address,
    // request 4 bytes from the device
    // and then read them, MSB first
    if (error == kNOERROR)
    {
        Wire.requestFrom(busAddress, 4);
        value = Wire.read() << 24;
        value += Wire.read() << 16;
        value += Wire.read() << 8;
        value += Wire.read();
    }
    return error;
}

//
// SignExtendBitfield
//
// Sign extend a right justified value
//

long SignExtendBitfield(uint32_t data, int width)
{
    long x = (long)data;
    long mask = 1L << (width - 1);

    if (width < 32)
    {
        x = x & ((1 << width) - 1); // make sure the upper bits are zero
    }

    return (long)((x ^ mask) - mask);
}