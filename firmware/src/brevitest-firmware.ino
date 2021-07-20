/*
 * Project brevitest_v1_0
 * Description: firmware for Acuity™ Sample Processing Unit, part of the Brevitest™ Diagnostic Platform
 * Author: Leo Linbeck III
 * Date: April 2020-July 2021
 */

#include "brevitest-firmware.h"

SYSTEM_THREAD(ENABLED);
PRODUCT_ID(PRODUCT_NUMBER);
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
            /*if (serial_messaging_on) Log.info("Table: number = %d, indx1 = %d, indx2 = %d, result = %d", table_number, indx1, indx2, result);*/
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
//                       DETECTOR                          //
//                                                         //
/////////////////////////////////////////////////////////////

void detector_changed_interrupt()
{
    detector_changed = true;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        MOTOR                            //
//                                                         //
/////////////////////////////////////////////////////////////

void sleep_motor()
{
    digitalWrite(pinMotorSleep, LOW);
    motor_awake = false;
}

void wake_motor()
{
    digitalWrite(pinMotorSleep, HIGH);
    delay(10);
    motor_awake = true;
}

bool move_one_eighth_step(int dir, int step_delay)
{
    if (dir == HIGH) {
        if (digitalRead(pinStageLimit) == LOW) {
            delay(10);
            if (digitalRead(pinStageLimit) == LOW) {
                Log.info("Proximal stage limit switch detected at position %d", stage_position);
                stage_position = 0;
                microns_error = 0;
                return false;
            }
        }
        if (stage_position <= 0) {
            Log.info("Stage position at zero");
            stage_position = 0;
            microns_error = 0;
            return false;
        }
    }
    else if (stage_position >= STAGE_POSITION_LIMIT) {
        Log.info("Stage distal limit reached");
        return false;
    }

    digitalWrite(pinMotorStep, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(pinMotorStep, LOW);
    delayMicroseconds(step_delay);
    stage_position += dir == HIGH ? -MOTOR_MICRONS_PER_EIGHTH_STEP : MOTOR_MICRONS_PER_EIGHTH_STEP;
    if (stage_position <= 0) {
        stage_position = 0;
        microns_error = 0;
    }

    return true;
}

void move_stage(int microns, int step_delay)
{
    // move a specific number of microns - negative for reverse movement
    // microns: negative => move proximally, positive => move distally

    int eighth_steps, abs_microns, dir, i, floored_step_delay;

    if (!motor_awake) {
        wake_motor();
    }
    dir = (microns < 0) ? HIGH : LOW;
    digitalWrite(pinMotorDir, dir);
    // Log.info("Stepping, dir = %c", dir == LOW ? 'L' : 'H');

    abs_microns = abs(microns) + microns_error;
    eighth_steps = abs_microns / MOTOR_MICRONS_PER_EIGHTH_STEP;
    microns_error = abs_microns % MOTOR_MICRONS_PER_EIGHTH_STEP;
    floored_step_delay = step_delay < MOTOR_MINIMUM_STEP_DELAY ? MOTOR_MINIMUM_STEP_DELAY : step_delay;
    // Log.info("move_stage: microns = %d, dir = %c, eighth_steps = %d, microns_error = %d", microns, dir == LOW ? 'L' : 'H', eighth_steps, microns_error);

    // delay(10);
    for (i = 0; i < eighth_steps; i++) {
        if (!move_one_eighth_step(dir, floored_step_delay)) {
            break;
        }
    }
    /*Log.info("Move complete, stage location = %d, limit = %d", stage_position, STAGE_POSITION_LIMIT);*/
}

void move_stage_until_proximal_limit(int step_delay) {
    int count = STAGE_POSITION_LIMIT / MOTOR_MICRONS_PER_EIGHTH_STEP + 40;
    digitalWrite(pinMotorDir, HIGH);
    while (digitalRead(pinStageLimit) == HIGH && count-- > 0) {
        digitalWrite(pinMotorStep, HIGH);
        delayMicroseconds(step_delay);

        digitalWrite(pinMotorStep, LOW);
        delayMicroseconds(step_delay);
        
        stage_position -= MOTOR_MICRONS_PER_EIGHTH_STEP;
    }
    Log.info("Proximal stage limit switch detected at position %d", stage_position);
    stage_position = 0;
    microns_error = 0;
}

void reset_stage(bool sleep)
{
    if (!motor_awake) {
        wake_motor();
    }
    move_stage_until_proximal_limit(MOTOR_RESET_STEP_DELAY);
    move_stage(STAGE_MICRONS_TO_INITIAL_POSITION, MOTOR_RESET_STEP_DELAY);
    if (sleep) {
        sleep_motor();
    }
}

void move_stage_to_optical_read_position()
{
    move_stage_to_position(STAGE_OPTICAL_SENSOR_READ_POSITION, MOTOR_SLOW_STEP_DELAY);
}

void move_stage_to_test_start_position()
{
    move_stage_to_position(STAGE_MICRONS_TO_TEST_START_POSITION, MOTOR_SLOW_STEP_DELAY);
}

void move_stage_to_position(int position, int step_delay)
{
    int move_distance = position - stage_position;
    if (move_distance != 0) {
        move_stage(move_distance, step_delay);
    }
}

int BCODE_loop(void);
void oscillate_stage(int amplitude, int step_delay, int cycles, bool inBCODE)
{
    int i;

    for (i = 0; i < cycles; i++)
    {
        move_stage(amplitude, step_delay);
        if (inBCODE) BCODE_loop();
        move_stage(-amplitude, step_delay);
        if (inBCODE) BCODE_loop();
        if (test_cancelled) return;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                  BARCODE SCANNER                        //
//                                                         //
/////////////////////////////////////////////////////////////

int scan_barcode()
{
    int buf; // a little buffer for reading barcode (we will coerce into character)
    int i = 0; // index for reading barcode from serial port into barcode_uuid
    int result; // return the type of cartridge (test, magentometer, temperature, optical) or error
    bool success; // set to true if barcode read is successful
    unsigned long timeout; // keep this from taking too long

    Serial1.begin(9600); // barcode scanner interface through RX/TX pins
    barcode_uuid[0] = '\0'; // reset barcode_uuid
    timeout = millis() + BARCODE_READ_TIMEOUT; // set timeout for overall process

    Log.info("Start barcode reader");
    digitalWrite(pinBarcodeTrigger, LOW); // start read by pulling trigger pin low

    Log.info("Wait for read to complete");
    while (digitalRead(pinBarcodeReady) == LOW && millis() < timeout) { // if read is not complete or timed out, wait and check again
        delay(100);
        Particle.process();
    };
    success = digitalRead(pinBarcodeReady) == HIGH; // successful if read is completed before timeout
    digitalWrite(pinBarcodeTrigger, HIGH); // stop read by setting trigger pin back to high

    if (success) {
        Log.info("Read successful - waiting for barcode data");
        while (!Serial1.available() && millis() < timeout) { // if data buffer is empty, wait and check again
            delay(100);
            Particle.process();
        };
        delay(100); // allow barcode buffer to fill before reading
        do {
            buf = Serial1.read(); // read a byte of data (returns -1 if no data is available)
            if (buf != -1) {
                barcode_uuid[i++] = (char)buf; // coerce byte to character and append to barcode_uuid
            }
        } while (Serial1.available() && i <= BARCODE_UUID_LENGTH); // continue while data is available and there's no overflow
        barcode_uuid[--i] = '\0'; // 
    } else {
        Log.info("Timeout - read failure");
    }

    Serial1.end(); // close serial port to barcode scanner

    switch (i) { // find out what this barcode is
        case BARCODE_UUID_LENGTH: // is the barcode a test cartridge?
            result = BARCODE_TYPE_CARTRIDGE;
            break;
        case VALIDATION_UUID_LENGTH: // is the barcode a validation cartridge? if so, check the validation prefix
            if (strncmp(barcode_uuid, MAGNETOMETER_PREFIX, BARCODE_PREFIX_LENGTH) == 0) { // is it a magnetometer?
                result = BARCODE_TYPE_MAGNETOMETER;
            } else if (strncmp(barcode_uuid, TEMPERATURE_PREFIX, BARCODE_PREFIX_LENGTH) == 0) { // is it a temperature probe?
                result = BARCODE_TYPE_TEMPERATURE;
            } else if (strncmp(barcode_uuid, SHIPPING_PREFIX, BARCODE_PREFIX_LENGTH) == 0) { // is it an shipping bolt installation?
                result = BARCODE_TYPE_SHIPPING;
            } else {
                strcpy(barcode_uuid, BARCODE_ERROR_MESSAGE); // replace whatever is there with an error message
                barcode_uuid[BARCODE_ERROR_MESSAGE_LENGTH] = '\0';
                result = BARCODE_TYPE_VALIDATION_ERROR;
            }
            break;
        case OPTICAL_UUID_LENGTH: // is the barcode a validation cartridge? if so, check the validation prefix
            if (strncmp(barcode_uuid, OPTICAL_PREFIX, BARCODE_PREFIX_LENGTH) == 0) { // is it an optical probe?
                result = BARCODE_TYPE_OPTICAL;
            } else { // we must have goofed up somewhere
                strcpy(barcode_uuid, BARCODE_ERROR_MESSAGE); // replace whatever is there with an error message
                barcode_uuid[BARCODE_ERROR_MESSAGE_LENGTH] = '\0';
                result = BARCODE_TYPE_OPTICAL_ERROR;
            }
            break;
        default: // if you're not a test cartridge, or a validation cartridge, you're an error
            strcpy(barcode_uuid, BARCODE_ERROR_MESSAGE); // replace whatever is there with an error message
            barcode_uuid[BARCODE_ERROR_MESSAGE_LENGTH] = '\0';
            result =  BARCODE_TYPE_GENERAL_ERROR;
    }

    Log.info("Barcode read: %s, length: %d, type: %d", barcode_uuid, i, result);
    return result;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        BUZZER                           //
//                                                         //
/////////////////////////////////////////////////////////////

void turn_on_buzzer_for_duration(int duration, int frequency)
{
    if (serial_messaging_on)
        Log.info("Turning on buzzer for duration %d at frequency %d", duration, frequency);
    tone(pinBuzzer, frequency, duration);
}

void check_buzzer() {
    if (buzzer_problem_running) {
        start_problem_buzzer = true;
    } else if (buzzer_alert_running) {
        start_alert_buzzer = true;
    }
}

void turn_on_buzzer_alert() {
    if (!buzzer_alert_running) {
        buzzer_problem_running = false;
        start_problem_buzzer = false;
        buzzer_alert_running = true;
        start_alert_buzzer = true;
        buzzer_timer.changePeriod(BUZZER_ALERT_PERIOD);
        buzzer_timer.reset();
    }
}

void turn_on_buzzer_problem() {
    if (!buzzer_problem_running) {
        buzzer_problem_running = true;
        start_problem_buzzer = true;
        buzzer_alert_running = false;
        start_alert_buzzer = false;
        buzzer_timer.changePeriod(BUZZER_PROBLEM_PERIOD);
        buzzer_timer.reset();
    }
}

void turn_off_buzzer_timer() {
    buzzer_alert_running = false;
    start_alert_buzzer = false;
    buzzer_problem_running = false;
    start_problem_buzzer = false;
    buzzer_timer.stop();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                    INDICATOR LED                        //
//                                                         //
/////////////////////////////////////////////////////////////

void turn_off_indicator_LEDs() {
    if (indicatorProblem.isActive()) indicatorProblem.setActive(false);
    if (indicatorBusy.isActive()) indicatorBusy.setActive(false);
    if (indicatorAvailable.isActive()) indicatorAvailable.setActive(false);
    if (indicatorAsync.isActive()) indicatorAsync.setActive(false);
    if (indicatorValidation.isActive()) indicatorValidation.setActive(false);
}

void turn_on_problem_LED() {
    if (!indicatorProblem.isActive()) {
        turn_off_indicator_LEDs();
        indicatorProblem.setActive(true);
    }
}

void turn_on_busy_LED() {
    if (!indicatorBusy.isActive()) {
        turn_off_indicator_LEDs();
        indicatorBusy.setActive(true);
    }
}

void turn_on_available_LED() {
    if (!indicatorAvailable.isActive()) {
        turn_off_indicator_LEDs();
        indicatorAvailable.setActive(true);
    }
}

void turn_on_async_LED() {
    if (!indicatorAsync.isActive()) {
        turn_off_indicator_LEDs();
        indicatorAsync.setActive(true);
    }
}

void turn_on_validation_LED() {
    if (!indicatorValidation.isActive()) {
        turn_off_indicator_LEDs();
        indicatorValidation.setActive(true);
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                       HEATER                            //
//                                                         //
/////////////////////////////////////////////////////////////

void turn_on_heater(int power)
{
    power = limit(power, HEATER_MAX_POWER, 0);
    analogWrite(heater.heater_pin, power, HEATER_PWM_FREQUENCY);
    heater.power = power;
    heater.heater_on = true;
    if (serial_messaging_on) Log.info("Heater set to power %d", power);
}

void turn_off_heater()
{
    analogWrite(heater.heater_pin, 0);
    heater.heater_on = false;
    heater.power = 0;
    if (serial_messaging_on) Log.info("Heater turned off");
}

int limit(int value, int max, int min)
{
    return value > max ? max : (value < min ? min : value);
}

int set_heater_power(int power)
{
    unsigned long start = millis();
    if (power != 0) {
        if (!optical_read_in_progress) turn_on_heater(power);
    } else {
        turn_off_heater();
    }

    return (int)(millis() - start);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                     STAGE LEDS                          //
//                                                         //
/////////////////////////////////////////////////////////////

void set_power_on_all_LEDs(int power) {
    baseline.led_assay = power;
    baseline.led_c1 = power;
    baseline.led_c2 = power;
}

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

void scanResultCallback(const BleScanResult &scanResult, void *context) {

    String name = scanResult.advertisingData().deviceName();
    if (name.length() > 0) {
        Log.info("Advertising name: %s", name.c_str());
    }

    uint8_t data[27];
    char *id = (char *) &data[2];
    if (scanResult.scanResponse().customData(data, 26)) {
        *(id + 24) = '\0';
        Log.info("Device ID: %s", id);
        if (strncmp(id, &barcode_uuid[8], 24) == 0 && strncmp(name, "Magnetometer", 12) == 0) {
            magnetometer_found = true;
            magnetometer_address = scanResult.address();
            Log.info("Barcode matched. MAC: %02X:%02X:%02X:%02X:%02X:%02X | RSSI: %ddBm",
                    magnetometer_address[0], magnetometer_address[1], magnetometer_address[2],
                    magnetometer_address[3], magnetometer_address[4], magnetometer_address[5], scanResult.rssi());
            BLE.stopScanning();
        }
    }
}

int BLE_scan() {
    int count = BLE.scan(scanResultCallback);
    if (count > 0) {
        Log.info("%d devices found", count);
    }
    return count;
}

int check_magnets_in_one_well(int well, int mark) {
    BleCharacteristic characteristic;

    move_stage(well_move[well], MOTOR_SLOW_STEP_DELAY);
    delay(magnetometer_heating_delay);
    if (magnetometer.getCharacteristicByUUID(characteristic, bleCharUuid[well])) {
        String result;
        characteristic.getValue(result);
        int len = sprintf(&particle_register[mark], "%d\t%s\n", well + 1, result.c_str());
        return len + mark;
    } else {
        Log.info("Could not find magnetometer data for well %d", well + 1);
        return -1;
    }
}

int validate_magnets() {
    magnetometer_validation_mode = false;
    magnetometer_found = false;
    BLE_scan();
    if (magnetometer_found) {
        Log.info("Magnetometer found, connecting...");
        magnetometer = BLE.connect(magnetometer_address);
        if (magnetometer.connected()) {
            delay(magnetometer_initial_heating_delay);
            int mark = 32;
            Log.info("Connected to magnetometer");
            strncpy(particle_register, barcode_uuid, 32);
            particle_register[mark++] = '\n';
            reset_stage(false);
            move_stage_to_test_start_position();
            for (int i = 0; i < 5; i++) {
                mark = check_magnets_in_one_well(i, mark);
                if (mark > PARTICLE_REGISTER_SIZE || mark == -1) {
                    return 0;
                }
            }
            magnetometer.disconnect();
            reset_stage(true);
            particle_register[mark] = '\0';
            brevitest_publish("validate-magnets", particle_register);
            return 1;
        } else {
            Log.info("Could not connect to magnetometer %s", barcode_uuid);
            return 0;
        }
    } else {
        Log.info("Magnetometer not found");
        return 0;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                    OPTICAL SENSORS                      //
//                                                         //
/////////////////////////////////////////////////////////////

void config_optical_sensors(char channel, int param, int addr)
{
    int bytes_received, bytes_sent, reg, result;

    if (serial_messaging_on)
        Log.info("Configuring optical sensor %c", channel);
    Wire.beginTransmission(addr);
    bytes_sent = Wire.write(0x00);
    bytes_sent += Wire.write(0x02); // enter configuration mode
    result = Wire.endTransmission(true);
    if (result != 0)
    {
        Log.info("Config optics %c, %d bytes, result: %d", channel, bytes_sent, result);
    }

    Wire.beginTransmission(addr);
    bytes_sent = Wire.write(0x06);
    bytes_sent += Wire.write((uint8_t)param); // write param to CREG1
    result = Wire.endTransmission(true);
    if (result != 0)
    {
        Log.info("Config optics %c, %d bytes, result: %d", channel, bytes_sent, result);
    }

    Wire.beginTransmission(addr);
    bytes_sent = Wire.write(0x06); // confirm register was written
    result = Wire.endTransmission(false);
    if (result != 0)
    {
        Log.info("Send error, %d bytes, result: %d", bytes_sent, result);
    }
    bytes_received = Wire.requestFrom(addr, 1);

    reg = Wire.read();
    if (serial_messaging_on)
        Log.info("bytes = %d, param = %d, CREG1 = %d", bytes_received, param, reg);
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
        Log.info("Send error, %d bytes, result: %d", bytes, result);
    }
    bytes = Wire.requestFrom(addr, (uint8_t) 4);

    // status
    osr = Wire.read();
    status = Wire.read();
    if (serial_messaging_on)
        Log.info("Status = %d, OSR = %d", status, osr);

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
        Log.info("Config optics: address %d, %d bytes, result: %d", addr, bytes, result);
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
        Log.info("Read failure: address = %d", addr);
        return false;
    }

    if (serial_messaging_on)
        Log.info("Starting optical sensor data addr = %d read", addr);

    Wire.beginTransmission(addr);
    bytes = Wire.write(0x00);
    result = Wire.endTransmission(false);
    if (result != 0)
    {
        Log.info("Send error, %d bytes, result: %d", bytes, result);
    }
    bytes = Wire.requestFrom(addr, (uint8_t) 10);

    // status
    osr = Wire.read();
    status = Wire.read();
    if (serial_messaging_on)
        Log.info("Status = %d, OSR = %d", status, osr);

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
        Serial.printf("%d ", msb);
    *z = (msb << 8) + lsb;

    return true;
}

void get_data_from_one_optical_sensor(char channel, int param, int pwr, bool log)
{
    uint8_t addr;
    int read_attempt, sum_x, sum_y, sum_z, sum_t;
    uint16_t tempC, x, y, z, l_value;
    BrevitestOpticalSensorRecord *reading;

    if (channel == 'A') {
        addr = 0x74;
    } else if (channel == '1') {
        addr = 0x75;
    } else if (channel == '2') {
        addr = 0x76;
    } else {
        Log.info("ERROR: Channel %c not found", channel);
        return;
    }

    reading = &(test.reading[test.number_of_readings % OPTICAL_MAXIMUM_NUMBER_OF_READINGS]);
    test.number_of_readings++;
    
    turn_on_LED(channel, pwr);
    delay(100);
    
    config_optical_sensors(channel, param, addr);

    reading->channel = channel;
    reading->samples = OPTICAL_SENSOR_NUMBER_OF_SAMPLES;
    sum_x = sum_y = sum_z = sum_t = 0;
    if (take_one_sample_from_optical_sensor(addr, &x, &y, &z, &tempC)) { // first pancake
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
                    Log.info("Read optical sensor failed, channel %c, try %d", channel, read_attempt);
                    read_attempt++;
                }
            }
        }
    }
    turn_off_LED(channel);

    reading->msec = millis();
    reading->x = sum_x / reading->samples;
    reading->y = sum_y / reading->samples;
    reading->z = sum_z / reading->samples;
    reading->temperature = sum_t / reading->samples;

    if (log) {
        l_value = integerSqrt((reading->x * reading->x) + (reading->y * reading->y) + (reading->z * reading->z));
        Log.info("%c\t%d\t%d\t%d\t%d\t%lu\t%d\t%d\t%d\t%d\t%d", channel, param, pwr, stage_position, reading->samples, reading->msec, reading->temperature, reading->x, reading->y, reading->z, l_value);
    }
}

bool startI2C() {
    if (Wire.isEnabled()) return true;

    /*Log.info("Attempting to read optical sensors");*/
    Wire.setSpeed(CLOCK_SPEED_100KHZ);
    Wire.begin();
    delay(100);

    return Wire.isEnabled();
}

bool enable_optical_system(bool force_read)
{
    move_stage_to_optical_read_position();
    optical_read_in_progress = true;
    turn_off_heater();
    delay(100);

    if (startI2C()) {
        return true;
    }
    else
    {
        Log.info("Unable to start communication with optical sensors");
        return false;
    }
    
}

void disable_optical_system()
{
    Wire.end();
    optical_read_in_progress = false;
}

void read_optical_sensors(int param, bool inBCODE)
{
    unsigned long elapsed = millis();

    get_data_from_one_optical_sensor('A', param, baseline.led_assay, true);
    if (inBCODE) BCODE_loop();
    move_stage_to_position(baseline.pos_c1, MOTOR_SLOW_STEP_DELAY);
    get_data_from_one_optical_sensor('1', param, baseline.led_c1, true);
    if (inBCODE) BCODE_loop();
    move_stage_to_position(baseline.pos_c2, MOTOR_SLOW_STEP_DELAY);
    get_data_from_one_optical_sensor('2', param, baseline.led_c2, true);
    if (inBCODE) BCODE_loop();

    if (serial_messaging_on)
        Log.info("Elapsed time: %u", (unsigned int) (millis() - elapsed));
}

int start_optical_sensor_test(int distance, int readings, int period)
{
    test.number_of_readings = 0;
    optical_test_readings = readings <= 0 ? 1 : (readings > OPTICAL_MAXIMUM_NUMBER_OF_READINGS ? OPTICAL_MAXIMUM_NUMBER_OF_READINGS : readings);
    optical_test_count = 0;
    optical_test_move = distance;
    optical_test_take_reading = true;
    async_command_timer.changePeriod((unsigned long) period);
    async_command_timer.start();
    async_command_running = true;
    async_command_optical_running = true;
    return readings;
}

void validate_optics() {
    optical_validation_mode = false;

    char c;
    int len = sprintf(particle_register, "%s%c%c%c",barcode_uuid, ITEM_DELIM, TEST_DATA_FORMAT_CODE, ITEM_DELIM);;

    Log.info("Validating optics...");
    test.number_of_readings = 0;
    set_power_on_all_LEDs(LED_DEFAULT_POWER);
    reset_stage(false);
    if (enable_optical_system(true)) read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, false);
    disable_optical_system();

    if (test.number_of_readings == 3) { // test completed
        for (int i = 0; i < 3; i++)
        {
            c = test.reading[i].channel;
            if (c == 'A' || c == '1' || c == '2') {
                len += append_test_reading(len, &(test.reading[i]));
            }
        }
        particle_register[len - 1] = '\0';
    }

    Log.info("particle_register: %s", particle_register);
    brevitest_publish("validate-optics", particle_register);
    reset_stage(true);
}

uint16_t calculate_L(int index) {
    return integerSqrt((test.reading[index].x * test.reading[index].x) + (test.reading[index].y * test.reading[index].y) + (test.reading[index].z * test.reading[index].z));
}

uint8_t find_baseline_led_power(char channel, int *error) {
    int pwr, last_pwr;
    int l_value, last_l_value;
    int i, last_error, offset;

    move_stage_to_position(STAGE_OPTICAL_SENSOR_READ_POSITION, MOTOR_SLOW_STEP_DELAY);
    pwr = last_pwr = LED_DEFAULT_POWER;
    l_value = OPTICAL_TARGET_L_VALUE;
    for (i = 0; i < 10; i++) {
        test.number_of_readings = 0;
        get_data_from_one_optical_sensor(channel, OPTICAL_SENSOR_DEFAULT_PARAM, (uint8_t) pwr, false);
        last_l_value = l_value;
        l_value = calculate_L(0);
        *error = OPTICAL_TARGET_L_VALUE - l_value;
        // Log.info("i = %d, pwr = %d, L = %d, err = %d", i, pwr, l_value, error);
        if (i <= 1) {
            pwr += *error > 0 ? 10 - 5 * i : -10 + 5 * i;
            continue;
        } else if (abs(*error) <= OPTICAL_SEARCH_THRESHOLD) {
            break;
        } else {
            offset = *error * (last_pwr - pwr) / (last_l_value - l_value);
            if (offset == 0) {
                if (*error > 0) {
                    last_pwr = pwr + 1;
                } else {
                    last_pwr = pwr - 1;
                }
                test.number_of_readings = 0;
                get_data_from_one_optical_sensor(channel, OPTICAL_SENSOR_DEFAULT_PARAM, (uint8_t) last_pwr, false);
                last_error = OPTICAL_TARGET_L_VALUE - calculate_L(0);
                if (abs(*error) > abs(last_error)) {
                    pwr = last_pwr;
                    *error = last_error;
                }
                break;
            } else {
                last_pwr = pwr;
                pwr = pwr + offset > 255 ? 255 : pwr + offset;
            }
        }
    }
    // Log.info("last, pwr = %d, err = %d", pwr, *error);
    return pwr;
}

bool set_baselines() {
    int attempt = 0;
    int error;
    int max_error = OPTICAL_FAILURE_THRESHOLD;
    while (max_error > OPTICAL_ERROR_THRESHOLD && attempt++ < 3) {
        baseline.led_assay = find_baseline_led_power('A', &error);
        Log.info("baseline for assay channel: attempt = %d, pwr = %d, pos = %d, error = %d", attempt, baseline.led_assay, baseline.pos_assay, error);
        max_error = abs(error);

        baseline.led_c1 = find_baseline_led_power('1', &error);
        Log.info("baseline for control 1 channel: attempt = %d, pwr = %d, pos = %d, error = %d", attempt, baseline.led_c1, baseline.pos_c1, error);
        max_error = abs(error) > max_error ? abs(error) : max_error;

        baseline.led_c2 = find_baseline_led_power('2', &error);
        Log.info("baseline for control 2 channel: attempt = %d, pwr = %d, pos = %d, error = %d", attempt, baseline.led_c2, baseline.pos_c2, error);
        max_error = abs(error) > max_error ? abs(error) : max_error;
    }
    test.number_of_readings = 0;
    return max_error <= OPTICAL_FAILURE_THRESHOLD;
}

/////////////////////////////////////////////////////////////
//                                                         //
//               TEMPERATURE CONTROL SYSTEM                //
//                                                         //
/////////////////////////////////////////////////////////////

int get_heater_temperature()
{
    analogWrite(heater.heater_pin, 0);
    delay(10);
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
        Log.info("Temperature: %d.%d˚C, %d.%d˚F", heater.temp_C_10X / 10, heater.temp_C_10X % 10, heater.temp_F_10X / 10, heater.temp_F_10X % 10);
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
                    Log.info("raw = %d, T = %d.%d˚C, target = %d.%d, dt = %d, error = %d, integral = %d, derivative = %d, output = %d",
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
    control_heater_temperature_flag = !optical_read_in_progress;
}

void start_temperature_control()
{
    heater.read_time = 0;
    control_heater_temperature_timer.start();
    Log.info("Temperature control system started");
}

void stop_temperature_control()
{
    control_heater_temperature_timer.stop();
    set_heater_power(0);
    Log.info("Temperature control system stopped");
}

/////////////////////////////////////////////////////////////
//                                                         //
//                 PUBLISH AND CALLBACKS                   //
//                                                         //
/////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////
//                 VERIFY DEVICE                   //
/////////////////////////////////////////////////////

void pubsub_verify_device()
{
    callback_timeout = millis() + RETRY_VERIFY_DEVICE;
    device_verification_in_progress = true;
    device_verified = false;
    if (Particle.connected()) {
        Log.info("Verifying device");
        brevitest_publish("verify-device", (char *)device_id.c_str());
    } else {
        Log.info("Not connected to the cloud. Wait and retry.");
        delay(PARTICLE_CLOUD_DELAY);
    }
}

void startup_device(void);
void callback_verify_device() {
    clear_current_event();
    if (strncmp(callback_status, SUCCESS, 7) == 0) {
        Log.info("Device verified - starting up...");
        device_verification_in_progress = false;
        device_verified = true;
        startup_device();
    }
}

/////////////////////////////////////////////////////
//              VALIDATE CARTRIDGE                 //
/////////////////////////////////////////////////////

void pubsub_validate_cartridge()
{
    callback_timeout = millis() + RETRY_VALIDATE_CARTRIDGE;
    cartridge_validation_in_progress = true;
    cartridge_validated = false;
    if (Particle.connected()) {
        Log.info("Validating cartridge");
        brevitest_publish("validate-cartridge", barcode_uuid);
    } else {
        Log.info("Not connected to the cloud. Remove cartridge.");
        cartridge_validation_in_progress = false;
        cartridge_validation_mode = false;
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

    crc_calculated = abs((int) checksum(assay.BCODE, assay.BCODE_length));
    Log.info("crc_loaded: %d, crc_calculated: %d", crc_loaded, crc_calculated);
    return (crc_loaded == crc_calculated); // bcode loaded if checksums match
}

void callback_validate_cartridge() {
    clear_current_event();
    cartridge_validation_in_progress = false;
    if (!detector_on) {
        cartridge_validation_mode = false;
        cartridge_validated = false;
    } else {
        cartridge_validated = (strncmp(callback_status, SUCCESS, 7) == 0);
        Log.info("Cartridge %s %s", barcode_uuid, cartridge_validated ? "validated" : "invalid");
        if (cartridge_validated) { // valid cartridge found
            if (load_assay_record(callback_data)) {
                Log.info("Assay information loaded. Test ready to start.");
                cartridge_validation_mode = false;
                test_start_mode = true;
            } else {
                Log.info("Failed to load assay record. Will retry later.");
                cartridge_validated = false;
            }
        } else {
            Log.info("Invalid cartridge: %s", callback_data);
            cartridge_validation_mode = false;
        }
    }
}

/////////////////////////////////////////////////////
//                   START TEST                    //
/////////////////////////////////////////////////////

void pubsub_start_test() {
    callback_timeout = millis() + RETRY_START_TEST;
    test_start_in_progress = true;
    test_underway = false;
    if (Particle.connected()) {
        Log.info("Starting test");
        brevitest_publish("start-test", test.cartridge_uuid);
    } else {
        Log.info("Not connected to the cloud. Wait and retry.");
    }
}

void callback_start_test() {
    clear_current_event();
    test_start_in_progress = false;
    test_underway = (strncmp(callback_status, SUCCESS, 7) == 0);
    Log.info("Test %s %s", callback_data, test_underway ? "underway" : "failed to start");
    if (!detector_on) {
        test_underway = false;
        test_cancelled = true;
        write_test_record_to_eeprom();
        test_upload_mode = true;
    } else if (test_underway) {
        test_start_mode = false;
        run_test();
    }
}

/////////////////////////////////////////////////////
//                  UPLOAD TEST                    //
/////////////////////////////////////////////////////

bool test_in_cache() {
    return eeprom.cache.cartridge_uuid[0] != '\0';
}
void pubsub_upload_test() {
    if (test_in_cache()) {
        test_upload_in_progress = true;
        test_upload_finished = false;
        callback_timeout = millis() + RETRY_UPLOAD_TEST;
        if (Particle.connected()) {
            process_test_record();
            Log.info("Uploading test, payload length: %d, payload: %s", strlen(particle_register), particle_register);
            brevitest_publish("upload-test", particle_register);
        } else {
            Log.info("Not connected to the cloud. Wait and retry.");
        }
    } else {
        test_upload_in_progress = false;
        test_upload_finished = true;
    }
}

void remove_test_from_cache(char *testToRemove)
{
    if (strncmp(testToRemove, eeprom.cache.cartridge_uuid, CARTRIDGE_UUID_LENGTH) == 0) {
        memset(&eeprom.cache, 0, sizeof(BrevitestTestRecord));
        store_eeprom();
        return;
    }
}

void callback_upload_test() {
    clear_current_event();
    test_upload_in_progress = false;
    bool success = (strncmp(callback_status, SUCCESS, 7) == 0);
    bool invalid = (strncmp(callback_status, INVALID, 7) == 0);
    test_upload_finished = (success || invalid);
    if (test_upload_finished) {
        test_invalid = invalid;
        remove_test_from_cache(callback_data);
        test_upload_mode = false;
    }

    Log.info("Test %s upload %s", callback_data, invalid ? "invalid" : success ? "succeeded" : "failed, will retry later");
}

/////////////////////////////////////////////////////
//                VALIDATE MAGNETS                 //
/////////////////////////////////////////////////////

void callback_validate_magnets() {
    clear_current_event();
    bool success = (strncmp(callback_status, SUCCESS, 7) == 0);
    if (success) {
        if (strncmp(callback_data, "validated", 9) != 0) {
            Log.info("Magnets %s", callback_data);
            delay(2000);
            System.reset();
        }
    }

    Log.info("Magnet validation %s", success ? "succeeded" : "failed, will retry later");
}

/////////////////////////////////////////////////////
//                VALIDATE OPTICS                  //
/////////////////////////////////////////////////////

void callback_validate_optics() {
    clear_current_event();
    bool success = (strncmp(callback_status, SUCCESS, 7) == 0);
    if (success) {
        if (strncmp(callback_data, "validated", 9) != 0) {
            Log.info("Optics %s", callback_data);
            delay(2000);
            System.reset();
        }
    }

    Log.info("Optics validation %s", success ? "succeeded" : "failed, will retry later");
}

/////////////////////////////////////////////////////
//               PUBSUB FUNCTIONS                  //
/////////////////////////////////////////////////////

void set_current_event(String event_name) {
    strcpy(current_event, event_name.c_str());
    if (strcmp(current_event, "verify-device") == 0) {
        current_event_code = PUBSUB_VERIFY_DEVICE;
    } else if (strcmp(current_event, "validate-cartridge") == 0) {
        current_event_code = PUBSUB_VALIDATE_CARTRIDGE;
    } else if (strcmp(current_event, "start-test") == 0) {
        current_event_code = PUBSUB_START_TEST;
    } else if (strcmp(current_event, "upload-test") == 0) {
        current_event_code = PUBSUB_UPLOAD_TEST;
    } else if (strcmp(current_event, "validate-magnets") == 0) {
        current_event_code = PUBSUB_VALIDATE_MAGNETS;
    } else if (strcmp(current_event, "validate-optics") == 0) {
        current_event_code = PUBSUB_VALIDATE_OPTICS;
    } else {
        current_event_code = 0;
    }
}

void clear_current_event() {
    current_event[0] = '\0';
    current_event_code = 0;
}

void set_publish_params(String event_name) {
    set_current_event(event_name);
    callback_complete = false;
    callback_buffer[0] = '\0';
    callback_buffer[PUBSUB_CALLBACK_BUFFER_SIZE] = '\0';
}

void brevitest_publish(String event_name, char *payload)
{
    set_publish_params(event_name);
    Particle.publish(String(PUBSUB_EVENT_NAME), event_name + String(ITEM_DELIM) + String(payload), PRIVATE, NO_ACK);
    Log.info("PUBLISH: event = %s, payload = %s", event_name.c_str(), payload);
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

    if (strcmp(callback_event, current_event) != 0) {
        Log.info("Wrong event callback: expecting event %s, received event %s", current_event, callback_event);
    } else {
        switch (current_event_code) {
            case PUBSUB_VERIFY_DEVICE:
                callback_verify_device();
                break;
            case PUBSUB_VALIDATE_CARTRIDGE:
                callback_validate_cartridge();
                break;
            case PUBSUB_START_TEST:
                callback_start_test();
                break;
            case PUBSUB_UPLOAD_TEST:
                callback_upload_test();
                break;
            case PUBSUB_VALIDATE_MAGNETS:
                callback_validate_magnets();
                break;
            case PUBSUB_VALIDATE_OPTICS:
                callback_validate_optics();
                break;
            default:
                Log.info("Unknown event code %d", current_event_code);
                clear_current_event();
        }
    }
}

void brevitest_error(const char *event, const char *data)
{
    strcat(callback_buffer, data);
    int last = strlen(data) - 1;
    callback_complete = (data[last] == END_DELIM);
    Log.info("ERROR | callback_buffer: %s, callback_complete: %c", callback_buffer, callback_complete ? 'Y' : 'N');
}

void brevitest_callback(const char *event, const char *data)
{
    strcat(callback_buffer, data);
    int last = strlen(data) - 1;
    callback_complete = (data[last] == END_DELIM);
    if (callback_complete) {
        Log.info("RESPONSE | callback_buffer: %s, callback_complete: %c", callback_buffer, callback_complete ? 'Y' : 'N');
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//               TEST CACHE AND UPLOADING                  //
//                                                         //
/////////////////////////////////////////////////////////////

void erase_test_from_cache() {
    memset(&eeprom.cache, 0, sizeof(BrevitestTestRecord));
}

void initialize_test_cache() {
    erase_test_from_cache();
    store_eeprom();
}

void store_test()
{
    memcpy(eeprom.cache.cartridge_uuid, test.cartridge_uuid, sizeof(BrevitestTestRecord));
    store_eeprom();
}

int append_test_reading(int start, BrevitestOpticalSensorRecord *reading)
{
    return sprintf(&(particle_register[start]), "%c%c%X%c%lX%c%X%c%X%c%X%c%X%c",
                   reading->channel, ARG_DELIM,
                   reading->samples, ARG_DELIM,
                   reading->msec, ARG_DELIM,
                   reading->x, ARG_DELIM,
                   reading->y, ARG_DELIM,
                   reading->z, ARG_DELIM,
                   reading->temperature, ATTR_DELIM);
}

void process_test_record()
{
    char c;
    BrevitestTestRecord *t = &eeprom.cache;
    int len = sprintf(particle_register, "%.24s%c%c%c",t->cartridge_uuid, ITEM_DELIM, TEST_DATA_FORMAT_CODE, ITEM_DELIM);;

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
    } else {
        test_completed = true;
    }
    memset(eeprom.running_test_uuid, 0, CARTRIDGE_UUID_LENGTH);
    store_test();
}

bool test_cached()
{
    if (eeprom.cache.cartridge_uuid[0] != '\0') {
        if (serial_messaging_on) {
            Log.info("Test cached for cartridge %s", eeprom.cache.cartridge_uuid);
        }
        return true;
    } else {
        return false;
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
    Log.info("%s, %d percent complete, temp = %d.%d", message.c_str(), test_percent_complete, heater.temp_C_10X / 10, heater.temp_C_10X % 10);
}

int BCODE_loop()
{
    unsigned long total_duration = millis();

    set_heater_power(pid_controller());
    if (digitalRead(pinCartridgeDetected) == HIGH) {
        Log.info("Cartridge movement detected...");
        delay(100);
        test_cancelled = digitalRead(pinCartridgeDetected) == HIGH;
        if (test_cancelled) {
            Log.info("Cartridge removed while test underway. Test cancelled.");
        } else {
            Log.info("Cartridge ok. Test continuing.");
        }
    }

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
    if (test_cancelled) return;
    if (residual > loop_time) {
      delay(residual - loop_time);
    }
}

int process_BCODE(int);
int process_one_BCODE_command(int cmd, int index)
{
    int param1, param2, param3, saved_position, start_index;
    unsigned long msec;

    if (test_cancelled) return index;

    switch (cmd) {
        case 0: // Start test()
            update_progress("Starting", 9000);
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
            update_progress("Moving", 2 * abs(param1) * param2 / MOTOR_MOVE_DURATION_UNIT);
            move_stage(param1, param2);
            BCODE_loop();
            break;
        case 3: // Oscillate Stage(microns, microseconds, cycles)
            index = get_BCODE_token(index, &param1); // microns to move
            index = get_BCODE_token(index, &param2); // step_delay_us
            index = get_BCODE_token(index, &param3); // number of cycles
            update_progress("Oscillating", 4 * abs(param1) * param2 * param3 / MOTOR_MOVE_DURATION_UNIT);
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
            // update_progress("Preparing", abs(stage_position - STAGE_OPTICAL_SENSOR_READ_POSITION) * MOTOR_FAST_STEP_DELAY / MOTOR_MOVE_DURATION_UNIT);
            update_progress("Reading", 2000);
            set_power_on_all_LEDs(LED_DEFAULT_POWER);
            if (enable_optical_system(true)) read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, false);
            disable_optical_system();
            break;
        case 11: // Read optical sensors with param1 = sensor parameters and param2 = LED power
            index = get_BCODE_token(index, &param1); // params
            index = get_BCODE_token(index, &param2); // LED power
            update_progress("Reading", 2000);
            set_power_on_all_LEDs(param2);
            if (enable_optical_system(true)) read_optical_sensors(param1, false);
            disable_optical_system();
            break;
        case 12: // Find baseline LED power and take param1 baseline readings - stage returns back to position prior to reading
            index = get_BCODE_token(index, &param1); // number of readings
            saved_position = stage_position;
            update_progress("Setting baselines", 10000);
            if (enable_optical_system(true) && set_baselines())
            {
                for (int i = 0; i < param1; i++) {
                    if (i > 0) {
                        update_progress("Pausing", 1000);
                        BCODE_delay(1000);
                    }
                    update_progress("Reading", 5000);
                    read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, false);
                }
                move_stage_to_position(saved_position, MOTOR_SLOW_STEP_DELAY);
            } else {
                test_cancelled = true;
            }
            disable_optical_system();
            break;
        case 13: // Take param1 readings - stage returns back to position prior to reading
            index = get_BCODE_token(index, &param1); // number of readings
            saved_position = stage_position;
            if (enable_optical_system(true))
            {
                for (int i = 0; i < param1; i++) {
                    update_progress("Reading", 5000);
                    read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, false);
                }
            }
            disable_optical_system();
            move_stage_to_position(saved_position, MOTOR_SLOW_STEP_DELAY);
            break;
        case 14: // Take param1 readings with pause of param2 ms between reads - stage returns back to position prior to reading
            index = get_BCODE_token(index, &param1); // number of readings
            index = get_BCODE_token(index, &param2); // pause between readings
            saved_position = stage_position;
            if (enable_optical_system(true))
            {
                for (int i = 0; i < param1; i++) {
                    if (i > 0) {
                        update_progress("Pausing", param2);
                        BCODE_delay(param2);
                    }
                    update_progress("Reading", 5000);
                    read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, false);
                }
            }

            disable_optical_system();
            move_stage_to_position(saved_position, MOTOR_SLOW_STEP_DELAY);
            break;
        case 15: // Set baseline time for param1 number of readings
            index = get_BCODE_token(index, &param1); // number of readings
            param1 *= 3; // three channels per reading
            msec = millis();
            update_progress("Timestamping", 10);
            for (int i = 0; i < param1; i++) {
                test.reading[i].msec = msec;
            }
            break;
        case 20: // Repeat begin(number of iterations)
            index = get_BCODE_token(index, &param1);

            Log.info("Begin repeating %d times", param1);
            start_index = index + 1;
            for (int i = 0; i < param1; i += 1) {
                if (test_cancelled) break;
                index = process_BCODE(start_index);
            }
            break;
        case 21: // Repeat end
            Log.info("End repeating");
            return -index;
            break;
        case 99: // Finish test
            Log.info("Finish test");
            update_progress("Finishing test", 8000);
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
//                      STRESS TEST                        //
//                                                         //
/////////////////////////////////////////////////////////////

void do_stress_test_step(int step) {
    Serial.print('.');
    switch(step % 16) {
        case 0: // restart stress test
            reset_stage(false);
            move_stage_to_test_start_position();
            break;
        case 1: // move to start of well 2 and wait one minute
            move_stage(-1500, MOTOR_SLOW_STEP_DELAY);
            delay(60000);
            break;
        case 2: // oscillate in well 2 and wait for beads to gather
            oscillate_stage(3000, 250, 100, true);
            delay(3000);
            break;
        case 3: // move to well 1
            move_stage(-6000, 65000);
            break;
        case 4: // oscillate in well 1
            oscillate_stage(-4500, 250, 250, true);
            delay(4000);
            break;
        case 5: // move to well 2
            move_stage(6000, 80000);
            delay(3000);
            move_stage(3750, 10000);
            break;
        case 6: // oscillate in well 2
            oscillate_stage(-4500, 250, 200, true);
            delay(3000);
            break;
        case 7: // move to well 3
            move_stage(4800, 80000);
            delay(3000);
            move_stage(3200, 10000);
            break;
        case 8: // oscillate in well 3
            oscillate_stage(-4500, 250, 150, true);
            break;
        case 9: // read baseline sensors
            if (enable_optical_system(true))
            {
                set_baselines();
                read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, true);
                read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, true);
            }
            disable_optical_system();
            delay(1500);
            break;
        case 10: // move to well 4
            move_stage(4800, 80000);
            delay(3000);
            move_stage(3200, 10000);
            break;
        case 11: // oscillate in well 4
            oscillate_stage(-4500, 250, 500, true);
            delay(1500);
            break;
        case 12: // move to well 5
            move_stage(4400, 80000);
            delay(3000);
            break;
        case 13: // oscillate in well 5
            oscillate_stage(6500, 250, 400, true);
            delay(5000);
            break;
        case 14: // read sensors
            if (enable_optical_system(true))
            {
                read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, true);
                read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, true);
                read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, true);
            }
            disable_optical_system();
            break;
        case 15: // save cycle number
            eeprom.stress_test_cycles++;
            Log.info("Stress test cycle %d complete, record is %d", eeprom.stress_test_cycles, eeprom.maximum_stress_test_cycles);
            if (eeprom.stress_test_cycles > eeprom.maximum_stress_test_cycles) {
                eeprom.maximum_stress_test_cycles = eeprom.stress_test_cycles;
            }
            store_eeprom();
            if (stress_test_limit != 0 && eeprom.stress_test_cycles >= stress_test_limit) {
                async_command_stress_test_running = false;
            }
            break;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                         COMMAND                         //
//                                                         //
/////////////////////////////////////////////////////////////

void async_command()
{
    if (async_command_magnet_running) {
        magnet_test_take_reading = true;
    } else if (async_command_optical_running) {
        optical_test_take_reading = true;
    }
}

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

    if (enable_optical_system(true)) {
        Log.info("Starting optical I2C bus scan");
        for (addr = 110; addr < 120; addr++) {
            Wire.beginTransmission(addr);
            Wire.write(0x00);
            result = Wire.endTransmission();
            if (result == 0) {
                Log.info("I2C device found at address %X", addr);
            }
        }
    }
    disable_optical_system();
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
//
//  LOW LEVEL COMMANDS
//
        case 1: // system reset
            System.reset();
            break;
        case 2: // reset EEPROM
            reset_eeprom();
            result = (int)eeprom.data_format_version;
            break;
        case 3: // clear test cache
            initialize_test_cache();
            result = eeprom.cache.cartridge_uuid[ 0] == '\0' ? 1 : 0;
            break;
        case 4: // scan i2c bus
            i2c_bus_scan();
            result = 1;
            break;
//
//  SERIAL PORT MESSAGING
//
        case 10: // turn on serial messaging
            serial_messaging_on = true;
            result = 1;
            break;
        case 11: // turn off serial messaging
            serial_messaging_on = false;
            result = 0;
            break;
//
//  STAGE MOTION
//
        case 20: // reset stage
            reset_stage(true);
            result = stage_position;
            break;
        case 21: // wake motor
            wake_motor();
            result = stage_position;
            break;
        case 22: // move microns, param1 microns with param2 step
            indx = get_next_command_param(arg, indx, &param1, 0);
            indx = get_next_command_param(arg, indx, &param2, MOTOR_SLOW_STEP_DELAY);
            move_stage(param1, param2);
            Log.info("Move stage %d microns, cumulative %d, error = %d", param1, stage_position, microns_error);
            result = stage_position;
            break;
        case 23: // move to specified location param1 at step delay param2
            indx = get_next_command_param(arg, indx, &param1, 0);
            indx = get_next_command_param(arg, indx, &param2, MOTOR_SLOW_STEP_DELAY);
            reset_stage(false);
            move_stage_to_position(param1, param2);
            sleep_motor();
            result = stage_position;
            break;
        case 24: // move stage to test start position
            reset_stage(false);
            move_stage_to_test_start_position();
            sleep_motor();
            result = stage_position;
            break;
        case 25: // move stage to optical read position
            reset_stage(false);
            move_stage_to_optical_read_position();
            sleep_motor();
            result = stage_position;
            break;
        case 26: // oscillate - param1 microns, param2 step_delay, param3 number of cycles
            indx = get_next_command_param(arg, indx, &param1, 25);
            indx = get_next_command_param(arg, indx, &param2, MOTOR_OSCILLATION_STEP_DELAY);
            indx = get_next_command_param(arg, indx, &param3, 10);
            oscillate_stage(param1, param2, param3, false);
            result = stage_position;
            break;
        case 27: // move to shipping bolt location
            move_stage_to_position(STAGE_SHIPPING_BOLT_LOCATION, MOTOR_SLOW_STEP_DELAY);
            Log.info("Ready to insert shipping bolt");
            result = stage_position;
            break;
        case 28: // move to position zero at param1 step_delay
            indx = get_next_command_param(arg, indx, &param1, MOTOR_SLOW_STEP_DELAY);
            move_stage(-stage_position, param1);
            result = stage_position;
            break;
        case 29: // sleep motor
            sleep_motor();
            result = stage_position;
            break;

//
//  STAGE LEDs
//
        case 30: // turn on assay LED for param1 milliseconds at power param2
            indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
            indx = get_next_command_param(arg, indx, &param2, LED_DEFAULT_POWER);
            turn_on_assay_LED_for_duration(param1, param2);
            result = param1;
            break;
        case 31: // turn on control 1 LED for param1 milliseconds at power param2
            indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
            indx = get_next_command_param(arg, indx, &param2, LED_DEFAULT_POWER);
            turn_on_control_1_LED_for_duration(param1, param2);
            result = param1;
            break;
        case 32: // turn on control 2 LED for param1 milliseconds at power param2
            indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
            indx = get_next_command_param(arg, indx, &param2, LED_DEFAULT_POWER);
            turn_on_control_2_LED_for_duration(param1, param2);
            result = param1;
            break;
        case 33: // turn on all LEDs for param1 milliseconds at power param2
            indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
            indx = get_next_command_param(arg, indx, &param2, LED_DEFAULT_POWER);
            turn_on_all_LEDs_for_duration(param1, param2);
            result = param1;
            break;
//
//  BUZZER
//
        case 40: // turn on buzzer param1 duration param2 frequency
            indx = get_next_command_param(arg, indx, &param1, BUZZER_FREQUENCY);
            indx = get_next_command_param(arg, indx, &param2, BUZZER_DURATION);
            turn_on_buzzer_for_duration(param1, param2);
            result = param1;
            break;
        case 41: // turn on alert buzzer
            turn_on_buzzer_alert();
            result = 1;
            break;
        case 42: // turn on problem buzzer
            turn_on_buzzer_problem();
            result = 1;
            break;
        case 43: // turn off buzzer
            turn_off_buzzer_timer();
            result = 1;
            break;
//
//  HEATER
//
        case 50: // read heater temperature
            Log.info("Heater: T = %d.%d˚C", heater.temp_C_10X / 10, heater.temp_C_10X % 10);
            result = heater.temp_C_10X;
            break;
        case 51: // turn on heater at power param1
            indx = get_next_command_param(arg, indx, &param1, HEATER_DEFAULT_POWER);
            turn_on_heater(param1);
            result = param1;
            break;
        case 52: // turn off heater
            turn_off_heater();
            result = 1;
            break;
        case 53: // set heater target temperature
            indx = get_next_command_param(arg, indx, &param1, HEATER_DEFAULT_TEMP_TARGET);
            if (param1 > 0 && param1 < HEATER_MAX_TEMPERATURE)
            {
                heater.target_C_10X = param1;
                heater.read_time = 0;
            }
            result = param1;
            break;
        case 54: // start temperature control
            start_temperature_control();
            result = 1;
            break;
        case 55: // stop temperature control
            stop_temperature_control();
            result = 1;
            break;
//
//  BARCODE SCANNER
//
        case 60: // scan barcode
            result = scan_barcode();
            break;
//
//  MAGNETOMETER
//
        case 70: // validate magnets
            result = validate_magnets();
            break;
        case 71: // set initial heating delay
            indx = get_next_command_param(arg, indx, &param1, MAGNETOMETER_INITIAL_HEATING_DELAY);
            magnetometer_initial_heating_delay = param1;
            break;
        case 72: // set initial heating delay
            indx = get_next_command_param(arg, indx, &param1, MAGNETOMETER_HEATING_DELAY);
            magnetometer_heating_delay = param1;
            break;
//
//  OPTICAL SENSORS
//
        case 80: // read optical sensors param1 times at interval param2 after moving to read position
            reset_stage(false);
            move_stage_to_optical_read_position();
            indx = get_next_command_param(arg, indx, &param1, OPTICAL_TEST_DEFAULT_READINGS);
            indx = get_next_command_param(arg, indx, &param2, ASYNC_COMMAND_DEFAULT_INTERVAL);
            result = start_optical_sensor_test(0, param1, param2);
            break;
        case 81: // read optical sensors param1 times at interval param2 at current location
            indx = get_next_command_param(arg, indx, &param1, OPTICAL_TEST_DEFAULT_READINGS);
            indx = get_next_command_param(arg, indx, &param2, ASYNC_COMMAND_DEFAULT_INTERVAL);
            result = start_optical_sensor_test(0, param1, param2);
            break;
        case 82: // start optical sensor sweep test, param1 = distance, param2 = readings, param3 = delay between readings
            test.number_of_readings = 0;
            indx = get_next_command_param(arg, indx, &param1, STAGE_OPTICAL_SENSOR_READ_POSITION);
            indx = get_next_command_param(arg, indx, &param2, OPTICAL_TEST_DEFAULT_DISTANCE);
            indx = get_next_command_param(arg, indx, &param3, OPTICAL_TEST_DEFAULT_READINGS);
            indx = get_next_command_param(arg, indx, &param4, ASYNC_COMMAND_DEFAULT_INTERVAL);
            reset_stage(false);
            move_stage_to_position(param1 - (param2 * param3), MOTOR_SLOW_STEP_DELAY);
            param3 *= 2;
            result = start_optical_sensor_test(param2, param3, param4);
            break;
        case 83: // find optical baseline LED power for each channel
            reset_stage(false);
            if (enable_optical_system(true))
            {
                set_baselines();
                read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, false);
            }
            disable_optical_system();
            reset_stage(true);
            result = 1;
            break;
//
//  STRESS TEST
//
        case 90: // start stress test, up to param1 cycles (0 means no limit)
            eeprom.stress_test_cycles = 0;
            store_eeprom();
            indx = get_next_command_param(arg, indx, &param1, 25);
            stress_test_limit = param1;
            stress_test_step = 0;
            async_command_stress_test_running = true;
            async_command_running = true;
            result = 1;
            break;
        case 91: // stop stress test
            async_command_stress_test_running = false;
            async_command_running = true;
            result = eeprom.stress_test_cycles;
            break;
//
//  VALIDATION
//
        case 100: // validate magnets
            result = validate_magnets();
            break;
        case 110: // start validate temperature
            start_temperature_validation();
            result = 0;
            break;
        case 111: // stop validate temperature
            stop_temperature_validation();
            result = 1;
            break;
        case 120: // validate optics
            validate_optics();
            result = 1;
            break;
//
//  BLUETOOTH LE
//
        case 200: // scan BLE
            result = BLE_scan();
            if (result > 0) {
                Log.info("%d devices found", result);
            }
            break;
//
//  ASYNC COMMAND
//
        case 999: // stop async command
            async_command_timer.stop();
            async_command_running = false;
            result = 1;
            break;
        default:
            result = 0;
    }

    Log.info("Completed command: %d, result: %d, p1: %d, p2: %d, p3: %d, p4: %d, p5: %d", cmd, result, param1, param2, param3, param4, param5);

    return result;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                       VALIDATION                        //
//                                                         //
/////////////////////////////////////////////////////////////

void start_temperature_validation() {
    temperature_validation_in_progress = true;
}

void stop_temperature_validation() {
    temperature_validation_in_progress = false;
    temperature_validation_mode = false;
}

void start_optical_validation() {
    optical_validation_in_progress = true;
}

void stop_optical_validation() {
    optical_validation_in_progress = false;
    optical_validation_mode = false;
}


/////////////////////////////////////////////////////////////
//                                                         //
//                           TESTS                         //
//                                                         //
/////////////////////////////////////////////////////////////


void reset_globals()
{
    test_underway = false;
    cartridge_validated = false;
    callback_complete = false;
    optical_read_in_progress = false;

    test_progress = 0;
    test_percent_complete = 0;

    barcode_uuid[0] = '\0';
    barcode_uuid[BARCODE_UUID_LENGTH] = '\0';
    test.cartridge_uuid[0] = '\0';
    test.cartridge_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
    assay.uuid[0] = '\0';
    assay.uuid[ASSAY_UUID_LENGTH] = '\0';
    test.number_of_readings = 0;

    particle_register[0] = '\0';
    particle_register[PARTICLE_REGISTER_SIZE] = '\0';
}

void disconnect_from_cloud() {
    Log.info("Disconnecting from cloud...");
    while (Particle.connected()) {
        Particle.disconnect();
        delay(PARTICLE_CLOUD_DELAY);
    }
    Log.info("Disconnected from cloud");
}

void reconnect_to_cloud() {
    Log.info("Reconnecting to cloud...");
    Particle.connect();
    delay(PARTICLE_CLOUD_DELAY);
    while (!Particle.connected()) {
        Particle.connect();
        delay(PARTICLE_CLOUD_DELAY);
    }
    Log.info("Reconnected to cloud");
}

void run_test()
{
    reset_stage(false);
    move_stage_to_test_start_position();
    turn_on_buzzer_for_duration(1000, 600);
    delay(2000);

    update_progress("Running test", 0);

    disconnect_from_cloud();
    stop_temperature_control();

    SINGLE_THREADED_BLOCK()
    {
        memcpy(eeprom.running_test_uuid, test.cartridge_uuid, CARTRIDGE_UUID_LENGTH);
        store_eeprom();
        process_BCODE(0);
        write_test_record_to_eeprom();
    }

    start_temperature_control();

    test_underway = false;
    test_upload_mode = true;

    reconnect_to_cloud();
    pubsub_upload_test();

    reset_stage(true);
    reset_globals();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                          SETUP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void clear_state() {
    barcode_scan_mode = false;
    cartridge_validation_in_progress = false;
    test_start_in_progress = false;
    test_upload_in_progress = false;

    cartridge_inserted = false;
    magnetometer_inserted = false;
    temperature_probe_inserted = false;
    optical_probe_inserted = false;

    barcode_scan_mode = false;
    cartridge_validation_mode = false;
    test_start_mode = false;
    test_underway = false;
    test_upload_mode = test_in_cache();
    magnetometer_validation_mode = false;
    temperature_validation_mode = false;
    optical_validation_mode = false;
    
    test_invalid = false;
    barcode_invalid = false;
    cartridge_validated = false;
    cartridge_inserted = false;
}

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

void startup_device()
{
    i2c_bus_scan();

    Log.info("Resetting stage");
    reset_stage(true);

    Log.info("Testing LEDs");
    // turn_on_all_LEDs(LED_DEFAULT_POWER);
    turn_on_assay_LED_for_duration(500, LED_DEFAULT_POWER);
    delay(500);
    turn_on_control_1_LED_for_duration(500, LED_DEFAULT_POWER);
    delay(500);
    turn_on_control_2_LED_for_duration(500, LED_DEFAULT_POWER);

    Log.info("Buzzing");
    turn_on_buzzer_for_duration(250, 330);

    reset_globals();
    clear_current_event();

    clear_state();

    bool test_interrupted = eeprom.running_test_uuid[0] != '\0';

    Log.info("device id: %s", device_id.c_str());
    Log.info("eeprom.firmware_version: %d", eeprom.firmware_version);
    Log.info("eeprom.data_format_version: %d", eeprom.data_format_version);
    Log.info("eeprom.maximum_stress_test_cycles: %d", eeprom.maximum_stress_test_cycles);
    Log.info("Interrupted test ? %c", test_interrupted ? 'Y' : 'N');    
    Log.info("Cached test ? %c", test_cached() ? 'Y' : 'N');    

    if (test_interrupted) {
        memcpy(eeprom.cache.cartridge_uuid, eeprom.running_test_uuid, CARTRIDGE_UUID_LENGTH);
        eeprom.cache.number_of_readings = 0;
        memset(eeprom.running_test_uuid, 0, CARTRIDGE_UUID_LENGTH);
        store_eeprom();
        test_upload_mode = true;
    }

    device_starting_up = false;
}

void setup() {
    init_digital_pin(pinStageLimit, INPUT_PULLUP, 0);
    init_digital_pin(pinCartridgeDetected, INPUT_PULLUP, 0);

    init_digital_pin(pinBarcodeTrigger, OUTPUT, HIGH);
    init_digital_pin(pinBarcodeReady, INPUT, 0);

    init_analog_pin(pinLEDAssay, OUTPUT, 0);
    init_analog_pin(pinLEDControl1, OUTPUT, 0);
    init_analog_pin(pinLEDControl2, OUTPUT, 0);

    init_analog_pin(pinHeaterThermistor, INPUT, 0);
    init_analog_pin(pinHeater, OUTPUT, 0);

    init_digital_pin(pinMotorSleep, OUTPUT, LOW);
    init_digital_pin(pinMotorStep, OUTPUT, LOW);
    init_digital_pin(pinMotorDir, OUTPUT, LOW);
    init_digital_pin(pinMotorReset, OUTPUT, HIGH);

    init_analog_pin(pinBuzzer, OUTPUT, 0);

    while (!Particle.connected()) {
        delay(PARTICLE_CLOUD_DELAY);
    }

    indicatorBusy.setActive(true);

    device_id = System.deviceID();
    Particle.subscribe(String(device_id + "/hook-response/" + PUBSUB_EVENT_NAME + "/"), brevitest_callback, MY_DEVICES);
    Particle.subscribe(String(device_id + "/hook-error/" + PUBSUB_EVENT_NAME + "/"), brevitest_error, MY_DEVICES);

    setup_eeprom();

    Serial.begin(115200); // standard serial port

    attachInterrupt(pinCartridgeDetected, detector_changed_interrupt, CHANGE);
    detector_on = digitalRead(pinCartridgeDetected) == LOW;
    if (detector_on) {
        barcode_invalid = true;
    }

    start_temperature_control();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                   DEVICE INDICATORS                     //
//                                                         //
/////////////////////////////////////////////////////////////

bool heater_debounced() {
    if (previous_heater_ready != heater_ready) {
        heater_debouncing_in_progress = true;
        heater_debounce_time = millis() + HEATER_READY_DEBOUNCE_DELAY;
    } else if (heater_debouncing_in_progress) {
        heater_debouncing_in_progress = millis() < heater_debounce_time;
    } else if (heater_ready) {
        return true;
    }
    return false;
}

void turn_on_ready_indicator(bool force) {
    if (force) {
        turn_on_available_LED();
    } else if (heater_debounced()) {
        turn_on_available_LED();
    } else {
        turn_on_busy_LED();
    }
}

void set_device_indicators()
{
    previous_heater_ready = heater_ready;
    heater_ready = (heater.target_C_10X - heater.temp_C_10X) < HEATER_READY_TEMP_DELTA;

    if (!Particle.connected()) {
        turn_off_indicator_LEDs();
    } else if (test_invalid) {
        turn_on_problem_LED();
        turn_on_buzzer_problem();
    } else if (async_command_running) {
        turn_on_async_LED();
    } else if (barcode_scan_mode || cartridge_validation_mode || test_start_mode || test_underway || test_upload_mode) {
        turn_on_busy_LED();
    } else if (device_starting_up || magnetometer_validation_mode || temperature_validation_mode || optical_validation_mode) {
        if (heater_debounced()) {
            turn_on_validation_LED();
        } else {
            turn_on_busy_LED();
        }
    } else if (barcode_invalid) {
        turn_on_ready_indicator(true);
        turn_on_buzzer_alert();
    } else if (cartridge_inserted || magnetometer_inserted || optical_probe_inserted) {
        if (cartridge_validated) {
            turn_on_busy_LED();
            if (test_completed || test_cancelled) {
                turn_on_buzzer_alert();
            }
        } else {
            turn_on_buzzer_alert();
            turn_on_ready_indicator(true);
        }
    } else {
        turn_on_ready_indicator(false);
        turn_off_buzzer_timer();
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void verify_device_loop()
{
    barcode_scan_loop();
    if (magnetometer_validation_mode) {
        validate_magnets();
    } else if (temperature_validation_mode) {
        start_temperature_validation();
    } else if (optical_validation_mode) {
        validate_optics();
    } else if (device_verification_in_progress) {
        if (callback_complete) {
            process_callback_buffer();
        } else if (millis() > callback_timeout) {
            Log.info("Device verification timed out. Retrying.");
            pubsub_verify_device();
        }
    } else if (heater_debounced()) {
        pubsub_verify_device();
    }
}

void barcode_scan_loop() {
    if (detector_on) {
        if (barcode_scan_mode) {
            barcode_scan_in_progress = true;
            cartridge_inserted = cartridge_validation_mode = false;
            magnetometer_inserted = magnetometer_validation_mode = false;
            temperature_probe_inserted = temperature_validation_mode = false;
            optical_probe_inserted = optical_validation_mode = false;
            switch (scan_barcode()) {
                case BARCODE_TYPE_CARTRIDGE:
                    cartridge_inserted = true;
                    cartridge_validation_mode = true;
                    Log.info("Cartridge inserted");
                    break;
                case BARCODE_TYPE_MAGNETOMETER:
                    magnetometer_inserted = true;
                    magnetometer_validation_mode = true;
                    Log.info("Magnetometer inserted");
                    break;
                case BARCODE_TYPE_TEMPERATURE:
                    temperature_probe_inserted = true;
                    temperature_validation_mode = true;
                    Log.info("Temperature probe inserted");
                    break;
                case BARCODE_TYPE_OPTICAL:
                    optical_probe_inserted = true;
                    optical_validation_mode = true;
                    Log.info("Optical probe inserted");
                    break;
                case BARCODE_TYPE_SHIPPING:
                    // wake_motor();
                    move_stage_to_position(STAGE_SHIPPING_BOLT_LOCATION, MOTOR_SLOW_STEP_DELAY);
                    Log.info("Ready to insert shipping bolt");
                    break;
                default:
                    barcode_invalid = true;
                    Log.info("Unknown barcode format");
            }
            barcode_scan_in_progress = false;
            barcode_scan_mode = false;
        }
    }
}

void cartridge_validation_loop() {
    if (cartridge_validation_in_progress) {
        if (millis() > callback_timeout) {
            Log.info("Cartridge validation timed out. Retrying.");
            pubsub_validate_cartridge();
        }
    } else {
        pubsub_validate_cartridge();
    }
}

void test_start_loop() {
    if (test_start_in_progress) {
        if (millis() > callback_timeout) {
            Log.info("Test start timed out. Retrying.");
            pubsub_start_test();
        }
    } else {
        pubsub_start_test();
    }
}

void test_upload_loop() {
    if (test_upload_in_progress) {
        if (millis() > callback_timeout) {
            Log.info("Test upload timed out. Retrying.");
            pubsub_upload_test();
        }
    } else {
        pubsub_upload_test();
    }
}

void async_command_loop() {
    if (async_command_stress_test_running) {
        do_stress_test_step(stress_test_step);
        stress_test_step++;
    } else if (async_command_optical_running && optical_test_take_reading) {
        optical_test_take_reading = false;
        Log.info("Stage location: %d", stage_position);
        if (enable_optical_system(true)) {
            get_data_from_one_optical_sensor('A', OPTICAL_SENSOR_DEFAULT_PARAM, baseline.led_assay, true);
            get_data_from_one_optical_sensor('1', OPTICAL_SENSOR_DEFAULT_PARAM, baseline.led_c1, true);
            get_data_from_one_optical_sensor('2', OPTICAL_SENSOR_DEFAULT_PARAM, baseline.led_c2, true);
        }
        disable_optical_system();
        if (optical_test_move) {
            move_stage(optical_test_move, MOTOR_SLOW_STEP_DELAY);
        }
        optical_test_count++;
        if (optical_test_count >= optical_test_readings) {
            async_command_optical_running = false;
            async_command_running = false;
            async_command_timer.stop();
            sleep_motor();
        }
    } else if (async_command_running && !async_command_stress_test_running) {
        async_command_running = false;
        async_command_timer.stop();
        sleep_motor();
    }
}

void hardware_loop() {
    if (detector_changed) {
        if (detector_debouncing) {
            detector_debouncing = false;
            detector_changed = false;
            detector_on = digitalRead(pinCartridgeDetected) == LOW;
            Log.info("%s detected", detector_on ? "Insertion" : "Removal");
            if (detector_on) {
                reset_stage(false);
                move_stage_to_test_start_position();
                sleep_motor();
                if (heater_ready) {
                    turn_on_buzzer_for_duration(BUZZER_INSERT_DURATION, BUZZER_INSERT_FREQUENCY);
                    turn_on_busy_LED();
                    barcode_scan_mode = true;
                    barcode_invalid = false;
                } else {
                    barcode_invalid = true;
                }
            } else {
                turn_on_buzzer_for_duration(BUZZER_REMOVE_DURATION, BUZZER_REMOVE_FREQUENCY);
                turn_off_buzzer_timer();
                clear_state();
                reset_stage(true);
            }
        } else {
            delay(50);
            detector_debouncing = true;
        }
    }

    if (control_heater_temperature_flag) {
        control_heater_temperature_flag = false;
        set_heater_power(pid_controller());
    }

    set_device_indicators();

    if (start_problem_buzzer) {
        start_problem_buzzer = false;
        turn_on_buzzer_for_duration(BUZZER_PROBLEM_DURATION, BUZZER_PROBLEM_FREQUENCY);
    }
    else if (start_alert_buzzer) {
        start_alert_buzzer = false;
        turn_on_buzzer_for_duration(BUZZER_ALERT_DURATION, BUZZER_ALERT_FREQUENCY);
    }
}

void process_serial_port() {
    if (Serial.available()) {
        char c = Serial.read();
        serial_buffer[serial_buffer_index] = c;    
        serial_buffer_index++;
        serial_buffer_index %= SERIAL_COMMAND_BUFFER_SIZE;
        if (c == '\n') {
            serial_buffer[serial_buffer_index] = '\0';
            Log.info(serial_buffer);
            serial_buffer_index = 0;
            particle_command(String(serial_buffer));
        }
    }
}

void loop()
{
    process_serial_port();
    hardware_loop();
    
    if (async_command_running) {
        async_command_loop();
    } else if (device_starting_up) {
        verify_device_loop();
    } else if (callback_complete) {
        process_callback_buffer();
    } else if (test_upload_mode) {
        test_upload_loop();
    } else if (test_start_mode) {
        test_start_loop();
    } else if (cartridge_validation_mode) {
        cartridge_validation_loop();
    } else if (barcode_scan_mode) {
        barcode_scan_loop();
    } else if (magnetometer_validation_mode) {
        validate_magnets();
    } else if (temperature_validation_mode) {
        start_temperature_validation();
    } else if (optical_validation_mode) {
        validate_optics();
    }

    delay(100);
}
