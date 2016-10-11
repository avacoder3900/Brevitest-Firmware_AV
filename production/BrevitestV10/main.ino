#include "TCS34725.h"
#include "main.h"
#include "Serial4/Serial4.h"

SYSTEM_THREAD(ENABLED);
PRODUCT_ID(204);
PRODUCT_VERSION(5);

/////////////////////////////////////////////////////////////
//                                                         //
//                        UTLITY                           //
//                                                         //
/////////////////////////////////////////////////////////////

int extract_int_from_string(char *str, int pos, int len) {
        char buf[12];

        len = len > 12 ? 12 : len;
        strncpy(buf, &str[pos], len);
        buf[len] = '\0';
        return atoi(buf);
}

int extract_int_from_delimited_string(char *str, int *posPtr, char delim) {
        char buf[14];
        char *mark;
        int len;

        delim_string[0] = delim;
        mark = &str[*posPtr];
        len = strcspn(mark, delim_string);
        len = len > 14 ? 14 : len;
        strncpy(buf, mark, len);
        *posPtr += len + 1;
        buf[len] = '\0';
        return atoi(buf);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                         EEPROM                          //
//                                                         //
/////////////////////////////////////////////////////////////

void load_eeprom() {
        uint8_t *e = (uint8_t *) &eeprom;

        for (int addr = 0; addr < (int) sizeof(Particle_EEPROM); addr++, e++) {
                *e = EEPROM.read(addr);
        }
}

void store_eeprom() {
        uint8_t *e = (uint8_t *) &eeprom;

        for (int addr = 0; addr < (int) sizeof(Particle_EEPROM); addr++, e++) {
                EEPROM.write(addr, *e);
        }
}

void erase_eeprom() {
        for (int addr = 0; addr < (int) sizeof(Particle_EEPROM); addr++) {
                EEPROM.write(addr, 0);
        }
}

void dump_eeprom() {
        uint8_t *e = (uint8_t *) &eeprom;
        uint8_t buf;

        Serial.println("EEPROM contents: ");
        for (int addr = 0; addr < (int) sizeof(Particle_EEPROM); addr++) {
                buf = EEPROM.read(addr);
                Serial.write(buf);
        }
        Serial.println();

        Serial.println("eeprom contents: ");
        for (int addr = 0; addr < (int) sizeof(Particle_EEPROM); addr++) {
                Serial.write(*e++);
        }
        Serial.println();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        SOLENOID                         //
//                                                         //
/////////////////////////////////////////////////////////////

void move_solenoid(int duration) {
        uint8_t surge = eeprom.param.solenoid_power >> 8;
        uint8_t sustain = (uint8_t) eeprom.param.solenoid_power;

        Particle.process();

        if (cancel_process) {
                return;
        }

        int sustain_time = abs(duration) - eeprom.param.solenoid_surge_period_ms;
        sustain_time = sustain_time < 0 ? 0 : sustain_time;

        pinMode(pinSolenoid, OUTPUT);
        analogWrite(pinSolenoid, surge);
        delay(eeprom.param.solenoid_surge_period_ms);

        if (sustain_time) {
                pinMode(pinSolenoid, OUTPUT);
                analogWrite(pinSolenoid, sustain);
                delay(sustain_time);
        }

        pinMode(pinSolenoid, OUTPUT);
        analogWrite(pinSolenoid, 0);

        STATUS("surge: %d, surge_time: %d, sustain: %d, sustain_time: %d", surge, eeprom.param.solenoid_surge_period_ms, sustain, sustain_time);
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
                if (cancel_process) {
                        break;
                }

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
        delay(eeprom.param.stepper_wake_delay_ms);
}

void reset_stage() {
        cumulative_steps = CUMULATIVE_STEP_LIMIT;
        move_steps(-eeprom.param.reset_steps, eeprom.param.step_delay_us);
        move_steps(1300, eeprom.param.step_delay_us);
}

void save_calibration_point() {
        uint8_t lsb, msb;
        int addr = offsetof(Particle_EEPROM, param.calibration_steps);

        msb = (uint8_t) (eeprom.param.calibration_steps >> 8);
        lsb = (uint8_t) (eeprom.param.calibration_steps & 0x0F);
        EEPROM.write(addr, msb);
        EEPROM.write(addr + 1, lsb);
}

void move_to_calibration_point() {
        CANCELLABLE(reset_stage(); )
        delay(500);
        CANCELLABLE(move_steps(eeprom.param.calibration_steps, eeprom.param.step_delay_us); )
}

/////////////////////////////////////////////////////////////
//                                                         //
//                       QR SCANNER                        //
//                                                         //
/////////////////////////////////////////////////////////////

int scan_QR_code() {
        unsigned long timeout;
        int buf, i = 0;

        Serial.println("Start scan_QR_code");
        Serial1.begin(115200); // QR scanner interface through RX/TX pins

        Serial.println("Trigger QR reader");
        digitalWrite(pinQRTrigger, HIGH);

        timeout = millis() + QR_READ_TIMEOUT;

        Serial.println("Wait for QR code");
        while (!Serial1.available() && millis() < timeout) {
                Particle.process();
        }

        Serial.println("Stop triggering QR reader");
        digitalWrite(pinQRTrigger, LOW);

        delay(100); // allow QR code buffer to fill before reading

        Serial.println("Reading QR code from Serial1");
        do {
                buf = Serial1.read();
                if (buf != -1) {
                        qr_uuid[i++] = (char) buf;
                }
        } while (Serial1.available() && i < CARTRIDGE_UUID_LENGTH);

        qr_uuid[CARTRIDGE_UUID_LENGTH] = '\0';

        Serial1.end();
        Serial.println("Finish reading QR code");

        return i;
}

int validate_cartridge_uuid() {
        scan_QR_code();
        return strncmp(cartridge_uuid, qr_uuid, CARTRIDGE_UUID_LENGTH);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                       DEVICE LED                        //
//                                                         //
/////////////////////////////////////////////////////////////

void set_device_LED_color(uint8_t red, uint8_t green, uint8_t blue) {
        device_LED.red = red;
        device_LED.green = green;
        device_LED.blue = blue;
}

int turn_on_device_LED() {
        analogWrite(pinDeviceLEDRed, device_LED.red);
        analogWrite(pinDeviceLEDGreen, device_LED.green);
        analogWrite(pinDeviceLEDBlue, device_LED.blue);
        device_LED.currently_on = true;
        return 1;
}

int turn_off_device_LED() {
        analogWrite(pinDeviceLEDRed, 0);
        analogWrite(pinDeviceLEDGreen, 0);
        analogWrite(pinDeviceLEDBlue, 0);
        device_LED.currently_on = false;
        return 1;
}

void start_blinking_device_LED(int total_duration, int rate, int red = 255, int green = 255, int blue = 255) {
        set_device_LED_color((uint8_t) red, (uint8_t) green, (uint8_t) blue);
        device_LED.blink_rate = rate;
        device_LED.blink_timeout = total_duration ? millis() + (unsigned long) total_duration : 0;
        device_LED.blinking = true;

        device_LED_timer.changePeriod(rate);
        device_LED_timer.start();
        turn_on_device_LED();
}

void stop_blinking_device_LED() {
        device_LED.blinking = false;
        device_LED_timer.stop();
        turn_off_device_LED();
}

void update_blinking_device_LED() {
        unsigned long now = millis();

        if (device_LED.currently_on) {
                turn_off_device_LED();
        }
        else {
                turn_on_device_LED();
        }

        if (device_LED.blink_timeout && now > device_LED.blink_timeout) {
                stop_blinking_device_LED();
        }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        SENSORS                          //
//                                                         //
/////////////////////////////////////////////////////////////

void init_sensor(TCS34725 *sensor, uint8_t sensor_number) {
        *sensor = TCS34725(sensor_number);
}

void read_one_sensor(char sensor_code, int sample_number) {
        BrevitestSensorSampleRecord *sample;
        TCS34725 *sensor;
        int led_power, led_delay, tries;

        Particle.process();

        if (sensor_code == 'A') {
                sample = &assay_buffer[sample_number];
                led_power = SENSOR_LED_ASSAY;
                sensor = &tcsAssay;
                led_delay = SENSOR_LED_TRANSITION_HIGH_DELAY_MS;
        }
        else {
                sample = &control_buffer[sample_number];
                led_power = SENSOR_LED_ASSAY;
                sensor = &tcsControl;
                led_delay = SENSOR_LED_TRANSITION_LOW_DELAY_MS;
        }

        analogWrite(pinSensorLED, led_power);
        delay(led_delay);

        sensor->begin(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_4X);

        sample->sample_time = Time.now();
        sample->red = sample->green = sample->blue = sample->clear = tries = 0;
        while (sample->clear == 0 && tries++ < 5) {
            sensor->getRawData(&sample->red, &sample->green, &sample->blue, &sample->clear);
        }

        sensor->end();
}

void convert_samples_to_reading(int reading_code, char sensor_code) {
        int i, j, k;
        int red, green, blue, clear;
        int index[SENSOR_NUMBER_OF_SAMPLES];
        BrevitestSensorSampleRecord *buffer;
        BrevitestSensorRecord *reading;

        Particle.process();

        if (sensor_code == 'A') {
                buffer = assay_buffer;
                if (reading_code == 0) {
                        reading = &(test_record.sensor_reading_initial_assay);
                }
                else if (reading_code == 1) {
                        reading = &(test_record.sensor_reading_final_assay);
                }
                else {
                        reading = &sensor_reading;
                }
        }
        else {
                buffer = control_buffer;
                if (reading_code == 0) {
                        reading = &(test_record.sensor_reading_initial_control);
                }
                else if (reading_code == 1) {
                        reading = &(test_record.sensor_reading_final_control);
                }
                else {
                        reading = &sensor_reading;
                }
        }

        Particle.process();

        index[0] = 0;
        for (i = 1; i < SENSOR_NUMBER_OF_SAMPLES; i += 1) {
                for (j = 0; j < i; j += 1) {
                        if (buffer[i].clear < buffer[index[j]].clear) {
                                for (k = i; k > j; k -= 1) {
                                        index[k] = index[k - 1];
                                }
                                index[j] = i;
                                break;
                        }
                        if (j == i - 1) {
                                index[i] = i;
                        }
                }
        }

        red = green = blue = 0;
        for (j = 0; j < SENSOR_NUMBER_OF_SAMPLES; j += 1) {
                Particle.process();

                i = index[j];
                Serial.print("s: ");
                Serial.print(sensor_code);
                Serial.print(" i: ");
                Serial.print(i);
                Serial.print(" t: ");
                Serial.print(buffer[i].sample_time);
                Serial.print(" C: ");
                Serial.print(buffer[i].clear);
                Serial.print(" R: ");
                Serial.print(buffer[i].red);
                Serial.print(" G: ");
                Serial.print(buffer[i].green);
                Serial.print(" B: ");
                Serial.println(buffer[i].blue);
                clear = buffer[i].clear;
                if (clear && j > 0 && j < (SENSOR_NUMBER_OF_SAMPLES - 1)) {
                        red += (int) (10000 * (clear - (int) buffer[i].red)) / clear;
                        green += (int) (10000 * (clear - (int) buffer[i].green)) / clear;
                        blue += (int) (10000 * (clear - (int) buffer[i].blue)) / clear;
                }
        }
        reading->red_norm = red / (SENSOR_NUMBER_OF_SAMPLES - 2);
        reading->green_norm = green / (SENSOR_NUMBER_OF_SAMPLES - 2);
        reading->blue_norm = blue / (SENSOR_NUMBER_OF_SAMPLES - 2);

        reading->start_time = buffer[0].sample_time;
}

int read_sensors(int reading_code) { // 0 -> baseline, 1 -> test
        int i;

        analogWrite(pinSensorLED, SENSOR_LED_ASSAY);
        delay(SENSOR_LED_WARMUP_DELAY_MS);

        if (reading_code == 0) {
            Serial.println("Baseline reading");
        }
        else {
            Serial.println("Test reading");
        }

        for (i = 0; i < SENSOR_NUMBER_OF_SAMPLES; i += 1) {
                read_one_sensor('A', i);
                read_one_sensor('C', i);
                Serial.printlnf("%d %d %d %d %d %d %d %d %d %d %d", i, \
                    assay_buffer[i].sample_time, assay_buffer[i].clear, assay_buffer[i].red, assay_buffer[i].green, assay_buffer[i].blue, \
                    control_buffer[i].sample_time, control_buffer[i].clear, control_buffer[i].red, control_buffer[i].green, control_buffer[i].blue);
        }

        analogWrite(pinSensorLED, 0);

        /*convert_samples_to_reading();*/

        return 1;
}

void print_samples_from_both_sensors_to_serial(int num) {
        int i;
        BrevitestSensorSampleRecord *buf;

        for (i = 0; i < SENSOR_NUMBER_OF_SAMPLES; i += 1) {
                /*Particle.process();*/
                buf = &assay_buffer[i];
                Serial.printlnf(" n: %d i: %d s: A t: %d C: %d R: %d G: %d B: %d", num, i, buf->sample_time, buf->clear, buf->red, buf->green, buf->blue);
        }
        for (i = 0; i < SENSOR_NUMBER_OF_SAMPLES; i += 1) {
            buf = &control_buffer[i];
            Serial.printlnf(" n: %d i: %d s: C t: %d C: %d R: %d G: %d B: %d", num, i, buf->sample_time, buf->clear, buf->red, buf->green, buf->blue);
        }
}

int read_both_sensors(char *cmdStr) {
    int i, j, index = 0;
    int number_of_cycles = extract_int_from_delimited_string(cmdStr, &index, ',');

    analogWrite(pinSensorLED, SENSOR_LED_ASSAY);
    delay(SENSOR_LED_WARMUP_DELAY_MS);

    for (i = 0; i < number_of_cycles; i += 1) {

            for (j = 0; j < SENSOR_NUMBER_OF_SAMPLES; j += 1) {
                    read_one_sensor('A', j);
                    read_one_sensor('C', j);
            }

            print_samples_from_both_sensors_to_serial(i);

            delay(1000);
    }

    analogWrite(pinSensorLED, 0);

    return(1);
}

int sensor_test() {
    int i, j, tries;
    int assay_led_power = SENSOR_LED_ASSAY;
    int control_led_power;
    BrevitestSensorSampleRecord *sample;

    analogWrite(pinSensorLED, assay_led_power);
    delay(SENSOR_LED_WARMUP_DELAY_MS);

    for (i = 0; i < 25; i += 1) {

            for (j = 0; j < SENSOR_NUMBER_OF_SAMPLES; j += 1) {
                analogWrite(pinSensorLED, assay_led_power);
                delay(SENSOR_LED_TRANSITION_HIGH_DELAY_MS);

                tcsAssay.begin(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_4X);

                sample = &assay_buffer[j];
                sample->sample_time = Time.now();
                sample->red = sample->green = sample->blue = sample->clear = tries = 0;
                while (sample->clear == 0 && tries++ < 5) {
                    tcsAssay.getRawData(&(sample->red), &(sample->green), &(sample->blue), &(sample->clear));
                }

                tcsAssay.end();

                control_led_power = (898 * assay_led_power) / 1000;
                analogWrite(pinSensorLED, control_led_power);
                delay(SENSOR_LED_TRANSITION_LOW_DELAY_MS);

                tcsControl.begin(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_4X);

                sample = &control_buffer[j];
                sample->sample_time = Time.now();
                sample->red = sample->green = sample->blue = sample->clear = tries = 0;
                while (sample->clear == 0 && tries++ < 5) {
                    tcsControl.getRawData(&(sample->red), &(sample->green), &(sample->blue), &(sample->clear));
              }

                tcsControl.end();
            }

            print_samples_from_both_sensors_to_serial(i);

            delay(1000);

            assay_led_power -= 10;
            if (assay_led_power < 0) {
                assay_led_power = 0;
            }
    }

    analogWrite(pinSensorLED, 0);

    return(1);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                   EEPROM TEST CACHE                     //
//                                                         //
/////////////////////////////////////////////////////////////

int find_test_index_by_uuid(char *uuid) {
        if (uuid[0] == '\0') {
                return -1;
        }

        for (int i = 0; i < TEST_CACHE_SIZE; i += 1) {
                if (strncmp(uuid, eeprom.test_cache[i].test_uuid, TEST_UUID_LENGTH) == 0) {
                        return i;
                }
        }

        return -1;
}

void write_test_record_to_eeprom() {
        // increment test_index (check for overflow and if so reset circular buffer)
        int test_index = eeprom.most_recent_test + 1;
        test_index %= TEST_CACHE_SIZE;
        store_test(test_index);
        process_test_record(test_index);
}

void store_test(int index) {
        uint8_t *e = (uint8_t *) &test_record;
        int start_addr = offsetof(Particle_EEPROM, test_cache) + index * (int) sizeof(BrevitestTestRecord);

        memcpy(&eeprom.test_cache[index], e, sizeof(BrevitestTestRecord));
        for (int addr = start_addr; addr < (start_addr + (int) sizeof(BrevitestTestRecord)); addr++, e++) {
                EEPROM.write(addr, *e);
        }
        eeprom.most_recent_test = index;
        EEPROM.write(offsetof(Particle_EEPROM, most_recent_test), eeprom.most_recent_test);
}

int process_test_record(int index) {
        BrevitestTestRecord *test;
        int len;

        test = &eeprom.test_cache[index];

        len = snprintf(particle_register, PARTICLE_REGISTER_SIZE, \
                       "%11d\t%11d\t%.24s\n%11d\t%5d\t%5d\t%5d\n%11d\t%5d\t%5d\t%5d\n%11d\t%5d\t%5d\t%5d\n%11d\t%5d\t%5d\t%5d\n", \
                       test->start_time, test->finish_time, test->test_uuid, \
//
                       test->sensor_reading_initial_assay.start_time, test->sensor_reading_initial_assay.red_norm, \
                       test->sensor_reading_initial_assay.green_norm, test->sensor_reading_initial_assay.blue_norm, \
                       test->sensor_reading_initial_control.start_time, test->sensor_reading_initial_control.red_norm, \
                       test->sensor_reading_initial_control.green_norm, test->sensor_reading_initial_control.blue_norm, \
                       test->sensor_reading_final_assay.start_time, test->sensor_reading_final_assay.red_norm, \
                       test->sensor_reading_final_assay.green_norm, test->sensor_reading_final_assay.blue_norm, \
                       test->sensor_reading_final_control.start_time, test->sensor_reading_final_control.red_norm, \
                       test->sensor_reading_final_control.green_norm, test->sensor_reading_final_control.blue_norm);

        particle_register[len] = '\0';
        return 1;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                         PARAMS                          //
//                                                         //
/////////////////////////////////////////////////////////////

int reset_params() {
        Param reset;

        memcpy(&eeprom.param, &reset, (int) sizeof(Param));
        store_params();

        return 1;
}

void load_params() {
        uint8_t *e = (uint8_t *) &eeprom.param;
        int indx, addr = offsetof(Particle_EEPROM, param);

        for (indx = 0; indx < (int) sizeof(Param); indx++, e++) {
                *e = EEPROM.read(addr + indx);
        }
}

void store_params() {
        uint8_t *e = (uint8_t *) &eeprom.param;
        int indx, addr = offsetof(Particle_EEPROM, param);

        for (indx = 0; indx < (int) sizeof(Param); indx++, e++) {
                EEPROM.write(addr + indx, *e);
        }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                     STRESS TEST                         //
//                                                         //
/////////////////////////////////////////////////////////////

bool stop_stress_test() {
        process_bluetooth_input();
        return (!stress_test);
}

void raster_well() {
        int i, j;

        delay(1000);
        for (i = 0; i < 4; i++) {
                move_solenoid(1200);
                delay(1000);
                for (j = 0; j < 7; j++) {
                        if (stop_stress_test()) return;
                        move_solenoid(750);
                        delay(1000);
                }
                move_steps(200, eeprom.param.step_delay_us);
        }
        move_solenoid(1200);
        delay(1000);
        for (j = 0; j < 7; j++) {
                if (stop_stress_test()) return;
                move_solenoid(750);
                delay(1000);
        }
        delay(2000);
        for (i = 0; i < 20; i++) {
                if (stop_stress_test()) return;
                move_steps(100, eeprom.param.step_delay_us);
                delay(1500);
        }
        move_steps(-800, eeprom.param.step_delay_us);
}

void do_next_stress_test() {
        // based upon CDC Ebola IgG assay
        int i, j;

        if (stop_stress_test()) return;

        switch (eeprom.stress_test_count % STRESS_TEST_STEPS) {
        case 0: // reset stage
                reset_stage();
                break;
        case 1:
                scan_QR_code();
                break;
        case 2: // scan QR code
                read_sensors(0);
                break;
        case 3: // move to microbead well
                move_steps(1200, eeprom.param.step_delay_us);
                delay(4000);
                break;
        case 4: // collect microbeads and drag to analyte well
                for (i = 0; i < 20; i++) {
                        if (stop_stress_test()) return;
                        move_steps(100, eeprom.param.step_delay_us);
                        delay(1500);
                }
                move_steps(-800, eeprom.param.step_delay_us);
                break;
        case 5: // raster analyte well
                raster_well();
                break;
        case 6: // raster buffer well
                raster_well();
                break;
        case 7: // raster antibody well
                raster_well();
                break;
        case 8: // raster indicator well
                delay(1000);
                for (i = 0; i < 12; i++) {
                        move_solenoid(1200);
                        delay(1000);
                        for (j = 0; j < 10; j++) {
                                if (stop_stress_test()) return;
                                move_solenoid(750);
                                delay(1000);
                        }
                        move_steps(100, eeprom.param.step_delay_us);
                }
                for (i = 0; i < 12; i++) {
                        move_solenoid(1200);
                        delay(1000);
                        for (j = 0; j < 10; j++) {
                                if (stop_stress_test()) return;
                                move_solenoid(750);
                                delay(1000);
                        }
                        move_steps(-100, eeprom.param.step_delay_us);
                }
                break;
        case 9: // read assay sensor
                read_sensors(1);
                break;
        }
        if (eeprom.stress_test_count % STRESS_TEST_STEPS == 0) {
                EEPROM.write(offsetof(Particle_EEPROM, stress_test_count), eeprom.stress_test_count / STRESS_TEST_STEPS);
        }

        eeprom.stress_test_count++;
        STATUS("Stress test - cycles = %d\n", eeprom.stress_test_count / STRESS_TEST_STEPS);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                  BLUETOOTH COMMANDS                     //
//                                                         //
/////////////////////////////////////////////////////////////

int bluetooth_scan_qr_code() {
        if (test_in_progress) {
                send_bluetooth_message("QR:BUSY", "");
                return 0;
        }
        int qr_char_count = scan_QR_code();
        if (qr_char_count == 0) {
                send_bluetooth_message("QR:NONE", "");
                return 0;
        }
        if (qr_char_count != CARTRIDGE_UUID_LENGTH) {
                send_bluetooth_message("QR:MISREAD", "");
                return 0;
        }

        send_bluetooth_message("QR:", qr_uuid);
        return 1;
}

bool load_assay_record(char *assayString) {
        int indx = 0;

        strncpy(assay.uuid, cartridge_uuid, ASSAY_UUID_LENGTH);
        assay.uuid[ASSAY_UUID_LENGTH] = '\0';

        assay.duration = extract_int_from_delimited_string(assayString, &indx, '\t');
        assay.sensor_integration_time = extract_int_from_delimited_string(assayString, &indx, '\t');
        assay.sensor_gain = extract_int_from_delimited_string(assayString, &indx, '\t');
        assay.led_power = extract_int_from_delimited_string(assayString, &indx, '\t');
        assay.delay_between_sensor_readings_ms = extract_int_from_delimited_string(assayString, &indx, '\t');
        assay.BCODE_length = extract_int_from_delimited_string(assayString, &indx, '\t');
        assay.BCODE_version = extract_int_from_delimited_string(assayString, &indx, '\t');
        strncpy(assay.BCODE, &assayString[indx], assay.BCODE_length);

        indx += assay.BCODE_length;
        return (assayString[indx] == '\n'); // should be end of test string; if not don't start test
}

int bluetooth_run_brevitest(char *runBrevitestString) {
        if (test_in_progress) {
                return 0;
        }
        // copy cartridge id to cartridge_uuid
        Serial.println("Start bluetooth_run_brevitest");
        strncpy(cartridge_uuid, runBrevitestString, CARTRIDGE_UUID_LENGTH);
        cartridge_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
        // read QR code and verify that it matches cartridge_uuid
        if (validate_cartridge_uuid() == 0) {
                // move runBrevitestString pointer to point to test id
                Serial.println("Cartridge validated");
                runBrevitestString += CARTRIDGE_UUID_LENGTH + 1;
                // copy test id to test_uuid of current test
                strncpy(test_record.test_uuid, runBrevitestString, TEST_UUID_LENGTH);
                test_record.test_uuid[TEST_UUID_LENGTH] = '\0';
                // move runBrevitestString pointer to point to assay
                runBrevitestString += TEST_UUID_LENGTH + 1;
                // try to load assay record
                if (load_assay_record(runBrevitestString)) {
                        // assay loaded, set start_test flag and push initial progress update
                        start_test = true;
                        test_last_progress_update = 0;
                        update_progress("Resetting device and starting test", 0);
                        return 1;
                }
                else {
                        return -320;
                }
        }
        else {
                return -321;
        }
}

int bluetooth_command_move_stage(char *cmdParam) {
        Particle.publish("brevitest-command", "bluetooth_command_move_stage", 60, PRIVATE);
        int steps = extract_int_from_string(cmdParam, 0, PARTICLE_COMMAND_PARAM_LENGTH);
        move_steps(steps, eeprom.param.step_delay_us);
        return cumulative_steps;
}

int bluetooth_command_energize_solenoid(char *cmdParam) {
        Particle.publish("brevitest-command", "bluetooth_command_energize_solenoid", 60, PRIVATE);
        int duration = extract_int_from_string(cmdParam, 0, PARTICLE_COMMAND_PARAM_LENGTH);
        move_solenoid(duration);
        return 1;
}

int bluetooth_command_turn_on_device_LED(char *cmdParam) {
        Particle.publish("brevitest-command", "bluetooth_command_turn_on_device_LED", 60, PRIVATE);
        int red, green, blue, indx = 0;

        red = extract_int_from_delimited_string(cmdParam, &indx, ',');
        green = extract_int_from_delimited_string(cmdParam, &indx, ',');
        blue = extract_int_from_delimited_string(cmdParam, &indx, '\n');

        device_LED.blinking = false;
        device_LED_timer.stop();
        set_device_LED_color((uint8_t) red, (uint8_t) green, (uint8_t) blue);
        return turn_on_device_LED();
}

int bluetooth_command_turn_off_device_LED() {
        Particle.publish("brevitest-command", "bluetooth_command_turn_off_device_LED", 60, PRIVATE);
        device_LED.blinking = false;
        device_LED_timer.stop();
        return turn_off_device_LED();
}

int bluetooth_command_blink_device_LED(char *cmdParam) {
        Particle.publish("brevitest-command", "bluetooth_command_blink_device_LED", 60, PRIVATE);
        int duration, rate, red, green, blue, indx = 0;

        duration = extract_int_from_delimited_string(cmdParam, &indx, ',');
        rate = extract_int_from_delimited_string(cmdParam, &indx, ',');
        red = extract_int_from_delimited_string(cmdParam, &indx, ',');
        green = extract_int_from_delimited_string(cmdParam, &indx, ',');
        blue = extract_int_from_delimited_string(cmdParam, &indx, '\n');

        start_blinking_device_LED(duration, rate, red, green, blue);
        return 1;
}

int bluetooth_command_reset_stage() {
        Particle.publish("brevitest-command", "bluetooth_command_reset_stage", 60, PRIVATE);
        reset_stage();
        return 1;
}

int bluetooth_command_read_sensors() {
        Particle.publish("brevitest-command", "bluetooth_command_read_assay_sensor", 60, PRIVATE);
        return read_sensors(0);
}

int bluetooth_command_read_both_sensors(char *cmdParam) {
        Particle.publish("brevitest-command", "bluetooth_command_read_both_sensors", 60, PRIVATE);
        return read_both_sensors(cmdParam);
}

int bluetooth_command_start_new_stress_test() {
        Particle.publish("brevitest-command", "bluetooth_command_start_new_stress_test", 60, PRIVATE);
        start_blinking_device_LED(0, 500, 255, 0, 255);
        stress_test = true;
        eeprom.stress_test_count = 0;
        STATUS("Starting stress test\n");
        return 1;
}

int bluetooth_command_pause_stress_test() {
        Particle.publish("brevitest-command", "bluetooth_command_pause_stress_test", 60, PRIVATE);
        set_device_LED_color(0, 0, 255);
        turn_on_device_LED();
        device_LED.blinking = false;
        stress_test = false;
        STATUS("Stress test paused\n");
        return 1;
}

int bluetooth_command_get_stress_test_count() {
        Particle.publish("brevitest-command", "bluetooth_command_get_stress_test_count", 60, PRIVATE);
        if (stress_test) {
                return eeprom.stress_test_count;
        } else {
                return (int) EEPROM.read(offsetof(Particle_EEPROM, stress_test_count));
        }
}

int bluetooth_command_resume_stress_test() {
        Particle.publish("brevitest-command", "bluetooth_command_resume_stress_test", 60, PRIVATE);
        start_blinking_device_LED(0, 500, 255, 0, 255);
        stress_test = true;
        STATUS("Resuming stress test\n");
        return 1;
}

int bluetooth_command_turn_on_sensor_LED(char *cmdParam) {
        Particle.publish("brevitest-command", "bluetooth_command_turn_on_sensor_LED", 60, PRIVATE);
        assay.led_power = extract_int_from_string(cmdParam, 0, PARTICLE_COMMAND_PARAM_LENGTH);
        analogWrite(pinSensorLED, assay.led_power);
        return 1;
}

int bluetooth_command_turn_off_sensor_LED() {
        Particle.publish("brevitest-command", "bluetooth_command_turn_off_sensor_LED", 60, PRIVATE);
        analogWrite(pinSensorLED, 0);
        return 1;
}

int process_bluetooth_command(char *cmdStr) {
        int indx = 0;
        int code = extract_int_from_delimited_string(cmdStr, &indx, ',');
        Serial.print("Bluetooth command: ");
        Serial.println(cmdStr);
        cmdStr += indx;

        switch (code) {
        case 1:     // start Brevitest
                return bluetooth_run_brevitest(cmdStr);
        case 10:     // start stress test
                return bluetooth_command_start_new_stress_test();
        case 11:     // stop stress test
                return bluetooth_command_pause_stress_test();
        case 12:     // stop stress test
                return bluetooth_command_get_stress_test_count();
        case 13:
                return bluetooth_command_resume_stress_test();
        case 50:
                return bluetooth_command_move_stage(cmdStr);
        case 51:
                return bluetooth_command_energize_solenoid(cmdStr);
        case 52:
                return bluetooth_command_turn_on_device_LED(cmdStr);
        case 53:
                return bluetooth_command_turn_off_device_LED();
        case 54:
                return bluetooth_command_blink_device_LED(cmdStr);
        case 55:
                return bluetooth_command_reset_stage();
        case 100:
                return bluetooth_command_read_sensors();
        case 102:
                return bluetooth_command_turn_on_sensor_LED(cmdStr);
        case 103:
                return bluetooth_command_turn_off_sensor_LED();
        case 104:
                return bluetooth_command_read_both_sensors(cmdStr);
        }

        return -123;
}

void post_process_bluetooth(int result) {
        char restr[13];
        itoa(result, restr, 10);

        if (result < 0) {
                send_bluetooth_message("ERR:", restr);
        }
        else {
                send_bluetooth_message("END:", restr);
        }
}

void bluetooth_send_test_uuids_that_are_complete(char *test_uuids) {
        int i, count = 0;
        int indx = 0;
        int test_index[TEST_CACHE_SIZE];
        char *mark = particle_status;

        while (test_uuids[indx] != '\n' && indx < BLUETOOTH_BUFFER_SIZE - 8) {
                i = find_test_index_by_uuid(&test_uuids[indx]);
                if (i != -1) {
                        test_index[count++] = i;
                }
                indx += TEST_UUID_LENGTH + 1;
        }

        if (count) {
                for (i = 0; i < count; i += 1) {
                        strncpy(mark, eeprom.test_cache[test_index[i]].test_uuid, TEST_UUID_LENGTH);
                        mark += TEST_UUID_LENGTH;
                        if (i == (count - 1)) {
                                *mark = '\n';
                        }
                        else {
                                *mark = '\t';
                        }
                        mark++;
                }

                *mark = '\0';
                send_bluetooth_message("TESTS:", particle_status);
        }
}

void send_bluetooth_message(char *cmd, char *payload) {
        int len = strlen(payload);

        Serial4.print(cmd);
        if (len) {
                Serial4.print(payload);
                if (payload[len - 1] != '\n') {
                        Serial4.write('\n');
                }
        }
        else {
                Serial4.write('\n');
        }
}

bool read_serial4_until_ok() {
    int match_count = 0;
    int inchar;
    char c;
    unsigned long timeout = millis() + 5000;

    delay(100);
    while (!Serial4.available() && millis() < timeout) {
        Particle.process();
    }

    while (millis() < timeout) {
        inchar = Serial4.read();
        if (inchar != -1) {
            Serial.write(inchar);
            c = (char) inchar;
            switch (match_count) {
                case 0:
                    match_count = (c == 'O') ? 1 : 0;
                    break;
                case 1:
                    match_count = (c == 'K') ? 2 : 0;
                    break;
                case 2:
                    match_count = (c == '\r') ? 3 : 0;
                    break;
                case 3:
                    if (c == '\n') {
                        return true;
                    }
                    match_count = 0;
                    break;
                default:
                    match_count = 0;
            }
        }
    };

    return false;
}

void enter_bluetooth_command_mode() {
    /*digitalWrite(pinBluetoothMode, HIGH);*/
    Serial4.println("ATZ");
    read_serial4_until_ok();
    Serial4.println("+++");
    read_serial4_until_ok();
}

void exit_bluetooth_command_mode() {
    Serial4.println("+++");
    read_serial4_until_ok();
    /*digitalWrite(pinBluetoothMode, LOW);*/
}

void send_bluetooth_command(char *cmd, char *payload) {

        int len = strlen(payload);

        // bluetooth debugging code
        /*Serial.print("BT: ");
        Serial.print(cmd);
        Serial.println(payload);*/

        enter_bluetooth_command_mode();

        Serial4.print(cmd);
        delay(50);
        if (len) {
                Serial4.print(payload);
                delay(50);
                if (payload[len - 1] != '\n') {
                        Serial4.write('\n');
                }
        }
        else {
                Serial4.write('\n');
        }

        read_serial4_until_ok();

        exit_bluetooth_command_mode();
}

void process_bluetooth_input() {
        int serialDatum, result;

        if (!Serial4.available()) {
                return;
        }
        Serial.println("Bluetooth input detected");
        bluetooth_buffer_count = 0;
        do {
                serialDatum = Serial4.read();
                if (serialDatum != -1) {
                        Serial.write(serialDatum);
                        bluetooth_buffer[bluetooth_buffer_count++] = (char) serialDatum;
                }
        } while ((serialDatum != (int) '\n') && (bluetooth_buffer_count < BLUETOOTH_BUFFER_SIZE));

        if (strncmp(bluetooth_buffer, "OK", 2) == 0) {
                return;
        }

        if (strncmp(bluetooth_buffer, "ERROR", 5) == 0) {
                Serial.println("Command error detected");
                return;
        }

        if (strncmp(bluetooth_buffer, "CONNECT", 7) == 0) {
                if (!stress_test) {
                        set_device_LED_color(0, 0, 255);
                        turn_on_device_LED();
                }
                send_bluetooth_message("CONNECTED", "");
                return;
        }

        if (strncmp(bluetooth_buffer, "SCANQR", 6) == 0) {
                bluetooth_scan_qr_code();
                return;
        }

        if (test_in_progress) {
                if (strncmp(bluetooth_buffer, "CANCEL", 6) == 0) {
                        if (strncmp(&bluetooth_buffer[7], test_record.test_uuid, TEST_UUID_LENGTH) == 0) {
                                Serial.println("CANCELLED");
                                send_bluetooth_message("CANCELLED:", test_record.test_uuid);
                                reset_stage();
                                cancel_process = true;
                        }
                        else {
                                post_process_bluetooth(0);
                        }
                }
                else if (strncmp(bluetooth_buffer, "UPDATE", 6) == 0) {
                        send_bluetooth_message("UPDATE:", particle_status);
                }
                else {
                        post_process_bluetooth(-136);
                }
                return;
        }

        if (strncmp(bluetooth_buffer, "DISCONNECT", 10) == 0) {
                if (!stress_test) {
                        set_device_LED_color(0, 255, 0);
                        turn_on_device_LED();
                }
                send_bluetooth_message("DISCONNECTED", "");
                return;
        }

        if(strncmp(bluetooth_buffer, "CMD", 3) == 0) {
                post_process_bluetooth(process_bluetooth_command(&bluetooth_buffer[3]));
                return;
        }

        if (strncmp(bluetooth_buffer, "UPDATE", 6) == 0) {
                if (bluetooth_buffer[6] == '\n') { // no records to update
                        send_bluetooth_message("NO_UPDATE", "");
                        return;
                }
                else {
                        bluetooth_send_test_uuids_that_are_complete(&bluetooth_buffer[7]); // skip past UPDATE:
                        return;
                }
        }

        post_process_bluetooth(-111);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                 TEST RESULTS UPLOADING                  //
//                                                         //
/////////////////////////////////////////////////////////////

bool tests_to_upload() {
        int i;

        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                if (eeprom.test_cache[i].test_uuid[0] != '\0') {
                        return true;
                }
        }

        return false;
}

void remove_test_from_cache(const char *event, const char *data)
{
        int i;
        BrevitestTestRecord *test;

        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                if (strncmp(&data[1], eeprom.test_cache[i].test_uuid, TEST_UUID_LENGTH) == 0) {
                        memset(&eeprom.test_cache[i].start_time, '\0', sizeof(BrevitestTestRecord));
                        store_eeprom();
                        Particle.publish("brevitest-test-cleared-from-cache", data, 60, PRIVATE);
                        return;
                }
        }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                    PARTICLE COMMANDS                    //
//                                                         //
/////////////////////////////////////////////////////////////

int particle_command_read_serial_number() {
        Particle.publish("brevitest-command", "particle_command_read_serial_number", 60, PRIVATE);
        memcpy(particle_register, eeprom.serial_number, SERIAL_NUMBER_LENGTH + 1); // includes trailing \0
        return 1;
}

int particle_command_write_serial_number() {
        Particle.publish("brevitest-command", "particle_command_write_serial_number", 60, PRIVATE);
        for (int i = 0; i < SERIAL_NUMBER_LENGTH; i += 1) {
                eeprom.serial_number[i] = particle_command.param[i];
                EEPROM.write(offsetof(Particle_EEPROM, serial_number) + i, particle_command.param[i]);
        }
        eeprom.serial_number[SERIAL_NUMBER_LENGTH] = '\0';
        EEPROM.write(offsetof(Particle_EEPROM, serial_number) + SERIAL_NUMBER_LENGTH, 0);
        return 1;
}

int particle_command_write_and_move_to_calibration_point() {
        Particle.publish("brevitest-command", "particle_command_write_and_move_to_calibration_point", 60, PRIVATE);
        eeprom.param.calibration_steps = extract_int_from_string(particle_command.param, 0, PARTICLE_COMMAND_PARAM_LENGTH);
        calibrate = true;
        return 1;
}

int particle_command_read_param() {
        Particle.publish("brevitest-command", "particle_command_read_param", 60, PRIVATE);

        int index = 0;
        int param_num;
        uint16_t *addr = &eeprom.param.reset_steps;

        param_num = extract_int_from_delimited_string(particle_command.param, &index, '\n');
        if (param_num < 0) {
                return -148;
        }
        if (param_num >= PARAM_NUMBER_OF_PARAMS) {
                return -149;
        }

        return addr[param_num];
}

int particle_command_write_param() {
        Particle.publish("brevitest-command", "particle_command_write_param", 60, PRIVATE);

        int index = 0;
        int param_num, value;
        uint16_t *addr = &eeprom.param.reset_steps;

        param_num = extract_int_from_delimited_string(particle_command.param, &index, ',');
        if (param_num < 0) {
                return -150;
        }
        if (param_num >= PARAM_NUMBER_OF_PARAMS) {
                return -151;
        }

        value = extract_int_from_delimited_string(particle_command.param, &index, '\n');
        addr[param_num] = (uint16_t) value & 0xFFFF;

        store_eeprom();

        return 1;
}

int particle_command_reset_params() {
        Particle.publish("brevitest-command", "particle_command_reset_params", 60, PRIVATE);
        reset_params();
        return 1;
}

int particle_command_read_all_params() {
        Particle.publish("brevitest-command", "particle_command_read_all_params", 60, PRIVATE);
        uint16_t *value = &eeprom.param.reset_steps;
        int i, index = 0;

        for (i = 0; i < PARAM_NUMBER_OF_PARAMS; i += 1) {
                index += sprintf(&particle_register[index], "%d,", value[i]);
        }
        particle_register[--index] = '\0';
        return 1;
}

int particle_command_read_firmware_version() {
        Particle.publish("brevitest-command", "particle_command_read_firmware_version", 60, PRIVATE);
        int version = EEPROM.read(0);
        return version;
}

int particle_command_read_QR_code() {
        Particle.publish("brevitest-command", "particle_command_read_QR_code", 60, PRIVATE);

        particle_register[0] = '\0';
        cartridge_uuid[0] = '\0';

        int scan_result = scan_QR_code();

        if (scan_result == CARTRIDGE_UUID_LENGTH) {
                memcpy(particle_register, qr_uuid, CARTRIDGE_UUID_LENGTH);
                particle_register[CARTRIDGE_UUID_LENGTH] = '\0';
                return 1;
        }
        else {
                return scan_result;
        }
}

int particle_command_read_battery_level() {
        Particle.publish("brevitest-command", "particle_command_read_battery_level", 60, PRIVATE);
        return analogRead(pinBatteryAin);
}

int particle_command_read_DC_in_status() {
        Particle.publish("brevitest-command", "particle_command_read_DC_in_status", 60, PRIVATE);
        return digitalRead(pinDCinDetect);
}

int particle_command_read_percent_complete() {
        Particle.publish("brevitest-command", "particle_command_read_percent_complete", 60, PRIVATE);
        return test_percent_complete;
}

int particle_command_read_test_record_by_uuid() {
        Particle.publish("brevitest-command", "particle_command_read_test_record_by_uuid", 60, PRIVATE);
        int index = find_test_index_by_uuid(particle_command.param);
        if (index == -1) {
                return -3;
        }

        return process_test_record(index);
}

int particle_command_read_test_record_by_index() {
        Particle.publish("brevitest-command", "particle_command_read_test_record_by_index", 60, PRIVATE);
        int index = extract_int_from_string(particle_command.param, 0, PARTICLE_COMMAND_PARAM_LENGTH);
        return process_test_record(index);
}

int particle_command_read_last_test_record() {
        Particle.publish("brevitest-command", "particle_command_read_last_test_record", 60, PRIVATE);
        return process_test_record(eeprom.most_recent_test);
}

int particle_command_read_test_cache_uuids() {
        Particle.publish("brevitest-command", "particle_command_read_test_cache_uuids", 60, PRIVATE);

        int index = 0;
        for (int i = 0; i < TEST_CACHE_SIZE; i++) {
                if (eeprom.test_cache[i].test_uuid[0] != '\0') {
                        memcpy(&particle_register[index], eeprom.test_cache[i].test_uuid, TEST_UUID_LENGTH);
                        index += TEST_UUID_LENGTH;
                        if (i == TEST_CACHE_SIZE - 1) {
                                particle_register[index++] = '\0';
                        }
                        else {
                                particle_register[index++] = '\n';
                        }
                }
        }
        return 1;
}

int particle_command_erase_test_cache() {
        Particle.publish("brevitest-command", "particle_command_erase_test_cache", 60, PRIVATE);

        int i;

        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                memset(&eeprom.test_cache[i].start_time, '\0', sizeof(BrevitestTestRecord));
        }

        eeprom.most_recent_test = -1;
        store_eeprom();

        return 1;
}

int particle_command_dump_eeprom() {
        Particle.publish("brevitest-command", "particle_command_dump_eeprom", 60, PRIVATE);
        dump_eeprom();
        snprintf(particle_register, PARTICLE_REGISTER_SIZE, \
                 "%6d\t%5d\t%5d\t%5d\t%5d\t%5d\t%5d\n", \
                 eeprom.param.reset_steps, eeprom.param.step_delay_us, eeprom.param.publish_interval_during_move,
                 eeprom.param.stepper_wake_delay_ms, eeprom.param.solenoid_power, eeprom.param.solenoid_surge_period_ms, \
                 eeprom.param.calibration_steps);
        return 1;
}

int particle_command_erase_eeprom() {
        Particle.publish("brevitest-command", "particle_command_erase_eeprom", 60, PRIVATE);
        erase_eeprom();
        return 1;
}

int particle_command_read_both_sensors() {
        Particle.publish("brevitest-command", "particle_command_read_both_sensors", 60, PRIVATE);
        return read_both_sensors(particle_command.param);
}

int particle_command_test_sensors() {
        Particle.publish("brevitest-command", "particle_command_test_sensors", 60, PRIVATE);
        return sensor_test();
}


//
//

void parse_particle_command(String msg) {
        int len = msg.length();
        msg.toCharArray(particle_command.arg, len + 1);

        particle_command.code = extract_int_from_string(particle_command.arg, PARTICLE_COMMAND_CODE_INDEX, PARTICLE_COMMAND_CODE_LENGTH);
        len -= PARTICLE_COMMAND_CODE_LENGTH;
        len = len < 0 ? 0 : len;
        strncpy(particle_command.param, &particle_command.arg[PARTICLE_COMMAND_PARAM_INDEX], len);
        particle_command.param[len] = '\0';
}

int run_command(String msg) {
        parse_particle_command(msg);

        switch (particle_command.code) {
        // configuration functions
        case 1: // set device serial number
                return particle_command_read_serial_number();
        case 2: // set device serial number
                return particle_command_write_serial_number();
        case 3: // set and move to calibration point
                return particle_command_write_and_move_to_calibration_point();
        case 4: // read device parameter
                return particle_command_read_param();
        case 5: // write device parameter
                return particle_command_write_param();
        case 6: // reset device parameters to default
                return particle_command_reset_params();
        case 7: // reset device parameters to default
                return particle_command_read_all_params();
        case 8: // get current firmware version number
                return particle_command_read_firmware_version();
        case 9: // read QR code of cartridge
                return particle_command_read_QR_code();
        case 10:
                return particle_command_read_battery_level();
        case 11:
                return particle_command_read_DC_in_status();
        case 12:
                return particle_command_read_percent_complete();
        case 13:
                return particle_command_read_last_test_record();
        case 14:
                return particle_command_read_test_record_by_uuid();
        case 15:
                return particle_command_read_test_record_by_index();
        case 16:
                return particle_command_read_test_cache_uuids();
        case 17:
                return particle_command_erase_test_cache();
        case 18:
                return particle_command_dump_eeprom();
        case 19:
                return particle_command_erase_eeprom();
        case 22:
                return particle_command_read_both_sensors();
        case 23:
                return particle_command_test_sensors();
        default:
                return -1;
        }
        return -1;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                          BCODE                          //
//                                                         //
/////////////////////////////////////////////////////////////

int get_BCODE_token(int index, int *token) {
        int i;
        char *bcode = assay.BCODE;

        if (cancel_process) {
                return index;
        }

        if (bcode[index] == '\0' || bcode[index] == '\n') { // end of string
                return index; // return end of string location
        }

        if (bcode[index] == '\t') { // command has no parameter
                return index + 1; // skip past parameter
        }

        // there is a parameter to extract
        i = index;
        while (i < ASSAY_BCODE_CAPACITY) {
                if (bcode[i] == '\0' || bcode[i] == '\n') {
                        *token = extract_int_from_string(bcode, index, (i - index));
                        return i; // return end of string location
                }
                if ((bcode[i] == '\t') || (bcode[i] == ',')) {
                        *token = extract_int_from_string(bcode, index, (i - index));
                        i++; // skip past parameter
                        return i;
                }

                i++;
        }

        return i;
}

void update_bluetooth_status_data(char *message) {
        if (message[0] != '\0') {
                STATUS("%s\t%.24s\t%d\n", message, test_record.test_uuid, test_percent_complete);
        }
}

void update_progress(char *message, int duration) {
        int test_duration = assay.duration * 1000;

        Particle.process();
        if (cancel_process) {
                update_bluetooth_status_data(message);
        }
        else {
                if (duration == 0) {
                        test_progress = 0;
                        test_percent_complete = 0;
                }
                else if (duration < 0) {
                        test_progress = test_duration;
                        test_percent_complete = -1;
                        test_last_progress_update = 0;
                }
                else {
                        test_progress += duration;
                        test_percent_complete = 100 * test_progress / test_duration;
                        test_percent_complete = test_percent_complete > 100 ? 100 : test_percent_complete;
                }

                update_bluetooth_status_data(message);
        }
}

int process_one_BCODE_command(int cmd, int index) {
        int i, param1, param2, param3, start_index;

        if (cancel_process) {
                return index;
        }

        Particle.process();

        switch(cmd) {
        case 0: // Start test()
                test_record.start_time = Time.now();
                read_sensors(0); // read initial values
                break;
        case 1: // Delay(milliseconds)
                index = get_BCODE_token(index, &param1);
                update_progress("", param1);
                delay(param1);
                break;
        case 2: // Move(number of steps, step delay)
                index = get_BCODE_token(index, &param1);
                index = get_BCODE_token(index, &param2);
                update_progress("Moving magnets", (abs(param1) * param2) / 1000);
                move_steps(param1, param2);
                break;
        case 3: // Solenoid on(milliseconds)
                index = get_BCODE_token(index, &param1);
                update_progress("Rastering magnets", param1);
                move_solenoid(param1);
                break;
        case 4: // Device LED on white
                set_device_LED_color(255, 255, 255);
                turn_on_device_LED();
                break;
        case 5: // Device LED off
                turn_off_device_LED();
                break;
        case 6: // Device LED on with color
                index = get_BCODE_token(index, &param1); // red
                index = get_BCODE_token(index, &param2); // green
                index = get_BCODE_token(index, &param3); // blue
                set_device_LED_color((uint8_t) param1, (uint8_t) param2, (uint8_t) param3);
                turn_on_device_LED();
                break;
        case 7: // Sensor LED on(power)
                index = get_BCODE_token(index, &param1);
                analogWrite(pinSensorLED, param1);
                break;
        case 8: // Sensor LED off
                analogWrite(pinSensorLED, 0);
                break;
        case 9: // Read sensors
                read_sensors(1); // read final values
                break;
        case 10: // Read QR code
                scan_QR_code();
                break;
        case 11: // Beep (milliseconds)
                Serial.println("Beep not implemented");
                break;
        case 12: // Repeat begin(number of iterations)
                index = get_BCODE_token(index, &param1);

                start_index = index;
                for (i = 0; i < param1; i += 1) {
                        if (cancel_process) {
                                break;
                        }
                        index = process_BCODE(start_index);
                }
                break;
        case 13: // Repeat end
                return -index;
                break;
        case 14: // Enable sensor (integration time, gain)
                Serial.println("Enable sensor not implemented");
                break;
        case 15: // Disable sensor
                Serial.println("Disable sensor not implemented");
                break;
        case 99: // Finish test
                test_record.finish_time = Time.now();
                write_test_record_to_eeprom();
                break;
        }

        return index;
}

int process_BCODE(int start_index) {
        int cmd, index;

        Particle.process();
        index = get_BCODE_token(start_index, &cmd);
        if ((start_index == 0) && (cmd != 0)) { // first command
                cancel_process = true;
                ERROR_MESSAGE(-15);
                return -1;
        }
        else {
                index = process_one_BCODE_command(cmd, index);
        }

        while ((cmd != 99) && (index > 0) && !cancel_process) {
                Particle.process();
                index = get_BCODE_token(index, &cmd);
                index = process_one_BCODE_command(cmd, index);
                process_bluetooth_input();
        };

        return (index > 0 ? index : -index);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                          SETUP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void initialize_test_cache() {
        bool changed = false;
        int *ptr;
        int i;

        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                ptr = &eeprom.test_cache[i].start_time;
                if (*ptr == -1) {
                        memset(ptr, '\0', sizeof(BrevitestTestRecord));
                        changed = true;
                }
        }

        if (changed) {
                store_eeprom();
        }
}

void initialize_bluetooth() {
    char dev[25];

    send_bluetooth_command("ATZ", "");
    System.deviceID().toCharArray(dev, 25);
    dev[24] = '\0';
    send_bluetooth_command("AT+GAPDEVNAME=", dev);
    delay(500);
    send_bluetooth_command("ATZ", "");
}

void setup() {
        pinMode(pinBatteryAin, INPUT);
        pinMode(pinDCinDetect, INPUT);
        pinMode(pinLimitSwitch, INPUT_PULLUP);

        pinMode(pinAssaySDA, INPUT);
        pinMode(pinAssaySCL, INPUT);
        pinMode(pinControlSDA, INPUT);
        pinMode(pinControlSCL, INPUT);

        pinMode(pinBatteryLED, OUTPUT);
        pinMode(pinSensorLED, OUTPUT);
        pinMode(pinDeviceLEDRed, OUTPUT);
        pinMode(pinDeviceLEDGreen, OUTPUT);
        pinMode(pinDeviceLEDBlue, OUTPUT);
        pinMode(pinSolenoid, OUTPUT);
        pinMode(pinStepperStep, OUTPUT);
        pinMode(pinStepperSleep, OUTPUT);
        pinMode(pinStepperDir, OUTPUT);
        pinMode(pinQRTrigger, OUTPUT);
        pinMode(pinBluetoothMode, OUTPUT);

        digitalWrite(pinSensorLED, LOW);
        analogWrite(pinSolenoid, 0);
        digitalWrite(pinStepperStep, LOW);
        digitalWrite(pinStepperDir, LOW);
        digitalWrite(pinStepperSleep, LOW);
        digitalWrite(pinQRTrigger, LOW);
        digitalWrite(pinBluetoothMode, LOW);

        Particle.function("runcommand", run_command);
        Particle.variable("register", particle_register, STRING);
        Particle.variable("status", particle_status, STRING);
        Particle.variable("powerstatus", &power_status, INT);
        Particle.subscribe("hook-response/brevitest-upload-test", remove_test_from_cache, MY_DEVICES);

        turn_off_device_LED();

        Serial.begin(115200); // standard serial port
        Serial4.begin(9600); // bluetooth serial port

        load_eeprom();

        if (eeprom.firmware_version != FIRMWARE_VERSION) {
                EEPROM.write(offsetof(Particle_EEPROM, firmware_version), FIRMWARE_VERSION);
                eeprom.firmware_version = FIRMWARE_VERSION;
        }

        if (eeprom.data_format_version != DATA_FORMAT_VERSION) {
                EEPROM.write(offsetof(Particle_EEPROM, data_format_version), DATA_FORMAT_VERSION);
                eeprom.data_format_version = DATA_FORMAT_VERSION;
                reset_params();
        }

        init_sensor(&tcsAssay, SENSOR_NUMBER_ASSAY);
        init_sensor(&tcsControl, SENSOR_NUMBER_CONTROL);
        delim_string[1] = '\0';

        initialize_test_cache();
        initialize_bluetooth();

        reset_stage();
        reset_globals();

        set_device_LED_color(0, 255, 0);
        turn_on_device_LED();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void reset_globals() {
        start_test = false;
        cancel_process = false;

        qr_uuid[0] = '\0';
        cartridge_uuid[0] = '\0';
        test_record.test_uuid[0] = '\0';

        particle_register[0] = '\0';
        particle_status[0] = '\n';
        particle_status[1] = '\0';

        test_progress = 0;
        test_percent_complete = 0;
        test_in_progress = false;

        last_upload = 0;
}

void do_run_test() {
        unsigned long timeout;

        start_blinking_device_LED(0, 500, 0, 255, 255);
        start_test = false;
        test_in_progress = true;
        test_last_progress_update = millis();

        pinMode(pinSolenoid, OUTPUT);
        analogWrite(pinSolenoid, 0);
        analogWrite(pinSensorLED, 0);

        move_to_calibration_point();

        process_BCODE(0);

        reset_stage();
        update_progress("Test complete", -1);

        stop_blinking_device_LED();
        set_device_LED_color(0, 255, 0);
        turn_on_device_LED();

        Particle.publish("brevitest-test-complete", test_record.test_uuid, 60, PRIVATE);

        reset_globals();
}

void do_cancel_test() {
        update_progress("Test cancelled", -1);
        reset_globals();
}

void do_calibration() {
        calibrate = false;
        save_calibration_point();
        move_to_calibration_point();
}

void do_upload_tests() {
        int i;

        if (millis() < last_upload + UPLOAD_INTERVAL) {
                return;
        }

        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                if (eeprom.test_cache[i].test_uuid[0] != '\0') {
                        process_test_record(i);
                        Particle.publish("brevitest-upload-test", particle_register, 60, PRIVATE);
                        delay(4000);
                        Particle.process();
                }
        }

        last_upload = millis();
}

void loop() {
        int battery_level;
        bool online = Particle.connected();
        unsigned long timeout;
        int inchar;

        if (start_test && !test_in_progress) {
                Particle.publish("brevitest-start-test", test_record.test_uuid, 60, PRIVATE);
                do_run_test();
        }

        if (cancel_process) {
                do_cancel_test();
                Particle.publish("brevitest-cancel-test", test_record.test_uuid, 60, PRIVATE);
                /*}*/
        }

        process_bluetooth_input();

        if (online && calibrate) {
                do_calibration();
        }

        if (online && stress_test) {
                do_next_stress_test();
        }

        if (online && tests_to_upload()) {
                do_upload_tests();
        }

        /*if (Serial4.available()) {
            inchar = Serial4.read();
            Serial.write(inchar);
        }*/

        battery_level = analogRead(pinBatteryAin);
        power_status = (battery_level > BATTERY_CONVERSION_FACTOR * 100 ? 100 : battery_level / BATTERY_CONVERSION_FACTOR) * (digitalRead(pinDCinDetect) ? -1 : 1);
}
