#include "application.h"

#define KEEP_ALIVE 10
#define DEBOUNCE_DELAY 10
#define STEP_DELAY_FAST 250
#define STEP_DELAY_SLOW 1000
#define STEP_DELAY_DISPENSE 10000
#define STEP_DELAY_SPREAD 20000

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
int pinRotateServo = D0;
int pinTipServo = D1;
int pinRaiseServo = D2;
int pinExtendServo = D3;
int pinUVShutter = A7;

int pinAirSolenoid = D5;
int pinCardVacuumA = D6;
int pinCardVacuumB = D7;
int pinAdhesivePumpA = RX;
int pinAdhesivePumpB = TX;

int pinStepperSleep = A0;
int pinStepperDir = A1;
int pinStepperStep = A2;
