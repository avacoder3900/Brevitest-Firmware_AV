#include "application.h"

#define KEEP_ALIVE 10
#define DEBOUNCE_DELAY 10
#define STEP_DELAY 1000

struct Button {
    int pin;
    int reading;
    int last_state;
    unsigned long debounce_time;
    PinMode mode;
    Button() {
        last_state = HIGH;
        debounce_time = 0;
        mode = INPUT_PULLUP;
    }
};

int pinStepperSleep = A0;
int pinStepperDir = A1;
int pinStepperStep = A2;
int pinShutterState = D0;
