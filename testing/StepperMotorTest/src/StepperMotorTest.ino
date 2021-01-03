/*
 * Project StepperMotorTest
 * Description: Testing for stepper motor driver
 * Author: Leo Linbeck III
 * Date: 2 January 2021
 */

#define MOTOR_STEP_DELAY 1000

SerialLogHandler logHandler;

bool motorOn = false;
int pinMotorOn = D4;
int pinMotorDir = D5;
int pinMotorStep = D6;

void setup() {
    pinMode(pinMotorOn, INPUT_PULLDOWN);
    pinMode(pinMotorStep, OUTPUT);
    digitalWrite(pinMotorStep, LOW);
    pinMode(pinMotorDir, OUTPUT);
    digitalWrite(pinMotorDir, LOW);
    pinMode(D7, OUTPUT);
    digitalWrite(D7, LOW);

    // Serial.begin(115200); // standard serial port
    Log.info("Setup complete");
}

void loop() {
    if (motorOn != (digitalRead(pinMotorOn) == HIGH)) {
        delay(100);
        if (motorOn != (digitalRead(pinMotorOn) == HIGH)) {
            motorOn = digitalRead(pinMotorOn) == HIGH;
            Log.info("Motor %s", motorOn ? "on" : "off");
        }
    }

    if (motorOn) {
        digitalWrite(D7, HIGH);

        digitalWrite(pinMotorStep, HIGH);
        delayMicroseconds(MOTOR_STEP_DELAY);

        digitalWrite(D7, LOW);

        digitalWrite(pinMotorStep, LOW);
        delayMicroseconds(MOTOR_STEP_DELAY);
    }
}