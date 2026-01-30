/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "c:/GitHub/brevitest-device/magnetometer/src/magnetometer-v2.ino"
/*
 *    Code to validate the magnets on a Brevitest Acuity sample processing unit.
 *
 *    Written by Leo Linbeck III and Christine Luk, BreviTest Technologies, LLC
 *    ALS31300 elements adapted from code by K. Robert Bate, Allegro MicroSystems, LLC.
 *
 *    Copyright 2020 by BreviTest Technologies, LLC
 *    All rights reserved. Distribution, copying, or changes make without prior written consent is forbidden.
 * 
 */
#include <Wire.h>
#include <math.h>

void initialize_BLE();
void startAdvertising();
void setup();
void bleReadOneWell(int well);
void getMagnetometerReading();
bool check_one_well(int well);
void qualify_magnetometer();
void loop();
uint16_t write(int busAddress, uint8_t address, uint32_t value);
uint16_t read(int busAddress, uint8_t address, uint32_t& value);
long SignExtendBitfield(uint32_t data, int width);
#line 14 "c:/GitHub/brevitest-device/magnetometer/src/magnetometer-v2.ino"
SYSTEM_THREAD(ENABLED);
PRODUCT_VERSION(4);

// Return values of endTransmission in the Wire library
#define kNOERROR 0
#define kDATATOOLONGERROR 1
#define kRECEIVEDNACKONADDRESSERROR 2
#define kRECEIVEDNACKONDATAERROR 3
#define kOTHERERROR 4

SerialLogHandler logHandler;

struct MagnetometerReading
{
    int address;
    float mx;
    float my;
    float mz;
    float temperature;
} sample, controlLow, controlHigh;

bool ledState = false;
int ledPin = D7;
unsigned long nextTime;

#define READ_CYCLE 5000

#define FIRSTCHANNEL 0x60
#define LASTCHANNEL 0x6E

#define WELL_1_ADDR 0x64
#define OFFSET_SAMPLE 0
#define OFFSET_CONTROL_LOW 5
#define OFFSET_CONTROL_HIGH 10

#define Z_ERROR 0.1

bool qualified = false;
LEDStatus blinkRed(RGB_COLOR_RED, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);

char result[100];

unsigned long read_time = 0;
bool log_readings = false;
bool ble_connected = false;

static float z_mean[5][3] = {
    {1630, 1530, 1630},
    {1370, 1360, 1470},
    {1390, 1290, 1400},
    {1420, 1360, 1455},
    {1480, 1460, 1585}
};
// float z_max[5][3];
// float z_min[5][3];

#define BLE_TYPE BleCharacteristicProperty::READ

BleAdvertisingData advertData, scanResponse;

BleUuid magnetometerService("4d2b2311-bb00-43e3-a284-5c73b737c369");

BleUuid well1uuid("b1c14499-8e1d-41b2-b1bc-c89faa88d62a");
BleUuid well2uuid("2216cfb5-38a7-46a3-9509-ad7f287a569a");
BleUuid well3uuid("2230e907-583b-4328-84a4-8f9023a681c1");
BleUuid well4uuid("8c58309c-7c2d-4805-b71f-8137eb4a01f8");
BleUuid well5uuid("b9dc1dd4-a0da-4328-8003-6c72a526a12b");
BleUuid bleCharUuid[5] = { well1uuid, well2uuid, well3uuid, well4uuid, well5uuid };

BleCharacteristic wellChar1("well_1", BLE_TYPE, well1uuid, magnetometerService);
BleCharacteristic wellChar2("well_2", BLE_TYPE, well2uuid, magnetometerService);
BleCharacteristic wellChar3("well_3", BLE_TYPE, well3uuid, magnetometerService);
BleCharacteristic wellChar4("well_4", BLE_TYPE, well4uuid, magnetometerService);
BleCharacteristic wellChar5("well_5", BLE_TYPE, well5uuid, magnetometerService);
BleCharacteristic bleWell[5] = { wellChar1, wellChar2, wellChar3, wellChar4, wellChar5 };

