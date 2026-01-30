#include "main.h"

Button limitSwitch;

bool shutterClosed;

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

void move_stepper(int steps){
    //rotate a specific number of steps - negative for reverse movement
    int dir = (steps > 0) ? LOW : HIGH;
    digitalWrite(pinStepperDir, dir);
    steps = abs(steps);
    for(int i = 0; i < steps; i++) {
        if (i % KEEP_ALIVE == 0) {
            Particle.process();
        }

        if (dir == LOW && button_pushed(limitSwitch)) {
            break;
        }

        digitalWrite(pinStepperStep, HIGH);
        delayMicroseconds(STEP_DELAY);
        digitalWrite(pinStepperStep, LOW);
        delayMicroseconds(STEP_DELAY);
    }
}

void setup() {
    pinMode(pinShutterState, INPUT_PULLDOWN);

    limitSwitch.pin = A4;
    pinMode(limitSwitch.pin, limitSwitch.mode);

    pinMode(pinStepperSleep, OUTPUT);
    pinMode(pinStepperDir, OUTPUT);
    pinMode(pinStepperStep, OUTPUT);

    digitalWrite(pinStepperDir, LOW);
    digitalWrite(pinStepperStep, LOW);
    digitalWrite(pinStepperSleep, HIGH);
    delay(5);

    shutterClosed = true;
}

void openShutter() {
    move_stepper(50);
    move_stepper(-50);
    shutterClosed = false;
}

void closeShutter() {
    move_stepper(50);
    shutterClosed = true;
}

void loop() {
    if (digitalRead(pinShutterState) == HIGH) {
        if (shutterClosed) {
            openShutter();
        }
    }
    else {
        if (!shutterClosed) {
            closeShutter();
        }
    }
}
