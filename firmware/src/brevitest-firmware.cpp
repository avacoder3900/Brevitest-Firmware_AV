/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/leo3/github/brevitest-device/firmware/src/brevitest-firmware.ino"
/*
 * Project brevitest_v1_0
 * Description: firmware for Acuity Analyzer, part of the Brevitest Diagnostic Platform
 * Author: Leo Linbeck III
 * Date: 11 April 2020
 */

#include "brevitest-firmware.h"

int raw_table_lookup(int raw);
int extract_int_from_string(char *str, int pos, int len);
int extract_int_from_delimited_string(char *str, int *indx, char delim);
uint32_t checksum(char *buf, int size);
int integerSqrt(int n);
void load_eeprom();
void store_eeprom();
void erase_eeprom();
void reset_eeprom();
void setup_eeprom();
bool move_one_eighth_step(int dir, int step_delay);
void move_stage(int microns, int step_delay);
void wake_move_sleep_stage(int microns, int step_delay);
void sleep_motor();
void wake_motor();
void reset_stage(bool sleep);
void move_stage_to_optical_read_position();
void move_stage_to_test_start_position();
void move_stage_to_position(int position, int step_delay);
void oscillate_stage(int amplitude, int step_delay, int cycles, bool inBCODE);
bool scan_barcode();
void turn_on_buzzer_for_duration(int duration, int frequency);
void alert_buzzer();
void turn_on_alert_buzzer();
void turn_off_alert_buzzer();
void turn_on_heater(int power);
void turn_off_heater();
int limit(int value, int max, int min);
int set_heater_power(int power);
void turn_on_LED(char channel, int power);
void turn_off_LED(char channel);
void turn_on_all_LEDs(int power);
void turn_on_assay_LED(int power);
void turn_on_control_1_LED(int power);
void turn_on_control_2_LED(int power);
void turn_off_assay_LED();
void turn_off_control_1_LED();
void turn_off_control_2_LED();
void turn_off_all_LEDs();
void turn_on_assay_LED_for_duration(int duration, int power);
void turn_on_control_1_LED_for_duration(int duration, int power);
void turn_on_control_2_LED_for_duration(int duration, int power);
void turn_on_all_LEDs_for_duration(int duration, int power);
void magnetometer_read_confirm(const char *event, const char *data);
void read_magnetometer();
void test_optical_sensors();
void config_optical_sensors(char channel, int param, int addr);
bool optical_sensor_ready(uint8_t addr);
bool take_one_sample_from_optical_sensor(uint8_t addr, uint16_t *x, uint16_t *y, uint16_t *z, uint16_t *tempC);
void get_data_from_one_optical_sensor(char channel, int param, int led_power);
bool enable_optical_sensors(bool force_read);
void disable_optical_sensors();
void read_optical_sensors(int param, int led_power, bool inBCODE);
int get_heater_temperature();
void heater_temperature_read();
int pid_controller();
void control_heater_temperature();
void start_temperature_control();
void stop_temperature_control();
void validate_cartridge();
bool load_assay_record(char *responseString);
void do_stress_test_step(int step);
void set_current_event(String event_name);
void clear_current_event();
bool callback_pending();
void set_publish_params(String event_name);
void brevitest_publish(String event_name, char *uuid);
void callback_register();
void callback_validate();
void remove_test_from_cache(char *testToRemove);
void callback_test_upload();
int extract_callback_params(char *param, int paramLen, int indx, char delim);
void process_callback_buffer();
void brevitest_error(const char *event, const char *data);
void brevitest_callback(const char *event, const char *data);
void erase_test_from_cache(int index);
void initialize_test_cache();
int find_test_index_by_uuid(char *uuid);
void store_test(int index);
int append_test_reading(int start, BrevitestOpticalSensorRecord *reading);
void process_test_record(int index);
void write_test_record_to_eeprom();
bool tests_to_upload();
void upload_one_test(int test_number, char *cartridge_uuid);
void upload_tests();
int get_BCODE_token(int index, int *token);
void update_progress(String message, int duration);
int BCODE_loop();
void BCODE_delay(int target_duration);
int process_one_BCODE_command(int cmd, int index);
int process_BCODE(int start_index);
int get_next_command_param(String arg, int indx, int *param, int def);
void i2c_bus_scan();
int particle_command(String arg);
void cartridge_state_changed_interrupt();
void check_device_state(bool startup);
void reset_globals();
void run_test();
int particle_run_test(String arg);
void init_analog_pin(uint16_t pin, PinMode mode, uint8_t value);
void init_digital_pin(uint16_t pin, PinMode mode, uint8_t value);
void startup_analyzer();
void setup();
void registration_loop();
void validate_cartridge_loop();
void test_upload_loop();
void long_duration_command_loop();
void state_loop();
void loop();
#line 10 "/Users/leo3/github/brevitest-device/firmware/src/brevitest-firmware.ino"
SYSTEM_THREAD(ENABLED);
PRODUCT_ID(11170);
PRODUCT_VERSION(FIRMWARE_VERSION);

/////////////////////////////////////////////////////////////
//                                                         //
//                        TABLES                           //
//                                                         //
/////////////////////////////////////////////////////////////

//  temperature is 10x to get one decimal place of accuracy
static int table_temperature[] = {1000, 950, 900, 850, 800, 750, 700, 650, 600, 550, 500, 450, 400, 350, 300, 250, 200, 150, 100, 50, 0};
static int table_raw[] = {3698, 3478, 3248, 3010, 2767, 2521, 2275, 2033, 1799, 1575, 1364, 1169, 990, 830, 688, 564, 457, 367, 291, 228, 177};

static uint32_t crc32_tab[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
    0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
    0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
    0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
    0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
    0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
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
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

int raw_table_lookup(int raw)
{
    int result, indx1, indx2;

    if (raw > table_raw[0])
    {
        return table_raw[0];
    }

    for (indx1 = 0, indx2 = 1; indx1 < (TERMISTOR_TABLE_LENGTH - 1); indx1++, indx2++)
    {
        if (raw <= table_raw[indx1] && raw > table_raw[indx2])
        {
            result = table_temperature[indx1] + (((raw - table_raw[indx1]) * (table_temperature[indx2] - table_temperature[indx1])) / (table_raw[indx2] - table_raw[indx1]));
            /*if (serial_messaging_on) Serial.printlnf("Table: number = %d, indx1 = %d, indx2 = %d, result = %d", table_number, indx1, indx2, result);*/
            return result;
        }
    }

    return 0;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        UTLITY                           //
//                                                         //
/////////////////////////////////////////////////////////////

int extract_int_from_string(char *str, int pos, int len)
{
    char buf[12];

    len = len > 12 ? 12 : len;
    strncpy(buf, &str[pos], len);
    buf[len] = '\0';
    return atoi(buf);
}

int extract_int_from_delimited_string(char *str, int *indx, char delim)
{
    char buf[14];
    bool stop = false;

    for (int i = 0; i < 14 ; i++) {
        stop = str[*indx] == delim;
        buf[i] = stop ? '\0' : str[*indx];
        (*indx)++;
        if (stop) break;
    }
    return atoi(buf);
}

uint32_t checksum(char *buf, int size)
{
    uint8_t *p;
    uint32_t crc = ~0U;

    p = (uint8_t *)buf;

    while (size--)
        crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

    return crc ^ ~0U;
}

int integerSqrt(int n)
{
    int shift, nShifted, result, candidateResult;

    if (n < 0)
    {
        return -1;
    }

    shift = 2;
    nShifted = n >> shift;
    while ((nShifted != 0) && (nShifted != n))
    {
        shift += 2;
        nShifted = n >> shift;
    }
    shift -= 2;

    result = 0;
    while (shift >= 0)
    {
        result <<= 1;
        candidateResult = result + 1;
        if ((candidateResult * candidateResult) <= (n >> shift))
        {
            result = candidateResult;
        }
        shift -= 2;
    }

    return result;
}

////////////////////////////////////////////////////////////
//                                                         //
//                         EEPROM                          //
//                                                         //
/////////////////////////////////////////////////////////////

void load_eeprom()
{
    EEPROM.get(0, eeprom);
}

void store_eeprom()
{
    EEPROM.put(0, eeprom);
}

void erase_eeprom()
{
    EEPROM.clear();
}

void reset_eeprom()
{
    Particle_EEPROM e;

    erase_eeprom();
    memcpy(&eeprom, &e, (int)sizeof(Particle_EEPROM));
}

void setup_eeprom()
{
    int addr = 0;
    uint8_t value;

    EEPROM.get(addr, value);
    if (value == 0xFF) {
        reset_eeprom();
    } else {
        load_eeprom();
        if (eeprom.firmware_version != FIRMWARE_VERSION || eeprom.data_format_version != DATA_FORMAT_VERSION) {
            reset_eeprom();
        }
    }
}

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
            Serial.println("Stage limit switch detected");
            stage_position = 0;
            microns_error = 0;
            return false;
        }
        if (stage_position <= 0)
        {
            Serial.println("Stage position at zero");
            stage_position = 0;
            microns_error = 0;
            return false;
        }
    }
    else if (stage_position >= STAGE_POSITION_LIMIT)
    {
        Serial.println("Stage distal limit reached");
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

    int eighth_steps, abs_microns, dir, i;

    dir = (microns < 0) ? HIGH : LOW;
    digitalWrite(pinMotorDir, dir);
    /*Serial.printlnf("Stepping, dir = %c", dir == LOW ? 'L' : 'H');*/

    abs_microns = abs(microns) + microns_error;
    eighth_steps = abs_microns / MICRONS_PER_EIGHTH_STEP;
    microns_error = abs_microns % MICRONS_PER_EIGHTH_STEP;
    /*Serial.printlnf("move_stage: microns = %d, dir = %d, eighth_steps = %d, microns_error = %d", microns, dir == LOW ? 'L' : 'H', eighth_steps, microns_error);*/

    // delay(10);
    for (i = 0; i < eighth_steps; i++)
    {
        if (move_one_eighth_step(dir, step_delay)) {
            stage_position += microns < 0 ? -MICRONS_PER_EIGHTH_STEP : MICRONS_PER_EIGHTH_STEP;
            if (stage_position <= 0) {
                stage_position = 0;
                microns_error = 0;
                i = eighth_steps;
            }
        } else {
            i = eighth_steps;
        }
    }
    /*Serial.printlnf("Move complete, stage location = %d, limit = %d", stage_position, STAGE_POSITION_LIMIT);*/
}

