/*
 * Project StepperMotorTest
 * Description: Testing for stepper motor driver
 * Author: Leo Linbeck III
 * Date: 2 January 2021
 */

#include <Adafruit-MotorShield-V2.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61); 

// Connect a stepper motor with 200 steps per revolution (1.8 degree)
// to motor port #2 (M3 and M4)
Adafruit_StepperMotor *myMotor = AFMS.getStepper(200, 2);


SerialLogHandler logHandler;

void setup() {
    AFMS.begin();  // create with the default frequency 1.6KHz
    //AFMS.begin(1000);  // OR with a different frequency, say 1KHz

    myMotor->setSpeed(10);  // rpm   

    Log.info("StepperTest setup complete");
}

void loop() {
  Log.info("Move forward");
  myMotor->step(2000, FORWARD, MICROSTEP); 
  delay(2000);
  Log.info("Move backward");
  myMotor->step(2000, BACKWARD, MICROSTEP); 

  delay(5000);

//   Log.info("Double coil steps");
//   myMotor->step(100, FORWARD, DOUBLE); 
//   myMotor->step(100, BACKWARD, DOUBLE);
  
//   Log.info("Interleave coil steps");
//   myMotor->step(100, FORWARD, INTERLEAVE); 
//   myMotor->step(100, BACKWARD, INTERLEAVE); 
  
//   Log.info("Microstep steps");
//   myMotor->step(50, FORWARD, MICROSTEP); 
//   myMotor->step(50, BACKWARD, MICROSTEP);
}
