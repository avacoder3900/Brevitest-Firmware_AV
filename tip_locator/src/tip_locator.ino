/*
 *    Code to locate and calibrate tip location on an Opentrons pipetting robot.
 *
 *    Written by Leo Linbeck III
 *
 *    Copyright 2021 by Brevitest Technologies, Inc
 *    All rights reserved. Distribution, copying, or changes make without prior written consent is forbidden.
 * 
 */

SYSTEM_THREAD(ENABLED);
// PRODUCT_ID(12430);
// PRODUCT_VERSION(1);

// Return values of endTransmission in the Wire library
#define kNOERROR 0
#define kDATATOOLONGERROR 1
#define kRECEIVEDNACKONADDRESSERROR 2
#define kRECEIVEDNACKONDATAERROR 3
#define kOTHERERROR 4

SerialLogHandler logHandler;

int pinLED = D7;
int pinXDetect = A0;
int pinYDetect = A1;
int pinButton = A2;
bool logReadings = false;

void setup() {
    // Initialize the I2C communication library
    Wire.begin();
    
    // Initialize the serial port
    Serial.begin(115200);

    // Setup hardware and variables for code which blinks the LED
    pinMode(pinLED, OUTPUT);
    digitalWrite(pinLED, LOW);
    pinMode(pinXDetect, INPUT);
    pinMode(pinYDetect, INPUT);
    pinMode(pinButton, INPUT);
}

void loop() {    
    if (Serial.available()) { // sending any data to serial port toggles logging
        while (Serial.available()) Serial.read();
        logReadings = !logReadings;
    }

    Log.info("pinXDetect: %c", digitalRead(pinXDetect) == HIGH ? 'H' : 'L');
    Log.info("pinYDetect: %c", digitalRead(pinYDetect) == HIGH ? 'H' : 'L');
    Log.info("pinButton: %c", digitalRead(pinButton) == HIGH ? 'H' : 'L');

    delay(2000);
    // if (digitalRead(pinXDetect) == HIGH) {
    //     Log.info("X detected");
    //     digitalWrite(pinLED, HIGH);
    //     delay(1000);
    //     digitalWrite(pinLED, LOW);
    // }
    // if (digitalRead(pinYDetect) == HIGH) {
    //     Log.info("Y detected");
    //     digitalWrite(pinLED, HIGH);
    //     delay(1000);
    //     digitalWrite(pinLED, LOW);
    // }
    // if (digitalRead(pinButton) == HIGH) {
    //     Log.info("Button detected");
    //     digitalWrite(pinLED, HIGH);
    //     delay(1000);
    //     digitalWrite(pinLED, LOW);
    // }
}
