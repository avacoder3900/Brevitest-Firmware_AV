#include "TCS34725.h"
#include "SoftI2CMaster.h"

#include "main.h"

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

/////////////////////////////////////////////////////////////
//                                                         //
//                         EEPROM                          //
//                                                         //
/////////////////////////////////////////////////////////////

void load_eeprom() {
  uint8_t *e = (uint8_t *) &eeprom;

  for (int addr = 0; addr < CACHE_SIZE_BYTES; addr++, e++) {
    *e = EEPROM.read(addr);
  }
}

void store_eeprom() {
  uint8_t *e = (uint8_t *) &eeprom;

  for (int addr = 0; addr < CACHE_SIZE_BYTES; addr++, e++) {
    EEPROM.write(addr, *e);
  }
}

void erase_eeprom() {
  for (int addr = 0; addr < CACHE_SIZE_BYTES; addr++) {
    EEPROM.write(addr, 0);
  }
}

void dump_eeprom() {
  uint8_t *e = (uint8_t *) &eeprom;
  uint8_t buf;

  Serial.println("EEPROM contents: ");
  for (int addr = 0; addr < CACHE_SIZE_BYTES; addr++) {
    buf = EEPROM.read(addr);
    Serial.write(buf);
  }
  Serial.println();

  Serial.println("eeprom contents: ");
  for (int addr = 0; addr < CACHE_SIZE_BYTES; addr++) {
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
    Particle.process();

    if (cancel_process) {
        return;
    }

    int sustain_time = abs(duration) - eeprom.param.solenoid_surge_period_ms;
    sustain_time = sustain_time < 0 ? 0 : sustain_time;

    pinMode(pinSolenoid, OUTPUT);
    analogWrite(pinSolenoid, eeprom.param.solenoid_surge_power);
    delay(eeprom.param.solenoid_surge_period_ms);

    if (sustain_time) {
      pinMode(pinSolenoid, OUTPUT);
      analogWrite(pinSolenoid, eeprom.param.solenoid_sustain_power);
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
        if (cancel_process) {
            break;
        }

        if (i % eeprom.param.publish_interval_during_move == 0) {
            Particle.process();
        }

        if ((dir == LOW) && (digitalRead(pinLimitSwitch) == LOW)) {
            cumulative_steps = 0;
            break;
        }

        if ((dir == HIGH) && (cumulative_steps > CUMULATIVE_STEP_LIMIT)) {
            break;
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
    move_steps(1000, eeprom.param.step_delay_us);
    move_steps(-eeprom.param.reset_steps, eeprom.param.step_delay_us);
}

void save_calibration_point() {
  uint8_t lsb, msb;

  msb = (uint8_t) (eeprom.param.calibration_steps >> 8);
  lsb = (uint8_t) (eeprom.param.calibration_steps & 0x0F);
  EEPROM.write(EEPROM_ADDR_PARAM + EEPROM_OFFSET_PARAM_CALIBRATION_STEPS, msb);
  EEPROM.write(EEPROM_ADDR_PARAM + EEPROM_OFFSET_PARAM_CALIBRATION_STEPS + 1, lsb);
}

void move_to_calibration_point() {
    CANCELLABLE(reset_stage();)
    delay(500);
    CANCELLABLE(move_steps(eeprom.param.calibration_steps, eeprom.param.step_delay_us);)
}

/////////////////////////////////////////////////////////////
//                                                         //
//                       QR SCANNER                        //
//                                                         //
/////////////////////////////////////////////////////////////

int scan_QR_code() {

    // test code

    // use following #defines for testing purposes
    //
    // CARTRIDGE_TEST_ID_LAPTOP
    // CARTRIDGE_TEST_ID_DESKTOP
    // CARTRIDGE_TEST_ID_CLOUD
    //

    /*char temp[] = CARTRIDGE_TEST_ID_CLOUD;   // cloud
    strncpy(qr_uuid, temp, UUID_LENGTH);
    return 0;*/

    // actual code

    unsigned long timeout;
    int buf, i = 0;

    Serial1.begin(115200);  // QR scanner interface through RX/TX pins

    digitalWrite(pinQRTrigger, HIGH);

    timeout = millis() + QR_READ_TIMEOUT;

    while (!Serial1.available() && millis() < timeout) {
      Particle.process();
    }

    digitalWrite(pinQRTrigger, LOW);

    delay(100); // allow QR code buffer to fill before reading

    do {
        buf = Serial1.read();
        if (buf != -1) {
          qr_uuid[i++] = (char) buf;
        }
    } while (Serial1.available() && i < UUID_LENGTH);

    qr_uuid[UUID_LENGTH] = '\0';

    Serial1.end();

    return i;
}

int validate_QR_code(char *uuid_to_validate) {
    int scan_result = scan_QR_code();

    if (scan_result == UUID_LENGTH) {
        return memcmp(qr_uuid, uuid_to_validate, UUID_LENGTH);
    }
    else {
        return scan_result;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                       DEVICE LED                        //
//                                                         //
/////////////////////////////////////////////////////////////

int turn_on_device_LED() {
  digitalWrite(pinDeviceLED, HIGH);
  device_LED.currently_on = true;
  return 1;
}

int turn_off_device_LED() {
  digitalWrite(pinDeviceLED, LOW);
  device_LED.currently_on = false;
  return 1;
}

void start_blinking_device_LED(int total_duration, int rate) {
  device_LED.blink_rate = rate;
  device_LED.blink_timeout = total_duration ? millis() + (unsigned long) total_duration : 0;
  device_LED.blink_change_time = 0;
  device_LED.blinking = true;
}

void stop_blinking_device_LED() {
  turn_off_device_LED();
  device_LED.blinking = false;
}

void update_blinking_device_LED() {
  unsigned long now = millis();

  if (now > device_LED.blink_change_time) { // change LED state and reset blink timer
    if (device_LED.currently_on) {
      turn_off_device_LED();
    }
    else {
      turn_on_device_LED();
    }
    device_LED.blink_change_time = now + device_LED.blink_rate;
  }
  if (device_LED.blink_timeout && now > device_LED.blink_timeout) {
    device_LED.blinking = false;
    turn_off_device_LED();
  }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        SENSORS                          //
//                                                         //
/////////////////////////////////////////////////////////////

void init_sensor(TCS34725 *sensor, int sdaPin, int sclPin) {
    uint8_t it, gain;

    it = ((int) eeprom.param.sensor_params) & 0x00FF;
    gain = ((int) eeprom.param.sensor_params) >> 8;
    *sensor = TCS34725((tcs34725IntegrationTime_t) it, (tcs34725Gain_t) gain, sdaPin, sclPin);

    if (sensor->begin()) {
        sensor->enable();
    }
    else {
        ERROR_MESSAGE("Sensor not found");
    }
}

void set_sensor_params(uint8_t it, uint8_t gain) {
    tcsAssay.setIntegrationTime((tcs34725IntegrationTime_t) it);
    tcsAssay.setGain((tcs34725Gain_t) gain);
    tcsControl.setIntegrationTime((tcs34725IntegrationTime_t) it);
    tcsControl.setGain((tcs34725Gain_t) gain);
}

void read_one_sensor(TCS34725 *sensor, char sensor_code, int sample_number) {
    BrevitestSensorSampleRecord *sample;

    if (sensor_code == 'A') {
        sample = &assay_buffer[sample_number];
    }
    else {
        sample = &control_buffer[sample_number];
    }
    sample->sample_time = Time.now();

    Particle.process();

    sensor->getRawData(&sample->red, &sample->green, &sample->blue, &sample->clear);
}

void convert_samples_to_reading(int reading_code, char sensor_code) {
    int i, j, k;
    int red, green, blue, clear;
    int index[SENSOR_NUMBER_OF_SAMPLES];
    BrevitestSensorSampleRecord *buffer;
    BrevitestSensorRecord *reading;

    if (sensor_code == 'A') {
        buffer = assay_buffer;
        if (reading_code == 0) {
            reading = &(test_record->sensor_reading_initial_assay);
        }
        else if (reading_code == 1) {
            reading = &(test_record->sensor_reading_final_assay);
        }
        else {
            reading = &sensor_reading;
        }
    }
    else {
        buffer = control_buffer;
        if (reading_code == 0) {
            reading = &(test_record->sensor_reading_initial_control);
        }
        else if (reading_code == 1) {
            reading = &(test_record->sensor_reading_final_control);
        }
        else {
            reading = &sensor_reading;
        }
    }

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
    for (j = 1; j < SENSOR_NUMBER_OF_SAMPLES - 1; j += 1) {
        i = index[j];
        Serial.print("i: ");
        Serial.print(i);
        Serial.print(" C: ");
        Serial.print(buffer[i].clear);
        Serial.print(" R: ");
        Serial.print(buffer[i].red);
        Serial.print(" G: ");
        Serial.print(buffer[i].green);
        Serial.print(" B: ");
        Serial.println(buffer[i].blue);
        clear = buffer[i].clear;
        if (clear) {
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

void read_sensors(int reading_code) { // 0 -> initial baseline, 1 -> assay, -1 -> testing and validation
    for (int i = 0; i < SENSOR_NUMBER_OF_SAMPLES; i += 1) {
        if (cancel_process) {
            return;
        }
        read_one_sensor(&tcsAssay, 'A', i);
        read_one_sensor(&tcsControl, 'C', i);
        delay(eeprom.param.delay_between_sensor_readings_ms);
        if (reading_code == 0) {
            update_progress("Taking baseline readings", eeprom.param.delay_between_sensor_readings_ms);
        }
        else if (reading_code == 1) {
            update_progress("Reading test results", eeprom.param.delay_between_sensor_readings_ms);
        }
    }

    convert_samples_to_reading(reading_code, 'A');
    convert_samples_to_reading(reading_code, 'C');
}

/////////////////////////////////////////////////////////////
//                                                         //
//                  EEPROM ASSAY RECORDS                   //
//                                                         //
/////////////////////////////////////////////////////////////

int find_assay_index_by_uuid(char *uuid) {
    if (uuid[0] == '\0') {
      return -1;
    }

    for (int i = 0; i < ASSAY_CACHE_SIZE; i += 1) {
        if (memcmp(uuid, eeprom.assay_cache[i].uuid, UUID_LENGTH) == 0) {
            return i;
        }
    }

    return -1;
}

void write_assay_record_to_eeprom() {
    store_assay(assay_index);
}

void store_assay(int index) {
    uint8_t *e = (uint8_t *) &eeprom.assay_cache[index];
    uint8_t assay_count;
    int start_addr = ASSAY_CACHE_INDEX + index * ASSAY_RECORD_LENGTH;

    for (int addr = start_addr; addr < (start_addr + ASSAY_RECORD_LENGTH); addr++, e++) {
        EEPROM.write(addr, *e);
    }
    assay_count = (index + 1) % ASSAY_CACHE_SIZE;
    eeprom.cache_count &= 0x0F;
    eeprom.cache_count += (assay_count << 4);
    EEPROM.write(CACHE_COUNT_INDEX, eeprom.cache_count);
}

int process_assay_record(int index) {
    BrevitestAssayRecord *assay;
    int len;

    assay = &eeprom.assay_cache[index];

    len = snprintf(particle_register, PARTICLE_REGISTER_SIZE, "%.24s\t%2u\t%s\t%5d\t%3u\t%3u\n%s\n", \
        assay->uuid, assay->name_length, assay->name, assay->duration, assay->BCODE_length, assay->BCODE_version, assay->BCODE);

    particle_register[len] = '\0';
    return 0;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                  EEPROM TEST RECORDS                    //
//                                                         //
/////////////////////////////////////////////////////////////

int find_test_index_by_uuid(char *uuid) {
    if (uuid[0] == '\0') {
      return -1;
    }

    for (int i = 0; i < TEST_CACHE_SIZE; i += 1) {
        if (memcmp(uuid, eeprom.test_cache[i].test_uuid, UUID_LENGTH) == 0) {
            return i;
        }
    }

    return -1;
}

void write_test_record_to_eeprom() {
    process_test_record(test_index);
    store_test(test_index);
}

void store_test(int index) {
  uint8_t *e = (uint8_t *) &eeprom.test_cache[index];
  int start_addr = TEST_CACHE_INDEX + index * TEST_RECORD_LENGTH;

  for (int addr = start_addr; addr < (start_addr + TEST_RECORD_LENGTH); addr++, e++) {
      EEPROM.write(addr, *e);
  }
  eeprom.cache_count &= 0xF0;
  eeprom.cache_count += (index + 1) % TEST_CACHE_SIZE;
  EEPROM.write(CACHE_COUNT_INDEX, eeprom.cache_count);
}

int process_test_record(int index) {
    BrevitestTestRecord *test;
    int len;

    test = &eeprom.test_cache[index];

    len = snprintf(particle_register, PARTICLE_REGISTER_SIZE, \
        "%11d\t%11d\t%.24s\t%.24s\t%.24s\n%5d\t%5d\t%5d\t%5d\t%3d\t%3d\t%5d\t%5d\t%3d\t%3d\t%5d\n%11d\t%5d\t%5d\t%5d\n%11d\t%5d\t%5d\t%5d\n%11d\t%5d\t%5d\t%5d\n%11d\t%5d\t%5d\t%5d\n", \
        test->start_time, test->finish_time, test->test_uuid, test->cartridge_uuid, test->assay_uuid, \
//
        test->param.reset_steps, test->param.step_delay_us, test->param.publish_interval_during_move, test->param.stepper_wake_delay_ms, \
        test->param.solenoid_surge_power, test->param.solenoid_sustain_power, test->param.solenoid_surge_period_ms, \
        test->param.delay_between_sensor_readings_ms, test->param.sensor_params & 0xFF, test->param.sensor_params >> 8, test->param.calibration_steps, \
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
    return 0;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                         PARAMS                          //
//                                                         //
/////////////////////////////////////////////////////////////

int change_byte_param(int index, int value) {
    uint8_t *addr = (uint8_t *) &eeprom.param.reset_steps;

    *(addr + index) = value;
    store_params();
    return value;
}

int change_word_param(int index, int value) {
    uint16_t *addr = &eeprom.param.reset_steps;

    if (index % 2 == 0) {
        *(addr + index) = value;
    }
    else {
        return -117;
    }

    store_params();

    return value;
}

int reset_params() {
    Param reset;

    memcpy(&eeprom.param, &reset, EEPROM_PARAM_LENGTH);
    store_params();

    return 1;
}

void load_params() {
  uint8_t *e = (uint8_t *) &eeprom.param;

  for (int addr = EEPROM_ADDR_PARAM; addr < (EEPROM_ADDR_PARAM + EEPROM_PARAM_LENGTH); addr++, e++) {
    *e = EEPROM.read(addr);
  }
}

void store_params() {
  uint8_t *e = (uint8_t *) &eeprom.param;

  for (int addr = EEPROM_ADDR_PARAM; addr < (EEPROM_ADDR_PARAM + EEPROM_PARAM_LENGTH); addr++, e++) {
    EEPROM.write(addr, *e);
  }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        REQUESTS                         //
//                                                         //
/////////////////////////////////////////////////////////////

int get_serial_number() {
    memcpy(particle_register, eeprom.serial_number, EEPROM_SERIAL_NUMBER_LENGTH + 1); // includes trailing \0
    return 0;
}

int get_test_record_by_uuid() {
    int index = find_test_index_by_uuid(particle_request.param);
    if (index == -1) {
      return -3;
    }

    return process_test_record(index);
}

int get_assay_record_by_uuid() {
    int index = find_assay_index_by_uuid(particle_request.param);
    if (index == -1) {
      return -2;
    }

    return process_assay_record(index);
}

int get_param_value() {
    int index, len;
    uint16_t *value16 = &eeprom.param.reset_steps;
    uint8_t *value8 = (uint8_t *) &eeprom.param.reset_steps;

    index = extract_int_from_string(particle_command.param, 0, PARAM_CHANGE_INDEX);
    if (index < 0) {
      return -150;
    }
    if (index >= EEPROM_PARAM_LENGTH) {
      return -151;
    }

    len = extract_int_from_string(particle_request.param, PARAM_CHANGE_INDEX, PARAM_CHANGE_LENGTH);
    if (len == 1) {
        value8 += index;
        sprintf(particle_register, "%d", *value8);
    }
    else if (len == 2) {
        if (index % 2 == 0) {
            value16 += index;
            sprintf(particle_register, "%d", *value16);
        }
        else {
            return -147;
        }
    }
    else {
        return -148;
    }

    return 0;
}

int get_all_param_values() {
    uint16_t *value = &eeprom.param.reset_steps;
    int i, len, index = 0;

    for (i = 0; i < EEPROM_PARAM_LENGTH; i += 2) {
        index += sprintf(&particle_register[index], "%d,", *value);
    }
    particle_register[--index] = '\0';

    return 0;
}

int get_QR_code_value() {
    particle_register[0] = '\0';
    qr_uuid[0] = '\0';

    int scan_result = scan_QR_code();

    if (scan_result == UUID_LENGTH) {
        memcpy(particle_register, qr_uuid, UUID_LENGTH);
        particle_register[UUID_LENGTH] = '\0';
        return 0;
    }
    else {
        return scan_result;
    }
}

int get_assay_cache_uuids() {
  int index;
  for (int i = 0; i < ASSAY_CACHE_SIZE; i++) {
    index = i * (UUID_LENGTH + 1);
    memcpy(&particle_register[index], eeprom.assay_cache[i].uuid, UUID_LENGTH);
    if (i == ASSAY_CACHE_SIZE - 1) {
      particle_register[index + UUID_LENGTH] = '\0';
    }
    else {
      particle_register[index + UUID_LENGTH] = '\n';
    }
  }
  return 0;
}

int get_test_cache_uuids() {
  int index;
  for (int i = 0; i < TEST_CACHE_SIZE; i++) {
    index = i * (UUID_LENGTH + 1);
    memcpy(&particle_register[index], eeprom.test_cache[i].test_uuid, UUID_LENGTH);
    if (i == TEST_CACHE_SIZE - 1) {
      particle_register[index + UUID_LENGTH] = '\0';
    }
    else {
      particle_register[index + UUID_LENGTH] = '\n';
    }
  }
  return 0;
}

int parse_particle_request(String msg) {
    int len = msg.length();
    int param_length = len - PARTICLE_REQUEST_CODE_LENGTH;
    param_length = param_length < 0 ? 0 : param_length;
    msg.toCharArray(particle_request.arg, len + 1);

    particle_request.code = extract_int_from_string(particle_request.arg, PARTICLE_REQUEST_CODE_INDEX, PARTICLE_REQUEST_CODE_LENGTH);
    strncpy(particle_request.param, &particle_request.arg[PARTICLE_REQUEST_PARAM_INDEX], param_length);
    particle_request.param[param_length] = '\0';

    return 1;
}

int request_data(String msg) {
    if (parse_particle_request(msg) > 0) {
        switch (particle_request.code) {
            case 1: // serial_number
                return get_serial_number();
            case 2: // get test record
                return get_test_record_by_uuid();
            case 3: // get assay record
                return get_assay_record_by_uuid();
            case 4: // get params
                return get_all_param_values();
            case 5: // get one parameter
                return get_param_value();
            case 6: // get QR code
                return get_QR_code_value();
            case 7: // get assay cache uuids
                return get_assay_cache_uuids();
            case 8: // get test cache uuids
                return get_test_cache_uuids();
        }
    }

    return -1;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        COMMANDS                         //
//                                                         //
/////////////////////////////////////////////////////////////

int command_run_brevitest() {
    if (test_index == -1) {
        return -72;
    }

    if (memcmp(test_record->test_uuid, particle_command.param, UUID_LENGTH) != 0) {
      return -71;
    }

    assay_index = find_assay_index_by_uuid(test_record->assay_uuid);
    if (assay_index < 0) {
      return -73;
    }

    start_test = true;
    test_last_progress_update = 0;
    update_progress("Resetting device and starting test", 0);

    return 1;
}

int command_cancel_brevitest() {
    cancel_process = true;
    return 1;
}

int command_claim_device() {
    // param: user_uuid
    int result;

    if(claimant_uuid[0] == '\0') {
      memcpy(claimant_uuid, particle_command.param, UUID_LENGTH);
      claimant_uuid[UUID_LENGTH] = '\0';
    }
    else if (memcmp(claimant_uuid, particle_command.param, UUID_LENGTH)) {
      Serial.println("Device already claimed by other user");
      return -200;
    }

    result = get_QR_code_value();
    if (result != 0) {
      Serial.println("Failed to read cartridge, device not claimed");
      return result;
    }
    assay_index = 9999;

    Serial.println("Device claimed");

    start_blinking_device_LED(DEVICE_LED_BLINK_NO_TIMEOUT, DEVICE_LED_BLINK_DELAY_CLAIMED);

    return assay_index;
}

int command_release_device() {
  if (test_in_progress) {
    Serial.println("Device busy, not released");
    return 0;
  }
  else if (claimant_uuid[0] != '\0' && memcmp(claimant_uuid, particle_command.param, UUID_LENGTH)) {
    Serial.println("User mismatch, device not released");
    Serial.println(claimant_uuid);
    Serial.println(particle_command.param);
    return 0;
  }
  else {
    claimant_uuid[0] = '\0';
    assay_index = -1;
    Serial.println("Device released");
    stop_blinking_device_LED();
    return 1;
  }
}

int command_check_assay_cache() {
    int result = find_assay_index_by_uuid(particle_command.param);
    return result == -1 ? 999 : result;
}

int command_check_test_cache() {
    int result = find_test_index_by_uuid(particle_command.param);
    return result == -1 ? 888 : result;
}

int command_start_transfer() {
    // first packet, payload is number of packets (not including this packet) and total payload length
    data_transfer.index = 0;
    data_transfer.packet_number = 0;

    memcpy(data_transfer.id, &particle_command.param[BUFFER_ID_INDEX], BUFFER_ID_LENGTH);
    data_transfer.message_type = extract_int_from_string(particle_command.param, BUFFER_MESSAGE_TYPE_INDEX, BUFFER_MESSAGE_TYPE_LENGTH);
    data_transfer.message_size = extract_int_from_string(particle_command.param, BUFFER_MESSAGE_SIZE_INDEX, BUFFER_MESSAGE_SIZE_LENGTH);
    data_transfer.number_of_packets = extract_int_from_string(particle_command.param, BUFFER_NUMBER_OF_PACKETS_INDEX, BUFFER_NUMBER_OF_PACKETS_LENGTH);

    return 0;
}

int command_receive_packet() {
    // payload packet, contains packet number, packet length, packet id, and payload

    Serial.println(particle_command.param);

    data_transfer.packet_number++;

    if (data_transfer.packet_number != extract_int_from_string(particle_command.param, BUFFER_PACKET_NUMBER_INDEX, BUFFER_PACKET_NUMBER_LENGTH)) {
        ERROR_MESSAGE("Data packet received out of order");
        ERROR_MESSAGE(particle_command.param);
        return -300;
    }

    if (data_transfer.packet_number > data_transfer.number_of_packets) { //
        ERROR_MESSAGE("Too many packets");
        ERROR_MESSAGE(particle_command.param);
        return -301;
    }

    if (memcmp(data_transfer.id, &particle_command.param[BUFFER_ID_INDEX], BUFFER_ID_LENGTH) != 0) {
        ERROR_MESSAGE("Data transfer buffer ID mismatch");
        ERROR_MESSAGE(particle_command.param);
        return -302;
    }

    data_transfer.payload_size = extract_int_from_string(particle_command.param, BUFFER_PAYLOAD_SIZE_INDEX, BUFFER_PAYLOAD_SIZE_LENGTH);
    memcpy(&data_transfer.buffer[data_transfer.index], &particle_command.param[BUFFER_PAYLOAD_INDEX], data_transfer.payload_size);

    if (data_transfer.index > data_transfer.message_size) {
        ERROR_MESSAGE("Buffer payload not transferred properly");
        ERROR_MESSAGE(data_transfer.index);
        ERROR_MESSAGE(data_transfer.message_size);
        return -303;
    }

    if (data_transfer.packet_number == data_transfer.number_of_packets) {   // last packet
        switch (data_transfer.message_type) {
            case 1: // assay
                load_assay_from_buffer();
                break;
            case 2: // test
                load_test_from_buffer();
                break;
            default:
                return -305;
        }
    }

    data_transfer.index += data_transfer.payload_size;

    return data_transfer.packet_number;
}

void load_assay_from_buffer() {
    BrevitestAssayRecord *assay;
    char *buf = data_transfer.buffer;
    int index = 0;

    assay_index = eeprom.cache_count >> 4;
    assay_index %= ASSAY_CACHE_SIZE;

    assay = &eeprom.assay_cache[assay_index];

    memcpy(assay->uuid, buf, UUID_LENGTH);
    index += UUID_LENGTH;

    assay->name_length = extract_int_from_string(buf, index, ASSAY_BUFFER_NAME_LENGTH_LENGTH);
    index += ASSAY_BUFFER_NAME_LENGTH_LENGTH;

    memcpy(assay->name, &buf[index], ASSAY_NAME_MAX_LENGTH);
    assay->name[assay->name_length] = '\0';
    index += ASSAY_NAME_MAX_LENGTH;

    assay->duration = extract_int_from_string(buf, index, ASSAY_BUFFER_DURATION_LENGTH);
    index += ASSAY_BUFFER_DURATION_LENGTH;

    assay->BCODE_length = extract_int_from_string(buf, index, ASSAY_BUFFER_BCODE_SIZE_LENGTH);
    index += ASSAY_BUFFER_BCODE_SIZE_LENGTH;

    assay->BCODE_version = extract_int_from_string(buf, index, ASSAY_BUFFER_BCODE_VERSION_LENGTH);
    index += ASSAY_BUFFER_BCODE_VERSION_LENGTH;

    buf[index + assay->BCODE_length + 1] = '\0';
    strncpy(assay->BCODE, &buf[index], ASSAY_BCODE_CAPACITY);

    store_assay(assay_index);
}

void load_test_from_buffer() {
    char *buf = data_transfer.buffer;

    test_index = eeprom.cache_count & 0x0F;
    test_record = &eeprom.test_cache[test_index];

    memcpy(user_uuid, buf, UUID_LENGTH);

    buf += UUID_LENGTH;
    memcpy(test_record->test_uuid, buf, UUID_LENGTH);

    buf += UUID_LENGTH;
    memcpy(test_record->assay_uuid, buf, UUID_LENGTH);

    buf += UUID_LENGTH;
    memcpy(test_record->cartridge_uuid, buf, UUID_LENGTH);

    store_test(test_index);
}

int command_write_serial_number() {
    for (int i = 0; i < EEPROM_SERIAL_NUMBER_LENGTH; i += 1) {
        eeprom.serial_number[i] = particle_command.param[i];
        EEPROM.write(EEPROM_ADDR_SERIAL_NUMBER + i, particle_command.param[i]);
    }
    eeprom.serial_number[EEPROM_SERIAL_NUMBER_LENGTH] = '\0';
    EEPROM.write(EEPROM_ADDR_SERIAL_NUMBER + EEPROM_SERIAL_NUMBER_LENGTH, 0);
    return 1;
}

int command_change_param() {
    int index, len, value;

    index = extract_int_from_string(particle_command.param, 0, PARAM_CHANGE_INDEX);
    if (index < 0) {
      return -110;
    }
    if (index >= EEPROM_PARAM_LENGTH) {
      return -111;
    }

    len = extract_int_from_string(particle_command.param, PARAM_CHANGE_INDEX, PARAM_CHANGE_LENGTH);
    value = extract_int_from_string(particle_command.param, PARAM_CHANGE_INDEX + PARAM_CHANGE_LENGTH, PARAM_CHANGE_VALUE);

    if (len == 1) {
        return change_byte_param(index, value);
    }
    if (len == 2) {
        return change_word_param(index, value);
    }
    return -112;
}

int command_reset_params() {
    reset_params();
}

int command_get_firmware_version() {
    int version = EEPROM.read(0);
    return version;
}

int command_set_and_move_to_calibration_point() {
    eeprom.param.calibration_steps = extract_int_from_string(particle_command.param, 0, PARTICLE_COMMAND_PARAM_LENGTH);
    calibrate = true;
    return 1;
}

int command_move_stage() {
  int steps = extract_int_from_string(particle_command.param, 0, PARTICLE_COMMAND_PARAM_LENGTH);
  move_steps(steps, eeprom.param.step_delay_us);
  return cumulative_steps;
}

int command_energize_solenoid() {
  int duration = extract_int_from_string(particle_command.param, 0, PARTICLE_COMMAND_PARAM_LENGTH);
  move_solenoid(duration);
  return 1;
}

int command_blink_device_LED() {
  int duration, rate, pos, len = strlen(particle_command.param);
  for (pos = 0; pos < len; pos++) {
    if (particle_command.param[pos] == ',') {
      break;
    }
  }

  if (pos == len) {
    duration = extract_int_from_string(particle_command.param, 0, len);
    rate = DEVICE_LED_BLINK_DELAY_DEFAULT;
  }
  else {
    duration = extract_int_from_string(particle_command.param, 0, pos);
    rate = extract_int_from_string(particle_command.param, pos + 1, len);
  }

  start_blinking_device_LED(duration, rate);
  return 1;
}

int command_verify_QR_code() {
    // param: cartridge_uuid

    int scan_result = scan_QR_code();

    if (scan_result == UUID_LENGTH) {
        return memcmp(qr_uuid, &particle_command.param, UUID_LENGTH);
    }
    else {
        return scan_result;
    }
}

int command_read_QR_code() {
  return get_QR_code_value();
}

int command_erase_assay_cache() {
    int i;

    for (i = 0; i < ASSAY_CACHE_SIZE; i++) {
        eeprom.assay_cache[i].uuid[0] = '\0';
    }

    store_eeprom();

    return 0;
}

int command_erase_test_cache() {
    int i;

    for (i = 0; i < TEST_CACHE_SIZE; i++) {
        eeprom.test_cache[i].test_uuid[0] = '\0';
    }

    store_eeprom();

    return 0;
}

int command_get_cache_count() {
    return eeprom.cache_count;
}

/*int command_dump_test_cache() {
    return 0;
}*/

int command_dump_eeprom() {
    dump_eeprom();
    snprintf(particle_register, PARTICLE_REGISTER_SIZE, \
        "%5d\t%5d\t%5d\t%5d\t%3d\t%3d\t%5d\t%5d\t%3d\t%3d\t%5d\n", \
        eeprom.param.reset_steps, eeprom.param.step_delay_us, eeprom.param.publish_interval_during_move, eeprom.param.stepper_wake_delay_ms, \
        eeprom.param.solenoid_surge_power, eeprom.param.solenoid_sustain_power, eeprom.param.solenoid_surge_period_ms, \
        eeprom.param.delay_between_sensor_readings_ms, eeprom.param.sensor_params & 0xFF, eeprom.param.sensor_params >> 8, eeprom.param.calibration_steps);
    return 1;
}

int command_erase_eeprom() {
    erase_eeprom();
    return 1;
}

int get_sensor_reading(char sensor_code, TCS34725 *sensor, int sensorLEDPower) {
    int i, len;

    analogWrite(pinSensorLED, sensorLEDPower);
    delay(SENSOR_LED_WARMUP_DELAY_MS);

    for (i = 0; i < SENSOR_NUMBER_OF_SAMPLES; i += 1) {
        read_one_sensor(sensor, sensor_code, i);
        delay(eeprom.param.delay_between_sensor_readings_ms);
    }

    analogWrite(pinSensorLED, 0);

    convert_samples_to_reading(-1, sensor_code);

    len = snprintf(particle_register, PARTICLE_REGISTER_SIZE, "%11d\t%5d\t%5d\t%5d\n", \
        sensor_reading.start_time, sensor_reading.red_norm, sensor_reading.green_norm, sensor_reading.blue_norm);

    particle_register[len] = '\0';
    return 1;
}

int command_read_assay_sensor() {
  int sensorLEDPower = extract_int_from_string(particle_command.param, 0, PARTICLE_COMMAND_PARAM_LENGTH);
  set_sensor_params((uint8_t) eeprom.param.sensor_params & 0x00FF, (uint8_t) (eeprom.param.sensor_params >> 8));
  return get_sensor_reading('A', &tcsAssay, sensorLEDPower);
}

int command_read_control_sensor() {
  int sensorLEDPower = extract_int_from_string(particle_command.param, 0, PARTICLE_COMMAND_PARAM_LENGTH);
  set_sensor_params((uint8_t) eeprom.param.sensor_params & 0x00FF, (uint8_t) (eeprom.param.sensor_params >> 8));
  return get_sensor_reading('C', &tcsControl, sensorLEDPower);
}

int command_start_stress_test() {
  stress_test = true;
  stress_test_count = 0;
  STATUS("Running stress test");
  return 1;
}

int command_stop_stress_test() {
  stress_test = false;
  STATUS("Stress test stopped");
  return 1;
}

int command_get_battery_level() {
    return analogRead(pinBatteryAin);
}

int command_get_stress_test_count() {
    return (int) EEPROM.read(EEPROM_ADDR_STRESS_TEST_COUNT);
}

int command_get_DC_in_status() {
    return digitalRead(pinDCinDetect);
}

//
//

void parse_particle_command(String msg) {
    int len = msg.length();
    msg.toCharArray(particle_command.arg, len + 1);

    particle_command.code = extract_int_from_string(particle_command.arg, PARTICLE_COMMAND_CODE_INDEX, PARTICLE_COMMAND_CODE_LENGTH);
    len -= PARTICLE_COMMAND_CODE_LENGTH;
    strncpy(particle_command.param, &particle_command.arg[PARTICLE_COMMAND_PARAM_INDEX], len);
    particle_command.param[len] = '\0';
}

int run_command(String msg) {
    parse_particle_command(msg);

    switch (particle_command.code) {
    // operating functions
        case 1: // run test
            return command_run_brevitest();
        case 2: // cancel test
            return command_cancel_brevitest();
        case 3: // claim device
            return command_claim_device();
        case 4: // release device
            return command_release_device();

    // data transfer
        case 10: // start data transfer
            return command_start_transfer();
        case 11: // process data transfer packet
            return command_receive_packet();

    // check cache
        case 20: // check assay cache
            return command_check_assay_cache();
        case 21: // check test cache
            return command_check_test_cache();

    // configuration functions
        case 30: // set device serial number
            return command_write_serial_number();
        case 31: // change device parameter
            return command_change_param();
        case 32: // reset device parameters to default
            return command_reset_params();
        case 33: // get current firmware version number
            return command_get_firmware_version();
        case 34: // set and move to calibration point
            return command_set_and_move_to_calibration_point();
        case 35: // dump eeprom to serial port
            return command_dump_eeprom();
        case 36: // erase eeprom from flash
            return command_erase_eeprom();

      // test functions
        case 50:
            return command_move_stage();
        case 51:
            return command_energize_solenoid();
        case 52:
            return turn_on_device_LED();
        case 53:
            return turn_off_device_LED();
        case 54:
            return command_blink_device_LED();
        case 55:
            return command_read_QR_code();
        case 56:
            return command_verify_QR_code();
        case 57:
            return command_erase_assay_cache();
        case 58:
            return command_erase_test_cache();
        case 59:
            return command_get_cache_count();
        /*case 60:
            return command_dump_test_cache();*/
        case 61:
            return command_read_assay_sensor();
        case 62:
            return command_read_control_sensor();
        case 63:
            return command_start_stress_test();
        case 64:
            return command_stop_stress_test();
        case 65:
            return command_get_battery_level();
        case 66:
            return command_get_DC_in_status();
        case 67:
            return command_get_stress_test_count();
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
    char *bcode = eeprom.assay_cache[assay_index].BCODE;

    if (cancel_process) {
        return index;
    }

    if (bcode[index] == '\0') { // end of string
        return index; // return end of string location
    }

    if (bcode[index] == '\n') { // command has no parameter
        return index + 1; // skip past parameter
    }

    // there is a parameter to extract
    i = index;
    while (i < ASSAY_BCODE_CAPACITY) {
        if (bcode[i] == '\0') {
            *token = extract_int_from_string(bcode, index, (i - index));
            return i; // return end of string location
        }
        if ((bcode[i] == '\n') || (bcode[i] == ',')) {
            *token = extract_int_from_string(bcode, index, (i - index));
            i++; // skip past parameter
            return i;
        }

        i++;
    }

    return i;
}

void publish_progress() {
  unsigned long now = millis();

  if (now - test_last_progress_update > PARTICLE_PUBLISH_INTERVAL) {
      Particle.publish(test_record->test_uuid, particle_status, 60, PRIVATE);
      test_last_progress_update = now;
  }
}

void update_progress(char *message, int duration) {
    int test_duration = eeprom.assay_cache[assay_index].duration * 1000;

    if (!cancel_process) {
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

        STATUS("%s\n%.24s\n%d", message, test_record->test_uuid, test_percent_complete);
        publish_progress();
    }
}

int process_one_BCODE_command(int cmd, int index) {
    int buf, i, param1, param2, start_index, timeout;

    if (cancel_process) {
        return index;
    }

    switch(cmd) {
        case 0: // Start test(integration time+gain, LED power)
            test_record->start_time = Time.now();
            index = get_BCODE_token(index, &param1);
            index = get_BCODE_token(index, &sensor_LED_power);

            test_record->param.sensor_params = param1;
            set_sensor_params((uint8_t) param1 & 0x00FF, (uint8_t) (param1 >> 8));

            update_progress("Warming up sensor LEDs", 1000);
            analogWrite(pinSensorLED, sensor_LED_power);
            delay(1000);
            read_sensors(0); // read initial baseline values
            analogWrite(pinSensorLED, 0);
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
        case 4: // Device LED on
            turn_on_device_LED();
            break;
        case 5: // Device LED off
            turn_off_device_LED();
            break;
        case 6: // Device LED blink(duration in milliseconds, rate in milliseconds)
            index = get_BCODE_token(index, &param1);
            index = get_BCODE_token(index, &param2);
            param1 = abs(param1);
            param2 = abs(param2);
            update_progress("Blinking device LED", param1);
            start_blinking_device_LED(param1, param2);
            break;
        case 7: // Sensor LED on(power)
            index = get_BCODE_token(index, &param1);
            analogWrite(pinSensorLED, param1);
            break;
        case 8: // Sensor LED off
            analogWrite(pinSensorLED, 0);
            break;
        case 9: // Read sensors
            update_progress("Warming up sensor LEDs", 1000);
            analogWrite(pinSensorLED, sensor_LED_power);
            delay(1000);
            read_sensors(1); // read final values
            analogWrite(pinSensorLED, 0);
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
            test_record->finish_time = Time.now();
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

void setup() {
  pinMode(pinBatteryAin, INPUT);
  pinMode(pinLimitSwitch, INPUT_PULLUP);

  pinMode(pinBatteryLED, OUTPUT);
  pinMode(pinSensorLED, OUTPUT);
  pinMode(pinDeviceLED, OUTPUT);
  pinMode(pinSolenoid, OUTPUT);
  pinMode(pinStepperStep, OUTPUT);
  pinMode(pinStepperSleep, OUTPUT);
  pinMode(pinStepperDir, OUTPUT);
  pinMode(pinQRTrigger, OUTPUT);

  digitalWrite(pinSensorLED, LOW);
  analogWrite(pinSolenoid, 0);
  digitalWrite(pinStepperStep, LOW);
  digitalWrite(pinStepperDir, LOW);
  digitalWrite(pinStepperSleep, LOW);
  digitalWrite(pinQRTrigger, LOW);

  turn_off_device_LED();

  Serial.begin(9600);     // standard serial port

  load_eeprom();

  if (eeprom.firmware_version != FIRMWARE_VERSION) {
    EEPROM.write(EEPROM_ADDR_FIRMWARE_VERSION, FIRMWARE_VERSION);
    eeprom.firmware_version = FIRMWARE_VERSION;
  }

  if (eeprom.data_format_version != DATA_FORMAT_VERSION) {
    EEPROM.write(EEPROM_ADDR_DATA_FORMAT_VERSION, DATA_FORMAT_VERSION);
    eeprom.data_format_version = DATA_FORMAT_VERSION;
    reset_params();
  }

  reset_globals();

  init_sensor(&tcsAssay, pinAssaySDA, pinAssaySCL);
  init_sensor(&tcsControl, pinControlSDA, pinControlSCL);

  Particle.function("runcommand", run_command);
  Particle.function("requestdata", request_data);
  Particle.variable("register", particle_register, STRING);
  Particle.variable("status", particle_status, STRING);
  Particle.variable("claimant", claimant_uuid, STRING);
  Particle.variable("powerstatus", &power_status, INT);

  reset_stage();

  STATUS("Device startup complete");
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void reset_globals() {
  start_test = false;
  test_in_progress = false;
  cancel_process = false;

  qr_uuid[0] = '\0';
  claimant_uuid[0] = '\0';
  test_uuid[0] = '\0';
  qr_uuid[0] = '\0';

  particle_register[0] = '\0';
  particle_status[0] = '\0';

  test_progress = 0;
  test_percent_complete = 0;
  test_in_progress = false;

  test_index = -1;
  assay_index = -1;
  test_record = 0;
}

void do_run_test() {
  if (claimant_uuid[0] != '\0') {
    stop_blinking_device_LED();
    turn_on_device_LED();
    start_test = false;
    test_in_progress = true;
    test_last_progress_update = millis();

    pinMode(pinSolenoid, OUTPUT);
    analogWrite(pinSolenoid, 0);
    analogWrite(pinSensorLED, 0);

    if (validate_QR_code(eeprom.test_cache[test_index].cartridge_uuid) == 0) {
      memcpy(&test_record->param, &eeprom.param, EEPROM_PARAM_LENGTH);
      move_to_calibration_point();

      process_BCODE(0);

      reset_stage();
      update_progress("Test complete", -1);

      reset_globals();
    }
    turn_off_device_LED();
  }
}

void do_cancel_test() {
  STATUS("%s\n%.24s\n%d", "Test cancelled", test_record->test_uuid, -1);
  Particle.publish(test_record->test_uuid, particle_status, 60, PRIVATE);
  reset_stage();
  publish_progress();
  reset_globals();
}

void do_calibration() {
  calibrate = false;
  save_calibration_point();
  move_to_calibration_point();
}

void raster_well() {
    int i, j;

    delay(1000);
    for (i = 0; i < 4; i++) {
        move_solenoid(1200);
        delay(1000);
        for (j = 0; j < 7; j++) {
            if (!stress_test) {
                return;
            }
            move_solenoid(750);
            delay(1000);
        }
        move_steps(200, eeprom.param.step_delay_us);
    }
    move_solenoid(1200);
    delay(1000);
    for (j = 0; j < 7; j++) {
        if (!stress_test) {
            return;
        }
        move_solenoid(750);
        delay(1000);
    }
    delay(2000);
    for (i = 0; i < 20; i++) {
        if (!stress_test) {
            return;
        }
        move_steps(100, eeprom.param.step_delay_us);
        delay(1500);
    }
    move_steps(-800, eeprom.param.step_delay_us);
}

void do_next_stress_test() {
    // based upon CDC Ebola IgG assay
    int i, j;

    if (!stress_test) {
        return;
    }

    switch (stress_test_count % STRESS_TEST_STEPS) {
        case 0: // reset stage
            reset_stage();
            break;
        case 1: // turn on device LED
            turn_on_device_LED();
            break;
        case 2: // scan QR code
            scan_QR_code();
            break;
        case 3: // turn off device LED
            turn_off_device_LED();
            break;
        case 4: // move to microbead well
            move_steps(5000, eeprom.param.step_delay_us);
            delay(4000);
            break;
        case 5: // collect microbeads and drag to analyte well
            for (i = 0; i < 20; i++) {
                if (!stress_test) {
                    return;
                }
                move_steps(100, eeprom.param.step_delay_us);
                delay(1500);
            }
            move_steps(-800, eeprom.param.step_delay_us);
            break;
        case 6: // raster analyte well
            raster_well();
            break;
        case 7: // raster buffer well
            raster_well();
            break;
        case 8: // raster antibody well
            raster_well();
            break;
        case 9: // move stage -5mm
            delay(1000);
            for (i = 0; i < 12; i++) {
                move_solenoid(1200);
                delay(1000);
                for (j = 0; j < 10; j++) {
                    if (!stress_test) {
                        return;
                    }
                    move_solenoid(750);
                    delay(1000);
                }
                move_steps(100, eeprom.param.step_delay_us);
            }
            for (i = 0; i < 12; i++) {
                move_solenoid(1200);
                delay(1000);
                for (j = 0; j < 10; j++) {
                    if (!stress_test) {
                        return;
                    }
                    move_solenoid(750);
                    delay(1000);
                }
                move_steps(-100, eeprom.param.step_delay_us);
            }
            break;
        case 10: // read assay sensor
            init_sensor(&tcsAssay, pinAssaySDA, pinAssaySCL);
            get_sensor_reading('A', &tcsAssay, 50);
            break;
        case 11: // read control sensor
            init_sensor(&tcsControl, pinControlSDA, pinControlSCL);
            get_sensor_reading('C', &tcsControl, 50);
            break;
    }
    if (stress_test_count % STRESS_TEST_STEPS == 0) {
      EEPROM.write(EEPROM_ADDR_STRESS_TEST_COUNT, stress_test_count / STRESS_TEST_STEPS);
    }

    stress_test_count++;
    STATUS("Stress test - cycles = %d", stress_test_count / STRESS_TEST_STEPS);
}

void loop(){
    int battery_level, dc_status;

    if (start_test && !test_in_progress) {
        do_run_test();
    }

    if (cancel_process) {
        do_cancel_test();
    }

    if (calibrate) {
        do_calibration();
    }

    if (device_LED.blinking) {
        update_blinking_device_LED();
    }

    if (stress_test) {
        do_next_stress_test();
    }

    battery_level = analogRead(pinBatteryAin);
    dc_status = digitalRead(pinDCinDetect);
    power_status = (battery_level > BATTERY_CONVERSION_FACTOR * 100 ? 100 : battery_level / BATTERY_CONVERSION_FACTOR) * (dc_status ? -1 : 1);
}
