/*
 * Project SLUStepperTest
 * Description: Testing for stepper motors for Sample Loading Unit
 * Author: Leo Linbeck III
 * Date: 7 January 2021
 */

// motor
#define MOTOR_MICRONS_PER_EIGHTH_STEP 25
#define MOTOR_MOVE_DURATION_UNIT 25000
#define MOTOR_MINIMUM_STEP_DELAY 150
#define MOTOR_FAST_STEP_DELAY 200
#define MOTOR_SLOW_STEP_DELAY 1000
#define MOTOR_OSCILLATION_STEP_DELAY 250

// stage
#define STAGE_POSITION_LIMIT 39500
#define STAGE_RESET_STEPS -60000
#define STAGE_MICRONS_TO_INITIAL_POSITION 12300

// pin definitions
int pinStageLimit = D0;
int pinMotorEnable = A0;
int pinMotorDir = A1;
int pinMotorStep = A2;

// global variables
int stage_position = 0;

// logging
SerialLogHandler logHandler;

/////////////////////////////////////////////////////////////
//                                                         //
//                        MOTOR                            //
//                                                         //
/////////////////////////////////////////////////////////////

bool move_one_eighth_step(int dir, int step_delay)
{
    if (dir == HIGH)
    {
        if (digitalRead(pinStageLimit) == LOW)
        {
            Log.info("Proximal stage limit switch detected");
            stage_position = 0;
            return false;
        }
        if (stage_position <= 0)
        {
            Log.info("Stage position at zero");
            stage_position = 0;
            return false;
        }
    }
    else if (stage_position >= STAGE_POSITION_LIMIT)
    {
        Log.info("Stage distal limit reached");
        return false;
    }

    digitalWrite(pinMotorStep, HIGH);
    delayMicroseconds(step_delay);

    digitalWrite(pinMotorStep, LOW);
    delayMicroseconds(step_delay);

    return true;
}

void move_stage(int microns, int step_delay)
{
    // move a specific number of microns - negative for reverse movement
    // microns: negative => move proximally, positive => move distally

    int eighth_steps, abs_microns, dir, i, floored_step_delay;

    dir = (microns < 0) ? HIGH : LOW;
    digitalWrite(pinMotorDir, dir);
    Log.info("Stepping, dir = %c", dir == LOW ? 'L' : 'H');

    abs_microns = abs(microns);
    eighth_steps = abs_microns / MOTOR_MICRONS_PER_EIGHTH_STEP;
    floored_step_delay = step_delay < MOTOR_MINIMUM_STEP_DELAY ? MOTOR_MINIMUM_STEP_DELAY : step_delay;
    Log.info("move_stage: microns = %d, dir = %c, eighth_steps = %d, step_delay = %d", microns, dir == LOW ? 'L' : 'H', eighth_steps, floored_step_delay);

    // delay(10);
    for (i = 0; i < eighth_steps; i++)
    {
        if (move_one_eighth_step(dir, floored_step_delay)) {
            // Log.info("moving one eighth step: %d", i);

            stage_position += microns < 0 ? -MOTOR_MICRONS_PER_EIGHTH_STEP : MOTOR_MICRONS_PER_EIGHTH_STEP;
            if (stage_position <= 0) {
                stage_position = 0;
                i = eighth_steps;
            }
        } else {
            i = eighth_steps;
        }
    }
    Log.info("Move complete, stage location = %d, limit = %d", stage_position, STAGE_POSITION_LIMIT);
}

void sleep_motor()
{
    digitalWrite(pinMotorEnable, LOW);
    delay(20);
}

void wake_motor()
{
    digitalWrite(pinMotorEnable, HIGH);
    delay(10);
}

void reset_stage(bool sleep)
{
    stage_position = STAGE_POSITION_LIMIT;
    wake_motor();
    move_stage(STAGE_RESET_STEPS, MOTOR_FAST_STEP_DELAY);
    delay(100);
    move_stage(2000, MOTOR_FAST_STEP_DELAY);
    delay(100);
    move_stage(-3000, MOTOR_SLOW_STEP_DELAY);
    move_stage(STAGE_MICRONS_TO_INITIAL_POSITION, MOTOR_SLOW_STEP_DELAY);
    if (sleep)
    {
        sleep_motor();
    }
}
void init_digital_pin(uint16_t pin, PinMode mode, uint8_t value)
{
    pinMode(pin, mode);
    if (mode == OUTPUT) {
        digitalWrite(pin, value);
    }
}

// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  Serial.begin(115200);
  init_digital_pin(pinStageLimit, INPUT_PULLDOWN, LOW);
  init_digital_pin(pinMotorEnable, OUTPUT, LOW);
  init_digital_pin(pinMotorStep, OUTPUT, LOW);
  init_digital_pin(pinMotorDir, OUTPUT, LOW);
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
    if (Serial.available()) {
        while (Serial.available()) {
          Serial.read();
        }
    }

    wake_motor();
    move_stage(1000, MOTOR_SLOW_STEP_DELAY);
    delay(1000);
    move_stage(-1000, MOTOR_SLOW_STEP_DELAY);
    sleep_motor();
    delay(3000);
}