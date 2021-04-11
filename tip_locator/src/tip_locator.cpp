/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/leo3/github/brevitest-device/tip_locator/src/tip_locator.ino"
/*
 *    Code to locate and calibrate tip location on an Opentrons pipetting robot.
 *
 *    Written by Leo Linbeck III
 *
 *    Copyright 2021 by Brevitest Technologies, Inc
 *    All rights reserved. Distribution, copying, or changes make without prior written consent is forbidden.
 * 
 */

void setup();
void loop();
#line 11 "/Users/leo3/github/brevitest-device/tip_locator/src/tip_locator.ino"
SYSTEM_THREAD(ENABLED);
PRODUCT_ID(14260);
PRODUCT_VERSION(1);

#define DEBOUNCE_TIME_MS 20
#define BLINK_TIME_MS 800

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
    pinMode(pinXDetect, INPUT_PULLDOWN);
    pinMode(pinYDetect, INPUT_PULLDOWN);
}

void loop() {
    if (Serial.available()) { // check to see if Opentrons has requested monitoring to start
        dir = Serial.read(); // read single character
        switch(dir) {
            case 'X': // let's track the X direction
                monitoringX = true;
                monitoringY = false;
                Particle.publish("X-start"); // send a message to the cloud for the central committee
                break;
            case 'Y': // how about the Y direction?
                monitoringX = false;
                monitoringY = true;
                Particle.publish("Y-start");
                break;
            default: // well, you must not want to watch anything..
                monitoringX = false;
                monitoringY = false;
                break;
        }
        while (Serial.available()) Serial.read(); // swallow rest of serial buffer
    }

    if (digitalRead(pinXDetect) == HIGH) { // hey, may be show time!
        delay(DEBOUNCE_TIME_MS); // first, let's wait a short bit before we get too excited
        if (digitalRead(pinXDetect) == HIGH) { // it's for real! let's go
            if (monitoringX) { // if we're monitoring, let everyone know we're done with our bit
                monitoringX = false;
                Serial.write('X'); // send the OK to the Opentrons
                Particle.publish("X-finish"); // publish a message that says we're done here
            }
            digitalWrite(pinLED, HIGH); // blink the LED so everyone knows it's game on
            delay(BLINK_TIME_MS);
            digitalWrite(pinLED, LOW);
        }
    }

    if (digitalRead(pinYDetect) == HIGH) { // like the above, but in the Y direction
        delay(DEBOUNCE_TIME_MS);
        if (digitalRead(pinYDetect) == HIGH) {
            if (monitoringY) {
                monitoringY = false;
                Serial.write('Y');
                Particle.publish("Y-finish");
            }
            digitalWrite(pinLED, HIGH);
            delay(BLINK_TIME_MS);
            digitalWrite(pinLED, LOW);
        }
    }
}
