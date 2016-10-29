#include "TCS34725.h"
#include "main.h"
#include "Serial4/Serial4.h"

SYSTEM_THREAD(ENABLED);
PRODUCT_ID(204);
PRODUCT_VERSION(FIRMWARE_VERSION);

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

int reset_eeprom() {
        Particle_EEPROM e;

        memcpy(&eeprom, &e, (int) sizeof(Particle_EEPROM));
        erase_test_cache();
        store_eeprom();

        return 1;
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

        if (cancel_test) {
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
                if (cancel_test) {
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

        Serial.printlnf("Stop triggering QR reader, ready=%c", Serial1.available() ? 'Y' : 'N');
        digitalWrite(pinQRTrigger, LOW);

        delay(100); // allow QR code buffer to fill before reading

        Serial.println("Reading QR code from Serial1");
        do {
                Serial.print('.');
                buf = Serial1.read();
                if (buf != -1) {
                        qr_uuid[i++] = (char) buf;
                }
        } while (Serial1.available() && i < CARTRIDGE_UUID_LENGTH);
        Serial.println();
        Serial.printlnf("QR code: %s, length: %d", qr_uuid, i);

        if (i < CARTRIDGE_UUID_LENGTH) {
            memcpy(qr_uuid, CARTRIDGE_ERROR_UUID, CARTRIDGE_UUID_LENGTH);
        }
        qr_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
        Serial.printlnf("QR code: %s, length: %d", qr_uuid, i);

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
        device_LED_timer.reset();
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
//               ASSAY AND CONTROL SENSORS                 //
//                                                         //
/////////////////////////////////////////////////////////////

void init_sensor(TCS34725 *sensor, uint8_t sensor_number) {
        *sensor = TCS34725(sensor_number);
}

uint16_t check_sensor_clear(char sensor_code) {
    TCS34725 *sensor;
    uint16_t red = 0, green = 0, blue = 0, clear = 0;
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
    /*Serial.printlnf("R: %d, G: %d, B: %d, C: %d, tries: %d", red, green, blue, clear, tries);*/
    return clear;
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

void convert_samples_to_reading(char sensor_code) {
        int i;
        int red, green, blue, clear, samples, clr, clear_max, clear_min;
        BrevitestSensorSampleRecord *buffer;
        BrevitestSensorRecord *reading;

        reading = &(test_record.reading[test_record.number_of_readings]);
        reading->channel = sensor_code;
        buffer = (sensor_code == 'A' ? assay_buffer : control_buffer);
        red = green = blue = clear = clear_max = samples = 0;
        clear_min = 0xFFFF;
        for (i = 0; i < SENSOR_NUMBER_OF_SAMPLES; i += 1) {
                if (buffer[i].clear && buffer[i].clear != 0xFFFF) {
                        samples++;
                        clr = (int) buffer[i].clear;
                        red += (int) buffer[i].red;
                        green += (int) buffer[i].green;
                        blue += (int) buffer[i].blue;
                        clear += clr;
                        clear_max = (clr > clear_max ? clr : clear_max);
                        clear_min = (clr < clear_min ? clr : clear_min);
                }
        }
        reading->clear_mean = (samples ? clear / samples : 0);
        reading->red_mean = (samples ? red / samples : 0);
        reading->green_mean = (samples ? green / samples : 0);
        reading->blue_mean = (samples ? blue / samples : 0);
        reading->clear_max = clear_max;
        reading->clear_min = clear_min;
        reading->start_time = buffer[0].sample_time;

        test_record.number_of_readings++;
}

int read_sensors() { // 0 -> baseline, 1 -> test
        int i;

        analogWrite(pinSensorLED, SENSOR_LED_ASSAY);
        delay(SENSOR_LED_WARMUP_DELAY_MS);

        Serial.println("Test reading");

        for (i = 0; i < SENSOR_NUMBER_OF_SAMPLES; i += 1) {
                read_one_sensor('A', i);
                read_one_sensor('C', i);
                Serial.printlnf("%d %d %d %d %d %d %d %d %d %d %d", i, \
                    assay_buffer[i].sample_time, assay_buffer[i].clear, assay_buffer[i].red, assay_buffer[i].green, assay_buffer[i].blue, \
                    control_buffer[i].sample_time, control_buffer[i].clear, control_buffer[i].red, control_buffer[i].green, control_buffer[i].blue);
        }

        convert_samples_to_reading('A');
        convert_samples_to_reading('C');

        analogWrite(pinSensorLED, 0);
        delay(SENSOR_LED_WARMUP_DELAY_MS);

        return 1;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                     DEVICE STATUS                       //
//                                                         //
/////////////////////////////////////////////////////////////

void set_check_device_status_flag() {
    check_device_status_flag = !qr_code_being_scanned;  // light from qr scanner confounds device open reading
}

bool cartridge_loaded() {
    analogWrite(pinSensorLED, SENSOR_CHECK_CARD_LED_POWER);
    delay(SENSOR_CHECK_CARD_LED_DELAY);
    uint16_t level = check_sensor_clear('A');
    analogWrite(pinSensorLED, 0);
    Serial.printlnf("Cartridge loaded? %c, level: %d", level > SENSOR_CARD_CHECK_THRESHOLD ? 'Y' : 'N', level);
    return (level > SENSOR_CARD_CHECK_THRESHOLD);
}

void validate_cartridge() {
    bluetooth_set_status(4);
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
        indx += CARTRIDGE_UUID_LENGTH + 1;
        memcpy(test_record.test_uuid, &assayString[indx], TEST_UUID_LENGTH);
        indx += TEST_UUID_LENGTH + 1;
        test_record.number_of_readings = 0;

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

    Serial.println(callback_buffer);
    if (strncmp(&callback_buffer[7], qr_uuid, CARTRIDGE_UUID_LENGTH) == 0) { // is this my cartridge?
        cartridge_validated = (strncmp(callback_buffer, SUCCESS, 7) == 0);
        Serial.printlnf("result | cartridge ID: %.31s", callback_buffer);
        Serial.printlnf("cartridge_validated: %c", cartridge_validated ? 'Y' : 'N');
        if (cartridge_validated) {    // cartridge found
            bluetooth_set_status(6);
            start_test = load_assay_record(&callback_buffer[7]);
            stop_blinking_device_LED();
        }
        else {
            bluetooth_set_status(5);
            stop_blinking_device_LED();
            set_device_LED_color(255, 0, 0);    // cartridge not found
            turn_on_device_LED();
            Serial.println(callback_buffer);
            start_test = false;
            bluetooth_set_error_code(2);
        }
    }

}

void validate_callback(const char *event, const char *data) {
    int i;
    int datalen = strlen(data);
    int len = strlen(callback_buffer);
    int offset = (len ? 0 : 1);
    for (i = offset; i < datalen; i++) {
        if (data[i] == '\\') {
            i++;
            if (data[i] == 't') {
                callback_buffer[len++] = '\t';
            }
            else if (data[i] == 'n') {
                callback_buffer[len++] = '\n';
            }
            else {
                callback_buffer[len++] = '\\';
                callback_buffer[len++] = data[i];
            }
        }
        else {
            callback_buffer[len++] = data[i];
        }
    }
    Serial.printlnf("Last character in callback: %c, len: %d", callback_buffer[len - 1], len);
    callback_complete = (callback_buffer[len - 1] == '\"');
    if (callback_complete) {
        callback_buffer[len - 1] = '\0';
    }
    else {
        callback_buffer[len] = '\0';
    }
}

void set_cancel_test_flag() {
    cancel_test = true;
    bluetooth_set_status(15);
}

bool device_is_open() {
    uint16_t level = check_sensor_clear('A');
    /*Serial.printlnf("Checking device status: %d, bluetooth count: %d", level, bluetooth_buffer_count);*/
    return (level > SENSOR_DEVICE_OPEN_THRESHOLD);
}

void check_device_status() {
    int tries = 0;

    check_device_status_flag = false;
    bool open_now = device_is_open();
    /*Serial.printlnf("device_open_state: %c, open_now: %c", device_open_state ? 'T' : 'F', open_now ? 'T' : 'F');*/
    if (open_now ^ device_open_state) {
        if (open_now) {
            Serial.printlnf("Device just opened");
            bluetooth_set_device_open_state("Device open");
            memcpy(qr_uuid, DEVICE_OPEN_UUID, CARTRIDGE_UUID_LENGTH);
            cartridge_validated = false;
            if (test_in_progress) {
                /*bluetooth_set_status(9);*/
                /*Serial.println("Device opened during test - starting cancel timer");
                device_open_cancel_timer.reset();*/
                Serial.println("Device opened during test");
                start_blinking_device_LED(0, 100, 255, 0, 0);
            }
            else {
                if (start_test_delay.isActive()) {
                    bluetooth_set_status(7);
                    Serial.println("Test cancelled during startup");
                    start_test_delay.stop();
                    run_test = false;
                    stop_blinking_device_LED();
                }
                else {
                    bluetooth_set_status(3);
                    Serial.println("Device opened - no test started or running");
                }
                set_device_LED_color(0, 255, 255);
                turn_on_device_LED();
            }
        }
        else {
            bluetooth_set_device_open_state("Device closed");
            /*if (device_open_cancel_timer.isActive()) {*/
            if (test_in_progress) {
                /*bluetooth_set_status(8);*/
                /*Serial.println("Device closed in time - test resumed");*/
                /*device_open_cancel_timer.stop();*/
                start_blinking_device_LED(0, 500, 0, 255, 0);
            }
            else {  // no test in progress
                Serial.printlnf("Device just closed");
                start_blinking_device_LED(0, 100, 0, 0, 255);
                if (cartridge_loaded()) {
                    Serial.println("Cartridge in device");
                    if (scan_QR_code() == CARTRIDGE_UUID_LENGTH) {
                        validate_cartridge();
                    }
                    else {
                        bluetooth_set_status(2);
                        stop_blinking_device_LED();
                        set_device_LED_color(255, 0, 0);    // bad cartridge uuid
                        turn_on_device_LED();
                        cartridge_validated = false;
                        bluetooth_set_error_code(1);
                    }
                }
                else {
                    bluetooth_set_status(1);
                    Serial.println("No cartridge loaded");
                    strncpy(qr_uuid, NO_CARTRIDGE_UUID, CARTRIDGE_UUID_LENGTH);
                    stop_blinking_device_LED();
                    set_device_LED_color(128, 128, 128);
                    turn_on_device_LED();
                    cartridge_validated = false;
                }
            }
        }

        while (!bluetooth_set_cartridge_id() && ++tries < 3) {
            Particle.process();
        }
    }
    device_open_state = open_now;
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

void write_test_record_to_eeprom() {
        // increment test_index (check for overflow and if so reset circular buffer)
        int test_index = (eeprom.most_recent_test == 255 ? 0 : eeprom.most_recent_test + 1);  // 255 is the reset value
        test_index %= TEST_CACHE_SIZE;
        store_test(test_index);
        process_test_record(test_index);
}

int append_test_reading(int start, BrevitestSensorRecord *reading) {
    return sprintf(&(particle_register[start]), "%c\t%11d\t%5d\t%5d\t%5d\t%5d\t%5d\t%5d\n", \
        reading->channel, reading->start_time, reading->red_mean, reading->green_mean, reading->blue_mean, \
        reading->clear_mean, reading-> clear_max, reading->clear_min);
}

int process_test_record(int index) {
        BrevitestTestRecord *test;
        int len, i;

        test = &eeprom.test_cache[index];

        len = sprintf(particle_register, "%11d\t%11d\t%.24s\n", test->start_time, test->finish_time, test->test_uuid);
        for (i = 0; i < TEST_MAXIMUM_NUMBER_OF_READINGS; i++) {
            if (test->reading[i].channel == 'A' || test->reading[i].channel == 'C') {
                len += append_test_reading(len, &(test->reading[i]));
            }
        }
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

int try_bluetooth_command(char *cmd) {
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

int bluetooth_command(char *cmd) {
    int tries = 0;

    int result = try_bluetooth_command(cmd);
    while (result < 1 && ++tries < 3) { // retry on ERROR or timeout
        delay(2000);
        result = try_bluetooth_command(cmd);
    }
    return result;
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

bool bluetooth_factory_reset() {
    if (bluetooth_command("AT+FACTORYRESET") < 1) {
        Serial.println("Failed to perform factory reset; retrying...");
        return false;
    }
    return true;
}

bool bluetooth_echo_off() {
    if (bluetooth_command("ATE=0") < 1) {
        Serial.println("Failed to turn off echo");
        return false;
    }
    return true;
}

bool bluetooth_add_characteristic(char *cmdStr, char *name, char *value, int *characteristic) {
    sprintf(bluetooth_buffer, "%s%s", cmdStr, value);
    if (bluetooth_command(bluetooth_buffer) < 1) {
        Serial.printlnf("Failed to add %s characteristic", name);
        return false;
    }
    else {
        *characteristic = bluetooth_extract_int(0);
    }
    return true;
}

bool bluetooth_update_characteristic(char *name, char *value, int characteristic) {
    sprintf(bluetooth_buffer, "%s%d,%s", BLUETOOTH_UPDATE_CHARACTERISTIC_STRING, characteristic, value);
    if (bluetooth_command(bluetooth_buffer) < 1) {
        Serial.printlnf("Failed to update %s characteristic", name);
        return false;
    }
    return true;
}

bool bluetooth_update_characteristic(char *name, int value, int characteristic) {
    sprintf(bluetooth_buffer, "%s%d,%d", BLUETOOTH_UPDATE_CHARACTERISTIC_STRING, characteristic, value);
    if (bluetooth_command(bluetooth_buffer) < 1) {
        Serial.printlnf("Failed to update %s characteristic", name);
        return false;
    }
    return true;
}

bool bluetooth_add_service(char *cmdStr, char *name, int *service) {
    if (bluetooth_command(cmdStr) < 1) {
        Serial.printlnf("Failed to add %s service", name);
        return false;
    }
    else {
        *service = bluetooth_extract_int(0);
    }
    return true;
}

bool bluetooth_reset() {
    if (bluetooth_command("ATZ") < 1) {
        Serial.println("Failed to reset");
        return false;
    }
    return true;
}

bool bluetooth_set_cartridge_id() {
    return bluetooth_update_characteristic("cartridge ID", qr_uuid, gatt.cartridge_id_characteristic);
}

bool bluetooth_set_status(int code) {
    return bluetooth_update_characteristic("status code", code, gatt.status_code_characteristic);
}

bool bluetooth_set_device_open_state(char *state) {
    return bluetooth_update_characteristic("device open", state, gatt.device_open_characteristic);
}

bool bluetooth_set_percent_complete(int percent_complete) {
    return bluetooth_update_characteristic("percent complete", percent_complete, gatt.percent_complete_characteristic);
}

bool bluetooth_set_error_code(int code) {
    return bluetooth_update_characteristic("error message", code, gatt.error_code_characteristic);
}

bool bluetooth_set_battery_life() {
    return bluetooth_update_characteristic("battery life", power_status, gatt.battery_life_characteristic);
}


/////////////////////////////////////////////////////////////
//                                                         //
//                 TEST RESULTS UPLOADING                  //
//                                                         //
/////////////////////////////////////////////////////////////

bool tests_to_upload() {
        int i;

        if (millis() < next_upload) {
                return false;
        }

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

        if (strncmp(&data[1], SUCCESS, 7) == 0) {
            for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                    if (strncmp(&data[8], eeprom.test_cache[i].test_uuid, TEST_UUID_LENGTH) == 0) {
                            memset(&eeprom.test_cache[i].start_time, '\0', sizeof(BrevitestTestRecord));
                            store_eeprom();
                            Particle.publish("brevitest-test-cleared-from-cache", data, 60, PRIVATE);
                            return;
                    }
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

        if (cancel_test) {
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
                bluetooth_set_percent_complete(0);
        }
        else if (duration < 0) {
                test_progress = test_duration;
                test_percent_complete = -1;
                test_last_progress_update = 0;
                bluetooth_set_percent_complete(100);
        }
        else {
                test_progress += duration;
                test_percent_complete = 100 * test_progress / test_duration;
                test_percent_complete = test_percent_complete > 100 ? 100 : test_percent_complete;
                bluetooth_set_percent_complete(test_percent_complete);
        }
}

int process_one_BCODE_command(int cmd, int index) {
        int i, param1, param2, param3, start_index;

        if (cancel_test) {
                return index;
        }

        Particle.process();

        switch(cmd) {
        case 0: // Start test()
                test_record.start_time = Time.now();
                break;
        case 1: // Delay(milliseconds)
                CHECK_SENSOR_DEVICE_STATUS;
                index = get_BCODE_token(index, &param1);
                update_progress("", param1);
                delay(param1);
                break;
        case 2: // Move(number of steps, step delay)
                CHECK_SENSOR_DEVICE_STATUS;
                index = get_BCODE_token(index, &param1);
                index = get_BCODE_token(index, &param2);
                update_progress("Moving magnets", (abs(param1) * param2) / 1000);
                move_steps(param1, param2);
                break;
        case 3: // Solenoid on(milliseconds)
                CHECK_SENSOR_DEVICE_STATUS;
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
                bluetooth_set_status(11);
                read_sensors();
                bluetooth_set_status(9);
                break;
        case 10: // Read QR code
                CHECK_SENSOR_DEVICE_STATUS;
                scan_QR_code();
                break;
        case 11: // Beep (milliseconds)
                Serial.println("Beep not implemented");
                break;
        case 12: // Repeat begin(number of iterations)
                index = get_BCODE_token(index, &param1);

                start_index = index;
                for (i = 0; i < param1; i += 1) {
                        if (cancel_test) {
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
                cancel_test = true;
                ERROR_MESSAGE(-15);
                return -1;
        }
        else {
                index = process_one_BCODE_command(cmd, index);
        }

        while ((cmd != 99) && (index > 0) && !cancel_test) {
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

void erase_test_cache() {
        int *ptr;
        int i;

        Serial.println("Erasing test cache");
        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                ptr = &eeprom.test_cache[i].start_time;
                memset(ptr, '\0', sizeof(BrevitestTestRecord));
        }
}

void initialize_bluetooth() {
    int tries = 0;
    int success = false;

    while (!success && ++tries < 5) {
        delay(500);
        if (!bluetooth_factory_reset()) {
            continue;
        }
        delay(500);
        if (!bluetooth_echo_off()) {
            continue;
        }
        delay(500);
        if (!(bluetooth_add_service(BLUETOOTH_ADD_SERVICE_STRING, "brevitest", &gatt.service) && gatt.service == 1)) {
            continue;
        }
        delay(500);
        if (!(bluetooth_add_characteristic(BLUETOOTH_ADD_DEVICE_ID_CHARACTERISTIC_STRING, "device ID", device_id, &gatt.device_id_characteristic) && gatt.device_id_characteristic == 1)) {
            continue;
        }
        delay(500);
        if (!(bluetooth_add_characteristic(BLUETOOTH_ADD_CARTRIDGE_ID_CHARACTERISTIC_STRING, "cartridge ID", "", &gatt.cartridge_id_characteristic) && gatt.cartridge_id_characteristic == 2)) {
            continue;
        }
        delay(500);
        if (!(bluetooth_add_characteristic(BLUETOOTH_ADD_STATUS_CODE_CHARACTERISTIC_STRING, "status", "", &gatt.status_code_characteristic) && gatt.status_code_characteristic == 3)) {
            continue;
        }
        delay(500);
        if (!(bluetooth_add_characteristic(BLUETOOTH_ADD_DEVICE_OPEN_CHARACTERISTIC_STRING, "device open", "", &gatt.device_open_characteristic) && gatt.device_open_characteristic == 4)) {
            continue;
        }
        delay(500);
        if (!(bluetooth_add_characteristic(BLUETOOTH_ADD_PERCENT_COMPLETE_CHARACTERISTIC_STRING, "percent complete", "", &gatt.percent_complete_characteristic) && gatt.percent_complete_characteristic == 5)) {
            continue;
        }
        delay(500);
        if (!(bluetooth_add_characteristic(BLUETOOTH_ADD_CANCEL_TEST_CHARACTERISTIC_STRING, "cancel test", "", &gatt.cancel_test_characteristic) && gatt.cancel_test_characteristic == 6)) {
            continue;
        }
        delay(500);
        if (!(bluetooth_add_characteristic(BLUETOOTH_ADD_ERROR_CODE_CHARACTERISTIC_STRING, "error code", "", &gatt.error_code_characteristic) && gatt.error_code_characteristic == 7)) {
            continue;
        }
        delay(500);
        if (!(bluetooth_add_characteristic(BLUETOOTH_ADD_BATTERY_LIFE_CHARACTERISTIC_STRING, "battery life", "", &gatt.battery_life_characteristic) && gatt.battery_life_characteristic == 8)) {
            continue;
        }
        delay(500);
        if (!bluetooth_reset()) {
            continue;
        }
        success = true;
    }

    if (success) {
        Serial.printlnf("Bluetooth service %d started successfully!", gatt.service);
    }
    else {
        Serial.printlnf("Bluetooth unable to initialize after %d tries - rebooting", tries);
        System.reset();
    }
}

void set_update_battery_life_flag() {
    update_battery_life = true;
}
void calculate_power_status() {
    int battery_level = analogRead(pinBatteryAin);
    int new_power_status = (battery_level > BATTERY_CONVERSION_FACTOR * 100 ? 100 : battery_level / BATTERY_CONVERSION_FACTOR) * (digitalRead(pinDCinDetect) ? -1 : 1);
    if (abs(new_power_status - power_status) > 1) {
        power_status = new_power_status;
        bluetooth_set_battery_life();
    }
}

void watchdog() {
    Serial.println("Watchdog!");
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
        if (eeprom.firmware_version != FIRMWARE_VERSION || eeprom.data_format_version != DATA_FORMAT_VERSION) {
                reset_eeprom();
        }

        init_sensor(&tcsAssay, SENSOR_NUMBER_ASSAY);
        init_sensor(&tcsControl, SENSOR_NUMBER_CONTROL);
        delim_string[1] = '\0';
        newline[0] = '\n';
        newline[1] = '\0';
        System.deviceID().toCharArray(device_id, DEVICE_ID_LENGTH + 1);
        device_id[DEVICE_ID_LENGTH] = '\0';

        reset_stage();
        reset_globals();

        initialize_test_cache();
        initialize_bluetooth();
        calculate_power_status();

        device_open_state = !device_is_open();
        device_status_timer.reset();
        battery_check_timer.reset();

        Serial.printlnf("eeprom.firmware_version: %d, eeprom.data_format_version: %d, eeprom.most_recent_test: %d", eeprom.firmware_version, eeprom.data_format_version, eeprom.most_recent_test);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void reset_globals() {
        start_test = false;
        run_test = false;
        cancel_test = false;
        test_in_progress = false;

        test_progress = 0;
        test_percent_complete = 0;

        qr_uuid[0] = '\0';
        qr_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
        cartridge_uuid[0] = '\0';
        cartridge_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
        test_record.test_uuid[0] = '\0';
        test_record.test_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
        test_record.number_of_readings = 0;
        assay.uuid[0] = '\0';
        assay.uuid[CARTRIDGE_UUID_LENGTH] = '\0';

        particle_register[0] = '\0';
        particle_status[0] = '\n';
        particle_status[1] = '\0';

        next_upload = 0;
}

void do_run_test() {
        if (!device_is_open()) {
            Serial.println("Started test run");
            Particle.publish("brevitest-test-started", test_record.test_uuid, 60, PRIVATE);

            start_blinking_device_LED(0, 500, 0, 255, 0);

            test_in_progress = true;
            test_last_progress_update = 0;
            update_progress("Resetting device and starting test", 0);

            pinMode(pinSolenoid, OUTPUT);
            analogWrite(pinSolenoid, 0);
            analogWrite(pinSensorLED, 0);

            Particle.process();
            process_BCODE(0);

            if (cancel_test) {
                bluetooth_set_status(14);
                Serial.println("Test cancelled");
                update_progress("Test cancelled", -1);
                Particle.publish("brevitest-test-cancelled", test_record.test_uuid, 60, PRIVATE);

                stop_blinking_device_LED();
                set_device_LED_color(255, 0, 0);
                turn_on_device_LED();
            }
            else {
                bluetooth_set_status(12);
                Serial.println("Test completed");
                update_progress("Test complete", -1);
                Particle.publish("brevitest-test-completed", test_record.test_uuid, 60, PRIVATE);

                stop_blinking_device_LED();
                set_device_LED_color(0, 255, 0);
                turn_on_device_LED();
            }

            reset_globals();
            reset_stage();
        }
}

void do_calibration() {
        Particle.publish("brevitest-calibrate-device", "", 60, PRIVATE);
        calibrate = false;
        save_calibration_point();
        move_to_calibration_point();
}

void do_upload_tests() {
        int i;

        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                if (eeprom.test_cache[i].test_uuid[0] != '\0') {
                        Serial.printlnf("Processing test record: %s", eeprom.test_cache[i].test_uuid);
                        process_test_record(i);
                        Serial.println(particle_register);
                        Particle.publish("brevitest-upload-test", eeprom.test_cache[i].test_uuid, 60, PRIVATE);
                        delay(4000);
                }
        }

        next_upload = millis() + UPLOAD_INTERVAL;
}

void set_run_test_flag() {
    run_test = true;
}

void loop() {
        bool online = Particle.connected();
        int inchar;

        if (start_test) {
            Serial.println("Starting test");
            start_test = false;
            if (!test_in_progress) {
                bluetooth_set_status(7);
                Serial.println("Starting test delay");
                start_test_delay.reset();
                start_blinking_device_LED(0, 100, 0, 255, 0);
            }
        }

        if (run_test) {
            Serial.println("Test delay complete");
            run_test = false;
            if (!test_in_progress) {
                bluetooth_set_status(9);
                Serial.println("Running test");
                do_run_test();
            }
        }

        if (online && calibrate) {
                Serial.println("Calibrating");
                do_calibration();
        }

        if (online && tests_to_upload()) {
                Serial.println("Uploading tests");
                do_upload_tests();
        }

        CHECK_SENSOR_DEVICE_STATUS;

        if (callback_complete) {
            Serial.println("Processing validation callback");
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

        if (update_battery_life) {
            update_battery_life = false;
            calculate_power_status();
        }
}
