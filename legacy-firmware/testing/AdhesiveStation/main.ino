#include "main.h"

Button forwardNozzle, reverseNozzle, pushPlunger, pullPlunger, applyLayer, leftLimitSwitch, rightLimitSwitch;

void setup() {
    forwardNozzle.pin = D0;
    reverseNozzle.pin = D1;
    pushPlunger.pin = D2;
    pullPlunger.pin = D3;
    applyLayer.pin = D4;
    leftLimitSwitch.pin = TX;
    rightLimitSwitch.pin = RX;

    pinMode(forwardNozzle.pin, forwardNozzle.mode);
    pinMode(reverseNozzle.pin, reverseNozzle.mode);
    pinMode(pushPlunger.pin, pushPlunger.mode);
    pinMode(pullPlunger.pin, pullPlunger.mode);
    pinMode(applyLayer.pin, applyLayer.mode);
    pinMode(leftLimitSwitch.pin, leftLimitSwitch.mode);
    pinMode(rightLimitSwitch.pin, rightLimitSwitch.mode);

    pinMode(pinLED, OUTPUT);
    pinMode(pinNozzleSleep, OUTPUT);
    pinMode(pinNozzleDir, OUTPUT);
    pinMode(pinNozzleStep, OUTPUT);
    pinMode(pinPlungerSleep, OUTPUT);
    pinMode(pinPlungerDir, OUTPUT);
    pinMode(pinPlungerStep, OUTPUT);
    pinMode(pinAirSolenoid, OUTPUT);

    digitalWrite(pinLED, HIGH);
    digitalWrite(pinNozzleDir, LOW);
    digitalWrite(pinNozzleStep, LOW);
    digitalWrite(pinPlungerDir, LOW);
    digitalWrite(pinPlungerStep, LOW);

    digitalWrite(pinNozzleSleep, HIGH);
    digitalWrite(pinPlungerSleep, HIGH);
    delay(5);

    digitalWrite(pinAirSolenoid, LOW);
}

void sleep_stepper(int sleepPin) {
    digitalWrite(sleepPin, LOW);
}

void wake_stepper(int sleepPin) {
    digitalWrite(sleepPin, HIGH);
    delay(5);
}

void move_nozzle(int steps){
    //rotate a specific number of steps - negative for reverse movement
    int dir = (steps > 0) ? LOW : HIGH;
    digitalWrite(pinNozzleDir, dir);

    steps = abs(steps);
    for(int i = 0; i < steps; i++) {
        if (i % KEEP_ALIVE == 0) {
            Particle.process();
        }

        if ((dir == HIGH && button_pushed(rightLimitSwitch)) || (dir == LOW && button_pushed(leftLimitSwitch))) {
            break;
        }

        digitalWrite(pinNozzleStep, HIGH);
        delayMicroseconds(NOZZLE_STEP_DELAY);
        digitalWrite(pinNozzleStep, LOW);
        delayMicroseconds(NOZZLE_STEP_DELAY);
    }
}

void move_nozzle_to_dispense(int steps){
    //rotate a specific number of steps - negative for reverse movement
    int dir = (steps > 0) ? LOW : HIGH;
    digitalWrite(pinNozzleDir, dir);

    steps = abs(steps);
    for(int i = 0; i < steps; i++) {
        if (i % KEEP_ALIVE == 0) {
            Particle.process();
        }

        if ((dir == HIGH && button_pushed(rightLimitSwitch)) || (dir == LOW && button_pushed(leftLimitSwitch))) {
            break;
        }

        digitalWrite(pinNozzleStep, HIGH);
        delayMicroseconds(DISPENSE_STEP_DELAY);
        digitalWrite(pinNozzleStep, LOW);
        delayMicroseconds(DISPENSE_STEP_DELAY);
    }
}

void move_nozzle_with_air(int steps){
    //rotate a specific number of steps - negative for reverse movement

    int dir = (steps > 0) ? LOW : HIGH;
    digitalWrite(pinNozzleDir, dir);

    digitalWrite(pinAirSolenoid, HIGH);

    steps = abs(steps);
    for(int i = 0; i < steps; i++) {
        if (i % KEEP_ALIVE == 0) {
            Particle.process();
        }

        if ((dir == HIGH && button_pushed(rightLimitSwitch)) || (dir == LOW && button_pushed(leftLimitSwitch))) {
            break;
        }

        digitalWrite(pinNozzleStep, HIGH);
        delayMicroseconds(AIR_STEP_DELAY);
        digitalWrite(pinNozzleStep, LOW);
        delayMicroseconds(AIR_STEP_DELAY);
    }

    digitalWrite(pinAirSolenoid, LOW);
}

