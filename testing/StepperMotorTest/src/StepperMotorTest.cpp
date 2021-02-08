/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/leo3linbeck/github/brevitest-device/testing/StepperMotorTest/src/StepperMotorTest.ino"
/*
 * Project StepperMotorTest
 * Description: Testing for stepper motor driver
 * Author: Leo Linbeck III
 * Date: 2 January 2021
 */

#include <Stepper.h>

void setup();
void loop();
#line 10 "/Users/leo3linbeck/github/brevitest-device/testing/StepperMotorTest/src/StepperMotorTest.ino"
const int stepsPerRevolution = 64;  // change this to fit the number of steps per revolution
// for your motor

// initialize the stepper library on pins 8 through 11:
Stepper myStepper(stepsPerRevolution, A0, A2, A1, A3);

void setup() {
  // set the speed at 60 rpm:
  myStepper.setSpeed(60);
  // initialize the serial port:
  Serial.begin(115200);
}

void loop() {
  // step one revolution  in one direction:
  Serial.println("clockwise");
  myStepper.step(4 * stepsPerRevolution);
  delay(500);

  // // step one revolution in the other direction:
  // Serial.println("counterclockwise");
  // myStepper.step(-stepsPerRevolution);
  // delay(500);
}
