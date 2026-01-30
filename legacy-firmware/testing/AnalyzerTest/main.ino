#include "TCS34725.h"
#include "main.h"
#include "Serial4/Serial4.h"

SYSTEM_THREAD(ENABLED);
PRODUCT_ID(2045);
PRODUCT_VERSION(FIRMWARE_VERSION);

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
        /*erase_test_cache();*/
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

                /*if (i % MOVE_STEPS_BETWEEN_PARTICLE_PROCESS == 0) {
                        Particle.process();
                }*/

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
        move_steps(STEPS_TO_MICROBEAD_WELL, eeprom.param.step_delay_us);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                    CARTRIDGE HEATER                     //
//                                                         //
/////////////////////////////////////////////////////////////

void cartridge_heat_on() {
    digitalWrite(pinCartridgeHeater, HIGH);
    cartridge_heater_is_on = TRUE;
    digitalWrite(pinCartridgeHeaterLED, HIGH);
    Serial.printlnf("Heater ON");
}

void cartridge_heat_off() {
    digitalWrite(pinCartridgeHeater, LOW);
    cartridge_heater_is_on = false;
    digitalWrite(pinCartridgeHeaterLED, LOW);
    Serial.printlnf("Heater OFF");
}

void change_cartridge_heater_state() {
    int red_norm = check_sensor_red_norm('A');
    if (red_norm > CARTRIDGE_HEATER_LED_THRESHOLD) {    // red above threshold means reagent still below 33 deg C, use high heat
        if (cartridge_heater_is_on) {
            cartridge_heat_off();
            cartridge_heater_timer.changePeriod(CARTRIDGE_HEATER_HIGH_OFF_PERIOD);
        }
        else {
            cartridge_heat_on();
            cartridge_heater_timer.changePeriod(CARTRIDGE_HEATER_HIGH_ON_PERIOD);
        }
    }
    else {    // red below threshold means reagent at or above 33 deg C, use low heat
        if (cartridge_heater_is_on) {
            cartridge_heat_off();
            cartridge_heater_timer.changePeriod(CARTRIDGE_HEATER_LOW_OFF_PERIOD);
        }
        else {
            cartridge_heat_on();
            cartridge_heater_timer.changePeriod(CARTRIDGE_HEATER_LOW_ON_PERIOD);
        }
    }
}

void turn_on_cartridge_heater() {
    cartridge_heat_on();
    cartridge_heater_timer.changePeriod(CARTRIDGE_HEATER_LOW_ON_PERIOD);
    cartridge_heater_timer.reset();
}