void move_nozzle_slowly(int steps){
    //rotate a specific number of steps - negative for reverse movement
    int dir = (steps > 0) ? LOW : HIGH;
    digitalWrite(pinNozzleDir, dir);

    steps = abs(steps);
    for(int i = 0; i < steps; i++) {
        if (i % KEEP_ALIVE == 0) {
            Particle.process();
        }

        if ((dir == HIGH && button_pushed(rightLimitSwitch)) || (dir == LOW && button_pushed(leftLimitSwitch))) {
            break;
        }

        digitalWrite(pinNozzleStep, HIGH);
        delayMicroseconds(SLOW_NOZZLE_STEP_DELAY);
        digitalWrite(pinNozzleStep, LOW);
        delayMicroseconds(SLOW_NOZZLE_STEP_DELAY);
    }
}

void move_plunger(int steps){
    //rotate a specific number of steps - negative for reverse movement

    int step_delay = (steps > 0) ? PLUNGER_STEP_DELAY_PULL : PLUNGER_STEP_DELAY_PUSH;

    digitalWrite(pinPlungerDir, (steps > 0) ? HIGH : LOW);

    steps = abs(steps);
    for(int i = 0; i < steps; i++) {
        if (i % KEEP_ALIVE == 0) {
            Particle.process();
        }

        digitalWrite(pinPlungerStep, HIGH);
        delayMicroseconds(step_delay);
        digitalWrite(pinPlungerStep, LOW);
        delayMicroseconds(step_delay);
    }
}

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

void apply_layer(){
    int i;

    move_nozzle(-200);
    delay(500);
    move_nozzle(STEPS_TO_RESET);
    delay(500);
    move_nozzle(-3300);
    delay(500);
    move_plunger(-1500);
    delay(500);
    for (i = 0; i < 16; i ++) {
      move_nozzle_to_dispense(-100);
      move_plunger(-300);
      delay(500);
    }
    for (i = 0; i < 14; i ++) {
      move_nozzle_to_dispense(100);
      move_plunger(-300);
      delay(500);
    }
    move_plunger(1000);
    move_nozzle(-800);
    delay(10000);
    move_nozzle_with_air(-1400);
    move_nozzle_slowly(-200);
    move_nozzle_with_air(1500);

    move_nozzle(STEPS_TO_RESET);
}

void move_nozzle_while_button_pushed(int pin, int dir) {
    int count = 0;

    digitalWrite(pinNozzleDir, dir);
    while (digitalRead(pin) == LOW) {
        digitalWrite(pinNozzleStep, HIGH);
        delayMicroseconds(NOZZLE_STEP_DELAY);
        digitalWrite(pinNozzleStep, LOW);
        delayMicroseconds(NOZZLE_STEP_DELAY);
        if (++count % KEEP_ALIVE == 0) {
            Particle.process();
        }
    }
}

void move_plunger_while_button_pushed(int pin, int dir) {
    int count = 0;
    int step_delay = (dir == HIGH) ? PLUNGER_STEP_DELAY_PULL : PLUNGER_STEP_DELAY_PUSH;

    digitalWrite(pinPlungerDir, dir);
    while (digitalRead(pin) == LOW) {
        digitalWrite(pinPlungerStep, HIGH);
        delayMicroseconds(step_delay);
        digitalWrite(pinPlungerStep, LOW);
        delayMicroseconds(step_delay);
        if (++count % KEEP_ALIVE == 0) {
            Particle.process();
        }
    }
}

void loop() {
    if (button_pushed(forwardNozzle)) {
      move_nozzle_while_button_pushed(forwardNozzle.pin, HIGH);
    }

    if (button_pushed(reverseNozzle)) {
        move_nozzle_while_button_pushed(reverseNozzle.pin, LOW);
    }

    if (button_pushed(pushPlunger)) {
        move_plunger_while_button_pushed(pushPlunger.pin, LOW);
    }

    if (button_pushed(pullPlunger)) {
        move_plunger_while_button_pushed(pullPlunger.pin, HIGH);
    }

    if (button_pushed(applyLayer)) {
        apply_layer();
    }

    if ((digitalRead(leftLimitSwitch.pin) == LOW) || (digitalRead(rightLimitSwitch.pin) == LOW)) {
        digitalWrite(pinLED, HIGH);
    }
    else {
        digitalWrite(pinLED, LOW);
    }
}
