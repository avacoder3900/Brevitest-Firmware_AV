#include "main.h"

Button leftLimitSwitch, rightLimitSwitch, distalLimitSwitch, proximalLimitSwitch;
Stepper stage, nozzle;
Pump adhesive, cardPicker;

char buf[BUFFER_SIZE];


//
//
//           BUTTON FUNCTIONS
//
//

void initialize_button(Button &button, int pin) {
    button.pin = pin;
    pinMode(button.pin, button.mode);
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

//
//
//           STEPPER FUNCTIONS
//
//

void initialize_stepper(Stepper &stepper, int pinSleep, int pinDir, int pinStep) {
    stepper.pinSleep = pinSleep;
    stepper.pinDir = pinDir;
    stepper.pinStep = pinStep;

    pinMode(stepper.pinSleep, OUTPUT);
    pinMode(stepper.pinDir, OUTPUT);
    pinMode(stepper.pinStep, OUTPUT);

    digitalWrite(stepper.pinDir, LOW);
    digitalWrite(stepper.pinStep, LOW);

    wake_stepper(stepper);
}

void sleep_stepper(Stepper &stepper) {
    digitalWrite(stepper.pinSleep, LOW);
}

void wake_stepper(Stepper &stepper) {
    digitalWrite(stepper.pinSleep, HIGH);
    delay(5);
}

void move_stepper(Stepper &stepper, int steps, int speed) {
    //rotate a specific number of steps - negative for reverse movement
    int dir = (steps > 0) ? LOW : HIGH;
    digitalWrite(stepper.pinDir, dir);
    steps = abs(steps);
    for(int i = 0; i < steps; i++) {
        if (i % KEEP_ALIVE == 0) {
            Particle.process();
        }

        if ((dir == LOW && button_pushed(stepper.startLimitSwitch)) || (dir == HIGH && button_pushed(stepper.endLimitSwitch))) {
            break;
        }

        digitalWrite(stepper.pinStep, HIGH);
        delayMicroseconds(stepper.step_delay[speed]);
        digitalWrite(stepper.pinStep, LOW);
        delayMicroseconds(stepper.step_delay[speed]);
    }
}

void move_stepper_until_stop_received(Stepper &stepper, int dir, int speed){
    bool stopped = false;
    int data;

    while (!stopped) {
        move_stepper(stepper, (dir == HIGH ? -10 : 10), speed);
        if (Serial1.available()) {
            data = Serial1.read();
            if (data == START_CODE) {
                data = Serial1.read();
                stopped = (data == MESSAGE_STOP_STEPPER);
            }
            do {
                data = Serial1.read();
            } while (data != '/n');
        }
    }
}

//
//
//           PUMP FUNCTIONS
//
//

void initialize_pump(Pump &pump, int pinA, int pinB) {
    pump.pinA = pinA;
    pump.pinB = pinB;
    pinMode(pump.pinA, OUTPUT);
    pinMode(pump.pinB, OUTPUT);
    digitalWrite(pump.pinA, LOW);
    digitalWrite(pump.pinB, LOW);
}

void turn_on_pump_forward(Pump &pump) {
    digitalWrite(pump.pinA, HIGH);
}

void turn_off_pump_forward(Pump &pump) {
    digitalWrite(pump.pinA, LOW);
}

void turn_on_pump_reverse(Pump &pump) {
    digitalWrite(pump.pinB, HIGH);
}

void turn_off_pump_reverse(Pump &pump) {
    digitalWrite(pump.pinB, LOW);
}

//
//
//           STAGE FUNCTIONS
//
//

void reset_stage() {
    move_stage_fast(STAGE_RESET_STEPS);
    delay(200);
    move_stage_slow(100);
    delay(200);
    move_stage_slow(-150);
}

void move_stage_fast(int steps){
    move_stepper(stage, steps, STEPPER_SPEED_FAST);
}

void move_stage_slow(int steps){
    move_stepper(stage, steps, STEPPER_SPEED_SLOW);
}

void move_stage_dispense(int steps){
    move_stepper(stage, steps, STEPPER_SPEED_DISPENSE);
}

void move_stage_spread(int steps){
    move_stepper(stage, steps, STEPPER_SPEED_SPREAD);
}

//
//
//           NOZZLE FUNCTIONS
//
//

void reset_nozzle() {
    move_nozzle_fast(NOZZLE_RESET_STEPS);
    delay(200);
    move_nozzle_slow(100);
    delay(200);
    move_stage_slow(-150);
}

void move_nozzle_fast(int steps){
    move_stepper(nozzle, steps, STEPPER_SPEED_FAST);
}

void move_nozzle_slow(int steps){
    move_stepper(nozzle, steps, STEPPER_SPEED_SLOW);
}

void move_nozzle_dispense(int steps){
    move_stepper(nozzle, steps, STEPPER_SPEED_DISPENSE);
}

void move_nozzle_spread(int steps){
    move_stepper(nozzle, steps, STEPPER_SPEED_SPREAD);
}

//
//
//           DISPENSE FUNCTIONS
//
//

void dispense_adhesive_dot(int duration) {
    turn_on_pump_forward(adhesive);
    delay(duration + PUMP_SUCK_BACK);
    turn_off_pump_forward(adhesive);

    turn_on_pump_reverse(adhesive);
    delay(PUMP_SUCK_BACK);
    turn_off_pump_reverse(adhesive);
}

void dispense_and_move_stage(int steps) {
    turn_on_pump_forward(adhesive);
    delay(PUMP_SUCK_BACK);
    move_stage_dispense(steps);
    turn_off_pump_forward(adhesive);

    turn_on_pump_reverse(adhesive);
    delay(PUMP_SUCK_BACK);
    turn_off_pump_reverse(adhesive);
}

void dispense_and_move_nozzle(int steps) {
    turn_on_pump_forward(adhesive);
    delay(PUMP_SUCK_BACK);
    move_nozzle_dispense(steps);
    turn_off_pump_forward(adhesive);

    turn_on_pump_reverse(adhesive);
    delay(PUMP_SUCK_BACK);
    turn_off_pump_reverse(adhesive);
}

void dispense_support_dots() {
    reset_stage();
    reset_nozzle();

    move_stage_fast(1000);
    move_nozzle_slow(100);
    dispense_adhesive_dot(3000);

    move_nozzle_slow(500);
    dispense_adhesive_dot(3000);

    move_stage_fast(1500);
    dispense_adhesive_dot(3000);

    move_nozzle_slow(-500);
    dispense_adhesive_dot(3000);
}

void dispense_adhesive_fill() {
    reset_stage();
    reset_nozzle();

    move_stage_fast(1100);
    move_nozzle_slow(50);

    for (int i = 0; i < 15; i++) {
        dispense_and_move_nozzle(600);
        dispense_and_move_stage(50);
        dispense_and_move_nozzle(-600);
        dispense_and_move_stage(50);
    }
    dispense_and_move_nozzle(600);
}

//
//
//           MESSAGE HANDLING
//
//

void process_command(int code, int param) {
    switch(code) {
        case MESSAGE_RESET_STAGE:
            reset_stage();
            break;
        case MESSAGE_RESET_NOZZLE:
            reset_nozzle();
            break;
        case MESSAGE_START_STAGE_LEFT:
            move_stepper_until_stop_received(stage, HIGH, STEPPER_SPEED_FAST);
            break;
        case MESSAGE_START_STAGE_RIGHT:
            move_stepper_until_stop_received(stage, LOW, STEPPER_SPEED_FAST);
            break;
        case MESSAGE_START_NOZZLE_DISTALLY:
            move_stepper_until_stop_received(nozzle, HIGH, STEPPER_SPEED_FAST);
            break;
        case MESSAGE_START_NOZZLE_PROXIMALLY:
            move_stepper_until_stop_received(nozzle, LOW, STEPPER_SPEED_FAST);
            break;
        case MESSAGE_APPLY_AIR_PRESSURE:
            digitalWrite(pinAirSolenoid, HIGH);
            delay(param);
            digitalWrite(pinAirSolenoid, LOW);
            break;
        case MESSAGE_MOVE_STAGE_FAST:
            move_stage_fast(param);
            break;
        case MESSAGE_MOVE_STAGE_UNDER_UV:
            move_stage_fast(STAGE_UV_STEPS);
            break;
        case MESSAGE_DISPENSE_SUPPORT_DOTS:
            dispense_support_dots();
            break;
        case MESSAGE_DISPENSE_ADHESIVE_FILL:
            dispense_adhesive_fill();
            break;
        case MESSAGE_CARD_PICKER_PULL:
            if (param) {
                turn_on_pump_forward(cardPicker);
            }
            else {
                turn_off_pump_forward(cardPicker);
            }
            break;
        case MESSAGE_CARD_PICKER_PUSH:
            if (param) {
                turn_on_pump_reverse(cardPicker);
            }
            else {
                turn_off_pump_reverse(cardPicker);
            }
            break;
        default:
            break;
    }
}

char command[MESSAGE_COMMAND_LENGTH + 1];

void check_and_process_messages() {
    char indx = 0;
    int i, data, code, param;

    if (!Serial1.available()) {
        return;
    }

    digitalWrite(D7, HIGH);
    do {
        data = Serial1.read();
        if (data == -1) {
            for (i = 0; i < 3; i++) {
                digitalWrite(D7, LOW);
                delay(100);
                digitalWrite(D7, HIGH);
                delay(100);
            }
            digitalWrite(D7, LOW);
            return;
        }
    } while (data != START_CODE);

    do {
        if (indx >= BUFFER_SIZE) {
            digitalWrite(D7, LOW);
            return;
        }
        buf[indx] = Serial1.read();
    } while (buf[indx++] != '\n');

    for (i = 0; i < indx; i++) {
        digitalWrite(D7, LOW);
        delay(500);
        digitalWrite(D7, HIGH);
        delay(500);
    }
    strncpy(command, buf, MESSAGE_COMMAND_LENGTH);
    command[MESSAGE_COMMAND_LENGTH] = '\0';
    code = atoi(command);

    buf[--indx] = '\0';
    param = atoi(&buf[MESSAGE_COMMAND_LENGTH]);

    Serial1.write(ACK_CODE);

    process_command(code, param);
    digitalWrite(D7, LOW);
}

//
//
//           DA SETUP
//
//

void setup() {
    pinAirSolenoid = D2;
    pinMode(pinAirSolenoid, OUTPUT);
    digitalWrite(pinAirSolenoid, LOW);

    initialize_button(leftLimitSwitch, D3);
    initialize_button(rightLimitSwitch, D4);
    initialize_button(distalLimitSwitch, A6);
    initialize_button(proximalLimitSwitch, A7);

    initialize_stepper(stage, A0, A1, A2);
    initialize_stepper(nozzle, A3, A4, A5);

    initialize_pump(cardPicker, D0, D1);
    initialize_pump(adhesive, D5, D6);

    pinMode(D7, OUTPUT);

    Serial1.begin(115200);
}

//
//
//           DA LOOP
//
//

void loop() {
    check_and_process_messages();
}
 
 
