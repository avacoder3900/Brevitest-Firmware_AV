#include "main.h"

Button userButton, stepperLeft, stepperRight, leftLimitSwitch, rightLimitSwitch;
Servo rotateServo, raiseServo, extendServo, tipServo, uvShutterServo;
int pos = 0;

bool button_pushed(Button &button) {
    button.reading = digitalRead(button.pin);

    if (button.reading != button.last_state) {
        button.debounce_time = millis();
    }

    if ((millis() - button.debounce_time) > DEBOUNCE_DELAY) {
        if(button.reading == LOW) {
            return true;
        }
    }

    button.last_state = button.reading;

    return false;
}

void sleep_stepper(int sleepPin) {
    digitalWrite(sleepPin, LOW);
}

void wake_stepper(int sleepPin) {
    digitalWrite(sleepPin, HIGH);
    delay(5);
}

void move_stepper(int steps, int step_delay){
    //rotate a specific number of steps - negative for reverse movement
    int dir = (steps > 0) ? LOW : HIGH;
    digitalWrite(pinStepperDir, dir);
    steps = abs(steps);
    for(int i = 0; i < steps; i++) {
        if (i % KEEP_ALIVE == 0) {
            Particle.process();
        }

        if ((dir == LOW && button_pushed(rightLimitSwitch)) || (dir == HIGH && button_pushed(leftLimitSwitch))) {
            break;
        }

        digitalWrite(pinStepperStep, HIGH);
        delayMicroseconds(step_delay);
        digitalWrite(pinStepperStep, LOW);
        delayMicroseconds(step_delay);
    }
}

void move_stepper_fast(int steps){
    move_stepper(steps, STEP_DELAY_FAST);
}

void move_stepper_slow(int steps){
    move_stepper(steps, STEP_DELAY_SLOW);
}

void move_stepper_dispense(int steps){
    move_stepper(steps, STEP_DELAY_DISPENSE);
}

void move_stepper_spread(int steps){
    move_stepper(steps, STEP_DELAY_SPREAD);
}

void move_servo(Servo &servo, int toAngle, int step_delay) {
    int fromAngle = servo.read();
    if (fromAngle > toAngle) {
        for (int i = fromAngle; i >= toAngle; i--) {
            servo.write(i);
            delay(step_delay);
        }
    }
    else if (fromAngle < toAngle) {
        for (int i = fromAngle; i <= toAngle; i++) {
            servo.write(i);
            delay(step_delay);
        }
    }
}

void move_servo_fast(Servo &servo, int toAngle) {
    move_servo(servo, toAngle, 10);
}

void move_servo_slow(Servo &servo, int toAngle) {
    move_servo(servo, toAngle, 100);
}

void initialize_robot_arm() {
    raiseServo.write(90);
    extendServo.write(80);
    rotateServo.write(86);
    tipServo.write(20);
}

void reset_robot_arm() {
    move_servo_fast(raiseServo, 90);
    move_servo_fast(extendServo, 80);
    move_servo_fast(rotateServo, 86);
    move_servo_fast(tipServo, 20);
}

void reset_stepper() {
    move_stepper_fast(-50000);
    delay(200);
    move_stepper_slow(100);
    delay(200);
    move_stepper_slow(-150);
}

void setup() {
    userButton.pin = D4;
    pinMode(userButton.pin, userButton.mode);

    stepperLeft.pin = A3;
    pinMode(stepperLeft.pin, stepperLeft.mode);
    leftLimitSwitch.pin = A4;
    pinMode(leftLimitSwitch.pin, leftLimitSwitch.mode);

    stepperRight.pin = A5;
    pinMode(stepperRight.pin, stepperRight.mode);
    rightLimitSwitch.pin = A6;
    pinMode(rightLimitSwitch.pin, rightLimitSwitch.mode);


    pinMode(pinStepperSleep, OUTPUT);
    pinMode(pinStepperDir, OUTPUT);
    pinMode(pinStepperStep, OUTPUT);

    digitalWrite(pinStepperDir, LOW);
    digitalWrite(pinStepperStep, LOW);
    digitalWrite(pinStepperSleep, HIGH);
    delay(5);

    pinMode(pinAirSolenoid, OUTPUT);
    digitalWrite(pinAirSolenoid, LOW);

    pinMode(pinCardVacuumA, OUTPUT);
    digitalWrite(pinCardVacuumA, LOW);
    pinMode(pinCardVacuumB, OUTPUT);
    digitalWrite(pinCardVacuumB, LOW);

    pinMode(pinAdhesivePumpA, OUTPUT);
    digitalWrite(pinAdhesivePumpA, LOW);
    pinMode(pinAdhesivePumpB, OUTPUT);
    digitalWrite(pinAdhesivePumpB, LOW);

    rotateServo.attach(pinRotateServo);
    raiseServo.attach(pinRaiseServo);
    extendServo.attach(pinExtendServo);
    tipServo.attach(pinTipServo);

    initialize_robot_arm();

    uvShutterServo.attach(pinUVShutter);
    uvShutterServo.write(0);
}
void move_stepper_while_button_pushed(int pin, int dir) {
    int count = 0;

    digitalWrite(pinStepperDir, dir);
    while (digitalRead(pin) == LOW) {
        digitalWrite(pinStepperStep, HIGH);
        delayMicroseconds(STEP_DELAY_FAST);
        digitalWrite(pinStepperStep, LOW);
        delayMicroseconds(STEP_DELAY_FAST);
        if (++count % KEEP_ALIVE == 0) {
            Particle.process();
        }
    }
}