void wake_move_sleep_stage(int microns, int step_delay)
{
    wake_motor();
    move_stage(microns, step_delay);
    sleep_motor();
}

void sleep_motor()
{
    digitalWrite(pinMotorSleep, LOW);
    delay(20);
}

void wake_motor()
{
    digitalWrite(pinMotorSleep, HIGH);
    delay(10);
}

void reset_stage(bool sleep)
{
    stage_position = STAGE_POSITION_LIMIT;
    wake_motor();
    move_stage(-60000, FAST_STEP_DELAY);
    delay(100);
    move_stage(2000, FAST_STEP_DELAY);
    delay(100);
    move_stage(-3000, SLOW_STEP_DELAY);
    move_stage(MICRONS_TO_INITIAL_POSITION, SLOW_STEP_DELAY);
    if (sleep)
    {
        sleep_motor();
    }
}

void move_stage_to_optical_read_position()
{
    move_stage_to_position(OPTICAL_SENSOR_READ_POSITION, SLOW_STEP_DELAY);
}

void move_stage_to_test_start_position()
{
    move_stage_to_position(MICRONS_TO_TEST_START_POSITION, SLOW_STEP_DELAY);
}

void update_progress(String, int);
void move_stage_to_position(int position, int step_delay)
{
    int move_distance = position - stage_position;
    update_progress("Moving magnets to position", (abs(move_distance) * step_delay / MICRONS_PER_EIGHTH_STEP) / 1000);
    Serial.printlnf("Moving stage to position %d, distance = %d", position, move_distance);
    move_stage(move_distance, step_delay);
}

