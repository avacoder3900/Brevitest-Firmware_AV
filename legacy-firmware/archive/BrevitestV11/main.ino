#include "TCS34725.h"
#include "main.h"
#include "Serial4/Serial4.h"

SYSTEM_THREAD(ENABLED);
PRODUCT_ID(4347);
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

static uint32_t crc32_tab[] = {
	0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
	0xe963a535, 0x9e6495a3,	0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
	0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
	0xf3b97148, 0x84be41de,	0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
	0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,	0x14015c4f, 0x63066cd9,
	0xfa0f3d63, 0x8d080df5,	0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
	0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,	0x35b5a8fa, 0x42b2986c,
	0xdbbbc9d6, 0xacbcf940,	0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
	0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
	0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
	0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,	0x76dc4190, 0x01db7106,
	0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
	0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
	0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
	0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
	0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
	0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
	0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
	0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
	0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
	0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
	0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
	0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
	0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
	0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
	0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
	0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
	0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
	0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
	0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
	0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
	0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
	0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
	0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
	0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
	0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
	0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
	0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
	0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
	0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
	0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
	0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
	0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

uint32_t checksum(char *buf, int size) {
	uint8_t *p;
    uint32_t crc = ~0U;

	p = (uint8_t *) buf;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
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

        if (cancelling_test) {
                return;
        }

        int sustain_time = abs(duration) - eeprom.param.solenoid_surge_period_ms;
        sustain_time = sustain_time < 0 ? 0 : sustain_time;

        pinMode(pinSolenoid, OUTPUT);
        analogWrite(pinSolenoid, surge);
        delay(eeprom.param.solenoid_surge_period_ms);

        if (sustain_time) {
                pinMode(pinSolenoid, OUTPUT);
                analogWrite(pinSolenoid, sustain, SOLENOID_PWM_FREQUENCY);
                delay(sustain_time);
        }

        pinMode(pinSolenoid, OUTPUT);
        analogWrite(pinSolenoid, 0);

        /*Serial.printlnf("surge: %d, surge_time: %d, sustain: %d, sustain_time: %d", surge, eeprom.param.solenoid_surge_period_ms, sustain, sustain_time);*/
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        STEPPER                          //
//                                                         //
/////////////////////////////////////////////////////////////

void move_steps(int steps, int step_delay){
        //rotate a specific number of steps - negative for reverse movement

        int dir = (steps > 0) ? HIGH : LOW;

        steps = abs(steps);

        digitalWrite(pinStepperDir,dir);

        for(long i = 0; i < steps; i += 1) {
                if (cancelling_test) {
                        break;
                }

                if (Particle.connected() && (i % MOVE_STEPS_BETWEEN_PARTICLE_PROCESS) == 0) {
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

        //sleep_stepper();
}

void wake_move_sleep_stepper(int steps, int step_delay) {
    wake_stepper();
    move_steps(steps, step_delay);
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
        wake_stepper();
        move_steps(-eeprom.param.reset_steps, eeprom.param.step_delay_us);
        move_steps(STEPS_TO_MICROBEAD_WELL + eeprom.param.steps_to_calibration_point, eeprom.param.step_delay_us);
        sleep_stepper();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                       QR SCANNER                        //
//                                                         //
/////////////////////////////////////////////////////////////

int scan_qr_code() {
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
//                        LASERS                           //
//                                                         //
/////////////////////////////////////////////////////////////

void turn_on_assay_laser() {
    digitalWrite(pinAssayLaser, HIGH);
}

void turn_on_control_laser() {
    digitalWrite(pinControlLaser, HIGH);
}

void turn_off_assay_laser() {
    digitalWrite(pinAssayLaser, LOW);
}

void turn_off_control_laser() {
    digitalWrite(pinControlLaser, LOW);
}

void turn_off_both_lasers() {
    digitalWrite(pinAssayLaser, LOW);
    digitalWrite(pinControlLaser, LOW);
}

void turn_on_assay_laser_for_duration(int duration) {
    turn_on_assay_laser();
    delay(duration);
    turn_off_assay_laser();
}

void turn_on_control_laser_for_duration(int duration) {
    turn_on_control_laser();
    delay(duration);
    turn_off_control_laser();
}

void turn_on_both_lasers_for_duration(int duration) {
    turn_on_assay_laser();
    turn_on_control_laser();
    delay(duration);
    turn_off_assay_laser();
    turn_off_control_laser();
}

/////////////////////////////////////////////////////////////
//                                                         //
//               ASSAY AND CONTROL SENSORS                 //
//                                                         //
/////////////////////////////////////////////////////////////

void init_sensor(TCS34725 *sensor, uint8_t sensor_number) {
        *sensor = TCS34725(sensor_number);
        sensor->begin(SENSOR_DEFAULT_INTEGRATION_TIME, SENSOR_DEFAULT_GAIN);
        /*delay(STATE_SENSOR_STARTUP_DELAY);*/
}

void read_one_sensor(char sensor_code, int it, int gain, int samples) {
        BrevitestSensorRecord *reading = &(test_record.reading[test_record.number_of_readings]);
        TCS34725 *sensor;
        int tries, lvalue, it_delay;

        Particle.process();

        if (sensor_code == 'A') {
            sensor = &tcsAssay;
            turn_on_assay_laser();
        }
        else {
            sensor = &tcsControl;
            turn_on_control_laser();
        }

        delay(LASER_WARMUP_DELAY_MS);

        it_delay = (24 * (256 - it)) / 10;
        sensor->begin((tcs34725IntegrationTime_t) it, (tcs34725Gain_t) gain);
        delay(it_delay + 20);

        reading->channel = sensor_code;
        reading->red = reading->green = reading->blue = reading->clear = reading->time_ms = reading->samples = tries = 0;
        while (reading->clear == 0 && tries++ < 10) {
            lvalue = sensor->takeReading(reading, samples, it_delay, true);
        }

        sensor->end();

        turn_off_both_lasers();

        /*Serial.printlnf("%c %d %u %d %d %d %d %d", \
            reading->channel, reading->samples, reading->time_ms, reading->clear, reading->red, reading->green, reading->blue, lvalue);*/

        ++test_record.number_of_readings %= TEST_MAXIMUM_NUMBER_OF_READINGS;
}

int read_sensors_with_parameters(int integrationTime, int gain, int samples) {
        RGB.control(true);
        RGB.color(0, 0, 0);

        turn_off_device_LED();

        read_one_sensor('A', integrationTime, gain, samples);
        read_one_sensor('C', integrationTime, gain, samples);

        turn_on_device_LED();

        RGB.control(false);

        return 1;
}

int read_sensors() {
        read_sensors_with_parameters(assay.sensor_integration_time, assay.sensor_gain, 30);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                CARTRIDGE VALIDATION                     //
//                                                         //
/////////////////////////////////////////////////////////////

void validate_cartridge() {
    Serial.println("Validating cartridge");

    if (Particle.connected()) {
        waiting_for_validation = true;
        validation_timeout = millis() + TIMEOUT_VALIDATION;

        start_blinking_device_LED(0, 100, 255, 0, 255);
        cartridge_validated = false;
        brevitest_publish("validate-cartridge", qr_uuid, false);
    }
    else {
        Serial.println("Validation failed - not connected to the cloud");
        waiting_for_validation = false;
        cartridge_validated = false;
        stop_blinking_device_LED();
        set_device_LED_color(255, 0, 0);    // not connected to the cloud
        turn_on_device_LED();
    }
}

bool load_assay_record(char *cartridgeId, char *assayString) {
    int indx = 0;
    int crc_loaded, crc_calculated;

    memcpy(cartridge_uuid, cartridgeId, CARTRIDGE_UUID_LENGTH);
    memcpy(assay.uuid, cartridgeId, ASSAY_UUID_LENGTH);
    memcpy(test_record.test_uuid, assayString, TEST_UUID_LENGTH);
    indx += TEST_UUID_LENGTH + 1;
    test_record.number_of_readings = 0;

    assay.duration = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
    assay.sensor_integration_time = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
    assay.sensor_gain = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
    assay.reserved = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
    assay.delay_between_sensor_readings_ms = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
    assay.BCODE_length = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
    assay.BCODE_version = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
    crc_loaded = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
    strncpy(assay.BCODE, &assayString[indx], assay.BCODE_length);
    assay.BCODE[assay.BCODE_length] = '\0';
    crc_calculated = abs(checksum(assay.BCODE, assay.BCODE_length - 2));
    Serial.printlnf("BCODE checksums: %u, %u", crc_loaded, crc_calculated);
    return (crc_loaded == crc_calculated); // bcode loaded if checksums match
}

/////////////////////////////////////////////////////////////
//                                                         //
//                 PUBLISH AND CALLBACKS                   //
//                                                         //
/////////////////////////////////////////////////////////////

void brevitest_publish(char *event_name, char *data, bool retry) {
    if (!retry) {
      current_event_tries = 0;
    }
    current_event_tries++;

    strcpy(current_event, event_name);
    strcpy(current_data, data);

    callback_complete = false;
    callback_buffer[0] = '\0';
    Particle.publish(String("brevitest"), String(event_name) + String("\n") + String(data), PRIVATE, NO_ACK);
    Serial.printlnf("Publish: %s, %s, try: %d", event_name, data, current_event_tries);
}

void callback_validate(char *cartridgeId, char *assayString) {
    waiting_for_validation = false;
    check_device_state();
    if (device_open) {
        stop_blinking_device_LED();
        set_device_LED_color(0, 255, 255);    // cartridge not found
        turn_on_device_LED();
    }
    else {
        cartridge_validated = (strncmp(callback_status, SUCCESS, 7) == 0);
        Serial.printlnf("Cartridge validated? %c", cartridge_validated ? 'Y' : 'N');
        if (cartridge_validated) {    // cartridge found
            if (load_assay_record(cartridgeId, assayString)) {
                starting_test = true;
            }
            else {
                Serial.println("Failed to load assay record");
                start_blinking_device_LED(0, 100, 255, 0, 0);
            }
        }
        else {
            stop_blinking_device_LED();
            set_device_LED_color(255, 0, 0);    // cartridge not found
            turn_on_device_LED();
            Serial.printlnf("cartridgeId: %s, assayString: %s", cartridgeId, assayString);
        }
    }
}

void callback_test_start() {
    test_startup_successful = (strncmp(callback_status, SUCCESS, 7) == 0);
    if (test_startup_successful) {
        Serial.println("Test started");
        waiting_for_start_confirmation = false;
    }
    else {
        Serial.println("Test start failed!");
    }
}

void callback_test_finish() {
    bool success = (strncmp(callback_status, SUCCESS, 7) == 0);
    if (success) {
        Serial.println("Test finished");
        waiting_for_finish_confirmation = false;
        reset_stage();
        reset_globals();
        next_upload = 0;
    }
    else {
        Serial.println("Test finish failed!");
    }
}

void callback_test_cancel() {
    bool success = (strncmp(callback_status, SUCCESS, 7) == 0);
    if (success) {
        Serial.println("Test cancelled");
        waiting_for_cancel_confirmation = false;
        reset_stage();
        reset_globals();
        next_upload = 0;
    }
    else {
        Serial.println("Test cancel failed!");
    }
}

void callback_test_upload(char *testId) {
    bool success = (strncmp(callback_status, SUCCESS, 7) == 0);
    if (success) {
        Serial.printlnf("Test successfully uploaded; removing test %s from cache", testId);
        waiting_for_upload_confirmation = false;
        remove_test_from_cache(testId);
    }
    else {
        Serial.println("Test upload failed!");
    }
}

char *extract_callback_params() {
    int indx;
    char *mark;

    mark = callback_buffer;

    indx = strcspn(mark, RETURN_DELIM);
    strncpy(callback_event, mark, indx);
    callback_event[indx] = '\0';
    mark += indx + 1;

    indx = strcspn(mark, RETURN_DELIM);
    strncpy(callback_status, mark, indx);
    callback_status[indx] = '\0';
    mark += indx + 1;

    indx = strcspn(mark, RETURN_DELIM);
    strncpy(callback_target, mark, indx);
    callback_target[indx] = '\0';
    mark += indx + 1;

    return mark;
}

void clean_callback_buffer() {
    int i, len;
    char *from = callback_buffer;
    char *to = callback_buffer;

    len = strlen(callback_buffer);
    from++;
    for (i = 0; i < len - 2; i++) {
        if (*from == '\\') {
            from++;
            i++;
            if (*from == 't') {
                *to++ = '\t';
            }
            else if (*from == 'n') {
                *to++ = '\n';
            }
            else {
                *to++ = '\\';
                *to++ = *from;
            }
            from++;
        }
        else {
            *to++ = *from++;
        }
    }
    *to = '\0';
}

void cancel_or_retry_publish() {
  if (current_event_tries < 5) {
    Serial.printlnf("ERROR: bad callback for event %s - retrying", current_event);
    brevitest_publish(current_event, current_data, true);
  }
  else {
    Serial.printlnf("ERROR: bad callback for event %s - maximum number of retries exceeded", current_event);
  }
}

void process_callback_buffer() {
    char *data_mark;

    callback_complete = false;
    if (callback_buffer[0] != '\"') {
        Serial.printlnf("Missing lead quote. Callback buffer: %s", callback_buffer);
        cancel_or_retry_publish();
        return;
    }

    clean_callback_buffer();
    data_mark = extract_callback_params();
    Serial.printlnf("Event: %s, status: %s, target: %s, data: %s", callback_event, callback_status, callback_target, data_mark);

    if (strcmp(callback_event, current_event) != 0) {
        Serial.printlnf("Wrong event. Callback buffer: %s", callback_buffer);
        cancel_or_retry_publish();
        return;
    }

    if (strcmp(callback_event, "validate-cartridge") == 0) {
        callback_validate(callback_target, data_mark);
    }
    else if (strcmp(callback_event, "test-start") == 0) {
        callback_test_start();
    }
    else if (strcmp(callback_event, "test-finish") == 0) {
        callback_test_finish();
    }
    else if (strcmp(callback_event, "test-cancel") == 0) {
        callback_test_cancel();
    }
    else if (strcmp(callback_event, "test-upload") == 0) {
        callback_test_upload(callback_target);
    }
}

void brevitest_error(const char *event, const char *data) {
      strcat(callback_buffer, data);
      int len = strlen(data);
      callback_complete = (len < 512) || (data[len - 1] == '\"');
      Serial.printlnf("Callback error - event: %s, data: %s", event, data);
}

void brevitest_callback(const char *event, const char *data) {
      strcat(callback_buffer, data);
      int len = strlen(data);
      callback_complete = (len < 512) || (data[len - 1] == '\"');
      /*Serial.printlnf("callback_buffer: %s, callback_complete: %c", callback_buffer, callback_complete ? 'Y' : 'N');*/
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
    return sprintf(&(particle_register[start]), "%c\t%11d\t%5d\t%5d\t%5d\t%5d\n", \
        reading->channel, reading->time_ms, reading->red, reading->green, reading->blue, reading->clear);
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
//                 TEST RESULTS UPLOADING                  //
//                                                         //
/////////////////////////////////////////////////////////////

bool tests_to_upload() {
        int i;

        if (millis() < next_upload) {
                return false;
        }
        /*Serial.println("Looking for test to upload");*/
        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                if (eeprom.test_cache[i].test_uuid[0] != '\0') {
                        Serial.printlnf("Test found: %s", eeprom.test_cache[i].test_uuid[0]);
                        return true;
                }
        }
        next_upload = millis() + UPLOAD_INTERVAL;

        return false;
}

void remove_test_from_cache(char *testId)
{
        int i;
        BrevitestTestRecord *test;

        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                if (strncmp(testId, eeprom.test_cache[i].test_uuid, TEST_UUID_LENGTH) == 0) {
                        memset(&eeprom.test_cache[i].start_time, '\0', sizeof(BrevitestTestRecord));
                        store_eeprom();
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

        if (cancelling_test) {
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
        int new_percent_complete;
        int test_duration = assay.duration * 1000;

        /*Particle.process();*/
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
                new_percent_complete = 100 * test_progress / test_duration;
                new_percent_complete = new_percent_complete > 100 ? 100 : new_percent_complete;
                if (new_percent_complete != test_percent_complete) {
                    test_percent_complete = new_percent_complete;
                }
        }
}

int process_one_BCODE_command(int cmd, int index) {
        int i, j, mark, param1, param2, param3, param4, param5, param6, param7, param8, start_index, steps;

        if (cancelling_test) {
                return index;
        }

        Particle.process();

        switch(cmd) {
        case 0: // Start test()
                test_record.start_time = Time.now();
                break;
        case 1: // Delay(milliseconds)
                index = get_BCODE_token(index, &param1);
                update_progress("", param1);
                delay(param1);
                break;
        case 2: // Move(number of steps, step delay)
                index = get_BCODE_token(index, &param1);    // number of steps
                index = get_BCODE_token(index, &param2);    // step_delay_us
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
        case 9: // Read sensors with default values
                read_sensors();
                break;
        case 10: // Read sensors with parameters
                index = get_BCODE_token(index, &param1); // integration time
                index = get_BCODE_token(index, &param2); // gain
                read_sensors_with_parameters(param1, param2, 30);
                break;
        case 11: // Repeat in SINGLE_THREADED_BLOCK begin(number of iterations) - now the same as regular Repeat
        case 12: // Repeat begin(number of iterations)
                index = get_BCODE_token(index, &param1);

                start_index = index;
                for (i = 0; i < param1; i += 1) {
                        if (cancelling_test) {
                                break;
                        }
                        index = process_BCODE(start_index);
                }
                break;
        case 13: // Repeat end
                return -index;
                break;
        case 17: // Raster well
                index = get_BCODE_token(index, &param1);    // total steps
                index = get_BCODE_token(index, &param2);    // step_delay_us
                index = get_BCODE_token(index, &param3);    // number of rasters
                Serial.printlnf("Raster well - total steps: %d, rasters: %d", param1, param3);
                if (param3 == 1) {
                    steps = param1;
                }
                else {
                    steps = param1 / (param3 - 1);
                }
                index = get_BCODE_token(index, &param4);    // number of firings
                index = get_BCODE_token(index, &param5);    // gather_time_ms
                index = get_BCODE_token(index, &param6);    // number of firing segments
                mark = index;
                for (i = 0; i < param3 - 1; i++) {
                        if (cancelling_test) {
                                break;
                        }
                        index = mark;
                        for (j = 0; j < param4; j++) {
                            if (j < param6) {
                                index = get_BCODE_token(index, &param7);    // energize time
                                index = get_BCODE_token(index, &param8);    // delay time
                            }
                            move_solenoid(param7);
                            delay(param8);
                        }
                        move_steps(steps, param2);
                }

                index = mark;
                for (j = 0; j < param4; j++) {
                    if (j < param6) {
                        index = get_BCODE_token(index, &param7);    // energize time
                        index = get_BCODE_token(index, &param8);    // delay time
                    }
                    move_solenoid(param7);
                    delay(param8);
                }

                steps = param1 - steps * (param3 - 1);
                if (steps) {
                    move_steps(steps, param2);
                }

                delay(param5);   // gather beads
                break;
        case 18: // Well transit
                index = get_BCODE_token(index, &param1);    // step_delay_us
                index = get_BCODE_token(index, &param2);    // gather_time_ms
                index = get_BCODE_token(index, &param3);    // number of segments
                Serial.printlnf("Well transit - segments: %d", param3);
                for (i = 0; i < param3; i++) {
                        if (cancelling_test) {
                                break;
                        }
                        // read segment data
                        index = get_BCODE_token(index, &param4);    // number of iterations
                        index = get_BCODE_token(index, &param5);    // steps per iteration
                        index = get_BCODE_token(index, &param6);    // delay between steps, in milliseconds
                        for (j = 0; j < param4; j++) {
                            move_steps(param5, param1);
                            delay(param6);
                        }
                }
                delay(param2);   // gather beads
                break;
        case 19: // no action
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

        index = get_BCODE_token(start_index, &cmd);
        if ((start_index == 0) && (cmd != 0)) { // first command
                cancelling_test = true;
                Serial.println("First command not found");
                return -1;
        }
        else {
                index = process_one_BCODE_command(cmd, index);
        }

        while ((cmd != 99) && (index > 0) && !cancelling_test) {
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

void set_update_battery_life_flag() {
    update_battery_life = true;
}
void calculate_power_status() {
    int battery_level = analogRead(pinBatteryAin);
    int new_power_status = (battery_level > BATTERY_CONVERSION_FACTOR * 100 ? 100 : battery_level / BATTERY_CONVERSION_FACTOR) * (digitalRead(pinDCinDetect) ? -1 : 1);
    if (abs(new_power_status - power_status) > 1) {
        power_status = new_power_status;
    }
}

void watchdog() {
    Serial.println("Watchdog!");
}

int particle_command(String arg) {
    int cmd, indx1, indx2, indx3, param1 = 0, param2 = 0, param3 = 0, steps_to_alignment;

    indx1 = arg.indexOf(COMMA_DELIM);
    if (indx1 == -1) {
        cmd = arg.toInt();
    }
    else {
        cmd = arg.substring(0, indx1).toInt();
        indx1++;
        indx2 = arg.indexOf(COMMA_DELIM, indx1);
        if (indx2 == -1) {
            param1 = arg.substring(indx1).toInt();
        }
        else {
            param1 = arg.substring(indx1, indx2).toInt();
            indx2++;
            indx3 = arg.indexOf(COMMA_DELIM, indx2);
            if (indx3 == -1) {
                param2 = arg.substring(indx2).toInt();
            }
            else {
                param2 = arg.substring(indx2, indx3).toInt();
                indx3++;
                param3 = arg.substring(indx3).toInt();
            }
        }
    }

    Serial.printlnf("Command: %d, param1: %d, param2: %d, param3: %d", cmd, param1, param2, param3);

    switch (cmd) {
        case 1: // set and move to calibration point
            eeprom.param.steps_to_calibration_point = param1;
            store_eeprom();
            reset_stage();
            return param1;
        case 2: // reset stage
            reset_stage();
            return cumulative_steps;
        case 3: // move steps
            wake_move_sleep_stepper(param1, param2);
            return cumulative_steps;
        case 4: // read assay sensor (ledPower, integration_time, gain)
            steps_to_alignment = STEPS_TO_MICROBEAD_WELL + eeprom.param.steps_to_calibration_point;
            if (cumulative_steps != steps_to_alignment) {
                wake_move_sleep_stepper(steps_to_alignment - cumulative_steps, eeprom.param.step_delay_us);
            }
            read_sensors_command_integration_time = param1;
            read_sensors_command_gain = param2;
            read_sensors_samples = param3;
            read_sensors_command_flag = true;
            return 1;
        case 5: // change threshold
            eeprom.param.start_test_heat_red_threshold = param1;
            store_eeprom();
            return param1;
        case 6: // not used
            return 1;
        case 7: // not used
            return 1;
        case 8: // reset params
            reset_eeprom();
            return (int) eeprom.data_format_version;
        case 9: // set solenoid sustain power
            eeprom.param.solenoid_power = 0xFF00 + (uint8_t) param1;
            move_solenoid(2000);
            return eeprom.param.solenoid_power;
        case 10: // turn on assay laser for param1 milliseconds
            if (param1 > 10000 || param1 < 0) {
                param1 = 2000;
            }
            turn_on_assay_laser_for_duration(param1);
            return param1;
        case 11: // turn on control laser for param1 milliseconds
            if (param1 > 10000 || param1 < 0) {
                param1 = 2000;
            }
            turn_on_control_laser_for_duration(param1);
            return param1;
        case 12: // turn on both lasers for param1 milliseconds
            if (param1 > 10000 || param1 < 0) {
                param1 = 2000;
            }
            turn_on_both_lasers_for_duration(param1);
            return param1;
}

    return 0;

}

void setup() {
        Particle.variable("register", particle_register, STRING);
        Particle.variable("status", particle_status, STRING);
        Particle.variable("powerstatus", &power_status, INT);
        Particle.function("command", particle_command);
        device_id_string = System.deviceID();
        Particle.subscribe(String(device_id_string + "/hook-response/brevitest"), brevitest_callback, MY_DEVICES);
        Particle.subscribe(String(device_id_string + "/hook-error/brevitest"), brevitest_error, MY_DEVICES);
        device_id_string.toCharArray(device_id, DEVICE_ID_LENGTH + 1);
        device_id[DEVICE_ID_LENGTH] = '\0';

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
        pinMode(pinCartridgeHeater, OUTPUT);
        pinMode(pinCartridgeHeaterLED, OUTPUT);
        pinMode(pinAssayLaser, OUTPUT);
        pinMode(pinControlLaser, OUTPUT);

        digitalWrite(pinSensorLED, LOW);
        analogWrite(pinSolenoid, 0);
        digitalWrite(pinStepperStep, LOW);
        digitalWrite(pinStepperDir, LOW);
        digitalWrite(pinStepperSleep, LOW);
        digitalWrite(pinQRTrigger, LOW);
        digitalWrite(pinCartridgeHeater, LOW);
        digitalWrite(pinCartridgeHeaterLED, LOW);
        digitalWrite(pinAssayLaser, LOW);
        digitalWrite(pinControlLaser, LOW);

        _pmic.disableBATFET();
        _pmic.disableCharging(); // if you comment this out, the red LED will be steady ON

        turn_off_device_LED();
        set_device_LED_color(255, 255, 0);
        turn_on_device_LED();

        Serial.begin(115200); // standard serial port

        load_eeprom();
        if (eeprom.firmware_version != FIRMWARE_VERSION || eeprom.data_format_version != DATA_FORMAT_VERSION) {
                reset_eeprom();
        }

        reset_stage();
        move_solenoid(2000);

        reset_globals();

        init_sensor(&tcsAssay, SENSOR_NUMBER_ASSAY);
        init_sensor(&tcsControl, SENSOR_NUMBER_CONTROL);

        initialize_test_cache();
        calculate_power_status();

        initialize_device_state();
        battery_check_timer.reset();

        Serial.printlnf("device id: %s", device_id);
        Serial.printlnf("eeprom.firmware_version: %d, eeprom.data_format_version: %d, eeprom.most_recent_test: %d", eeprom.firmware_version, eeprom.data_format_version, eeprom.most_recent_test);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                    DEVICE STATE                         //
//                                                         //
/////////////////////////////////////////////////////////////

void initialize_device_state() {
    check_assay_sensor_state(false);

    device_open = !(sensor_state.clear > STATE_DEVICE_OPEN_THRESHOLD);
    if (device_open) {
        check_assay_sensor_state(true);
        cartridge_loaded = !(sensor_state.clear > STATE_DEVICE_CARTRIDGE_CLEAR_THRESHOLD);
    }

    /*tcsAssay.disable();*/
}

void check_assay_sensor_state(bool ledOn) {
    int tries = 0;
    uint16_t old_clear;

    if (ledOn) {
        analogWrite(pinSensorLED, STATE_SENSOR_LED_POWER);
        delay(STATE_SENSOR_LED_DELAY);
    }

    tcsAssay.begin(SENSOR_DEFAULT_INTEGRATION_TIME, SENSOR_DEFAULT_GAIN);

    sensor_state.clear = 0xFFFF;
    do {
        old_clear = sensor_state.clear;
        tcsAssay.takeReading(&sensor_state, 1, SENSOR_DEFAULT_IT_DELAY, false);
    } while (abs(old_clear - sensor_state.clear) > 5 && tries++ < 20);

    tcsAssay.end();
    /*Serial.printlnf("LED %c, R: %d, G: %d, B: %d, C: %d, OC: %d, tries: %d", ledOn ? 'Y' : 'N', sensor_state.red, sensor_state.green, sensor_state.blue, sensor_state.clear, old_clear, tries);*/

    if (ledOn) {
        analogWrite(pinSensorLED, 0);
        cartridge_is_heated = (sensor_state.red > eeprom.param.start_test_heat_red_threshold);   // reading below threshold means reagent still below 33 deg C, turn on heat
        /*Serial.printlnf("Checking cartridge pigment state - heated ? %c, R: %d, G: %d, B: %d, C: %d", cartridge_is_heated ? 'Y' : 'N', sensor_state.red, sensor_state.green, sensor_state.blue, sensor_state.clear);*/
        next_sensor_reading_time = millis() + CARTRIDGE_HEATER_TEST_START_CHECK_PERIOD;
    }
}

void check_device_state() {
    bool device_open_now;

    if (qr_code_being_scanned || waiting_for_validation || waiting_for_start_confirmation || test_in_progress || cancelling_test || finishing_test || reading_sensors || waiting_for_upload_confirmation) {
        return;
    }

    check_assay_sensor_state(false);
    /*Serial.printlnf("Device state (no LED) - R: %d, G: %d, B: %d, C: %d", sensor_state.red, sensor_state.green, sensor_state.blue, sensor_state.clear);*/

    device_open_now = (sensor_state.clear > STATE_DEVICE_OPEN_THRESHOLD);
    if (device_open ^ device_open_now) {    // device open state changed
        if (device_open_now) {
            Serial.println("Device just opened");
            memcpy(qr_uuid, DEVICE_OPEN_UUID, CARTRIDGE_UUID_LENGTH);
            cartridge_loaded = false;
            ready_to_scan_qr_code = false;
            cartridge_validated = false;
            stop_blinking_device_LED();
            set_device_LED_color(0, 255, 255);
            turn_on_device_LED();
        }
        else {
            Serial.printlnf("Device just closed");

            check_assay_sensor_state(true);

            Serial.printlnf("Device state (LED on) - R: %d, G: %d, B: %d, C: %d", sensor_state.red, sensor_state.green, sensor_state.blue, sensor_state.clear);
            cartridge_loaded = (sensor_state.clear > STATE_DEVICE_CARTRIDGE_CLEAR_THRESHOLD);
            cartridge_validated = false;
            if (cartridge_loaded) {
                Serial.println("Cartridge in device; ready to scan qr code");
                start_blinking_device_LED(0, 100, 0, 255, 255);
                ready_to_scan_qr_code = true;
            }
            else {
                Serial.println("No cartridge in device");
                strncpy(qr_uuid, NO_CARTRIDGE_UUID, CARTRIDGE_UUID_LENGTH);
                stop_blinking_device_LED();
                set_device_LED_color(128, 128, 128);
                turn_on_device_LED();
            }
      }
        device_open = device_open_now;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           TESTS                         //
//                                                         //
/////////////////////////////////////////////////////////////

void reset_globals() {
        test_in_progress = false;
        test_startup_successful = false;
        cartridge_validated = false;
        callback_complete = false;
        reading_sensors = false;

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

        next_upload = millis() + UPLOAD_INTERVAL;
}

void start_test() {
    Serial.println("Test starting");

    waiting_for_start_confirmation = true;
    start_timeout = millis() + TIMEOUT_START;
    wake_stepper();

    start_blinking_device_LED(0, 500, 0, 255, 0);
    brevitest_publish("test-start", test_record.test_uuid, false);
}

void cancel_test() {
    Serial.println("Test cancelled");

    waiting_for_cancel_confirmation = true;
    cancel_timeout = millis() + TIMEOUT_CANCEL;

    update_progress("Test cancelled", -1);
    brevitest_publish("test-cancel", test_record.test_uuid, false);

    sleep_stepper();
    stop_blinking_device_LED();
    set_device_LED_color(255, 0, 0);
    turn_on_device_LED();
}

void finish_test() {
    Serial.println("Test completed");

    waiting_for_finish_confirmation = true;
    finish_timeout = millis() + TIMEOUT_FINISH;

    update_progress("Test complete", -1);
    brevitest_publish("test-finish", test_record.test_uuid, false);

    sleep_stepper();
    stop_blinking_device_LED();
    set_device_LED_color(0, 255, 0);
    turn_on_device_LED();
}

void run_test() {
    test_in_progress = true;
    start_blinking_device_LED(0, 500, 0, 255, 0);
    Serial.println("Running test");

    test_last_progress_update = 0;
    update_progress("Running test", 0);

    pinMode(pinSolenoid, OUTPUT);
    analogWrite(pinSolenoid, 0);
    analogWrite(pinSensorLED, 0);

    Particle.disconnect();
    delay(PARTICLE_CLOUD_DELAY);
    while(!Particle.disconnected()) {
        Serial.println("-");
        Particle.process();
        delay(PARTICLE_CLOUD_DELAY);
    }

    SINGLE_THREADED_BLOCK() {
        process_BCODE(0);
    }

    Particle.connect();
    delay(PARTICLE_CLOUD_DELAY);
    while (!Particle.connected()) {
        Serial.println("+");
        Particle.process();
        delay(PARTICLE_CLOUD_DELAY);
    }

    finishing_test = !cancelling_test;
}

void upload_one_test(int test_number, char *test_id) {
    waiting_for_upload_confirmation = true;
    upload_timeout = millis() + TIMEOUT_UPLOAD;

    Serial.printlnf("Processing test %s for upload", test_id);
    process_test_record(test_number);

    Serial.println(particle_register);
    brevitest_publish("test-upload", test_id, false);
}

void upload_tests() {
        int i;

        Serial.println("Uploading tests");
        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
            if (eeprom.test_cache[i].test_uuid[0] != '\0') {
                upload_one_test(i, eeprom.test_cache[i].test_uuid);
                return;
            }
        }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void loop() {
        if (callback_complete) {
            Serial.println("Processing callback");
            process_callback_buffer();
            return;
        }

        if (test_in_progress) {
            if (cancelling_test) {
                cancelling_test = false;
                cancel_test();
                return;
            }

            if (finishing_test) {
                finishing_test = false;
                finish_test();
                return;
            }

            if (waiting_for_cancel_confirmation && millis() > cancel_timeout) {
                cancel_test();
                return;
            }

            if (waiting_for_finish_confirmation && millis() > finish_timeout) {
                finish_test();
                return;
            }
        }
        else {
            check_device_state();

            if (waiting_for_start_confirmation && millis() > start_timeout) {
                start_test();
                return;
            }

            if (test_startup_successful) {
                if (millis() > next_sensor_reading_time) {
                    check_assay_sensor_state(true);
                    Serial.printlnf("Waiting for cartridge to heat - target: %d, reading: %d", eeprom.param.start_test_heat_red_threshold, sensor_state.red);
                    tcsAssay.end();
                }
                if (cartridge_is_heated) {
                    test_startup_successful = false;
                    if (device_open) {
                        cancelling_test = true;
                    }
                    else {
                        run_test();
                    }
                    return;
                }
            }

            if (waiting_for_validation) {
                if (millis() > validation_timeout) {
                    validate_cartridge();
                }
                return;
            }

            if (ready_to_scan_qr_code) {
                ready_to_scan_qr_code = false;
                if (!device_open) {
                    if (scan_qr_code() == CARTRIDGE_UUID_LENGTH) {
                        validate_cartridge();
                    }
                    else {
                        stop_blinking_device_LED();
                        set_device_LED_color(255, 0, 0);    // bad cartridge uuid
                        turn_on_device_LED();
                        cartridge_validated = false;
                    }
                }
                return;
            }

            if (starting_test) {
                starting_test = false;
                start_test();
                return;
            }

            if (waiting_for_upload_confirmation) {
                if (millis() > upload_timeout) {
                    upload_tests();
                }
                return;
            }

            if (tests_to_upload()) {
                upload_tests();
                return;
            }

        }

        if (read_sensors_command_flag) {
            read_sensors_command_flag = false;
            /*SINGLE_THREADED_BLOCK() {*/
            read_sensors_with_parameters(read_sensors_command_integration_time, read_sensors_command_gain, read_sensors_samples);
            /*}*/
        }

        if (update_battery_life) {
            update_battery_life = false;
            calculate_power_status();
        }
}
