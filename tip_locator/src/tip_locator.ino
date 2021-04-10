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
int pinXDetect = A1;
int pinYDetect = A0;
bool monitoringX = false;
bool monitoringY = false;
char dir;

void setup() {
    // Initialize the serial port
    Serial.begin(115200);

    // Setup hardware and variables for code which blinks the LED
    pinMode(pinLED, OUTPUT);
    digitalWrite(pinLED, LOW);
    pinMode(pinXDetect, INPUT);
    pinMode(pinYDetect, INPUT);
}

void loop() {    
    if (Serial.available()) { // sending any data to serial port toggles monitoring
        dir = Serial.read();
        switch(dir) {
            case 'X':
                monitoringX = true;
                monitoringY = false;
                break;
            case 'Y':
                monitoringX = false;
                monitoringY = true;
                break;
            default:
                monitoringX = false;
                monitoringY = false;
                break;
        }
        while (Serial.available()) Serial.read();
    }

    if (monitoringX && digitalRead(pinXDetect) == HIGH) {
        Serial.write('X');
        monitoringX = false;
        digitalWrite(pinLED, HIGH);
        delay(200);
        digitalWrite(pinLED, LOW);
    }

    if (monitoringY && digitalRead(pinYDetect) == HIGH) {
        Serial.write('Y');
        monitoringY = false;
        digitalWrite(pinLED, HIGH);
        delay(200);
        digitalWrite(pinLED, LOW);
    }
}