void initialize_BLE() {
    byte idBuf[27];
    String id = System.deviceID();
    idBuf[0] = 0xFF;
    idBuf[1] = 0xFF;
    id.getBytes(&idBuf[2], 25);
    scanResponse.appendCustomData(idBuf, 26);

    BLE.setDeviceName("Magnetometer");
    advertData.appendLocalName("Magnetometer");

    for (int i = 0; i < 5; i++) {
        BLE.addCharacteristic(bleWell[i]);
    }
}

void startAdvertising() {
    BLE.advertise(&advertData, &scanResponse);
}

void setup() {
    // Initialize the I2C communication library
    Wire.begin();
    Wire.setClock(1000000);    // 1 MHz What the heck is this. (original CLOCK_SPEED_100KHZ) - CWL
    
    // Initialize the serial port
    Serial.begin(115200);

    // Setup hardware and variables for code which blinks the LED
    nextTime = millis();
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    // for (int i = 0; i < 5; i++) {
    //     for (int j = 0; j < 3; j++) {
    //         int z_error = z_mean[i][j] * Z_ERROR / 100;
    //         z_max[i][j] = z_mean[i][j] + z_error;
    //         z_min[i][j] = z_mean[i][j] - z_error;
    //     }
    // }

    for(int deviceAddress = FIRSTCHANNEL; deviceAddress <= LASTCHANNEL; deviceAddress++) {
        // Enter customer access mode on the ALS31300
        uint16_t error = write(deviceAddress, 0x24, 0x2C413534);
        if (error != kNOERROR)
        {
            Log.error("Error while trying to enter customer access mode. error = %d", error);
        }
    }

    initialize_BLE();

    qualified = ((char) EEPROM.read(0)) == 'Y';
    if (qualified) {
        startAdvertising();
    } else {
        Log.info("Magnetometer not qualified!");
        blinkRed.setActive(true);
    }
}

void readALS31300ADC(int busAddress, MagnetometerReading &reading);

void bleReadOneWell(int well) {
    int addr = WELL_1_ADDR - well;
    readALS31300ADC(addr + OFFSET_SAMPLE, sample);
    readALS31300ADC(addr + OFFSET_CONTROL_LOW, controlLow);
    readALS31300ADC(addr + OFFSET_CONTROL_HIGH, controlHigh);
    sprintf(result, "%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f",
        sample.temperature, sample.mx, sample.my, sample.mz,
        controlLow.temperature, controlLow.mx, controlLow.my, controlLow.mz,
        controlHigh.temperature, controlHigh.mx, controlHigh.my, controlHigh.mz);
    if (log_readings) Log.info("well %d: %s", well + 1, result);
    bleWell[well].setValue(result);
}

void getMagnetometerReading() {
    for(int i = 0; i < 5; i++) {
        bleReadOneWell(i);
    }
    if (log_readings) Log.info("-------------------------");
    // Blink the LED
    ledState = !ledState;
    digitalWrite(ledPin, ledState);
}

bool validate_reading(int well, char channel, MagnetometerReading reading) {
    int c = channel == 'A' ? 0 : (channel == '1' ? 1 : 2);
    float error = (z_mean[well][c] - reading.mz) / z_mean[well][c];
    bool isOK = abs(error) < Z_ERROR;
    Serial.printlnf("%d-%c:\t%s\t%.1f\t%.1f\t%.1f\t%.1f\t%.1f", well + 1, channel, isOK ? "OK" : "BAD", reading.temperature, reading.mx, reading.my, reading.mz, error * 100);
    return isOK;
}

bool check_one_well(int well) {
    bool isOK = true;
    int addr = WELL_1_ADDR - well;
    readALS31300ADC(addr + OFFSET_SAMPLE, sample);
    isOK = validate_reading(well, 'A', sample) && isOK;
    readALS31300ADC(addr + OFFSET_CONTROL_LOW, controlLow);
    isOK = validate_reading(well, '1', controlLow) && isOK;
    readALS31300ADC(addr + OFFSET_CONTROL_HIGH, controlHigh);
    return validate_reading(well, '2', controlHigh) && isOK;
}