int BCODE_loop(void);
void oscillate_stage(int amplitude, int step_delay, int cycles, bool inBCODE)
{
    int i;

    for (i = 0; i < cycles; i++)
    {
        move_stage(amplitude, step_delay);
        if (inBCODE) BCODE_loop(); else Particle.process();
        move_stage(-amplitude, step_delay);
        if (inBCODE) BCODE_loop(); else Particle.process();
        if (test_cancelled) return;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                  BARCODE SCANNER                        //
//                                                         //
/////////////////////////////////////////////////////////////

bool scan_barcode()
{
    unsigned long timeout;
    int i = 0;
    bool read_success = false;
    int buf;

    barcode_being_scanned = true;

    Serial.println("Start scanning barcode");
    Serial1.begin(9600); // barcode scanner interface through RX/TX pins

    Serial.println("Trigger barcode reader");
    digitalWrite(pinBarcodeTrigger, LOW);

    timeout = millis() + BARCODE_READ_TIMEOUT;

    Serial.println("Wait for success");
    do {
        read_success = digitalRead(pinBarcodeReady) == HIGH;
        Serial.print('.');
        Particle.process();
        delay(200);
    } while (!read_success && millis() < timeout);

    Serial.printlnf("Stop triggering barcode reader, ready=%c", Serial1.available() ? 'Y' : 'N');
    digitalWrite(pinBarcodeTrigger, HIGH);

    Serial.println("Wait for barcode data");
    while (!Serial1.available() && millis() < timeout) {
        Particle.process();
    };

    delay(100); // allow barcode buffer to fill before reading

    Serial.println("Reading barcode from Serial1");
    do {
        Serial.print('.');
        buf = Serial1.read();
        if (buf != -1) {
            barcode_uuid[i++] = (char)buf;
        }
    } while (Serial1.available() && i < CARTRIDGE_UUID_LENGTH);
    Serial.println();

    Serial1.end();
    barcode_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
    barcode_being_scanned = false;

    if (i < CARTRIDGE_UUID_LENGTH) {
        memcpy(barcode_uuid, CARTRIDGE_ERROR_MESSAGE, CARTRIDGE_UUID_LENGTH);
        return false;
    } else {
        Serial.printlnf("Barcode read: %s, length: %d", barcode_uuid, i);
        return true;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        BUZZER                           //
//                                                         //
/////////////////////////////////////////////////////////////

void turn_on_buzzer_for_duration(int duration, int frequency)
{
    if (serial_messaging_on)
        Serial.printlnf("Turning on buzzer for duration %d at frequency %d", duration, frequency);
    tone(pinBuzzer, frequency, duration);
}

void alert_buzzer() {
    run_alert_buzzer = true;
}

void turn_on_alert_buzzer() {
    alert_buzzer_timer.start();
}

void turn_off_alert_buzzer() {
    alert_buzzer_timer.stop();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                       HEATER                            //
//                                                         //
/////////////////////////////////////////////////////////////

void turn_on_heater(int power)
{
    power = power > HEATER_MAX_POWER ? HEATER_MAX_POWER : (power < 0 ? 0 : power);
    analogWrite(heater.heater_pin, power, HEATER_PWM_FREQUENCY);
    heater.power = power;
    heater.heater_on = true;
}

void turn_off_heater()
{
    analogWrite(heater.heater_pin, 0);
    heater.heater_on = false;
    heater.power = 0;
}

int limit(int value, int max, int min)
{
    return value > max ? max : (value < min ? min : value);
}

int set_heater_power(int power)
{
    unsigned long start = millis();
    power = limit(power, HEATER_MAX_POWER, 0);
    analogWrite(heater.heater_pin, power, HEATER_PWM_FREQUENCY);
    heater.power = power;
    heater.heater_on = power != 0;
    if (heater.heater_on)
    {
        if (serial_messaging_on)
            Serial.printlnf("Heater set to power %d", power);
    }

    return (int)(millis() - start);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                         LEDS                            //
//                                                         //
/////////////////////////////////////////////////////////////

void turn_on_LED(char channel, int power)
{
    if (channel == 'A')
    {
        turn_on_assay_LED(power);
    }
    else if (channel == '1')
    {
        turn_on_control_1_LED(power);
    }
    else if (channel == '2')
    {
        turn_on_control_2_LED(power);
    }
}

void turn_off_LED(char channel)
{
    if (channel == 'A')
    {
        turn_off_assay_LED();
    }
    else if (channel == '1')
    {
        turn_off_control_1_LED();
    }
    else if (channel == '2')
    {
        turn_off_control_2_LED();
    }
}

void turn_on_all_LEDs(int power)
{
    turn_on_assay_LED(power);
    turn_on_control_1_LED(power);
    turn_on_control_2_LED(power);
}

void turn_on_assay_LED(int power)
{
    analogWrite(pinLEDAssay, power);
}

void turn_on_control_1_LED(int power)
{
    analogWrite(pinLEDControl1, power);
}

void turn_on_control_2_LED(int power)
{
    analogWrite(pinLEDControl2, power);
}

void turn_off_assay_LED()
{
    analogWrite(pinLEDAssay, 0);
}

void turn_off_control_1_LED()
{
    analogWrite(pinLEDControl1, 0);
}

void turn_off_control_2_LED()
{
    analogWrite(pinLEDControl2, 0);
}

void turn_off_all_LEDs()
{
    turn_off_assay_LED();
    turn_off_control_1_LED();
    turn_off_control_2_LED();
}

void turn_on_assay_LED_for_duration(int duration, int power)
{
    turn_on_assay_LED(power);
    delay(duration);
    turn_off_assay_LED();
}

void turn_on_control_1_LED_for_duration(int duration, int power)
{
    turn_on_control_1_LED(power);
    delay(duration);
    turn_off_control_1_LED();
}

void turn_on_control_2_LED_for_duration(int duration, int power)
{
    turn_on_control_2_LED(power);
    delay(duration);
    turn_off_control_2_LED();
}

void turn_on_all_LEDs_for_duration(int duration, int power)
{
    turn_on_assay_LED(power);
    turn_on_control_1_LED(power);
    turn_on_control_2_LED(power);
    delay(duration);
    turn_off_assay_LED();
    turn_off_control_1_LED();
    turn_off_control_2_LED();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                     MAGNETOMETER                        //
//                                                         //
/////////////////////////////////////////////////////////////

void magnetometer_read_confirm(const char *event, const char *data) {
    Serial.printlnf("Read confirm: event = %s, data = %s", event, data);
    test_magnetometer_awaiting_confirmation = false;
    test_magnetometer_command_flag = true;
}

void read_magnetometer() {
    Serial.printlnf("Magnetometer test, time = %u, position = %d", millis(), stage_position);
    Particle.publish("magnetometer-read", String(stage_position), PRIVATE);
    if (stage_position + test_magnetometer_command_microns_to_move > STAGE_POSITION_LIMIT) {
        delay(5000);
        System.reset();
    } else {
        test_magnetometer_awaiting_confirmation = true;
        test_magnetometer_command_confirmation_timeout = millis() + MAGNETOMETER_TEST_CONFIRM_TIMEOUT;
    }
}


/////////////////////////////////////////////////////////////
//                                                         //
//                    OPTICAL SENSORS                      //
//                                                         //
/////////////////////////////////////////////////////////////

void test_optical_sensors()
{
    read_optical_sensors_command_flag = true;
}

void config_optical_sensors(char channel, int param, int addr)
{
    int bytes_received, bytes_sent, reg, result;

    if (serial_messaging_on)
        Serial.printlnf("Configuring optical sensor %c", channel);
    Wire.beginTransmission(addr);
    bytes_sent = Wire.write(0x00);
    bytes_sent += Wire.write(0x02); // enter configuration mode
    result = Wire.endTransmission(true);
    if (result != 0)
    {
        Serial.printlnf("Config optics %c, %d bytes, result: %d", channel, bytes_sent, result);
    }

    Wire.beginTransmission(addr);
    bytes_sent = Wire.write(0x06);
    bytes_sent += Wire.write((uint8_t)param); // write param to CREG1
    result = Wire.endTransmission(true);
    if (result != 0)
    {
        Serial.printlnf("Config optics %c, %d bytes, result: %d", channel, bytes_sent, result);
    }

    Wire.beginTransmission(addr);
    bytes_sent = Wire.write(0x06); // confirm register was written
    result = Wire.endTransmission(false);
    if (result != 0)
    {
        Serial.printlnf("Send error, %d bytes, result: %d", bytes_sent, result);
    }
    bytes_received = Wire.requestFrom(addr, 1);

    reg = Wire.read();
    if (serial_messaging_on)
        Serial.printlnf("bytes = %d, param = %d, CREG1 = %d", bytes_received, param, reg);
}

bool optical_sensor_ready(uint8_t addr)
{
    int bytes, result;
    uint8_t osr, status;

    Wire.beginTransmission(addr);
    bytes = Wire.write(0x00);
    result = Wire.endTransmission(false);
    if (result != 0)
    {
        Serial.printlnf("Send error, %d bytes, result: %d", bytes, result);
    }
    bytes = Wire.requestFrom(addr, (uint8_t) 4);

    // status
    osr = Wire.read();
    status = Wire.read();
    if (serial_messaging_on)
        Serial.printlnf("Status = %d, OSR = %d", status, osr);

    return ((status & 0x04) == 0 && osr == 3);
}

bool take_one_sample_from_optical_sensor(uint8_t addr, uint16_t *x, uint16_t *y, uint16_t *z, uint16_t *tempC)
{
    uint8_t osr, status, lsb, msb;
    int bytes, result;
    bool ready;
    unsigned long timeout;

    Wire.beginTransmission(addr);
    bytes = Wire.write(0x00);
    bytes += Wire.write(0x83);
    result = Wire.endTransmission(true);
    if (result != 0)
    {
        Serial.printlnf("Config optics: address %d, %d bytes, result: %d", addr, bytes, result);
        return false;
    }

    timeout = millis() + 5000;
    ready = optical_sensor_ready(addr);
    while (!ready && millis() < timeout)
    {
        /*Serial.print('.');*/
        delay(100);
        ready = optical_sensor_ready(addr);
    }

    if (!ready)
    {
        Serial.printlnf("Read failure: address = %d", addr);
        return false;
    }

    if (serial_messaging_on)
        Serial.printlnf("Starting optical sensor data addr = %d read", addr);

    Wire.beginTransmission(addr);
    bytes = Wire.write(0x00);
    result = Wire.endTransmission(false);
    if (result != 0)
    {
        Serial.printlnf("Send error, %d bytes, result: %d", bytes, result);
    }
    bytes = Wire.requestFrom(addr, (uint8_t) 10);

    // status
    osr = Wire.read();
    status = Wire.read();
    if (serial_messaging_on)
        Serial.printlnf("Status = %d, OSR = %d", status, osr);

    // temperature
    lsb = Wire.read();
    if (serial_messaging_on)
        Serial.printf("%d ", lsb);
    msb = Wire.read();
    if (serial_messaging_on)
        Serial.printf("%d ", msb);
    *tempC = ((((msb << 8) + lsb) * 5) / 100) - 67;

    // light
    lsb = Wire.read();
    if (serial_messaging_on)
        Serial.printf("%d ", lsb);
    msb = Wire.read();
    if (serial_messaging_on)
        Serial.printf("%d ", msb);
    *x = (msb << 8) + lsb;

    lsb = Wire.read();
    if (serial_messaging_on)
        Serial.printf("%d ", lsb);
    msb = Wire.read();
    if (serial_messaging_on)
        Serial.printf("%d ", msb);
    *y = (msb << 8) + lsb;

    lsb = Wire.read();
    if (serial_messaging_on)
        Serial.printf("%d ", lsb);
    msb = Wire.read();
    if (serial_messaging_on)
        Serial.printlnf("%d ", msb);
    *z = (msb << 8) + lsb;

    return true;
}

void get_data_from_one_optical_sensor(char channel, int param, int led_power)
{
    uint8_t addr;
    int read_attempt, sum_x, sum_y, sum_z, sum_t;
    uint16_t tempC, tempF, x, y, z, l_value;
    BrevitestOpticalSensorRecord *reading;

    if (channel == 'A') {
        addr = 0x74;
    } else if (channel == '1') {
        addr = 0x75;
    } else if (channel == '2') {
        addr = 0x76;
    } else {
        Serial.printlnf("ERROR: Channel %c not found", channel);
        return;
    }

    reading = &(test.reading[test.number_of_readings % OPTICAL_MAXIMUM_NUMBER_OF_READINGS]);
    test.number_of_readings++;

    sum_x = sum_y = sum_z = sum_t = 0;
    
    reading_optical_sensors = true;
    turn_off_heater();
    turn_on_LED(channel, led_power);
    delay(100);
    
    config_optical_sensors(channel, param, addr);

    reading->channel = channel;
    reading->samples = OPTICAL_SENSOR_NUMBER_OF_SAMPLES;
    for (int i = 0; i < reading->samples; i++) {
        read_attempt = 1;
        while (read_attempt <= 3) {
            if (take_one_sample_from_optical_sensor(addr, &x, &y, &z, &tempC)) {
                sum_x += x;
                sum_y += y;
                sum_z += z;
                sum_t += tempC;
                read_attempt = 4;
            } else {
                Serial.printlnf("Read optical sensor failed, channel %c, try %d", channel, read_attempt);
                read_attempt++;
            }
        }
    }
    reading->x = sum_x / reading->samples;
    reading->y = sum_y / reading->samples;
    reading->z = sum_z / reading->samples;
    reading->temperature = sum_t / reading->samples;

    tempF = ((reading->temperature * 9) / 5) + 32;
    l_value = integerSqrt((reading->x * reading->x) + (reading->y * reading->y) + (reading->z * reading->z));
    Serial.printlnf("S: %c %d %d => T = %d˚C %d˚F, X = %d, Y = %d, Z = %d, L = %d", channel, param, reading->samples, reading->temperature, tempF, reading->x, reading->y, reading->z, l_value);

    reading_optical_sensors = false;
    turn_off_LED(channel);
}

bool enable_optical_sensors(bool force_read)
{
    if (Wire.isEnabled())
    {
        return true;
    }

    /*Serial.println("Attempting to read optical sensors");*/
    Wire.setSpeed(CLOCK_SPEED_100KHZ);
    Wire.begin();
    delay(100);

    return Wire.isEnabled();
}

void disable_optical_sensors()
{
    Wire.end();
}

void read_optical_sensors(int param, int led_power, bool inBCODE)
{
    unsigned long elapsed = millis();

    if (enable_optical_sensors(true))
    {
        get_data_from_one_optical_sensor('A', param, led_power);
        if (inBCODE) BCODE_loop(); else Particle.process();
        get_data_from_one_optical_sensor('1', param, led_power);
        if (inBCODE) BCODE_loop(); else Particle.process();
        get_data_from_one_optical_sensor('2', param, led_power);
        if (inBCODE) BCODE_loop(); else Particle.process();
        Serial.println();
    }
    else
    {
        Serial.println("Unable to start communication with optical sensors");
    }

    disable_optical_sensors();

    if (serial_messaging_on)
        Serial.printlnf("Elapsed time: %u", millis() - elapsed);
}

/////////////////////////////////////////////////////////////
//                                                         //
//               TEMPERATURE CONTROL SYSTEM                //
//                                                         //
/////////////////////////////////////////////////////////////

int get_heater_temperature()
{
    analogWrite(heater.heater_pin, 0);
    int raw = analogRead(heater.thermistor_pin);
    analogWrite(heater.heater_pin, heater.power);
    
    if (raw == 0) {
        stop_temperature_control();
        heater.temp_C_10X = 0;
        heater.temp_F_10X = 0;
    } else {
        heater.temp_C_10X = raw_table_lookup(raw);
        heater.temp_F_10X = ((heater.temp_C_10X * 9) / 5) + 320;
        if (heater.temp_C_10X > HEATER_MAX_TEMPERATURE) {
            stop_temperature_control();
            raw = 0;
        }
    }
    return raw;
}

void heater_temperature_read()
{
    get_heater_temperature();
    if (serial_messaging_on)
        Serial.printlnf("Temperature: %d.%d˚C, %d.%d˚F", heater.temp_C_10X / 10, heater.temp_C_10X % 10, heater.temp_F_10X / 10, heater.temp_F_10X % 10);
}

int pid_controller()
{
    int dt, error, derivative, raw;
    int output = heater.power;
    unsigned long current_read_time, prev_read_time;

    current_read_time = millis();
    prev_read_time = heater.read_time;
    if ((current_read_time - prev_read_time) >= HEATER_CONTROL_INTERVAL)  {
        raw = get_heater_temperature();
        heater.read_time = current_read_time;
        if (raw != 0) {
            if (prev_read_time == 0) {
                heater.previous_error = 0;
                heater.integral = 0;
            } else {
                dt = heater.read_time - prev_read_time;
                error = heater.target_C_10X - heater.temp_C_10X;
                heater.integral += (error * dt) / 1000;
                derivative = (1000 * (error - heater.previous_error)) / dt;
                output = (heater.k_p_num * error) / heater.k_p_den;
                output += (heater.k_i_num * heater.integral) / heater.k_i_den;
                output += (heater.k_d_num * derivative) / heater.k_d_den;

                if (serial_messaging_on) {
                    Serial.printlnf("raw = %d, T = %d.%d˚C, target = %d.%d, dt = %d, error = %d, integral = %d, derivative = %d, output = %d",
                        raw, heater.temp_C_10X / 10, heater.temp_C_10X % 10, heater.target_C_10X / 10, heater.target_C_10X % 10,
                        dt, error, heater.integral, derivative, output);
                }
                heater.previous_error = error;
            }
        }
    }

    return output;
}

void control_heater_temperature() {
    control_heater_temperature_flag = !reading_optical_sensors;
}

void start_temperature_control()
{
    heater.read_time = 0;
    control_heater_temperature_timer.start();
    Serial.println("Temperature control system started");
}

void stop_temperature_control()
{
    control_heater_temperature_timer.stop();
    set_heater_power(0);
    Serial.println("Temperature control system stopped");
}

/////////////////////////////////////////////////////////////
//                                                         //
//                CARTRIDGE VALIDATION                     //
//                                                         //
/////////////////////////////////////////////////////////////

void validate_cartridge()
{
    cartridge_validated = false;
    if (Particle.connected()) {
        Serial.println("Validating cartridge");
        brevitest_publish("validate-cartridge", barcode_uuid);
    } else {
        Serial.println("Not connected to the cloud. Wait and retry.");
    }
}

bool load_assay_record(char *responseString)
{
    memcpy(test.cartridge_uuid, responseString, CARTRIDGE_UUID_LENGTH);
    test.cartridge_uuid[CARTRIDGE_UUID_LENGTH] = '\0';

    memcpy(assay.uuid, responseString, ASSAY_UUID_LENGTH);
    assay.uuid[ASSAY_UUID_LENGTH] = '\0';

    test.number_of_readings = 0;

    int indx = 25;
    int crc_loaded, crc_calculated;

    assay.BCODE_version = extract_int_from_delimited_string(responseString, &indx, ITEM_DELIM);
    crc_loaded = extract_int_from_delimited_string(responseString, &indx, ITEM_DELIM);
    assay.duration = extract_int_from_delimited_string(responseString, &indx, ITEM_DELIM);
    assay.BCODE_length = extract_int_from_delimited_string(responseString, &indx, ITEM_DELIM);

    strncpy(assay.BCODE, &responseString[indx], assay.BCODE_length);
    assay.BCODE[assay.BCODE_length] = '\0';

    crc_calculated = abs(checksum(assay.BCODE, assay.BCODE_length));

    return (crc_loaded == crc_calculated); // bcode loaded if checksums match
}

/////////////////////////////////////////////////////////////
//                                                         //
//                      STRESS TEST                        //
//                                                         //
/////////////////////////////////////////////////////////////

void do_stress_test_step(int step) {
    Serial.print('.');
    switch(step % 16) {
        case 0: // restart stress test
            reset_stage(false);
            break;
        case 1: // move to start of well 2
            move_stage(-2000, SLOW_STEP_DELAY);
            break;
        case 2: // oscillate in well 2
            oscillate_stage(3000, OSCILLATION_STEP_DELAY, 500, false);
            break;
        case 3: // move to well 1
            move_stage(-8600, SLOW_STEP_DELAY);
            break;
        case 4: // oscillate in well 1
            oscillate_stage(4500, OSCILLATION_STEP_DELAY, 600, false);
            break;
        case 5: // move to well 2
            move_stage(12300, SLOW_STEP_DELAY);
            break;
        case 6: // oscillate in well 2
            oscillate_stage(-4500, OSCILLATION_STEP_DELAY, 400, false);
            break;
        case 7: // move to well 3
            move_stage(8000, SLOW_STEP_DELAY);
            break;
        case 8: // oscillate in well 3
            oscillate_stage(-4500, OSCILLATION_STEP_DELAY, 300, false);
            break;
        case 9: // read baseline sensors
            move_stage_to_optical_read_position();
            read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, LED_DEFAULT_POWER, false);
            read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, LED_DEFAULT_POWER, false);
            break;
        case 10: // move to well 4
            move_stage(9425, SLOW_STEP_DELAY);
            break;
        case 11: // oscillate in well 4
            oscillate_stage(-4500, OSCILLATION_STEP_DELAY, 400, false);
            break;
        case 12: // move to well 5
            move_stage(8400, SLOW_STEP_DELAY);
            break;
        case 13: // oscillate in well 5
            oscillate_stage(-4500, OSCILLATION_STEP_DELAY, 600, false);
            break;
        case 14: // read sensors
            move_stage_to_optical_read_position();
            read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, LED_DEFAULT_POWER, false);
            read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, LED_DEFAULT_POWER, false);
            read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, LED_DEFAULT_POWER, false);
            read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, LED_DEFAULT_POWER, false);
            break;
        case 15: // save cycle number
            eeprom.stress_test_cycles++;
            Serial.printlnf("Stress test cycle %d complete, record is %d", eeprom.stress_test_cycles, eeprom.maximum_stress_test_cycles);
            if (eeprom.stress_test_cycles > eeprom.maximum_stress_test_cycles) {
                eeprom.maximum_stress_test_cycles = eeprom.stress_test_cycles;
            }
            store_eeprom();
            break;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                 PUBLISH AND CALLBACKS                   //
//                                                         //
/////////////////////////////////////////////////////////////

void set_current_event(String event_name) {
    strcpy(current_event, event_name.c_str());
    if (strcmp(current_event, "register-device") == 0) {
        current_event_code = PUBSUB_REGISTER_DEVICE;
    } else if (strcmp(current_event, "validate-cartridge") == 0) {
        current_event_code = PUBSUB_VALIDATE_CARTRIDGE;
    } else if (strcmp(current_event, "upload-test") == 0) {
        current_event_code = PUBSUB_TEST_UPLOAD;
    } else {
        current_event_code = 0;
    }
}

void clear_current_event() {
    current_event[0] = '\0';
    current_event_code = 0;
}

bool callback_pending() {
    if (callback_timeout < millis()) {
        clear_current_event();
    }
    return current_event_code;
}

void set_publish_params(String event_name) {
    set_current_event(event_name);
    callback_complete = false;
    callback_buffer[0] = '\0';
    callback_buffer[PUBSUB_CALLBACK_BUFFER_SIZE] = '\0';
    callback_timeout = millis() + PUBSUB_CALLBACK_TIMEOUT;
}

void brevitest_publish(String event_name, char *uuid)
{
    if (!callback_pending()) {
        set_publish_params(event_name);
        Particle.publish(String(PUBSUB_EVENT_NAME), event_name + String(ITEM_DELIM) + String(uuid), PRIVATE, NO_ACK);
        Serial.printlnf("PUBLISH: event = %s, uuid = %s", event_name.c_str(), uuid);
    } else {
        Serial.printlnf("Cannot publish event %s while event %s still outstanding - will retry later", event_name.c_str(), current_event);
    }
}

void startup_analyzer(void);
void callback_register() {
    Serial.printlnf("Device registration callback");
    if ((strncmp(callback_status, SUCCESS, 7) == 0))
    { // device registered
        Serial.printlnf("Device registered - starting up analyzer");
        device_registered = true;
        clear_current_event();
        startup_analyzer();
    } else {
        Serial.printlnf("Device registration failed. Resetting device in 60 seconds.");
        ledProblem.setActive(true);
        delay(60000);
        System.reset();
    }
}

void callback_validate() {
    cartridge_validated = (strncmp(callback_status, SUCCESS, 7) == 0);
    Serial.printlnf("Cartridge validated? %c", cartridge_validated ? 'Y' : 'N');
    if (cartridge_validated) { // valid cartridge found
        if (load_assay_record(callback_data)) {
            ready_to_start_test = true;
            Serial.printlnf("Assay information loaded. Test starting.");
        } else {
            Serial.println("Failed to load assay record. Will retry later.");
            cartridge_validated = false;
        }
    } else {
        Serial.printlnf("Invalid cartridge: %s", callback_data);
        cartridge_present = false;
        turn_on_alert_buzzer();
    }
    clear_current_event();
}

void remove_test_from_cache(char *testToRemove)
{
    for (int i = 0; i < CACHE_SIZE; i += 1) {
        if (strncmp(testToRemove, eeprom.cache[i].cartridge_uuid, CARTRIDGE_UUID_LENGTH) == 0) {
            memset(eeprom.cache[i].cartridge_uuid, '\0', sizeof(BrevitestTestRecord));
            store_eeprom();
            return;
        }
    }
}

void callback_test_upload() {
    bool success = (strncmp(callback_status, SUCCESS, 7) == 0);
    if (success) {
        Serial.printlnf("Test successfully uploaded; removing test %s from cache", callback_data);
        upload_test_pending = false;
        remove_test_from_cache(callback_data);
        clear_current_event();
    } else {
        Serial.println("Test upload failed. Will retry later.");
    }
}

int extract_callback_params(char *param, int paramLen, int indx, char delim)
{
    int i;
    bool stop = false;

    param[paramLen] = '\0';
    for (i = 0; i < paramLen ; i++, indx++) {
        stop = callback_buffer[indx] == delim;
        param[i] = stop ? '\0' : callback_buffer[indx];
        if (stop) break;
    }

    return indx + 1;
}

void process_callback_buffer()
{
    callback_complete = false;

    int indx = 0;
    indx = extract_callback_params(callback_event, PUBSUB_EVENT_MAX_LENGTH, indx, ITEM_DELIM);
    indx = extract_callback_params(callback_status, PUBSUB_STATUS_MAX_LENGTH, indx, ITEM_DELIM);
    callback_data = callback_buffer + indx;
    for (int i = indx; i < PUBSUB_CALLBACK_BUFFER_SIZE; i++) {
        if (callback_buffer[i] == END_DELIM) {
            callback_buffer[i] = '\0';
            break;
        }
    }

    // Serial.printlnf("Callback length: %d, event: %s, status: %s, data: %s", strlen(callback_buffer), callback_event, callback_status, callback_data);
    if (strcmp(callback_event, current_event) != 0) {
        Serial.printlnf("Wrong event callback: expecting event %s, received event %s", current_event, callback_event);
    } else {
        switch (current_event_code) {
            case PUBSUB_REGISTER_DEVICE:
                callback_register();
                break;
            case PUBSUB_VALIDATE_CARTRIDGE:
                callback_validate();
                break;
            case PUBSUB_TEST_UPLOAD:
                callback_test_upload();
                break;
            default:
                Serial.printlnf("Unknown event code %d", current_event_code);
                clear_current_event();
        }
    }
}

void brevitest_error(const char *event, const char *data)
{
    strcat(callback_buffer, data);
    int last = strlen(data) - 1;
    callback_complete = (data[last] == END_DELIM);
    Serial.printlnf("ERROR | callback_buffer: %s, callback_complete: %s", callback_buffer, callback_complete ? 'Y' : 'N');
}

void brevitest_callback(const char *event, const char *data)
{
    strcat(callback_buffer, data);
    int last = strlen(data) - 1;
    callback_complete = (data[last] == END_DELIM);
    if (callback_complete) {
        Serial.printlnf("RESPONSE | callback_buffer: %s, callback_complete: %c", callback_buffer, callback_complete ? 'Y' : 'N');
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//               TEST CACHE AND UPLOADING                  //
//                                                         //
/////////////////////////////////////////////////////////////

void erase_test_from_cache(int index)
{
    memset(eeprom.cache[index].cartridge_uuid, '\0', sizeof(BrevitestTestRecord));
}

void initialize_test_cache()
{    for (int i = 0; i < CACHE_SIZE; i += 1) {
        erase_test_from_cache(i);
    }

    eeprom.most_recent_test = 255;
    store_eeprom();
}

int find_test_index_by_uuid(char *uuid)
{
    if (uuid[0] == '\0') {
        return -1;
    }
    for (int i = 0; i < CACHE_SIZE; i += 1) {
        if (strncmp(uuid, eeprom.cache[i].cartridge_uuid, CARTRIDGE_UUID_LENGTH) == 0) {
            return i;
        }
    }
    return -1;
}

void store_test(int index)
{
    memcpy(eeprom.cache[index].cartridge_uuid, test.cartridge_uuid, sizeof(BrevitestTestRecord));
    eeprom.most_recent_test = index;
    store_eeprom();
}

int append_test_reading(int start, BrevitestOpticalSensorRecord *reading)
{
    return sprintf(&(particle_register[start]), "%c%c%X%c%X%c%X%c%X%c",
                   reading->channel, ARG_DELIM,
                   reading->x, ARG_DELIM,
                   reading->y, ARG_DELIM,
                   reading->z, ARG_DELIM,
                   reading->temperature, ATTR_DELIM);
}

void process_test_record(int index)
{
    BrevitestTestRecord *t;
    int len;
    char c;

    t = &eeprom.cache[index];

    len = sprintf(particle_register, "%.24s%c%c%c",t->cartridge_uuid, ITEM_DELIM, TEST_DATA_FORMAT_CODE, ITEM_DELIM);

    if (t->number_of_readings) { // test completed
        for (int i = 0; i < OPTICAL_MAXIMUM_NUMBER_OF_READINGS; i++)
        {
            c = t->reading[i].channel;
            if (c == 'A' || c == '1' || c == '2') {
                len += append_test_reading(len, &(t->reading[i]));
            }
        }
        particle_register[len - 1] = '\0';
    } else { // test cancelled
        particle_register[len] = '0';
        particle_register[len + 1] = '\0';
    }
}

void write_test_record_to_eeprom()
{
    // increment test_index (check for overflow and if so reset circular buffer)
    if (test_cancelled) {
        test.number_of_readings = 0;
        test_cancelled = false;
    }
    int test_index = (eeprom.most_recent_test == 255 ? 0 : eeprom.most_recent_test + 1) % CACHE_SIZE; // 255 is the reset value
    store_test(test_index);
    process_test_record(test_index);
}

bool tests_to_upload()
{
    for (int i = 0; i < CACHE_SIZE; i += 1) {
        if (eeprom.cache[i].cartridge_uuid[0] != '\0') {
            if (serial_messaging_on) {
                Serial.printlnf("Test found for cartridge %s", eeprom.cache[i].cartridge_uuid);
            }
            return true;
        }
    }
    return false;
}

void upload_one_test(int test_number, char *cartridge_uuid)
{
    process_test_record(test_number);
    Serial.printlnf("Payload length: %d, payload: %s", strlen(particle_register), particle_register);
    brevitest_publish("upload-test", particle_register);
}

void upload_tests() {
    Serial.println("Looking for tests to upload...");
    for (int i = 0; i < CACHE_SIZE; i += 1) {
        if (eeprom.cache[i].cartridge_uuid[0] != '\0') {
            upload_one_test(i, eeprom.cache[i].cartridge_uuid);
            return;
        }
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                          BCODE                          //
//                                                         //
/////////////////////////////////////////////////////////////

int get_BCODE_token(int index, int *token)
{
    int i;
    char *bcode = assay.BCODE;

    if (test_cancelled) return index;

    // end of string so return end of string location
    if (bcode[index] == ITEM_DELIM) return index;

     // command has no arguments so skip delim
    if (bcode[index] == ATTR_DELIM) return index + 1;

    // there are arguments to extract
    i = index;
    while (i < BCODE_CAPACITY) {
        if (bcode[i] == ATTR_DELIM) {
            *token = extract_int_from_string(bcode, index, (i - index));
            return i; // return end of string location
        }
        if (bcode[i] == ARG_DELIM) {
            *token = extract_int_from_string(bcode, index, (i - index));
            i++; // skip past parameter
            return i;
        }
        i++;
    }

    return i;
}

void update_progress(String message, int duration)
{
    int new_percent_complete;
    int test_duration = assay.duration * 1000;

    if (duration == 0) {
        test_progress = 0;
        test_percent_complete = 0;
    }
    else if (duration < 0) {
        test_progress = test_duration;
        test_percent_complete = -1;
    } else {
        test_progress += duration;
        new_percent_complete = 100 * test_progress / test_duration;
        new_percent_complete = new_percent_complete > 100 ? 100 : new_percent_complete;
        if (new_percent_complete != test_percent_complete) {
            test_percent_complete = new_percent_complete;
        }
    }
    Serial.printlnf("%s, %d percent complete, temp = %d.%d", message.c_str(), test_percent_complete, heater.temp_C_10X / 10, heater.temp_C_10X % 10);
}

int BCODE_loop()
{
    unsigned long total_duration = millis();

    if (!reading_optical_sensors) set_heater_power(pid_controller());
    test_cancelled = digitalRead(pinCartridgeLoaded) == HIGH;

    return (int) (millis() - total_duration);
}

void BCODE_delay(int target_duration)
{
    int cycles = target_duration / BCODE_MAX_DELAY;
    int residual = target_duration % BCODE_MAX_DELAY;
    int loop_time = 0;

    for (int i = 0; i < cycles; i++) {
        loop_time = BCODE_loop();
        delay(BCODE_MAX_DELAY - loop_time);
        if (test_cancelled) return;
    }
    loop_time = BCODE_loop();
    if (residual > loop_time) {
      delay(residual - loop_time);
    }
}

int process_BCODE(int);
int process_one_BCODE_command(int cmd, int index)
{
    int param1, param2, param3, start_index;

    if (test_cancelled) return index;

    switch (cmd) {
        case 0: // Start test()
            update_progress("Starting", 6000);
            BCODE_delay(1000);
            break;
        case 1: // Delay(milliseconds)
            index = get_BCODE_token(index, &param1);
            update_progress("Pausing", param1);
            BCODE_delay(param1);
            break;
        case 2: // Move Microns(microns, microseconds)
            index = get_BCODE_token(index, &param1); // microns to move
            index = get_BCODE_token(index, &param2); // step_delay_us
            update_progress("Moving", abs(param1) * param2 / MOVE_DURATION_UNIT);
            move_stage(param1, param2);
            BCODE_loop();
            break;
        case 3: // Oscillate Stage(microns, microseconds, cycles)
            index = get_BCODE_token(index, &param1); // microns to move
            index = get_BCODE_token(index, &param2); // step_delay_us
            index = get_BCODE_token(index, &param3); // number of cycles
            update_progress("Oscillating", abs(param1) * param2 * param3 / MOVE_DURATION_UNIT);
            oscillate_stage(param1, param2, param3, true);
            BCODE_loop();
            break;
        case 4: // Buzz(milliseconds, frequency)
            index = get_BCODE_token(index, &param1); // duration_ms
            index = get_BCODE_token(index, &param2); // frequency
            update_progress("Buzzing", param1);
            turn_on_buzzer_for_duration(param1, param2);
            BCODE_loop();
            break;
        case 10: // Read optical sensors with default param and LED power
            // update_progress("Preparing", abs(stage_position - OPTICAL_SENSOR_READ_POSITION) * FAST_STEP_DELAY / MOVE_DURATION_UNIT);
            Serial.printlnf("Moving stage to prepare for reading");
            move_stage_to_optical_read_position();
            update_progress("Reading", 6000);
            read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, LED_DEFAULT_POWER, false);
            break;
        case 11: // Read optical sensors with param1 = sensor parameters and param2 = LED power
            index = get_BCODE_token(index, &param1); // params
            index = get_BCODE_token(index, &param2); // LED power
            move_stage_to_optical_read_position();
            update_progress("Reading", 6000);
            read_optical_sensors(param1, param2, false);
            break;
        case 20: // Repeat begin(number of iterations)
            index = get_BCODE_token(index, &param1);

            Serial.printlnf("Begin repeating %d times", param1);
            start_index = index + 1;
            for (int i = 0; i < param1; i += 1) {
                if (test_cancelled) break;
                index = process_BCODE(start_index);
            }
            break;
        case 21: // Repeat end
            Serial.println("End repeating");
            return -index;
            break;
        case 99: // Finish test
            Serial.println("Finish test");
            update_progress("Finishing test", 6000);
            break;
        default:
            BCODE_loop();
    }

    return index + 1;
}

int process_BCODE(int start_index)
{
    int cmd, index;

    index = get_BCODE_token(start_index, &cmd);
    if ((start_index == 0) && (cmd != 0)) { // first command
        test_cancelled = true;
        return -1;
    } else {
        index = process_one_BCODE_command(cmd, index);
    }

    while ((cmd != 99) && (index > 0) && !test_cancelled) {
        index = get_BCODE_token(index, &cmd);
        index = process_one_BCODE_command(cmd, index);
    };

    return (index > 0 ? index : -index);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                         COMMAND                         //
//                                                         //
/////////////////////////////////////////////////////////////

int get_next_command_param(String arg, int indx, int *param, int def)
{
    int next;

    if (indx == -1) {
        *param = def;
        next = -1;
    } else {
        next = arg.indexOf(ARG_DELIM, indx);
        if (next == -1) {
            *param = arg.substring(indx).toInt();
        } else {
            *param = arg.substring(indx, next).toInt();
            next++;
        }
    }

    return next;
}

void i2c_bus_scan()
{
    int addr, result;

    if (enable_optical_sensors(true)) {
        Serial.println("Starting optical I2C bus scan");
        for (addr = 0; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            Wire.write(0x00);
            result = Wire.endTransmission();
            if (result == 0) {
                Serial.printlnf("I2C device found at address %X", addr);
            }
        }
    }
    disable_optical_sensors();
}

int particle_command(String arg)
{
    int cmd, result;
    int indx = 0;
    int param1 = 0;
    int param2 = 0;
    int param3 = 0;
    int param4 = 0;
    int param5 = 0;

    indx = get_next_command_param(arg, indx, &cmd, 0);
    switch (cmd) {
        case 1: // unused
            initialize_test_cache();
            result = 0;
            break;
        case 2: // reset stage
            reset_stage(true);
            result = stage_position;
            break;
        case 3: // move microns, param1 microns with param2 step
            indx = get_next_command_param(arg, indx, &param1, 0);
            indx = get_next_command_param(arg, indx, &param2, SLOW_STEP_DELAY);
            wake_move_sleep_stage(param1, param2);
            Serial.printlnf("Move stage %d steps, cumulative %d, error = %d", param1, stage_position, microns_error);
            result = stage_position;
            break;
        case 4: // read optical sensors param1 times after moving to read position
            reset_stage(false);
            move_stage_to_optical_read_position();
            test.number_of_readings = 0;
            indx = get_next_command_param(arg, indx, &param1, 1);
            read_optical_sensors_command_param = OPTICAL_SENSOR_DEFAULT_PARAM;
            read_optical_sensors_command_led_power = LED_DEFAULT_POWER;
            read_optical_sensors_command_flag = true;
            read_optical_sensors_command_count = param1;
            read_optical_sensor_command_microns_to_move = 0;
            result = param1;
            break;
        case 5: // read optical sensors param1 times without waking motor
            test.number_of_readings = 0;
            indx = get_next_command_param(arg, indx, &param1, 1);
            read_optical_sensors_command_param = OPTICAL_SENSOR_DEFAULT_PARAM;
            read_optical_sensors_command_led_power = LED_DEFAULT_POWER;
            read_optical_sensors_command_flag = true;
            read_optical_sensors_command_count = param1;
            read_optical_sensor_command_microns_to_move = 0;
            result = param1;
            break;
        case 6: // erase EEPROM and restart
            erase_eeprom();
            System.reset();
            result = 1;
            break;
        case 7: // read heater temperature
            indx = get_next_command_param(arg, indx, &param1, HEATER_DEFAULT_TEMP_TARGET);
            Serial.printlnf("Heater: T = %d.%d˚C", heater.temp_C_10X / 10, heater.temp_C_10X % 10);
            result = param1;
            break;
        case 8: // reset EEPROM
            reset_eeprom();
            result = (int)eeprom.data_format_version;
            break;
        case 9: // turn on assay LED for param1 milliseconds at power param2
            indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
            indx = get_next_command_param(arg, indx, &param2, LED_DEFAULT_POWER);
            turn_on_assay_LED_for_duration(param1, param2);
            result = param1;
            break;
        case 10: // turn on control 1 LED for param1 milliseconds at power param2
            indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
            indx = get_next_command_param(arg, indx, &param2, LED_DEFAULT_POWER);
            turn_on_control_1_LED_for_duration(param1, param2);
            result = param2;
            break;
        case 11: // turn on control 2 LED for param1 milliseconds at power param2
            indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
            indx = get_next_command_param(arg, indx, &param2, LED_DEFAULT_POWER);
            turn_on_control_2_LED_for_duration(param1, param2);
            result = param2;
            break;
        case 12: // turn on all LEDs for param1 milliseconds at power param2
            indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
            indx = get_next_command_param(arg, indx, &param2, LED_DEFAULT_POWER);
            turn_on_all_LEDs_for_duration(param1, param2);
            result = param2;
            break;
        case 13: // turn on buzzer param1 duration param2 frequency
            indx = get_next_command_param(arg, indx, &param1, BUZZER_FREQUENCY);
            indx = get_next_command_param(arg, indx, &param2, BUZZER_DURATION);
            turn_on_buzzer_for_duration(param1, param2);
            result = param1;
            break;
        case 14: // move to specified location param1 at step delay param2
            indx = get_next_command_param(arg, indx, &param1, 0);
            indx = get_next_command_param(arg, indx, &param2, SLOW_STEP_DELAY);
            reset_stage(false);
            move_stage_to_position(param1, param2);
            sleep_motor();
            result = stage_position;
            break;
        case 15: // move stage to test start position
            reset_stage(false);
            move_stage_to_test_start_position();
            sleep_motor();
            result = stage_position;
            break;
        case 16: // set heater target temperature
            indx = get_next_command_param(arg, indx, &param1, HEATER_DEFAULT_TEMP_TARGET);
            if (param1 > 0 && param1 < HEATER_MAX_TEMPERATURE)
            {
                heater.target_C_10X = param1;
                heater.read_time = 0;
            }
            result = param1;
            break;
        case 17: // move stage to optical read position
            reset_stage(false);
            move_stage_to_optical_read_position();
            sleep_motor();
            result = stage_position;
            break;
        case 18: // turn on heater at power param1
            indx = get_next_command_param(arg, indx, &param1, HEATER_DEFAULT_POWER);
            turn_on_heater(param1);
            result = param1;
            break;
        case 19: // turn off heater
            turn_off_heater();
            result = 1;
            break;
        case 20: // start temperature control
            start_temperature_control();
            result = 1;
            break;
        case 21: // stop temperature control
            stop_temperature_control();
            result = 1;
            break;
        case 22: // turn on serial messaging
            serial_messaging_on = true;
            result = 1;
            break;
        case 23: // turn off serial messaging
            serial_messaging_on = false;
            result = 0;
            break;
        case 24: // not used
            result = 0;
            break;
        case 25: // return stage position
            result = stage_position;
            break;
        case 26: // magnetometer test param1 = step distance
            reset_stage(false);
            move_stage(-MICRONS_TO_INITIAL_POSITION, SLOW_STEP_DELAY);
            indx = get_next_command_param(arg, indx, &param1, MAGNETOMETER_TEST_DEFAULT_STEP_DISTANCE);
            test_magnetometer_command_flag = true;
            test_magnetometer_command_microns_to_move = param1;
            Particle.subscribe("magnetometer-confirm", magnetometer_read_confirm, MY_DEVICES);
            read_magnetometer();
            result = 1;
            break;
        case 27: // scan i2c bus
            i2c_bus_scan();
            result = 1;
            break;
        case 28: // start optical sensor sweep test, param1 = distance, param2 = steps
            reset_stage(false);
            move_stage_to_optical_read_position();
            indx = get_next_command_param(arg, indx, &param1, 2000);
            indx = get_next_command_param(arg, indx, &param2, 5);
            serial_messaging_on = false;
            test.number_of_readings = 0;
            read_optical_sensors_command_param = OPTICAL_SENSOR_DEFAULT_PARAM;
            read_optical_sensors_command_led_power = LED_DEFAULT_POWER;
            read_optical_sensors_command_flag = true;
            read_optical_sensors_command_count = param2 + 1; // include baseline reading
            read_optical_sensor_command_microns_to_move = param2 ? (param1 / param2) : 0;
            test_optical_sensors_timer.start();
            result = 1;
            break;
        case 29: // stop optical sensor reading test
            test_optical_sensors_timer.stop();
            result = 1;
            break;
        case 30: // scan barcode
            result = scan_barcode() ? 1 : 0;
            break;
        case 31: // stop reading magnetometer
            System.reset();
            break;
        case 32: // not used
            result = 0;
            break;
        case 33: // oscillate - param1 microns, param2 step_delay, param3 number of cycles
            indx = get_next_command_param(arg, indx, &param1, 25);
            indx = get_next_command_param(arg, indx, &param2, OSCILLATION_STEP_DELAY);
            indx = get_next_command_param(arg, indx, &param3, 10);
            wake_motor();
            oscillate_stage(param1, param2, param3, false);
            sleep_motor();
            result = stage_position;
            break;
        case 34: // start stress test
            eeprom.stress_test_cycles = 0;
            store_eeprom();
            stress_test_step = 0;
            stress_test_running = true;
            result = 1;
            break;
        case 35: // stop stress test
            stress_test_running = false;
            result = eeprom.stress_test_cycles;
            break;
        default:
            result = 0;
    }

    Serial.printlnf("Completed command: %d, result: %d, p1: %d, p2: %d, p3: %d, p4: %d, p5: %d", cmd, result, param1, param2, param3, param4, param5);

    return result;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                    DEVICE STATE                         //
//                                                         //
/////////////////////////////////////////////////////////////

void cartridge_state_changed_interrupt()
{
    cartridge_state_changed = true;
}

void check_device_state(bool startup)
{
    if (!startup) {
        cartridge_present = digitalRead(pinCartridgeLoaded) == LOW;
    }

    if (cartridge_present) {
        ledBusy.setActive(true);
        if (heater.temp_C_10X > HEATER_READY_TEMP) {
            turn_off_alert_buzzer();
            turn_on_buzzer_for_duration(400, 400);
            ready_to_scan_barcode = true;
        } else {
            cartridge_present = false;
            turn_on_alert_buzzer();
        }
    } else {
        turn_off_alert_buzzer();
        turn_on_buzzer_for_duration(400, 300);
    }

    Serial.printlnf("Cartridge present? %c", cartridge_present ? 'Y' : 'N');
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           TESTS                         //
//                                                         //
/////////////////////////////////////////////////////////////

void reset_globals()
{
    test_in_progress = false;
    cartridge_validated = false;
    callback_complete = false;
    reading_optical_sensors = false;

    test_progress = 0;
    test_percent_complete = 0;

    barcode_uuid[0] = '\0';
    barcode_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
    test.cartridge_uuid[0] = '\0';
    test.cartridge_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
    assay.uuid[0] = '\0';
    assay.uuid[ASSAY_UUID_LENGTH] = '\0';
    test.number_of_readings = 0;

    particle_register[0] = '\0';
    particle_register[PARTICLE_REGISTER_SIZE] = '\0';

    next_upload = 0;
}

void run_test()
{
    int tries_remaining;

    reset_stage(false);
    turn_on_buzzer_for_duration(1000, 600);
    delay(2000);

    Serial.print("Disconnecting from cloud...");
    Particle.disconnect();
    delay(PARTICLE_CLOUD_DELAY);
    tries_remaining = 10;
    while (Particle.connected()) {
        if (--tries_remaining == 0) {
           return;
        }
        Serial.print(".");
        Particle.disconnect();
        delay(PARTICLE_CLOUD_DELAY);
    }
    Serial.println();
    Serial.println("Now disconnected from cloud");

    ready_to_start_test = false;
    test_in_progress = true;
    update_progress("Running test", 0);

    stop_temperature_control();

    SINGLE_THREADED_BLOCK()
    {
        process_BCODE(0);
        write_test_record_to_eeprom();
    }

    start_temperature_control();

    Serial.print("Reconnecting to cloud...");
    Particle.connect();
    delay(1000);
    tries_remaining = 10;
    while (!Particle.connected()) {
        Serial.print(".");
        Particle.connect();
        delay(PARTICLE_CLOUD_DELAY);
        if (--tries_remaining == 0) {
            System.reset();
        }
    }
    Serial.println();
    Serial.println("Now connected to cloud");

    reset_stage(true);
    reset_globals();
    cartridge_present = false;
    ledAvailable.setActive(true);
    turn_on_alert_buzzer();
}

int particle_run_test(String arg) {
    if (arg.length() == CARTRIDGE_UUID_LENGTH) {
        arg.toCharArray(barcode_uuid, CARTRIDGE_UUID_LENGTH + 1);
        barcode_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
        Serial.printlnf("Running test for cartridge %s", barcode_uuid);
        validate_cartridge();
        return 1;
    } else {
        return -1;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                          SETUP                          //
//                                                         //
/////////////////////////////////////////////////////////////

// void watchdog()
// {
//     Serial.println("Watchdog!");
// }

void init_analog_pin(uint16_t pin, PinMode mode, uint8_t value)
{
    pinMode(pin, mode);
    if (mode == OUTPUT) {
        analogWrite(pin, value);
    }
}

void init_digital_pin(uint16_t pin, PinMode mode, uint8_t value)
{
    pinMode(pin, mode);
    if (mode == OUTPUT) {
        digitalWrite(pin, value);
    }
}

void startup_analyzer()
{
    Serial.println("Registration complete. Starting up...");
    i2c_bus_scan();

    Serial.println("Resetting stage");
    reset_stage(true);

    Serial.println("Turning on LEDs");
    turn_on_assay_LED_for_duration(500, LED_DEFAULT_POWER);
    delay(500);
    turn_on_control_1_LED_for_duration(500, LED_DEFAULT_POWER);
    delay(500);
    turn_on_control_2_LED_for_duration(500, LED_DEFAULT_POWER);

    Serial.println("Buzzing");
    turn_on_buzzer_for_duration(250, 330);

    reset_globals();
    clear_current_event();

    check_device_state(true);
    attachInterrupt(pinCartridgeLoaded, cartridge_state_changed_interrupt, CHANGE);

    Serial.printlnf("device id: %s", device_id.c_str());
    Serial.printlnf("eeprom.firmware_version: %d", eeprom.firmware_version);
    Serial.printlnf("eeprom.data_format_version: %d", eeprom.data_format_version);
    Serial.printlnf("eeprom.most_recent_test: %d", eeprom.most_recent_test);
    Serial.printlnf("eeprom.maximum_stress_test_cycles: %d", eeprom.maximum_stress_test_cycles);

    start_temperature_control();
    periodic_event = millis() + 5000;
}

void setup() {
    ledBusy.setActive(true);

    // Particle.variable("register", particle_register, STRING);
    Particle.function("run_test", particle_run_test);

    device_id = System.deviceID();
    Particle.subscribe(String(device_id + "/hook-response/" + PUBSUB_EVENT_NAME + "/"), brevitest_callback, MY_DEVICES);
    Particle.subscribe(String(device_id + "/hook-error/" + PUBSUB_EVENT_NAME + "/"), brevitest_error, MY_DEVICES);

    init_digital_pin(pinStageLimit, INPUT_PULLUP, 0);
    init_digital_pin(pinCartridgeLoaded, INPUT_PULLUP, 0);

    init_digital_pin(pinBarcodeTrigger, OUTPUT, HIGH);
    init_digital_pin(pinBarcodeReady, INPUT, 0);

    init_analog_pin(pinLEDAssay, OUTPUT, 0);
    init_analog_pin(pinLEDControl1, OUTPUT, 0);
    init_analog_pin(pinLEDControl2, OUTPUT, 0);

    init_analog_pin(pinHeaterThermistor, INPUT, 0);
    init_analog_pin(pinIRThermistor, INPUT, 0);
    init_analog_pin(pinIRThermopile, INPUT, 0);

    init_digital_pin(pinMotorSleep, OUTPUT, LOW);
    init_digital_pin(pinMotorStep, OUTPUT, LOW);
    init_digital_pin(pinMotorDir, OUTPUT, LOW);
    init_analog_pin(pinMotorPFD, OUTPUT, 128);

    init_analog_pin(pinBuzzer, OUTPUT, 0);
    init_analog_pin(pinHeater, OUTPUT, 0);

    setup_eeprom();

    Serial.begin(115200); // standard serial port
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void registration_loop()
{
    if (!device_registered && next_registration < millis()) {
        if (Particle.connected()) {
            brevitest_publish("register-device", (char *)device_id.c_str());
            next_registration = millis() + PUBSUB_CALLBACK_TIMEOUT + RETRY_REGISTRATION;
        } else {
            Particle.connect();
            delay(PARTICLE_CLOUD_DELAY);
        }
    } 
}

void validate_cartridge_loop() {
    if (device_registered && cartridge_present) {
        if (ready_to_scan_barcode) {
            ready_to_scan_barcode = false;
            if (scan_barcode()) {
                validate_cartridge();
                next_validation = millis() + PUBSUB_CALLBACK_TIMEOUT + RETRY_VALIDATION;
            } else {
                cartridge_validated = false;
                cartridge_present = false;
                turn_on_alert_buzzer();
            }
        } else if (!cartridge_validated && next_validation < millis()) {
            Serial.println("Cartridge validation timed out. Retrying.");
            ready_to_scan_barcode = true;
        }
    }
}

void test_upload_loop() {
    if (device_registered && tests_to_upload() && next_upload < millis()) {
        upload_tests();
        next_upload = millis() + PUBSUB_CALLBACK_TIMEOUT + RETRY_UPLOAD;
    }
}

void long_duration_command_loop() {
    if (stress_test_running) {
        do_stress_test_step(stress_test_step);
        stress_test_step++;
    } else if (read_optical_sensors_command_flag) {
        read_optical_sensors_command_flag = false;
        stop_temperature_control();
        while (read_optical_sensors_command_count > 0) {
            Serial.printlnf("Stage location: %d", stage_position);
            read_optical_sensors(read_optical_sensors_command_param, read_optical_sensors_command_led_power, false);
            if (read_optical_sensor_command_microns_to_move) {
                move_stage(read_optical_sensor_command_microns_to_move, SLOW_STEP_DELAY);
            }
            read_optical_sensors_command_count--;
        }
        sleep_motor();
        test_optical_sensors_timer.stop();
        start_temperature_control();
    } else if (test_magnetometer_command_flag) {
        if (test_magnetometer_command_confirmation_timeout < millis()) {
            System.reset();
        } else if (!test_magnetometer_awaiting_confirmation) {
            test_magnetometer_command_flag = false;
            move_stage(test_magnetometer_command_microns_to_move, SLOW_STEP_DELAY);
            read_magnetometer();
        }
    }
}

void state_loop() {
    if (cartridge_state_changed) {
        if (cartridge_state_debounce) {
            cartridge_state_debounce = false;
            cartridge_state_changed = false;
            Serial.println("Cartridge state changed");
            check_device_state(false);
        } else {
            delay(50);
            cartridge_state_debounce = true;
        }
    }

    if (control_heater_temperature_flag) {
        control_heater_temperature_flag = false;
        set_heater_power(pid_controller());
    }

    if (cartridge_present) {
        ledBusy.setActive(true);
    } else if (heater.temp_C_10X < HEATER_READY_TEMP) {
        heater_ready_debounce_timeout = millis() + HEATER_READY_DEBOUNCE_DELAY;
    } else if (heater_ready_debounce_timeout < millis()) {
        ledAvailable.setActive(true);
    }

    if (run_alert_buzzer) {
        run_alert_buzzer = false;
        turn_on_buzzer_for_duration(BUZZER_ALERT_DURATION, BUZZER_ALERT_FREQUENCY);
    }
}

void loop()
{
    while (Serial.available())
    {
        char c = Serial.read();
        serial_buffer[serial_buffer_index] = c;
        
        if (Serial.available()) {
            serial_buffer_index++;
            serial_buffer_index %= SERIAL_COMMAND_BUFFER_SIZE;
        } else {
            serial_buffer[serial_buffer_index] = '\0';
            Serial.println(serial_buffer);
            serial_buffer_index = 0;
            particle_command(String(serial_buffer));
        }
    }

    state_loop();
    long_duration_command_loop();

    if (callback_complete) {
        process_callback_buffer();
    } else if (!callback_pending()) {
        if (ready_to_start_test && !test_in_progress) {
            run_test();
        } else {
            registration_loop();
            validate_cartridge_loop();
            test_upload_loop();
        }
    }
}
