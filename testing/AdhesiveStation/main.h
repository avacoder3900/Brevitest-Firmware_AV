#include "application.h"

#define STEPS_TO_DISCHARGE -10000
#define STEPS_TO_PULL_ADHESIVE 200
#define STEPS_TO_PRIME -2000
#define STEPS_TO_END_POINT 160000
#define STEPS_TO_APPLY_LAYER 60000
#define STEPS_TO_CLEAR_CARD_AREA (STEPS_TO_END_POINT - STEPS_TO_APPLY_LAYER)
#define NUMBER_OF_DEPOSITS 2
#define DELAY_AT_END 10000
#define STEPS_TO_RESET 200000
#define DISPENSE_STEP_DELAY 2000
#define AIR_STEP_DELAY 3000
#define NOZZLE_STEP_DELAY 400
#define SLOW_NOZZLE_STEP_DELAY 4000
#define PLUNGER_STEP_DELAY_PUSH 200
#define PLUNGER_STEP_DELAY_PULL 50
#define KEEP_ALIVE 10
#define DEBOUNCE_DELAY 100

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


// pin definitions
int pinLED = D7;

int pinNozzleSleep = A0;
int pinNozzleStep = A1;
int pinNozzleDir = A2;
int pinPlungerSleep = A3;
int pinPlungerStep = A4;
int pinPlungerDir = A5;
int pinAirSolenoid = A7;

/*int cumulative_steps = 0;*/
