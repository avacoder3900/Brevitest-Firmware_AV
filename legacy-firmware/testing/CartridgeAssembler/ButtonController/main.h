#include "application.h"

#define KEEP_ALIVE 10
#define DEBOUNCE_DELAY 10

#define START_CODE 1
#define ACK_CODE 0

#define MESSAGE_START_STAGE_LEFT 1
#define MESSAGE_START_STAGE_RIGHT 2
#define MESSAGE_START_NOZZLE_DISTALLY 3
#define MESSAGE_START_NOZZLE_PROXIMALLY 4
#define MESSAGE_STOP_STEPPER 5

#define MESSAGE_RESET_STAGE 10
#define MESSAGE_RESET_NOZZLE 11
#define MESSAGE_DISPENSE_SUPPORT_DOTS 12
#define MESSAGE_MOVE_STAGE_UNDER_UV 13
#define MESSAGE_DISPENSE_ADHESIVE_FILL 14
#define MESSAGE_APPLY_AIR_PRESSURE 15

#define MESSAGE_MOVE_STAGE_FAST 20
#define MESSAGE_MOVE_STAGE_SLOW 21
#define MESSAGE_CARD_PICKER_PULL 22
#define MESSAGE_CARD_PICKER_PUSH 23

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
