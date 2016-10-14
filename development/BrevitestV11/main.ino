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

int extract_int_from_delimited_string(char *str, int *posPtr, char *delim) {
        char buf[14];
        char *mark;
        int len;

        mark = &str[*posPtr];
        len = strstr(mark, delim) - mark;
        len = len > 14 ? 14 : len;
        strncpy(buf, mark, len);
        *posPtr += len + strlen(delim);
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

        qr_code_being_scanned = true;

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

        if (i < CARTRIDGE_UUID_LENGTH) {
            memcpy(qr_uuid, CARTRIDGE_ERROR_UUID, CARTRIDGE_UUID_LENGTH);
        }
        qr_uuid[CARTRIDGE_UUID_LENGTH] = '\0';

        Serial1.end();
        Serial.println("Finish reading QR code");

        qr_code_being_scanned = false;

        return i;
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

int check_sensor_clear(char sensor_code) {
    uint16_t red = 0, green = 0, blue = 0, clear = 0;
    TCS34725 *sensor;
    int tries = 0;

    if (sensor_code == 'A') {
            sensor = &tcsAssay;
    }
    else {
            sensor = &tcsControl;
    }
    sensor->begin(TCS34725_INTEGRATIONTIME_154MS, TCS34725_GAIN_4X);
    while (clear == 0 && tries++ < 5) {
        sensor->getRawData(&red, &green, &blue, &clear);
    }
    sensor->end();

    return (int) clear;
}

void set_check_device_status_flag() {
    check_device_status_flag = !qr_code_being_scanned;  // light from qr scanner confounds device open reading
}

bool cartridge_loaded() {
    analogWrite(pinSensorLED, SENSOR_CHECK_CARD_LED_POWER);
    delay(SENSOR_CHECK_CARD_LED_DELAY);
    int sensor_reading = check_sensor_clear('A');
    analogWrite(pinSensorLED, 0);
    return (sensor_reading > SENSOR_CARD_CHECK_THRESHOLD);
}

void validate_cartridge() {
    cartridge_validated = false;
    Particle.publish("brevitest-validate", qr_uuid, 60, PRIVATE);
    callback_buffer[0] = '\0';
    callback_complete = false;
}

bool load_assay_record(char *assayString) {
        int indx = 0;

        Serial.println(assayString);

        memcpy(cartridge_uuid, assayString, CARTRIDGE_UUID_LENGTH);
        memcpy(assay.uuid, assayString, ASSAY_UUID_LENGTH);
        indx += CARTRIDGE_UUID_LENGTH + 2;
        memcpy(test_record.test_uuid, &assayString[indx], TEST_UUID_LENGTH);
        indx += TEST_UUID_LENGTH + 2;

        assay.duration = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
        assay.sensor_integration_time = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
        assay.sensor_gain = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
        assay.led_power = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
        assay.delay_between_sensor_readings_ms = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
        assay.BCODE_length = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
        assay.BCODE_version = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
        strcpy(assay.BCODE, &assayString[indx]);

        Serial.printlnf("%.24s %.8s %.24s %d %d %d %d %d", cartridge_uuid, assay.uuid, test_record.test_uuid, assay.duration, assay.sensor_integration_time, assay.sensor_gain, assay.led_power, assay.delay_between_sensor_readings_ms);
        Serial.printlnf("%d %d %s", assay.BCODE_length, assay.BCODE_version, assay.BCODE);

        indx += assay.BCODE_length;
        return (assayString[indx] == '\n'); // should be end of test string; if not don't start test
}

void process_validate_callback_buffer() {
    callback_complete = false;
    char result[8];
    int len = strlen(callback_buffer);

    memcpy(result, callback_buffer, 7);
    result[7] = '\0';
    cartridge_validated = (strcmp(result, VALIDATE_CARTRIDGE_SUCCESS) == 0);
    if (cartridge_validated) {
        set_device_LED_color(0, 255, 0);    // cartridge found
        turn_on_device_LED();
        load_assay_record(&callback_buffer[9]);
    }
    else {
        set_device_LED_color(255, 0, 0);    // cartridge not found
        turn_on_device_LED();
        Serial.println(callback_buffer);
    }

}

void validate_callback(const char *event, const char *data) {
    int len = strlen(callback_buffer);
    strcpy(&callback_buffer[len], &data[len ? 0 : 1]);
    len += strlen(data) - (len ? 0 : 1);
    callback_complete = (callback_buffer[len - 1] == '\"');
    if (callback_complete) {
        callback_buffer[len - 1] = '\0';
    }
    else {
        callback_buffer[len] = '\0';
    }
}

void check_device_status() {
    int sensor_reading = check_sensor_clear('A');
    /*Serial.printlnf("Checking device status: %d", sensor_reading);*/
    bool open_now = sensor_reading > SENSOR_DEVICE_OPEN_THRESHOLD;
    if (open_now ^ device_open_state) {
        if (open_now) {
            Serial.printlnf("Device just opened: %d", sensor_reading);
            memcpy(qr_uuid, DEVICE_OPEN_UUID, CARTRIDGE_UUID_LENGTH);
            set_device_LED_color(0, 0, 255);
            turn_on_device_LED();
            cartridge_validated = false;
        }
        else {
            Serial.printlnf("Device just closed: %d", sensor_reading);
            if (cartridge_loaded()) {
                Serial.println("Cartridge in device");
                if (scan_QR_code() == CARTRIDGE_UUID_LENGTH) {
                    validate_cartridge();
                }
                else {
                    set_device_LED_color(255, 0, 0);    // bad cartridge uuid
                    turn_on_device_LED();
                    cartridge_validated = false;
                }
            }
            else {
                Serial.println("No cartridge loaded");
                memcpy(qr_uuid, NO_CARTRIDGE_UUID, CARTRIDGE_UUID_LENGTH);
                set_device_LED_color(255, 255, 0);
                turn_on_device_LED();
                cartridge_validated = false;
            }
        }
        bluetooth_update_characteristic("cartridge ID", qr_uuid, gatt.cartridge_id_characteristic);
    }
    device_open_state = open_now;
    check_device_status_flag = false;
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
    int number_of_cycles = extract_int_from_delimited_string(cmdStr, &index, COMMA_DELIM);

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
//                  BLUETOOTH COMMANDS                     //
//                                                         //
/////////////////////////////////////////////////////////////

int bluetooth_command(char *cmd) {
    unsigned long timeout;
    int readInt;
    char readChar;

    Serial.println(cmd);
    Serial4.println(cmd);

    bluetooth_buffer_line_count = 0;
    bluetooth_buffer_count = 0;
    timeout = millis() + BLUETOOTH_TIMEOUT;
    while (millis() < timeout) {
        if (Serial4.available()) {
            readInt = Serial4.read();
            if (readInt != -1) {
                readChar = (char) readInt;
                Serial.print(readChar);
                bluetooth_buffer[bluetooth_buffer_count++] = readChar;
                bluetooth_buffer[bluetooth_buffer_count] = '\0';
                if (readInt == 10) {
                    bluetooth_buffer_line_count++;
                    if (strncmp(&bluetooth_buffer[bluetooth_buffer_count - 4], "OK", 2) == 0) {
                        return 1;
                    }
                    if (strncmp(&bluetooth_buffer[bluetooth_buffer_count - 7], "ERROR", 5) == 0) {
                        return -1;
                    }
                }
            }
        }
    }
    return 0;
}

int bluetooth_extract_int(int lineNumber) {
    char *start;
    int i;

    start = bluetooth_buffer;
    for (i = 0; i < lineNumber; i++) {
        start = strpbrk(start, newline) + 1;
    }
    return atoi(start);
}

void bluetooth_factory_reset() {
    if (bluetooth_command("AT+FACTORYRESET") < 1) {
        Serial.println("Failed to perform factory reset");
    }
}

void bluetooth_echo_off() {
    if (bluetooth_command("ATE=0") < 1) {
        Serial.println("Failed to turn off echo");
    }
}

void bluetooth_add_characteristic(char *cmdStr, char *name, char *value, int *characteristic) {
    sprintf(bluetooth_buffer, "%s%s", cmdStr, value);
    if (bluetooth_command(bluetooth_buffer) < 1) {
        Serial.printlnf("Failed to add %s characteristic", name);
    }
    else {
        *characteristic = bluetooth_extract_int(0);
    }
}

void bluetooth_update_characteristic(char *name, char *value, int characteristic) {
    sprintf(bluetooth_buffer, "%s%d,%s", BLUETOOTH_UPDATE_CHARACTERISTIC_STRING, characteristic, value);
    if (bluetooth_command(bluetooth_buffer) < 1) {
        Serial.printlnf("Failed to update %s characteristic", name);
    }
}

void bluetooth_add_service(char *cmdStr, char *name, int *service) {
    if (bluetooth_command(cmdStr) < 1) {
        Serial.printlnf("Failed to add %s service", name);
    }
    else {
        *service = bluetooth_extract_int(0);
    }
}

void bluetooth_reset() {
    if (bluetooth_command("ATZ") < 1) {
        Serial.println("Failed to reset");
    }
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

void update_progress(char *message, int duration) {
        int test_duration = assay.duration * 1000;

        Particle.process();
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
    bluetooth_factory_reset();
    delay(500);
    bluetooth_echo_off();
    delay(500);
    bluetooth_add_service(BLUETOOTH_ADD_SERVICE_STRING, "brevitest", &gatt.service);
    delay(500);
    bluetooth_add_characteristic(BLUETOOTH_ADD_DEVICE_ID_CHARACTERISTIC_STRING, "device ID", device_id, &gatt.device_id_characteristic);
    delay(500);
    bluetooth_add_characteristic(BLUETOOTH_ADD_CARTRIDGE_ID_CHARACTERISTIC_STRING, "cartridge ID", "", &gatt.cartridge_id_characteristic);
    delay(500);
    bluetooth_reset();
    delay(500);
    Serial.printlnf("Service: %d, Characteristics: device_id=%d, cartridge_id=%d", gatt.service, gatt.device_id_characteristic, gatt.cartridge_id_characteristic);
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

        Particle.variable("register", particle_register, STRING);
        Particle.variable("status", particle_status, STRING);
        Particle.variable("powerstatus", &power_status, INT);
        Particle.subscribe("hook-response/brevitest-upload-test", remove_test_from_cache, MY_DEVICES);
        Particle.subscribe("hook-response/brevitest-validate", validate_callback, MY_DEVICES);

        turn_off_device_LED();
        set_device_LED_color(255, 255, 0);
        turn_on_device_LED();


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
        newline[0] = '\n';
        newline[1] = '\0';
        System.deviceID().toCharArray(device_id, DEVICE_ID_LENGTH);
        device_id[DEVICE_ID_LENGTH] = '\0';

        reset_stage();
        reset_globals();

        initialize_test_cache();
        initialize_bluetooth();

        device_open_timer.start();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void reset_globals() {
        start_test = false;
        load_test = false;
        cancel_process = false;

        qr_uuid[0] = '\0';
        qr_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
        cartridge_uuid[0] = '\0';
        cartridge_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
        test_record.test_uuid[0] = '\0';
        test_record.test_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
        assay.uuid[0] = '\0';
        assay.uuid[CARTRIDGE_UUID_LENGTH] = '\0';

        particle_register[0] = '\0';
        particle_status[0] = '\n';
        particle_status[1] = '\0';

        test_progress = 0;
        test_percent_complete = 0;
        test_in_progress = false;

        last_upload = 0;
}

void load_test_callback(const char *event, const char *data) {

}

void do_load_test() {
    start_blinking_device_LED(0, 500, 0, 255, 255);

    load_test = false;
    Particle.publish("brevitest-load-test", qr_uuid, 60, PRIVATE);

    stop_blinking_device_LED();
    set_device_LED_color(0, 255, 255);
    turn_on_device_LED();
}

void do_run_test() {
        Particle.publish("brevitest-start-test", test_record.test_uuid, 60, PRIVATE);

        start_blinking_device_LED(0, 500, 0, 255, 0);

        start_test = false;
        test_in_progress = true;
        test_last_progress_update = 0;
        update_progress("Resetting device and starting test", 0);

        pinMode(pinSolenoid, OUTPUT);
        analogWrite(pinSolenoid, 0);
        analogWrite(pinSensorLED, 0);

        process_BCODE(0);

        reset_stage();
        update_progress("Test complete", -1);

        reset_globals();

        stop_blinking_device_LED();
        set_device_LED_color(0, 255, 0);
        turn_on_device_LED();

        Particle.publish("brevitest-test-complete", test_record.test_uuid, 60, PRIVATE);
}

void do_cancel_test() {
        update_progress("Test cancelled", -1);
        reset_globals();
        Particle.publish("brevitest-cancel-test", test_record.test_uuid, 60, PRIVATE);
}

void do_calibration() {
        Particle.publish("brevitest-calibrate-device", "", 60, PRIVATE);
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
        int inchar;

        if (load_test) {
                do_load_test();
        }

        if (start_test && !test_in_progress) {
                do_run_test();
        }

        if (cancel_process) {
                do_cancel_test();
        }

        if (online && calibrate) {
                do_calibration();
        }

        if (online && tests_to_upload()) {
                do_upload_tests();
        }

        if (check_device_status_flag) {
                check_device_status();
        }

        if (callback_complete) {
            process_validate_callback_buffer();
        }

        if (Serial.available()) {
                inchar = Serial.read();
                Serial4.write(inchar);
        }

        if (Serial4.available()) {
                inchar = Serial4.read();
                Serial.write(inchar);
        }

        battery_level = analogRead(pinBatteryAin);
        power_status = (battery_level > BATTERY_CONVERSION_FACTOR * 100 ? 100 : battery_level / BATTERY_CONVERSION_FACTOR) * (digitalRead(pinDCinDetect) ? -1 : 1);
}