void load_card() {
    reset_stepper();
    move_stepper_slow(250);

    move_servo_fast(extendServo, 0);
    move_servo_fast(rotateServo, 9);
    move_servo_fast(tipServo, 17);
    move_servo_fast(raiseServo, 85);
    move_servo_fast(extendServo, 75);
    delay(500);
    digitalWrite(pinCardVacuumB, HIGH);
    delay(2000);
    move_servo_fast(extendServo, 0);
    move_servo_fast(raiseServo, 115);
    move_servo_fast(extendServo, 70);
    move_servo_fast(raiseServo, 110);
    move_servo_fast(extendServo, 80);
    move_servo_fast(raiseServo, 105);
    move_servo_fast(extendServo, 90);
    move_servo_fast(raiseServo, 90);
    move_servo_fast(extendServo, 110);
    move_servo_fast(raiseServo, 100);

    delay(500);
    digitalWrite(pinCardVacuumB, LOW);
    digitalWrite(pinCardVacuumA, HIGH);
    delay(500);

    move_servo_slow(extendServo, 105);
    move_servo_slow(raiseServo, 96);
    move_servo_slow(extendServo, 100);
    move_servo_slow(raiseServo, 92);
    move_servo_slow(extendServo, 90);
    move_servo_slow(raiseServo, 88);
    move_servo_slow(extendServo, 85);
    move_servo_slow(raiseServo, 84);
    move_servo_fast(extendServo, 80);
    move_servo_fast(raiseServo, 110);
    move_servo_fast(extendServo, 70);
    move_servo_fast(raiseServo, 115);
    move_servo_fast(extendServo, 0);
    digitalWrite(pinCardVacuumA, LOW);
    move_servo_fast(raiseServo, 90);
    move_servo_fast(tipServo, 20);
    move_servo_fast(rotateServo, 86);
    move_servo_fast(extendServo, 80);
}

void dispenseAdhesive() {
    move_stepper_fast(5050);

    digitalWrite(pinAdhesivePumpA, HIGH);
    delay(5000);
    move_stepper_dispense(-800);

    digitalWrite(pinAirSolenoid, HIGH);

    move_stepper_dispense(-800);
    digitalWrite(pinAdhesivePumpA, LOW);
    digitalWrite(pinAdhesivePumpB, HIGH);
    delay(1000);
    digitalWrite(pinAdhesivePumpB, LOW);

    move_stepper_dispense(-900);
    digitalWrite(pinAirSolenoid, LOW);
    move_stepper_dispense(-200);

    delay(30000);

    digitalWrite(pinAirSolenoid, HIGH);
    delay(500);
    move_stepper_dispense(1800);
    digitalWrite(pinAirSolenoid, LOW);
}

void dispensePrimerDots() {
    move_stepper_fast(2350);
    digitalWrite(pinAdhesivePumpA, HIGH);
    delay(800);
    digitalWrite(pinAdhesivePumpA, LOW);
    delay(5000);

    move_stepper_slow(400);
    delay(5000);
    move_stepper_fast(850);
    digitalWrite(pinAirSolenoid, HIGH);
    move_stepper_dispense(-250);
    digitalWrite(pinAirSolenoid, LOW);
    delay(2000);

    move_stepper_fast(800);
    digitalWrite(pinAdhesivePumpA, HIGH);
    delay(800);
    digitalWrite(pinAdhesivePumpA, LOW);
    delay(5000);

    move_stepper_slow(-400);
    delay(5000);
    move_stepper_fast(1070);
    digitalWrite(pinAirSolenoid, HIGH);
    move_stepper_dispense(250);
    digitalWrite(pinAirSolenoid, LOW);
    delay(5000);
}

void openUVShutter(int duration) {
    int i;

    for(i = 1; i <= 78; i++) {
        uvShutterServo.write(i);
        delay(10);
    }

    delay(duration);

    for(i = 77; i >= 0; i--) {
        uvShutterServo.write(i);
        delay(10);
    }
}

void make_cartridge() {
    reset_stepper();

    dispensePrimerDots();
    move_stepper_fast(50000);
    openUVShutter(12000);

    /*dispenseAdhesive();*/
    /*load_card();
    move_stepper_fast(50000);
    openUVShutter(3000);*/

    reset_stepper();
}

void loop() {
    if (button_pushed(stepperRight)) {
        move_stepper_while_button_pushed(stepperRight.pin, LOW);
    }

    if (button_pushed(stepperLeft)) {
        move_stepper_while_button_pushed(stepperLeft.pin, HIGH);
    }

    if (button_pushed(userButton)) {
        /*make_cartridge();*/
        reset_stepper();
        move_stepper_fast(2350);
        digitalWrite(pinAdhesivePumpA, HIGH);
        delay(4000);
        digitalWrite(pinAdhesivePumpA, LOW);
        delay(2000);
        digitalWrite(pinAdhesivePumpB, HIGH);
        delay(1000);
        digitalWrite(pinAdhesivePumpB, LOW);
        move_stepper_fast(800);
        digitalWrite(pinAirSolenoid, HIGH);
        move_stepper_spread(500);
        digitalWrite(pinAirSolenoid, LOW);
        reset_stepper();
    }
}
 
 
