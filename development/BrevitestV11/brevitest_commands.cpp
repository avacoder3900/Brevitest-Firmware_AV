#include "Particle.h"
#include "brevitest_commands.h"
#include "main.h"

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

/////////////////////////////////////////////////////////////
//                                                         //
//                      SENSOR TESTS                       //
//                                                         //
/////////////////////////////////////////////////////////////

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