void turn_off_cartridge_heater() {
    cartridge_heat_off();
    cartridge_heater_timer.stop();
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

int check_sensor_red_norm(char sensor_code) {
    TCS34725 *sensor;
    uint16_t red = 0, green = 0, blue = 0, clear = 0;
    int tries = 0;
    int red_norm;

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
    red_norm = (int) red * 10000;
    red_norm /= (int) clear;
    return red_norm;
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

void read_one_sensor(char sensor_code, int sample_number, int integrationTime, int gain) {
        BrevitestSensorSampleRecord *sample;
        TCS34725 *sensor;
        int tries;

        /*Particle.process();*/

        if (sensor_code == 'A') {
                sample = &assay_buffer[sample_number];
                sensor = &tcsAssay;
        }
        else {
                sample = &control_buffer[sample_number];
                sensor = &tcsControl;
        }

        sensor->begin((tcs34725IntegrationTime_t) integrationTime, (tcs34725Gain_t) gain);

        sample->sample_time = Time.now();
        sample->red = sample->green = sample->blue = sample->clear = tries = 0;
        while (sample->clear == 0 && tries++ < 5) {
            sensor->getRawData(&sample->red, &sample->green, &sample->blue, &sample->clear);
        }

        sensor->end();
}

int read_sensors_with_parameters(int ledPower, int integrationTime, int gain) {
        int i;

        analogWrite(pinSensorLED, ledPower);
        delay(SENSOR_LED_WARMUP_DELAY_MS);

        for (i = 0; i < SENSOR_NUMBER_OF_SAMPLES; i += 1) {
                read_one_sensor('A', i, integrationTime, gain);
                read_one_sensor('C', i, integrationTime, gain);

                Serial.printlnf("%d %d %d %d %d %d %d %d %d %d %d", i, \
                    assay_buffer[i].sample_time, assay_buffer[i].clear, assay_buffer[i].red, assay_buffer[i].green, assay_buffer[i].blue, \
                    control_buffer[i].sample_time, control_buffer[i].clear, control_buffer[i].red, control_buffer[i].green, control_buffer[i].blue);
        }

        analogWrite(pinSensorLED, 0);

        convert_samples_to_reading('A');
        convert_samples_to_reading('C');

        return 1;
}

int read_sensors() {
        read_sensors_with_parameters(assay.led_power, assay.sensor_integration_time, assay.sensor_gain);
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

bool device_is_open() {
    uint16_t level = check_sensor_clear('A');
    return (level > SENSOR_DEVICE_OPEN_THRESHOLD);
}

void check_device_status() {
    check_device_status_flag = false;
    bool open_now = device_is_open();
    /*Serial.printlnf("device_open_state: %c, open_now: %c", device_open_state ? 'T' : 'F', open_now ? 'T' : 'F');*/
    if (open_now ^ device_open_state) {   // device status changed
        if (open_now) {
            Serial.printlnf("Device just opened");
            turn_off_cartridge_heater();
            memcpy(qr_uuid, DEVICE_OPEN_UUID, CARTRIDGE_UUID_LENGTH);
        }
        else {
            /*if (device_open_cancel_timer.isActive()) {*/
            if (test_in_progress) {
                /*Serial.println("Device closed in time - test resumed");*/
                /*device_open_cancel_timer.stop();*/
                /*turn_on_cartridge_heater();*/
                start_blinking_device_LED(0, 500, 0, 255, 0);
            }
            else {  // no test in progress
                Serial.printlnf("Device just closed");
                start_blinking_device_LED(0, 100, 0, 0, 255);
                if (cartridge_loaded()) {
                    Serial.println("Cartridge in device");
                    /*turn_on_cartridge_heater();*/
                    if (scan_QR_code() == CARTRIDGE_UUID_LENGTH) {
                        /*validate_cartridge();*/
                        Serial.printlnf("QR code scanned: %s", qr_uuid);
                    }
                    else {
                        stop_blinking_device_LED();
                        set_device_LED_color(255, 0, 0);    // bad cartridge uuid
                        turn_on_device_LED();
                        cartridge_validated = false;
                    }
                }
                else {
                    Serial.println("No cartridge loaded");
                    turn_off_cartridge_heater();
                    strncpy(qr_uuid, NO_CARTRIDGE_UUID, CARTRIDGE_UUID_LENGTH);
                    stop_blinking_device_LED();
                    set_device_LED_color(128, 128, 128);
                    turn_on_device_LED();
                    cartridge_validated = false;
                }
            }
        }
    }
    device_open_state = open_now;
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
//                        COMMANDS                         //
//                                                         //
/////////////////////////////////////////////////////////////

int particle_command(String arg) {
    String cmd, params;
    int cmd_number, param1, param2, param3;
    int loc = arg.indexOf(' ');

    Serial.println(arg);

    if (loc == -1) {
        cmd = arg;
        particle_register[0] = '\0';
    }
    else {
        cmd = arg.substring(0, loc);
        params = arg.substring(loc + 1);
    }

    cmd_number = cmd.toInt();
    switch (cmd_number) {
        case 1:     // move_steps(steps, step_delay_us)
            Serial.printlnf("command %d: move_steps", cmd_number);
            loc = params.indexOf(',');
            if (loc == -1) {
                return -2;
            }
            param1 = params.substring(0, loc).toInt();
            param2 = params.substring(loc + 1).toInt();
            Serial.printlnf("steps = %d, step_delay = %d", param1, param2);
            move_steps(param1, param2);
            break;
        case 2:     // move_solenoid(duration)
            Serial.printlnf("command %d: move_solenoid", cmd_number);
            param1 = params.toInt();
            Serial.printlnf("duration = %d", param1);
            move_solenoid(param1);
            break;
        case 3:     // scan_QR_code()
            Serial.printlnf("command %d: scan_QR_code", cmd_number);
            scan_QR_code();
            memcpy(particle_register, qr_uuid, CARTRIDGE_UUID_LENGTH);
            break;
    }

    return cmd_number;
}


/////////////////////////////////////////////////////////////
//                                                         //
//                          SETUP                          //
//                                                         //
/////////////////////////////////////////////////////////////

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

void setup() {
        Particle.variable("register", particle_register, STRING);
        Particle.variable("status", particle_status, STRING);
        Particle.variable("powerstatus", &power_status, INT);
        Particle.function("command", particle_command);

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

        digitalWrite(pinSensorLED, LOW);
        analogWrite(pinSolenoid, 0);
        digitalWrite(pinStepperStep, LOW);
        digitalWrite(pinStepperDir, LOW);
        digitalWrite(pinStepperSleep, LOW);
        digitalWrite(pinQRTrigger, LOW);
        digitalWrite(pinCartridgeHeater, LOW);
        digitalWrite(pinCartridgeHeaterLED, LOW);

        device_id_string = System.deviceID();
        device_id_string.toCharArray(device_id, DEVICE_ID_LENGTH + 1);
        device_id[DEVICE_ID_LENGTH] = '\0';

        turn_off_device_LED();
        set_device_LED_color(255, 255, 0);
        turn_on_device_LED();

        Serial.begin(115200); // standard serial port

        load_eeprom();
        if (eeprom.firmware_version != FIRMWARE_VERSION || eeprom.data_format_version != DATA_FORMAT_VERSION) {
                reset_eeprom();
        }

        init_sensor(&tcsAssay, SENSOR_NUMBER_ASSAY);
        init_sensor(&tcsControl, SENSOR_NUMBER_CONTROL);

        reset_stage();
        reset_globals();

        calculate_power_status();

        device_open_state = !device_is_open();
        device_status_timer.reset();
        battery_check_timer.reset();

        Serial.printlnf("eeprom.firmware_version: %d, eeprom.data_format_version: %d, eeprom.most_recent_test: %d", eeprom.firmware_version, eeprom.data_format_version, eeprom.most_recent_test);
        /*turn_on_cartridge_heater();*/
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void reset_globals() {
        validating_cartridge = false;
        start_test = false;
        run_test = false;
        test_in_progress = false;
        cancel_test = false;
        test_record_created = false;
        uploading_test = false;
        cartridge_validated = false;

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

void loop() {
        int inchar;

        if (update_battery_life) {
            update_battery_life = false;
            calculate_power_status();
        }

        if (check_device_status_flag) {
             check_device_status();
        }
}