void qualify_magnetometer() {
    bool isOK = true;
    Log.info("Qualifying magnetometer...");
    for(int i = 0; i < 5; i++) {
        isOK = check_one_well(i) && isOK;
    }
    Serial.printlnf("ID: %s, Date: %s, Result: %s", System.deviceID().c_str(), Time.format(Time.now(), TIME_FORMAT_ISO8601_FULL).c_str(), isOK ? "QUALIFIED" : "UNACCEPTABLE");
    EEPROM.put(0, isOK ? 'Y' : 'N');

    if (!isOK) {
        if (BLE.advertising()) {
            BLE.stopAdvertising();
        }
        if (ble_connected || BLE.connected()) {
            BLE.disconnect();
            ble_connected = false;
        }
    } else if (!BLE.advertising()) {
        startAdvertising();
    }
    blinkRed.setActive(!isOK);
    qualified = isOK;
}

void loop() {
    if (Serial.available()) { // sending any data to serial port toggles logging
        char b = Serial.read();
        if (b == 'q' || b == 'Q') {
            qualify_magnetometer();
        } else {
            log_readings = !log_readings;
        }
        while (Serial.available()) Serial.read();
    }

    if (qualified) {
        if (BLE.connected() ^ ble_connected) {
            ble_connected = BLE.connected();
            Log.info("Bluetooth %sconnected", ble_connected ? "" : "dis");
            digitalWrite(ledPin, LOW);
        }

        if (BLE.connected() || log_readings) {
            getMagnetometerReading();
        }
    }
}

//
// readALS31300ADC
// Read the X, Y, Z 12 bit values from Register 0x28 and 0x29
// eight times quickly using the full loop mode.
//
void readALS31300ADC(int busAddress, MagnetometerReading &reading) {
    uint32_t value0x27;
    
    // // Read the register the I2C loop mode is in
    uint16_t error = read(busAddress, 0x27, value0x27);
    if (error != kNOERROR) {
        Log.error("Unable to read the ALS31300. error = %d", error);
        return;
    }
    
    // I2C loop mode is in bits 2 and 3 so mask them out and set them to the full loop mode
    value0x27 = (value0x27 & 0xFFFFFFF3) | (0x2 << 2);
    
    // // Write the new values to the register the I2C loop mode is in
    error = write(busAddress, 0x27, value0x27);
    if (error != kNOERROR) {
        Log.error("Unable to read the ALS31300. error = %d", error);
        return;
    }
    
    // Write the address that is going to be read from the ALS31300
    Wire.beginTransmission(busAddress);
    Wire.write(0x28);
    error = Wire.endTransmission(false);
    
    // The ALS31300 accepted the address
    if (error == kNOERROR) {
        int x;
        int y;
        int z;
        int t;

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
        
        reading.address = busAddress;

        // Display the values of x, y and z
        reading.mx = (float)x / 1.0;
        reading.my = (float)y / 1.0;
        reading.mz = (float)z / 0.25;
        
        // float temp = (float)t /8; //temperature slope = 8 LSB/deg C
        reading.temperature = (((float)t + 2000) / 8 + 25); //temperature slope = 8 LSB/deg C ***Nick says use 22 instead of 25 because 0xb0 = 22 in decimal
   
    } else {
        Log.error("%d: error = %d", busAddress, error);
    }
}

// 32 bit write utility function
//
uint16_t write(int busAddress, uint8_t address, uint32_t value) {
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
// 32 bit read utility function
//
// Using I2C, read 32 bits of data from the address on the device at the bus address
//
uint16_t read(int busAddress, uint8_t address, uint32_t& value) {
    // Write the address that is to be read to the device
    Wire.beginTransmission(busAddress);
    Wire.write(address);
    int error = Wire.endTransmission(false);
    // if the device accepted the address,
    // request 4 bytes from the device
    // and then read them, MSB first
    if (error == kNOERROR) {
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

long SignExtendBitfield(uint32_t data, int width) {
    long x = (long)data;
    long mask = 1L << (width - 1);

    if (width < 32) {
        x = x & ((1 << width) - 1); // make sure the upper bits are zero
    }

    return (long)((x ^ mask) - mask);
}
