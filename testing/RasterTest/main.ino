SYSTEM_THREAD(ENABLED);

#define CUMULATIVE_STEP_LIMIT 9700
#define MOVE_STEPS_BETWEEN_PARTICLE_PROCESS 10
#define LIMIT_SWITCH_RELEASE_LENGTH 250

/////////////////////////////////////////////////////////////
//                                                         //
//                        SOLENOID                         //
//                                                         //
/////////////////////////////////////////////////////////////

// ELECTRON PIN MAPPINGS
int pinBatteryLED = A0;
int pinBatteryAin = A1;
int pinDCinDetect = A2;
int pinQRTrigger = A3;
int pinSensorLED = A4;
int pinSolenoid = A5;
int pinDeviceLEDRed = B0;
int pinDeviceLEDGreen = B1;
int pinDeviceLEDBlue = B2;
int pinBluetoothMode = B3;
int pinBluetoothRX = C2;
int pinBluetoothTX = C3;
int pinAssaySDA = C4;
int pinAssaySCL = C5;
int pinControlSDA = D0;
int pinControlSCL = D1;
int pinLimitSwitch = D2;
int pinStepperSleep = D3;
int pinStepperDir = D4;
int pinStepperStep = D5;
int pinQRDecoderRX = RX;
int pinQRDecoderTX = TX;

int cumulative_steps = CUMULATIVE_STEP_LIMIT;

void move_solenoid(int duration) {
        uint8_t surge_power = 255;
        uint8_t sustain_power = 200;
        uint8_t surge_period = 150;

        Particle.process();

        int sustain_time = abs(duration) - surge_period;
        sustain_time = sustain_time < 0 ? 0 : sustain_time;

        pinMode(pinSolenoid, OUTPUT);
        analogWrite(pinSolenoid, surge_power);
        delay(surge_period);

        if (sustain_time) {
                pinMode(pinSolenoid, OUTPUT);
                analogWrite(pinSolenoid, sustain_power);
                delay(sustain_time);
        }

        pinMode(pinSolenoid, OUTPUT);
        analogWrite(pinSolenoid, 0);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        STEPPER                          //
//                                                         //
/////////////////////////////////////////////////////////////

void move_steps(int steps, int step_delay){
        //rotate a specific number of steps - negative for reverse movement

        wake_stepper();

        int dir = (steps > 0) ? HIGH : LOW;

        steps = abs(steps);

        digitalWrite(pinStepperDir,dir);

        for(long i = 0; i < steps; i += 1) {
                if (i % MOVE_STEPS_BETWEEN_PARTICLE_PROCESS == 0) {
                        Particle.process();
                }

                if ((dir == LOW) && (digitalRead(pinLimitSwitch) == HIGH)) {
                        cumulative_steps = 0;
                        break;
                }

                if (dir == HIGH) {
                        if (cumulative_steps > CUMULATIVE_STEP_LIMIT) {
                                break;
                        }

                        if (cumulative_steps < LIMIT_SWITCH_RELEASE_LENGTH) {
                                pinMode(pinLimitSwitch, OUTPUT);
                                digitalWrite(pinLimitSwitch, LOW);
                        }
                        else {
                                pinMode(pinLimitSwitch, INPUT_PULLUP);
                        }
                }

                digitalWrite(pinStepperStep, HIGH);
                delayMicroseconds(step_delay);

                digitalWrite(pinStepperStep, LOW);
                delayMicroseconds(step_delay);

                cumulative_steps += dir == HIGH ? 1 : -1;
        }

        sleep_stepper();
}

void sleep_stepper() {
        digitalWrite(pinStepperSleep, LOW);
}

void wake_stepper() {
        digitalWrite(pinStepperSleep, HIGH);
        delay(5);
}

void reset_stage() {
        int i, step_delay_us = 800;
        cumulative_steps = CUMULATIVE_STEP_LIMIT;
        for (i = 0; i < 140; i++) {
                move_steps(-100, step_delay_us);
        }
        move_steps(1300, step_delay_us);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                     RASTERING                           //
//                                                         //
/////////////////////////////////////////////////////////////

void raster_well() {
        int i, j;
        int step_delay_us = 800;
        int firings_per_raster = 3;
        int rasters_per_well = 3;
        int steps_per_well = 1200;
        int steps_per_raster = steps_per_well / (rasters_per_well + 1);
        int initial_steps = steps_per_raster / 2;
        int initial_delay = 2000;
        int solenoid_first_well = 2000;
        int solenoid_first_raster = 1000;
        int solenoid_other_rasters = 800;
        int pause_first_well = 4000;
        int pause_first_raster = 1000;
        int pause_other_rasters = 800;

        move_steps(initial_steps - 200, step_delay_us);
        delay(initial_delay);

        move_solenoid(solenoid_first_well);
        delay(pause_first_well);
        for (i = 0; i < (rasters_per_well - 1); i++) {
                for (j = 0; j < firings_per_raster; j++) {
                        move_solenoid(j ? solenoid_other_rasters : solenoid_first_raster);
                        delay(j ? pause_other_rasters : pause_first_raster);
                }
                move_steps(steps_per_raster, step_delay_us);
        }

        for (j = 0; j < firings_per_raster; j++) {
                move_solenoid(j ? solenoid_other_rasters : solenoid_first_raster);
                delay(j ? pause_other_rasters : pause_first_raster);
        }

        move_steps(initial_steps, step_delay_us);
}

void move_to_next_well() {
    int i, step_delay_us = 1200;

    for (i = 0; i < 20; i++) {
            move_steps(100, step_delay_us);
            delay(200);
    }
    move_steps(-800, step_delay_us);
    delay(2000);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                          SETUP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void setup() {
        pinMode(pinBatteryAin, INPUT);
        pinMode(pinDCinDetect, INPUT);
        pinMode(pinLimitSwitch, INPUT_PULLUP);

        pinMode(pinSolenoid, OUTPUT);
        pinMode(pinStepperStep, OUTPUT);
        pinMode(pinStepperSleep, OUTPUT);
        pinMode(pinStepperDir, OUTPUT);

        analogWrite(pinSolenoid, 0);
        digitalWrite(pinStepperStep, LOW);
        digitalWrite(pinStepperDir, LOW);
        digitalWrite(pinStepperSleep, LOW);

        Serial.begin(115200); // standard serial port

        reset_stage();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void loop() {
        int inchar;

        if (Serial.available()) {
            inchar = Serial.read();
            Serial.write(inchar);
            switch(inchar) {
                case 98: // b - move back .5mm
                    Serial.println();
                    Serial.println("Moving back .5mm");
                    move_steps(-100, 800);
                    break;
                case 102: // f - move forward .5mm
                    Serial.println();
                    Serial.println("Moving back .5mm");
                    move_steps(100, 800);
                    break;
                case 103: // g - raster well
                    Serial.println();
                    Serial.println("Rastering well");
                    raster_well();
                    break;
                case 110: // n - move to next well
                    Serial.println();
                    Serial.println("Moving to next well");
                    move_to_next_well();
                    break;
                case 114: // r - reset stage
                    Serial.println();
                    Serial.println("Resetting stage");
                    reset_stage();
                    break;
            }
        }
}
