/*
 * Project brevitest_v1_0
 * Description: firmware for Acuity™ Sample Processing Unit, part of the Brevitest™ Platform
 * Author: Leo Linbeck III
 * Date: December 2023
 */

#include "brevitest-firmware.h"
#include "DFRobot_AS7341.h"

PRODUCT_VERSION(FIRMWARE_VERSION);
SYSTEM_MODE(AUTOMATIC);

/////////////////////////////////////////////////////////////
//                                                         //
//                        TABLES                           //
//                                                         //
/////////////////////////////////////////////////////////////

//  temperature is 10x to get one decimal place of accuracy
static int table_temperature[] = {1000, 950, 900, 850, 800, 750, 700, 650, 600, 550, 500, 450, 400, 350, 300, 250, 200, 150, 100, 50, 0};
static int table_raw[] = {2438, 2290, 2136, 1977, 1814, 1651, 1488, 1329, 1174, 1028, 890, 763, 647, 544, 452, 372, 304, 245, 196, 155, 122};

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
            if (serial_messaging_on)
                Log.info("Table: indx1 = %d, indx2 = %d, raw = %d, raw1 = %d, temp1 = %d, temp2 = %d, result = %d", indx1, indx2, raw, table_raw[indx1], table_temperature[indx1], table_temperature[indx2], result);
            return result;
        }
    }

    return 0;
}

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

/////////////////////////////////////////////////////////////
//                                                         //
//                        UTLITY                           //
//                                                         //
/////////////////////////////////////////////////////////////

int limit(int value, int max, int min)
{
    return value > max ? max : (value < min ? min : value);
}

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

    for (int i = 0; i < 14; i++)
    {
        stop = str[*indx] == delim;
        buf[i] = stop ? '\0' : str[*indx];
        (*indx)++;
        if (stop)
            break;
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

int set_wifi_credentials(String params)
{
    String ssid, password;
    int indx = params.indexOf(":::");
    ssid = params.substring(0, indx);
    password = params.substring(indx + 3);
    Log.info("set_wifi_credentials, ssid = %s, password = %s", ssid.c_str(), password.c_str());
    WiFi.setCredentials(ssid, password);
    WiFi.connect();
    return 0;
}

////////////////////////////////////////////////////////////
//                                                         //
//                         EEPROM                          //
//                                                         //
/////////////////////////////////////////////////////////////

void reset_eeprom()
{
    Particle_EEPROM e;

    Log.info("reset_eeprom");
    int lifetime = eeprom.lifetime_stress_test_cycles;
    if (lifetime == -1)
    {
        lifetime = 0;
    }
    EEPROM.clear();
    memcpy(&eeprom, &e, (int)sizeof(Particle_EEPROM));
    eeprom.lifetime_stress_test_cycles = lifetime;
    EEPROM.put(0, eeprom);
}

void setup_eeprom()
{
    int addr = 0;
    uint8_t value;

    EEPROM.get(addr, value);
    if (value == 0xFF)
    {
        reset_eeprom();
    }
    else
    {
        EEPROM.get(0, eeprom);
        if (eeprom.firmware_version != FIRMWARE_VERSION || eeprom.data_format_version != DATA_FORMAT_VERSION)
        {
            reset_eeprom();
        }
    }
}

////////////////////////////////////////////////////////////
//                                                         //
//                      FILE SYSTEM                        //
//                                                         //
/////////////////////////////////////////////////////////////

bool create_dir_if_not_exists(const char *path)
{
    struct stat statbuf;

    int result = stat(path, &statbuf);
    if (result == 0)
    {
        if ((statbuf.st_mode & S_IFDIR) != 0)
        {
            Log.info("%s exists and is a directory", path);
            return true;
        }

        Log.error("file in the way, deleting %s", path);
        unlink(path);
    }
    else
    {
        if (errno != ENOENT)
        {
            // Error other than file does not exist
            Log.error("stat filed errno=%d", errno);
            return false;
        }
    }

    // File does not exist (errno == 2)
    result = mkdir(path, 0777);
    if (result == 0)
    {
        Log.info("created dir %s", path);
        return true;
    }
    else
    {
        Log.error("mkdir failed errno=%d", errno);
        return false;
    }
}

void write_test_to_file()
{
    event.name("upload-test");
    event.data((char *)&test, sizeof(test), ContentType::BINARY);
    String filename = "/cache/" + String(test.cartridge_id);
    event.saveData(filename);
}

void clear_cache() {
    int tries = 500;
    while (test_in_cache() && --tries > 0) {
        Log.info("Clearing cache: %s", cached_filename);
        unlink(cached_filename);
    }
}

bool test_in_cache()
{
    if (cached_filename[0] != '\0')
    {
        Log.info("Cached filename: %s", cached_filename);
        return true;
    }
    DIR *cache = opendir("/cache");
    int tries = 10;
    do
    {
        cache_entry = readdir(cache);
        if (cache_entry == NULL)
        {
            break;
        }
        if (cache_entry->d_type != DT_REG)
        {
            continue;
        }   
        if (strlen(cache_entry->d_name) == BARCODE_UUID_LENGTH)
        {
            snprintf(cached_filename, sizeof(cached_filename), "/cache/%s", cache_entry->d_name);
            Log.info("Cached filename: %s", cached_filename);
            break;
        }
    } while (cache_entry != NULL && --tries > 0);
    closedir(cache);
    return cached_filename[0] != '\0';
}

/////////////////////////////////////////////////////////////
//                                                         //
//                       DETECTOR                          //
//                                                         //
/////////////////////////////////////////////////////////////

void detector_changed_interrupt()
{
    detector_changed = true;
    // detector_changed = false; // uncomment to deactive cartride detection
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
    delayMicroseconds(10000);
    motor_awake = true;
}

bool move_one_eighth_step(int dir, int step_delay)
{
    if (dir == HIGH)
    {
        if (digitalRead(pinStageLimit) == LOW)
        {
            delayMicroseconds(10000);
            if (digitalRead(pinStageLimit) == LOW)
            {
                stage_position = 0;
                microns_error = 0;
                return false;
            }
        }
        if (stage_position <= 0)
        {
            stage_position = 0;
            microns_error = 0;
            return false;
        }
    }
    else if (stage_position >= STAGE_POSITION_LIMIT)
    {
        return false;
    }

    // Log.info("move_one_eighth_step, dir = %d, step_delay = %d, limit = %ld, position = %d", dir, step_delay, digitalRead(pinStageLimit), stage_position);

    digitalWrite(pinMotorStep, HIGH);
    delayMicroseconds(step_delay);
    digitalWrite(pinMotorStep, LOW);
    delayMicroseconds(step_delay);
    stage_position += dir == HIGH ? -MOTOR_MICRONS_PER_EIGHTH_STEP : MOTOR_MICRONS_PER_EIGHTH_STEP;
    if (stage_position <= 0)
    {
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

    if (!motor_awake)
    {
        wake_motor();
    }
    dir = (microns < 0) ? HIGH : LOW;
    digitalWrite(pinMotorDir, dir);

    abs_microns = abs(microns) + microns_error;
    eighth_steps = abs_microns / MOTOR_MICRONS_PER_EIGHTH_STEP;
    microns_error = abs_microns % MOTOR_MICRONS_PER_EIGHTH_STEP;
    floored_step_delay = step_delay < MOTOR_MINIMUM_STEP_DELAY ? MOTOR_MINIMUM_STEP_DELAY : step_delay;

    // Log.info("move_stage, microns = %d, step_delay = %d, floored_step_delay = %d, abs_microns = %d, eighth_steps = %d, microns_error = %d, dir = %d", microns, step_delay, floored_step_delay, abs_microns, eighth_steps, microns_error, dir);
    for (i = 0; i < eighth_steps; i++)
    {
        if (!move_one_eighth_step(dir, floored_step_delay))
        {
            break;
        }
    }
}

void move_stage_until_proximal_limit(int step_delay)
{
    int count = STAGE_POSITION_LIMIT / MOTOR_MICRONS_PER_EIGHTH_STEP + 40;
    digitalWrite(pinMotorDir, HIGH);
    while (digitalRead(pinStageLimit) == HIGH && count-- > 0)
    {
        digitalWrite(pinMotorStep, HIGH);
        delayMicroseconds(step_delay);

        digitalWrite(pinMotorStep, LOW);
        delayMicroseconds(step_delay);

        stage_position -= MOTOR_MICRONS_PER_EIGHTH_STEP;
    }
    stage_position = 0;
    microns_error = 0;
}

void reset_stage(bool sleep)
{
    if (!motor_awake)
    {
        wake_motor();
    }
    move_stage_until_proximal_limit(MOTOR_RESET_STEP_DELAY);
    move_stage(STAGE_MICRONS_TO_INITIAL_POSITION, MOTOR_RESET_STEP_DELAY);
    if (sleep)
    {
        sleep_motor();
    }
}

void move_stage_to_optical_read_position()
{
    move_stage_to_position(SPECTRO_STARTING_STAGE_POSITION, MOTOR_SLOW_STEP_DELAY);
}

void move_stage_to_test_start_position()
{
    move_stage_to_position(STAGE_MICRONS_TO_TEST_START_POSITION, MOTOR_SLOW_STEP_DELAY);
}

void move_stage_to_position(int position, int step_delay)
{
    int move_distance = position - stage_position;
    if (move_distance != 0)
    {
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
        if (inBCODE)
            BCODE_loop();
        move_stage(-amplitude, step_delay);
        if (inBCODE)
            BCODE_loop();
        if (test_cancelled)
            return;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                  BARCODE SCANNER                        //
//                                                         //
/////////////////////////////////////////////////////////////

int scan_barcode()
{
    int buf;               // a little buffer for reading barcode (we will coerce into character)
    int i = 0;             // index for reading barcode from serial port into barcode_uuid
    bool success;          // set to true if barcode read is successful
    unsigned long timeout; // keep this from taking too long

    Serial1.begin(9600);                       // barcode scanner interface through RX/TX pins
    barcode_uuid[0] = '\0';                    // reset barcode_uuid
    timeout = millis() + BARCODE_READ_TIMEOUT; // set timeout for overall process

    digitalWrite(pinBarcodeTrigger, LOW); // start read by pulling trigger pin low

    while (digitalRead(pinBarcodeReady) == LOW && millis() < timeout)
    { // if read is not complete or timed out, wait and check again
        delay(100);
    };
    success = digitalRead(pinBarcodeReady) == HIGH; // successful if read is completed before timeout
    digitalWrite(pinBarcodeTrigger, HIGH);          // stop read by setting trigger pin back to high

    if (success)
    {
        while (!Serial1.available() && millis() < timeout)
        { // if data buffer is empty, wait and check again
            delay(100);
        };
        delay(100); // allow barcode buffer to fill before reading
        do
        {
            buf = Serial1.read(); // read a byte of data (returns -1 if no data is available)
            if (buf != -1)
            {
                barcode_uuid[i++] = (char)buf; // coerce byte to character and append to barcode_uuid
            }
        } while (Serial1.available() && i <= BARCODE_UUID_LENGTH); // continue while data is available and there's no overflow
        barcode_uuid[--i] = '\0'; //
    }
    else
    {
        Serial.println("Timeout - read failure");
    }

    Serial1.end(); // close serial port to barcode scanner

    switch (i)
    {                         // find out what this barcode is
    case BARCODE_UUID_LENGTH: // is the barcode a test cartridge?
        return BARCODE_TYPE_CARTRIDGE;
        break;
    case MAGNETOMETER_UUID_LENGTH: // is the barcode a validation cartridge? if so, check the validation prefix
        if (strncmp(barcode_uuid, MAGNETOMETER_PREFIX, BARCODE_PREFIX_LENGTH) == 0)
        { // is it a magnetometer?
            return BARCODE_TYPE_MAGNETOMETER;
        }
        break;
    case STRESS_TEST_UUID_LENGTH:
        if (strncmp(barcode_uuid, STRESS_TEST_PREFIX, STRESS_TEST_PREFIX_LENGTH) == 0)
        { // is it a stress test cartridge?
            return BARCODE_TYPE_STRESS_TEST;
        }
        break;
    }
    Log.info("Barcode error read: %s, length: %d", barcode_uuid, i);
    strcpy(barcode_uuid, BARCODE_ERROR_MESSAGE); // replace whatever is there with an error message
    barcode_uuid[BARCODE_ERROR_MESSAGE_LENGTH] = '\0';
    return BARCODE_TYPE_VALIDATION_ERROR;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        BUZZER                           //
//                                                         //
/////////////////////////////////////////////////////////////

void turn_on_buzzer_for_duration(int duration, int frequency)
{
    bool reheat = false;
    if (heater.heater_on)
    {
        turn_off_heater();
        reheat = true;
    }
    tone(pinBuzzer, frequency, duration);
    delay(duration);
    if (reheat)
    {
        turn_on_heater(heater.power);
    }
}

void check_buzzer()
{
    if (buzzer_problem_running)
    {
        start_problem_buzzer = true;
    }
    else if (buzzer_alert_running)
    {
        start_alert_buzzer = true;
    }
}

void turn_on_buzzer_alert()
{
    if (!buzzer_alert_running)
    {
        buzzer_problem_running = false;
        start_problem_buzzer = false;
        buzzer_alert_running = true;
        start_alert_buzzer = true;
        buzzer_timer.changePeriod(BUZZER_ALERT_PERIOD);
        buzzer_timer.reset();
    }
}

void turn_on_buzzer_problem()
{
    if (!buzzer_problem_running)
    {
        buzzer_problem_running = true;
        start_problem_buzzer = true;
        buzzer_alert_running = false;
        start_alert_buzzer = false;
        buzzer_timer.changePeriod(BUZZER_PROBLEM_PERIOD);
        buzzer_timer.reset();
    }
}

void turn_off_buzzer_timer()
{
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

void turn_off_indicator_LEDs()
{
    if (indicatorRemove.isActive())
        indicatorRemove.setActive(false);
    if (indicatorDontTouch.isActive())
        indicatorDontTouch.setActive(false);
    if (indicatorInsert.isActive())
        indicatorInsert.setActive(false);
}

void turn_on_remove_cartridge_LED()
{
    if (!indicatorRemove.isActive())
    {
        turn_off_indicator_LEDs();
        indicatorRemove.setActive(true);
    }
}

void turn_on_dont_touch_LED()
{
    if (!indicatorDontTouch.isActive())
    {
        turn_off_indicator_LEDs();
        indicatorDontTouch.setActive(true);
    }
}

void turn_on_insert_cartridge_LED()
{
    if (!indicatorInsert.isActive())
    {
        turn_off_indicator_LEDs();
        indicatorInsert.setActive(true);
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
    digitalWrite(heater.heater_pin, HIGH);
    delay(power);
    digitalWrite(heater.heater_pin, LOW);
    heater.power = power;
    heater.heater_on = true;
}

void turn_off_heater()
{
    digitalWrite(heater.heater_pin, LOW);
    heater.heater_on = false;
    heater.power = 0;
}

int set_heater_power(int power)
{
    unsigned long start = millis();
    if (power != 0)
    {
        if (!spectrophotometer_read_in_progress)
            turn_on_heater(power);
    }
    else
    {
        turn_off_heater();
    }

    return (int)(millis() - start);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                     MAGNETOMETER                        //
//                                                         //
/////////////////////////////////////////////////////////////

void scanResultCallback(const BleScanResult &scanResult, void *context)
{

    String name = scanResult.advertisingData().deviceName();
    if (name.length() > 0)
    {
        Log.info("Advertising name: %s", name.c_str());
    }

    uint8_t data[27];
    char *id = (char *)&data[2];
    if (scanResult.scanResponse().customData(data, 26))
    {
        *(id + 24) = '\0';
        Log.info("Device ID: %s", id);
        if (strncmp(id, &barcode_uuid[8], 24) == 0 && strncmp(name, "Magnetometer", 12) == 0)
        {
            magnetometer_found = true;
            magnetometer_address = scanResult.address();
            Log.info("Barcode matched. MAC: %02X:%02X:%02X:%02X:%02X:%02X | RSSI: %ddBm",
                     magnetometer_address[0], magnetometer_address[1], magnetometer_address[2],
                     magnetometer_address[3], magnetometer_address[4], magnetometer_address[5], scanResult.rssi());
            BLE.stopScanning();
        }
    }
}

int BLE_scan()
{
    int count = BLE.scan(scanResultCallback);
    if (count > 0)
    {
        Log.info("%d devices found", count);
    }
    return count;
}

int check_magnets_in_one_well(int well, int mark)
{
    BleCharacteristic characteristic;

    move_stage(well_move[well], MOTOR_SLOW_STEP_DELAY);
    delay(magnetometer_heating_delay);
    if (magnetometer.getCharacteristicByUUID(characteristic, bleCharUuid[well]))
    {
        String result;
        characteristic.getValue(result);
        int len = sprintf(&particle_register[mark], "%d\t%s\n", well + 1, result.c_str());
        return len + mark;
    }
    else
    {
        Log.info("Could not find magnetometer data for well %d", well + 1);
        return -1;
    }
}

int validate_magnets()
{
    magnet_validation_in_progress = true;
    magnetometer_found = false;
    BLE_scan();
    if (magnetometer_found)
    {
        Log.info("Magnetometer found, connecting...");
        magnetometer = BLE.connect(magnetometer_address);
        if (magnetometer.connected())
        {
            delay(magnetometer_initial_heating_delay);
            int mark = 32;
            Log.info("Connected to magnetometer");
            strncpy(particle_register, barcode_uuid, 32);
            particle_register[mark++] = '\n';
            reset_stage(false);
            move_stage_to_test_start_position();
            for (int i = 0; i < 5; i++)
            {
                mark = check_magnets_in_one_well(i, mark);
                if (mark > PARTICLE_REGISTER_SIZE || mark == -1)
                {
                    return 0;
                }
            }
            magnetometer.disconnect();
            reset_stage(true);
            particle_register[mark] = '\0';
            return 1;
        }
        else
        {
            Log.info("Could not connect to magnetometer %s", barcode_uuid);
            return 0;
        }
    }
    else
    {
        Log.info("Magnetometer not found");
        return 0;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                    LASER DIODES                         //
//                                                         //
/////////////////////////////////////////////////////////////

Laser *get_laser(char channel)
{
    if (channel == 'C')
    {
        return &laserC;
    }
    else if (channel == 'B')
    {
        return &laserB;
    }
    else
    {
        return &laserA;
    }
}

void turn_on_laser(char channel)
{
    Laser *laser = get_laser(channel);

    laser->power = 255;
    // analogWrite(laser->power_pin, laser->power, LASER_PWM_FREQUENCY);
    digitalWrite(laser->power_pin, HIGH);
    laser->power_on = true;
    // if (serial_messaging_on)
    // {
    // int value = analogRead(laser->value_pin);
    // }
}

void turn_off_laser(char channel)
{
    Laser *laser = get_laser(channel);

    // analogWrite(laser->power_pin, 0, LASER_PWM_FREQUENCY);
    digitalWrite(laser->power_pin, LOW);
    laser->power_on = false;
    laser->power = 0;
    // if (serial_messaging_on)
}

void turn_off_all_lasers()
{
    turn_off_laser('A');
    turn_off_laser('B');
    turn_off_laser('C');
}

void turn_on_laser_for_duration(char channel, int duration)
{
    turn_on_laser(channel);
    delayMicroseconds(1000 * duration);
    turn_off_laser(channel);
}

void turn_on_all_lasers_for_duration(int duration)
{
    turn_on_laser('A');
    turn_on_laser('B');
    turn_on_laser('C');
    delayMicroseconds(1000 * duration);
    turn_off_all_lasers();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                  SPECTROPHOTOMETERS                     //
//                                                         //
/////////////////////////////////////////////////////////////

void init_spectrophotometer_switch()
{
    Wire.beginTransmission(SPECTRO_SWITCH_ADDR);
    Wire.write(SPECTRO_SWITCH_CONFIG_COMMAND);
    Wire.write(SPECTRO_SWITCH_SET_PORTS);
    byte result = Wire.endTransmission();
    if (result != 0)
    {
        Log.info("Error initializing spectrophotometer power: %d", result);
    }
    else
    {
        Log.info("Spectrophotometer switch initialized");
    }
    delay(10);
}

void set_spectrophotometer_power(byte code)
{
    Wire.beginTransmission(SPECTRO_SWITCH_ADDR);
    Wire.write(SPECTRO_SWITCH_OUTPUT_COMMAND);
    Wire.write(code);
    byte result = Wire.endTransmission();
    if (result != 0)
    {
        Serial.printlnf("Error setting spectrophotometer power: %X", result);
    }
}

bool power_on_spectrophotometer(char channel)
{
    // Turn on sensor
    switch (channel)
    {
    case 'A':
        set_spectrophotometer_power(SPECTRO_SWITCH_TURN_ON_A);
        break;
    case 'B':
        set_spectrophotometer_power(SPECTRO_SWITCH_TURN_ON_B);
        break;
    case 'C':
        set_spectrophotometer_power(SPECTRO_SWITCH_TURN_ON_C);
        break;
    default:
        set_spectrophotometer_power(SPECTRO_SWITCH_TURN_OFF_ALL);
        return false;
    }
    delay(5); // Wait for sensor to power on.
    return true;
}

void power_off_all_spectrophotometers()
{
    // Turn off all sensors.
    set_spectrophotometer_power(SPECTRO_SWITCH_TURN_OFF_ALL);
}

bool reset_spectrophotometer(char channel, DFRobot_AS7341 *as7341)
{
    int count = 5;
    while (as7341->begin() != 0)
    {
        if (count-- < 0)
        {
            return false;
        }
        Serial.println("IIC init failed, please check if the wire connection is correct");
        delay(1000);
    }
    delay(2);
    return true;
}

bool init_spectrophotometer(char channel, DFRobot_AS7341 *as7341)
{
    if (reset_spectrophotometer(channel, as7341))
    {
        as7341->setAstep(test.astep);
        as7341->setAtime(test.atime);
        as7341->setAGAIN(test.again);
        return true;
    }
    else
    {
        return false;
    }
}

void spectroMeasure(char channel, DFRobot_AS7341 *as7341, DFRobot_AS7341::eChChoose_t mode, BrevitestSpectrophotometerReading *reading)
{
    unsigned long startTime = millis();

    as7341->startMeasure(mode);
    while (!as7341->measureComplete() && (millis() - startTime) < SPECTRO_TIMEOUT)
    {
        delayMicroseconds(100);
    }
    if (millis() - startTime < SPECTRO_TIMEOUT)
    {
        if (mode == as7341->eF1F4ClearNIR)
        {
            DFRobot_AS7341::sModeOneData_t data1;
            data1 = as7341->readSpectralDataOne();
            reading->f1 = data1.ADF1;
            reading->f2 = data1.ADF2;
            reading->f3 = data1.ADF3;
            reading->f4 = data1.ADF4;
            reading->clear = data1.ADCLEAR;
            reading->nir = data1.ADNIR;
        }
        else
        {
            DFRobot_AS7341::sModeTwoData_t data2;
            data2 = as7341->readSpectralDataTwo();
            reading->f5 = data2.ADF5;
            reading->f6 = data2.ADF6;
            reading->f7 = data2.ADF7;
            reading->f8 = data2.ADF8;
        }
    }
    else
    {
        Log.info("Spectral measurement timed out");
    }
}
void take_spectrophotometer_reading(char channel, DFRobot_AS7341 *as7341, BrevitestSpectrophotometerReading *reading)
{
    if (reading == NULL)
    {
        return;
    }
    spectroMeasure(channel, as7341, as7341->eF1F4ClearNIR, reading);
    reading->msec = millis();
    spectroMeasure(channel, as7341, as7341->eF5F8ClearNIR, reading);
}

void print_spectrophotometer_heading()
{
    Serial.println("channel\tposition\ttime ms\tpreheat cycles\tpower cycles\tlaser power\tF1(405-425nm)\tF2(435-455nm)\tF3(470-490nm)\tF4(505-525nm)\tF5(545-565nm)\tF6(580-600nm)\tF7(620-640nm)\tF8(670-690nm)\tClear\t\tNIR");
}

void single_reading(int number, char channel)
{
    DFRobot_AS7341 as7341(&Wire);
    if (power_on_spectrophotometer(channel))
    {
        if (init_spectrophotometer(channel, &as7341))
        {
            BrevitestSpectrophotometerReading *reading = &(test.reading[reading_index]);
            reading->number = number;
            reading->channel = channel;
            reading->temperature = heater.temp_C_10X;
            reading->position = stage_position;
            reading->msec = millis();
            turn_on_laser(channel);
            delayMicroseconds(LASER_PWM_ON_US);
            reading->laser_output = analogRead(get_laser(channel)->value_pin);
            take_spectrophotometer_reading(channel, &as7341, reading);
            turn_off_all_lasers();
        }
    }
    power_off_all_spectrophotometers();

    if (reading_index++ >= SPECTRO_RAW_MAX_CYCLES)
        reading_index = 0;
}

void take_one_reading(int number, int chan_num)
{
    char channel;

    channel = 'A' + (limit(chan_num, 3, 1) - 1);

    if (chan_num == 0)
    {
        for (int i = 0; i < 3; i++)
        {
            single_reading(number, 'A' + i);
        }
    }
    else
    {
        single_reading(number, channel);
    }
}

void spectrophotometer_reading(bool baseline, bool log = false)
{
    if (baseline)
    {
        reading_index = 0;
        test.baseline_readings = 3 * SPECTRO_NUMBER_OF_READINGS;
    }
    else
    {
        reading_index = test.baseline_readings;
        test.test_readings = 3 * SPECTRO_NUMBER_OF_READINGS;
    }

    for (int i = 0; i < SPECTRO_NUMBER_OF_READINGS; i++)
    {
        move_stage_to_position(SPECTRO_STARTING_STAGE_POSITION + i * (SPECTRO_WELL_LENGTH / (SPECTRO_NUMBER_OF_READINGS - 1)), MOTOR_SLOW_STEP_DELAY);
        take_one_reading(i, 0);
    }
}

void stress_test_read_spectrophotometer()
{
    print_spectrophotometer_heading();
    spectrophotometer_reading(true, true);
}

/////////////////////////////////////////////////////////////
//                                                         //
//               TEMPERATURE CONTROL SYSTEM                //
//                                                         //
/////////////////////////////////////////////////////////////

#define HEATER_READINGS 10
int get_heater_temperature()
{
    int raw = 0;
    for (int i = 0; i < HEATER_READINGS; i++)
    {
        raw += analogRead(heater.thermistor_pin);
    }
    raw /= HEATER_READINGS;

    if (raw == 0)
    {
        Log.info("Thermistor read error");
        stop_temperature_control();
        heater.temp_C_10X = 0;
        current_temperature = 0;
        heater.temp_F_10X = 0;
    }
    else
    {
        heater.temp_C_10X = raw_table_lookup(raw);
        current_temperature = heater.temp_C_10X;
        heater.temp_F_10X = ((heater.temp_C_10X * 9) / 5) + 320;
        if (heater.temp_C_10X > HEATER_MAX_TEMPERATURE)
        {
            Log.info("Heater temperature too high: %d.%d˚C", heater.temp_C_10X / 10, heater.temp_C_10X % 10);
            stop_temperature_control();
            raw = 0;
        }
    }
    return raw;
}

int pid_controller()
{
    int dt, error, derivative, raw;
    int output = heater.power;
    unsigned long current_read_time, prev_read_time;

    current_read_time = millis();
    prev_read_time = heater.read_time;
    if ((current_read_time - prev_read_time) >= HEATER_CONTROL_INTERVAL)
    {
        raw = get_heater_temperature();
        heater.read_time = current_read_time;
        if (raw != 0)
        {
            if (prev_read_time == 0)
            {
                heater.previous_error = 0;
                heater.integral = 0;
            }
            else if (heater.target_C_10X > HEATER_MAX_TEMPERATURE)
            {
                output = 0;
            }
            else
            {
                dt = heater.read_time - prev_read_time;
                error = heater.target_C_10X - heater.temp_C_10X;
                heater.integral += (error * dt) / 1000;
                derivative = (1000 * (error - heater.previous_error)) / dt;
                output = (heater.k_p_num * error) / heater.k_p_den;
                output += (heater.k_i_num * heater.integral) / heater.k_i_den;
                output += (heater.k_d_num * derivative) / heater.k_d_den;

                if (serial_messaging_on)
                {
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

void start_temperature_control()
{
    heater.read_time = 0;
    temperature_control_on = true;
    Log.info("Temperature control system started");
}

void stop_temperature_control()
{
    temperature_control_on = false;
    set_heater_power(0);
    Log.info("Temperature control system stopped");
}

/////////////////////////////////////////////////////////////
//                                                         //
//                 PUBLISH AND CALLBACKS                   //
//                                                         //
/////////////////////////////////////////////////////////////

void publishStatusChangeHandler(CloudEvent event)
{
    if (event.isSent())
    {
        Log.info("%s succeeded", event.name());
        event.clear();
    }
    else if (!event.isOk())
    {
        Log.info("%s failed error=%d", event.name(), event.error());
        event.clear();
    }
}

/////////////////////////////////////////////////////
//                 WEBHOOK ERROR                   //
/////////////////////////////////////////////////////

void response_error(CloudEvent event)
{
    Log.info("Webhook error: event = %s, size = %d", event.name(), event.data().size());
}

/////////////////////////////////////////////////////
//              VALIDATE CARTRIDGE                 //
/////////////////////////////////////////////////////

void publish_validate_cartridge()
{
    particle::Variant data;

    if (!event.isSending() && ((lastPublish == 0) || (millis() - lastPublish >= publishPeriod.count())))
    {
        lastPublish = millis();
        cartridge_validation_in_progress = true;
        cartridge_validated = false;

        event.name("validate-cartridge");
        event.data(barcode_uuid);
        if (event.canPublish(event.size()))
        {
            Log.info("Publishing validate cartridge, %s", barcode_uuid);
            Particle.publish(event);
        }
    }
}

void response_validate_cartridge(const char *name, String result)
{
    int crc_loaded = 0, crc_calculated;

    cartridge_validation_in_progress = false;
    cartridge_validation_mode = false;
    if (!detector_on)
    {
        cartridge_validated = false;
    }
    else
    {
        JSONValue parsed = JSONValue::parseCopy(result);
        JSONObjectIterator iter(parsed);
        while (iter.next())
        {
            if (iter.name() == "status")
            {
                if (iter.value().toString() == "SUCCESS")
                {
                    Log.info("Cartridge %s %s", barcode_uuid, "validated");
                    cartridge_validated = true;
                    test.test_status_code = TEST_STATUS_UNDERWAY;
                }
                else
                {
                    Log.info("Cartridge %s %s", barcode_uuid, "invalid");
                    cartridge_validated = false;
                    test.test_status_code = TEST_STATUS_INVALID_CARTRIDGE;
                }
            }
            else if (iter.name() == "errorMessage")
            {
                char *error = (char *)iter.value().toString().data();
                Log.info("Validation error: %s", error);
            }
            else if (iter.name() == "uuid")
            {
                char *uuid = (char *)iter.value().toString().data();
                Log.info("Cartridge ID: %s", uuid);
                memcpy(test.cartridge_id, uuid, BARCODE_UUID_LENGTH);
                test.cartridge_id[BARCODE_UUID_LENGTH] = '\0';
            }
            else if (iter.name() == "assayId")
            {
                char *assayId = (char *)iter.value().toString().data();
                Log.info("Assay ID: %s", assayId);
                memcpy(assay.id, assayId, ASSAY_UUID_LENGTH);
                assay.id[ASSAY_UUID_LENGTH] = '\0';
            }
            else if (iter.name() == "checksum")
            {
                crc_loaded = iter.value().toInt();
                Log.info("checksum: %d", crc_loaded);
            }
            else if (iter.name() == "version")
            {
                assay.BCODE_version = iter.value().toInt();
                Log.info("BCODE version: %d", assay.BCODE_version);
            }
            else if (iter.name() == "bcode")
            {
                strcpy(assay.BCODE, (char *)iter.value().toString().data());
                Log.info("BCODE : %s", assay.BCODE);
            }
            else if (iter.name() == "duration")
            {
                assay.duration = iter.value().toInt();
                Log.info("duration: %d", assay.duration);
            }
        }
        crc_calculated = abs((int)checksum(assay.BCODE, strlen(assay.BCODE)));
        test_start_mode = crc_calculated && crc_loaded && crc_loaded == crc_calculated;
        Log.info("crc_loaded: %d, crc_calculated: %d, test_start_mode: %c", crc_loaded, crc_calculated, test_start_mode ? 'T' : 'F');
    }
}

/////////////////////////////////////////////////////
//                   START TEST                    //
/////////////////////////////////////////////////////

void publish_start_test()
{
    particle::Variant data;

    if (!event.isSending() && ((lastPublish == 0) || (millis() - lastPublish >= publishPeriod.count())))
    {
        lastPublish = millis();
        test_start_in_progress = true;
        test_underway = false;

        event.name("start-test");
        event.data(barcode_uuid);
        if (event.canPublish(event.size()))
        {
            Log.info("Publishing start test, %s", barcode_uuid);
            Particle.publish(event);
        }
    }
}

void response_start_test(const char *name, String result)
{
    test_start_in_progress = false;
    test_start_mode = false;

    JSONValue parsed = JSONValue::parseCopy(result);
    JSONObjectIterator iter(parsed);
    while (iter.next())
    {
        if (iter.name() == "status")
        {
            if (iter.value().toString() == "SUCCESS")
            {
                if (!detector_on)
                {
                    Log.info("Start test cancelled");
                    test_underway = false;
                    test_cancelled = true;
                    test.test_status_code = TEST_STATUS_START_CANCELLED;
                    test_cancel_mode = true;
                }
                else
                {
                    test_underway = true;
                }
            }
            else
            {
                Log.info("Test failed to start");
                test_underway = false;
                cartridge_validated = false;
                test.test_status_code = TEST_STATUS_FAILED_TO_START;
            }
        }
        else if (iter.name() == "errorMessage")
        {
            char *error = (char *)iter.value().toString().data();
            Log.info("Start test error: %s", error);
        }
    }

    if (test_underway)
    {
        Log.info("Test underway");
        run_test();
    }
}

/////////////////////////////////////////////////////
//                  CANCEL TEST                    //
/////////////////////////////////////////////////////

void publish_cancel_test()
{
    particle::Variant data;

    if (!event.isSending() && ((lastPublish == 0) || (millis() - lastPublish >= publishPeriod.count())))
    {
        lastPublish = millis();
        test_cancel_in_progress = true;
        test_cancelled = false;

        event.name("cancel-test");
        event.data(eeprom.running_test_uuid);
        if (event.canPublish(event.size()))
        {
            Log.info("Publishing cancel test, %s", barcode_uuid);
            Particle.publish(event);
        }
    }
}

void response_cancel_test(const char *name, String result)
{
    test_cancel_in_progress = false;

    JSONValue parsed = JSONValue::parseCopy(result);
    JSONObjectIterator iter(parsed);
    while (iter.next())
    {
        if (iter.name() == "status")
        {
            if (iter.value().toString() == "SUCCESS")
            {
                test_cancelled = true;
                test.test_status_code = TEST_STATUS_CANCELLED;
                test_cancel_mode = false;
                test_underway = false;
                eeprom.running_test_uuid[0] = '\0';
                EEPROM.put(0, eeprom);
                Log.info("Test cancelled");
            }
            else
            {
                Log.info("Test failed to cancel");
            }
        }
        else if (iter.name() == "errorMessage")
        {
            char *error = (char *)iter.value().toString().data();
            Log.info("Start test error: %s", error);
        }
    }
}

/////////////////////////////////////////////////////
//                  UPLOAD TEST                    //
/////////////////////////////////////////////////////

void publish_upload_test()
{
    particle::Variant data;

    if (!event.isSending() && ((lastPublish == 0) || (millis() - lastPublish >= publishPeriod.count())))
    {
        lastPublish = millis();
        test_upload_in_progress = true;

        event.loadData(cached_filename);
        if (event.canPublish(event.size()))
        {
            Log.info("Publishing upload test, %s", cached_filename);
            Particle.publish(event);
        }
    }
}

void response_upload_test(const char *name, String result)
{
    test_upload_in_progress = false;
    test_upload_mode = false;

    JSONValue parsed = JSONValue::parseCopy(result);
    JSONObjectIterator iter(parsed);
    while (iter.next())
    {
        if (iter.name() == "status")
        {
            if (strncmp(iter.value().toString().data(), SUCCESS, 7) == 0)
            {
                test_invalid = false;
                unlink(cached_filename);
                Log.info("Uploaded test successful");
            }
            else
            {
                test_invalid = true;
                Log.info("Uploaded test invalid");
            }
        }
        else if (iter.name() == "errorMessage")
        {
            char *error = (char *)iter.value().toString().data();
            Log.info("Upload test error: %s", error);
        }
        else if (iter.name() == "cartridgeId")
        {
            char *cartridgeId = (char *)iter.value().toString().data();
            Log.info("Upload test: cartridge %s", cartridgeId);
        }
    }
}

/////////////////////////////////////////////////////
//                VALIDATE MAGNETS                 //
/////////////////////////////////////////////////////

void publish_upload_magnet_validation()
{
    particle::Variant data;

    if (!event.isSending() && ((lastPublish == 0) || (millis() - lastPublish >= publishPeriod.count())))
    {
        lastPublish = millis();
        magnet_validation_in_progress = true;
        magnet_validation_mode = false;

        data.set("device_id", device_id);
        event.name("validate-magnets");
        event.data(data);
        if (event.canPublish(sizeof(event)))
        {
            Particle.publish(event);
        }
    }
}

void response_upload_magnet_validation(const char *name, String result)
{
    Log.info("Magnet validation: %s", result.c_str());
    delay(2000);
    System.reset();
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

    if (test_cancelled)
        return index;

    // end of string so return end of string location
    if (bcode[index] == ITEM_DELIM)
        return index;

    // command has no arguments so skip delim
    if (bcode[index] == ATTR_DELIM)
        return index + 1;

    // there are arguments to extract
    i = index;
    while (i < BCODE_CAPACITY)
    {
        if (bcode[i] == ATTR_DELIM)
        {
            *token = extract_int_from_string(bcode, index, (i - index));
            return i; // return end of string location
        }
        if (bcode[i] == ARG_DELIM)
        {
            *token = extract_int_from_string(bcode, index, (i - index));
            i++; // skip past parameter
            return i;
        }
        i++;
    }

    return i;
}

int BCODE_loop()
{
    unsigned long total_duration = millis();

    set_heater_power(pid_controller());
    if (digitalRead(pinCartridgeDetected) == HIGH)
    {
        delayMicroseconds(100000);
        test_cancelled = digitalRead(pinCartridgeDetected) == HIGH;
    }

    return (int)(millis() - total_duration);
}

void BCODE_delay(int target_duration)
{
    int cycles = target_duration / BCODE_MAX_DELAY;
    int residual = target_duration % BCODE_MAX_DELAY;
    int loop_time = 0;

    for (int i = 0; i < cycles; i++)
    {
        delayMicroseconds(1000 * (BCODE_MAX_DELAY - BCODE_loop()));
        if (test_cancelled)
            return;
    }
    loop_time = BCODE_loop();
    if (test_cancelled)
        return;
    if (residual > loop_time)
    {
        delayMicroseconds(1000 * (residual - loop_time));
    }
}

int process_BCODE(int);
int process_one_BCODE_command(int cmd, int index)
{
    int param1, param2, param3, param4, start_index, position;

    if (test_cancelled)
        return index;

    switch (cmd)
    {
    case 0: // Start test()
        BCODE_delay(1000);
        break;
    case 1: // Delay(milliseconds)
        index = get_BCODE_token(index, &param1);
        BCODE_delay(param1);
        break;
    case 2:                                      // Move Microns(microns, microseconds)
        index = get_BCODE_token(index, &param1); // microns to move
        index = get_BCODE_token(index, &param2); // step_delay_us
        move_stage(param1, param2);
        BCODE_loop();
        break;
    case 3:                                      // Oscillate Stage(microns, microseconds, cycles)
        index = get_BCODE_token(index, &param1); // microns to move
        index = get_BCODE_token(index, &param2); // step_delay_us
        index = get_BCODE_token(index, &param3); // number of cycles
        oscillate_stage(param1, param2, param3, true);
        BCODE_loop();
        break;
    case 11:                                     // Baseline reading
        index = get_BCODE_token(index, &param1); // nuumber of readings
        position = stage_position;
        spectrophotometer_reading(true, param1);
        move_stage_to_position(position, MOTOR_SLOW_STEP_DELAY);
        break;
    case 14:                                     // Test readings
        index = get_BCODE_token(index, &param1); // nuumber of readings
        position = stage_position;
        spectrophotometer_reading(false, param1);
        move_stage_to_position(position, MOTOR_SLOW_STEP_DELAY);
        break;
    case 15:                                     // Raw sensor readings
        index = get_BCODE_token(index, &param1); // channel (all = 0, A = 1, B = 2, C = 3)
        index = get_BCODE_token(index, &param2); // gain
        index = get_BCODE_token(index, &param3); // step
        index = get_BCODE_token(index, &param4); // integration
        test.again = param2;
        test.astep = param3;
        test.atime = param4;
        take_one_reading(reading_count, param1);
        reading_count++;
        break;
    case 20: // Repeat begin(number of iterations)
        index = get_BCODE_token(index, &param1);
        start_index = index + 1;
        for (int i = 0; i < param1; i += 1)
        {
            if (test_cancelled)
                break;
            index = process_BCODE(start_index);
        }
        break;
    case 21: // Repeat end
        return -index;
        break;
    case 99: // Finish test
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
    if ((start_index == 0) && (cmd != 0))
    { // first command
        test_cancelled = true;
        return -1;
    }
    else
    {
        index = process_one_BCODE_command(cmd, index);
    }

    while ((cmd != 99) && (index > 0) && !test_cancelled)
    {
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

int start_stress_test(int limit, int led_power)
{
    stress_test_mode = true;
    eeprom.stress_test_cycles = 0;
    eeprom.stress_test_reading_count = 0;
    EEPROM.put(0, eeprom);
    stress_test_limit = limit;
    stress_test_LED_power = led_power;
    stress_test_step = 0;
    stress_test_stop_flag = false;
    Particle.disconnect();
    return 1;
}

void stop_stress_test()
{
    stress_test_mode = false;
    stress_test_stop_flag = false;
}

int stress_test_loop_time()
{
    unsigned long total_duration = millis();
    set_heater_power(pid_controller());
    return (int)(millis() - total_duration);
}

void stress_test_delay(int target_duration)
{
    int cycles = target_duration / BCODE_MAX_DELAY;
    int residual = target_duration % BCODE_MAX_DELAY;
    int loop_time = 0;

    for (int i = 0; i < cycles; i++)
    {
        delayMicroseconds(1000 * (BCODE_MAX_DELAY - stress_test_loop_time()));
        if (stress_test_stop_flag)
            return;
    }
    loop_time = stress_test_loop_time();

    if (residual > loop_time)
    {
        delayMicroseconds(1000 * (residual - loop_time));
    }
}

void stress_test_oscillate_stage(int amplitude, int step_delay, int cycles)
{
    for (int i = 0; i < cycles; i++)
    {
        move_stage(amplitude, step_delay);
        set_heater_power(pid_controller());
        move_stage(-amplitude, step_delay);
        set_heater_power(pid_controller());
    }
}

void do_stress_test_step(int step)
{
    Serial.print('.');
    set_heater_power(pid_controller());
    switch (step % 16)
    {
    case 0: // restart stress test
        reset_stage(false);
        move_stage_to_test_start_position();
        break;
    case 1: // move to start of well 2 and wait one minute
        move_stage(-1500, MOTOR_SLOW_STEP_DELAY);
        stress_test_delay(60000);
        break;
    case 2: // oscillate in well 2 and wait for beads to gather
        stress_test_oscillate_stage(3000, 350, 70);
        stress_test_delay(3000);
        break;
    case 3: // move to well 1
        move_stage(-6000, 65000);
        break;
    case 4: // oscillate in well 1
        stress_test_oscillate_stage(-4500, 350, 175);
        stress_test_delay(4000);
        break;
    case 5: // move to well 2
        move_stage(6000, 80000);
        stress_test_delay(3000);
        move_stage(3750, 10000);
        break;
    case 6: // oscillate in well 2
        stress_test_oscillate_stage(-4500, 400, 125);
        stress_test_delay(3000);
        break;
    case 7: // move to well 3
        move_stage(4800, 80000);
        stress_test_delay(3000);
        move_stage(3200, 10000);
        break;
    case 8: // oscillate in well 3
        stress_test_oscillate_stage(-4500, 350, 100);
        break;
    case 9: // read baseline sensors
        stress_test_delay(1500);
        break;
    case 10: // move to well 4
        move_stage(4800, 80000);
        stress_test_delay(3000);
        move_stage(3200, 10000);
        break;
    case 11: // oscillate in well 4
        stress_test_oscillate_stage(-4500, 375, 200);
        stress_test_delay(1500);
        break;
    case 12: // move to well 5
        move_stage(4400, 80000);
        stress_test_delay(3000);
        break;
    case 13: // oscillate in well 5
        stress_test_oscillate_stage(6500, 400, 200);
        stress_test_delay(5000);
        move_stage(-5000, 65000);
        stress_test_delay(300);
        break;
    case 14: // read sensors
        stress_test_read_spectrophotometer();
        break;
    case 15: // save cycle number
        eeprom.stress_test_cycles++;
        eeprom.stress_test_cycles_since_reset++;
        eeprom.lifetime_stress_test_cycles++;
        Serial.printlnf("Stress test cycles: current = %d, since reset = %d, lifetime = %d", eeprom.stress_test_cycles, eeprom.stress_test_cycles_since_reset, eeprom.lifetime_stress_test_cycles);
        EEPROM.put(0, eeprom);
        if (stress_test_limit != 0 && eeprom.stress_test_cycles >= stress_test_limit)
        {
            stress_test_stop_flag = true;
            reset_stage(true);
        }
        break;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                         COMMAND                         //
//                                                         //
/////////////////////////////////////////////////////////////

int get_next_command_param(String arg, int indx, int *param, int def)
{
    int next;

    if (indx == -1)
    {
        *param = def;
        next = -1;
    }
    else
    {
        next = arg.indexOf(ARG_DELIM, indx);
        if (next == -1)
        {
            *param = arg.substring(indx).toInt();
        }
        else
        {
            *param = arg.substring(indx, next).toInt();
            next++;
        }
    }

    return next;
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
    bool pinState = false;

    indx = get_next_command_param(arg, indx, &cmd, 0);
    switch (cmd)
    {
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
    case 3: // check cache
        result = test_in_cache() ? 1 : 0;
        break;
    case 4: // check digital pin state
        indx = get_next_command_param(arg, indx, &param1, 0);
        result = digitalRead(param1);
        break;
    case 5: // limit switch state
        result = digitalRead(pinStageLimit);
        break;
    case 6: // set digital pin state, param1 = pin, param2 = state
        indx = get_next_command_param(arg, indx, &param1, pinMotorDir);
        indx = get_next_command_param(arg, indx, &param2, 0);
        digitalWrite((uint16_t)param1, (u_int8_t)param2);
        result = digitalRead((uint16_t)param1);
        break;
    case 7: // repeatedly toggle pin state, param1 = pin, param2 = number of iterations, param3 = delay
        indx = get_next_command_param(arg, indx, &param1, pinMotorDir);
        indx = get_next_command_param(arg, indx, &param2, 1);
        indx = get_next_command_param(arg, indx, &param3, 1000);
        pinState = digitalRead((uint16_t)param1) == HIGH ? true : false;
        for (int i = 0; i < param2; i++)
        {
            Log.info("Toggle pin %d, cycle = %d, state = %d", param1, i, pinState);
            digitalWrite((uint16_t)param1, pinState ? LOW : HIGH);
            delay(param3);
            digitalWrite((uint16_t)param1, pinState ? HIGH : LOW);
            delay(param3);
        }
        digitalWrite((uint16_t)param1, (u_int8_t)param2);
        result = param2;
        break;
    case 8: // check analog pin state
        indx = get_next_command_param(arg, indx, &param1, 0);
        result = analogRead(param1);
        break;
    case 9: // clear wifi credentials
        WiFi.clearCredentials();
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
    case 12: // display id number
        Log.info("ID number: %s", device_id.c_str());
        result = 1;
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
        // sleep_motor();
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
        //  LASER DIODES
        //
    case 30: // turn on laser A for param1 milliseconds
        indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
        turn_on_laser_for_duration('A', param1);
        result = param1;
        break;
    case 31: // turn on laser B for param1 milliseconds
        indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
        turn_on_laser_for_duration('B', param1);
        result = param1;
        break;
    case 32: // turn on laser C for param1 milliseconds
        indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
        turn_on_laser_for_duration('C', param1);
        result = param1;
        break;
    case 33: // turn on all lasers for param1 milliseconds
        indx = get_next_command_param(arg, indx, &param1, LED_DURATION);
        turn_on_all_lasers_for_duration(param1);
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
        if (param1 > 0 && param1 <= HEATER_MAX_TEMPERATURE)
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
        //  STRESS TEST
        //
    case 90: // reset counter, deactivate WiFi and start stress test, up to param1 cycles (0 means no limit), LED power (0 means use baseline values)
        eeprom.stress_test_cycles_since_reset = 0;
        EEPROM.put(0, eeprom);
        WiFi.off();
        break;
    case 91: // deactivate WiFi and start stress test, up to param1 cycles (0 means no limit), LED power (0 means use baseline values)
        WiFi.off();
        break;
    case 92: // start stress test, up to param1 cycles (0 means no limit), LED power (0 means use baseline values)
        indx = get_next_command_param(arg, indx, &param1, 25);
        indx = get_next_command_param(arg, indx, &param2, LED_DEFAULT_POWER);
        result = start_stress_test(param1, param2);
        break;
    case 93: // stop stress test
        stress_test_stop_flag = true;
        result = 1;
        break;
        //
        //  VALIDATION
        //
    case 100: // validate magnets
        result = validate_magnets();
        if (result == 1)
        {
            publish_upload_magnet_validation();
        }
        break;
        //
        //  BLUETOOTH LE
        //
    case 200: // scan BLE
        result = BLE_scan();
        if (result > 0)
        {
            Log.info("%d devices found", result);
        }
        break;
        //
        //  SPECTROPHOTOMETER
        //
    case 301: // set spectrophotometer params
        indx = get_next_command_param(arg, indx, &param1, SPECTRO_ASTEP_DEFAULT);
        indx = get_next_command_param(arg, indx, &param2, SPECTRO_ATIME_DEFAULT);
        indx = get_next_command_param(arg, indx, &param3, SPECTRO_AGAIN_DEFAULT);
        test.astep = param1;
        test.atime = param2;
        test.again = param3;
        result = stage_position;
        break;
    case 303: // power on channel param1
        indx = get_next_command_param(arg, indx, &param1, 1);
        power_on_spectrophotometer((param1 - 1) + 'A');
        result = stage_position;
        break;
    case 304: // power off all spectrophotometers
        power_off_all_spectrophotometers();
        result = stage_position;
        break;
    case 305: // baseline scan
        reset_stage(false);
        print_spectrophotometer_heading();
        spectrophotometer_reading(true, true);
        sleep_motor();
        result = stage_position;
        break;
    case 306: // test scan
        reset_stage(false);
        print_spectrophotometer_heading();
        spectrophotometer_reading(false, true);
        sleep_motor();
        result = stage_position;
        break;
    case 307: // set pulse params
        indx = get_next_command_param(arg, indx, &pulsesA, 10);
        indx = get_next_command_param(arg, indx, &pulsesB, 10);
        indx = get_next_command_param(arg, indx, &pulsesC, 10);
        result = stage_position;
        break;
    case 308: // take one reading
        indx = get_next_command_param(arg, indx, &param1, 5);
        indx = get_next_command_param(arg, indx, &param2, 0);
        indx = get_next_command_param(arg, indx, &param3, SPECTRO_ASTEP_DEFAULT);
        indx = get_next_command_param(arg, indx, &param4, SPECTRO_ATIME_DEFAULT);
        indx = get_next_command_param(arg, indx, &param5, SPECTRO_AGAIN_DEFAULT);
        param1 = limit(param1, param2 == 0 ? SPECTRO_RAW_MAX_CYCLES / 3 : SPECTRO_RAW_MAX_CYCLES, 1);
        test.astep = param3;
        test.atime = param4;
        test.again = param5;
        reading_index = 0;
        reading_count = 0;
        for (int i = 0; i < param1; i++)
        {
            take_one_reading(i, param2);
        }
        output_test_readings();
        result = reading_index;
        break;
    case 309: // output raw readings
        output_test_readings();
        result = reading_index;
        break;
        //
        //  CLOUD FUNCTIONS
        //
    case 400: // check cache
        result =  (int) test_in_cache();
        break;
    case 401: // clear cache
        clear_cache();
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
//                           TESTS                         //
//                                                         //
/////////////////////////////////////////////////////////////

void reset_globals()
{
    test_underway = false;
    cartridge_validated = false;
    spectrophotometer_read_in_progress = false;

    test_progress = 0;
    test_percent_complete = 0;

    barcode_uuid[0] = '\0';
    barcode_uuid[BARCODE_UUID_LENGTH] = '\0';
    test.cartridge_id[0] = '\0';
    test.cartridge_id[BARCODE_UUID_LENGTH] = '\0';
    assay.id[0] = '\0';
    assay.id[ASSAY_UUID_LENGTH] = '\0';
    test.test_status_code = TEST_STATUS_UNDERWAY;

    particle_register[0] = '\0';
    particle_register[PARTICLE_REGISTER_SIZE] = '\0';
}

void disconnect_from_cloud()
{
    Log.info("Disconnecting from cloud...");
    Particle.disconnect();
    while (Particle.connected())
    {
        Particle.disconnect();
        delay(PARTICLE_CLOUD_DELAY);
    }
    Log.info("Disconnected from cloud");
}

void output_test_readings()
{
    if (reading_index > 0)
    {
        Log.info("Raw sensor readings: %d", reading_index);
        Serial.println("number\tchannel\tposition\ttemp C\ttime ms\tlaser power\tF1(405-425nm)\tF2(435-455nm)\tF3(470-490nm)\tF4(505-525nm)\tF5(545-565nm)\tF6(580-600nm)\tF7(620-640nm)\tF8(670-690nm)\t\tClear\t\tNIR");
        for (int i = 0; i < reading_index; i++)
        {
            BrevitestSpectrophotometerReading *r = &(test.reading[i]);
            Serial.printlnf("%d\t%c\t\t%d\t\t%d\t%lu\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d", r->number, r->channel, r->position, r->temperature, r->msec, r->laser_output, r->f1, r->f2, r->f3, r->f4, r->f5, r->f6, r->f7, r->f8, r->clear, r->nir);
        }
    }
    else
    {
        Log.info("No raw sensor readings");
    }
}

void run_test()
{
    unsigned long start_millis;

    reset_stage(false);
    move_stage_to_test_start_position();
    turn_on_buzzer_for_duration(1000, 600);
    delay(2000);

    disconnect_from_cloud();
    stop_temperature_control();

    // SINGLE_THREADED_BLOCK()
    // {
    memcpy(eeprom.running_test_uuid, test.cartridge_id, BARCODE_UUID_LENGTH);
    EEPROM.put(0, eeprom);
    start_millis = millis();

    reading_index = 0;
    reading_count = 0;
    process_BCODE(0);

    test.duration = (millis() - start_millis) / 1000;
    write_test_to_file();
    // }d

    start_temperature_control();
    output_test_readings();

    reset_stage(true);
    reset_globals();

    test_underway = false;
    test_upload_mode = true;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                          SETUP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void clear_state()
{
    cartridge_validation_in_progress = false;
    test_start_in_progress = false;
    test_upload_in_progress = false;

    magnetometer_inserted = false;
    stress_test_cartridge_inserted = false;

    barcode_scan_mode = false;
    stress_test_mode = false;
    cartridge_validation_mode = false;
    test_start_mode = false;
    test_underway = false;
    cached_filename[0] = '\0';
    test_upload_mode = test_in_cache();
    magnet_validation_mode = false;

    test_invalid = false;
    barcode_invalid = false;
    cartridge_validated = false;
    cartridge_inserted = false;
}

void init_analog_pin(uint16_t pin, PinMode mode, uint8_t value)
{
    pinMode(pin, mode);
    if (mode == OUTPUT)
    {
        analogWrite(pin, value);
    }
}

void init_analog_pin(uint16_t pin, PinMode mode)
{
    if (mode == INPUT || mode == INPUT_PULLUP)
    {
        pinMode(pin, mode);
    }
}

void init_digital_pin(uint16_t pin, PinMode mode, uint8_t value)
{
    pinMode(pin, mode);
    if (mode == OUTPUT)
    {
        digitalWrite(pin, value);
    }
}

void init_digital_pin(uint16_t pin, PinMode mode)
{
    if (mode == INPUT || mode == INPUT_PULLUP)
    {
        pinMode(pin, mode);
    }
}

bool startI2C()
{
    if (Wire.isEnabled())
        return true;

    Wire.setSpeed(CLOCK_SPEED_400KHZ);
    Wire.begin();
    delay(10);

    return Wire.isEnabled();
}

void startup_device()
{
}

void setup()
{
    init_digital_pin(pinCartridgeDetected, INPUT_PULLUP);
    detector_on = digitalRead(pinCartridgeDetected) == LOW;
    if (detector_on)
    {
        cartridge_inserted = true;
        turn_on_remove_cartridge_LED();
    }
    else
    {
        cartridge_inserted = false;
        turn_on_dont_touch_LED();
    }

    Serial.begin(115200); // standard serial port
    waitFor(Serial.isConnected, 15000);
    delay(100);
    Log.info("====== Serial Connected, Begin Setup ======");

    init_analog_pin(pinBuzzer, OUTPUT, 0);

    init_digital_pin(pinStageLimit, INPUT_PULLUP);

    init_digital_pin(pinBarcodeTrigger, OUTPUT, HIGH);
    init_digital_pin(pinBarcodeReady, INPUT_PULLUP);

    init_digital_pin(pinLaserA, OUTPUT, LOW);
    laserA.power_pin = pinLaserA;
    laserA.value_pin = pinPhotoA;
    init_digital_pin(pinLaserB, OUTPUT, LOW);
    laserB.power_pin = pinLaserB;
    laserB.value_pin = pinPhotoB;
    init_digital_pin(pinLaserC, OUTPUT, LOW);
    laserC.power_pin = pinLaserC;
    laserC.value_pin = pinPhotoC;

    init_analog_pin(pinHeaterThermistor, INPUT);
    init_digital_pin(pinHeater, OUTPUT, LOW);
    // init_analog_pin(pinHeater, OUTPUT, 0);

    init_digital_pin(pinMotorReset, OUTPUT, HIGH);
    init_digital_pin(pinMotorSleep, OUTPUT, LOW);
    init_digital_pin(pinMotorStep, OUTPUT, LOW);
    init_digital_pin(pinMotorDir, OUTPUT, LOW);

    device_id = System.deviceID();
    Log.info("Device ID: %s", device_id.c_str());

    Particle.variable("temperature", current_temperature);
    Particle.function("set_wifi_credentials", set_wifi_credentials);

    start_temperature_control();

    Particle.subscribe(String(device_id + "/hook-response/cancel-test/"), response_cancel_test);
    Particle.subscribe(String(device_id + "/hook-response/validate-cartridge/"), response_validate_cartridge);
    Particle.subscribe(String(device_id + "/hook-response/start-test/"), response_start_test);
    Particle.subscribe(String(device_id + "/hook-response/upload-test/"), response_upload_test);
    Particle.subscribe(String(device_id + "/hook-response/validate-magnets/"), response_upload_magnet_validation);

    Particle.subscribe(String(device_id + "/hook-error/cancel-test/"), response_error);
    Particle.subscribe(String(device_id + "/hook-error/validate-cartridge/"), response_error);
    Particle.subscribe(String(device_id + "/hook-error/start-test/"), response_error);
    Particle.subscribe(String(device_id + "/hook-error/upload-test/"), response_error);
    Particle.subscribe(String(device_id + "/hook-error/validate-magnets/"), response_error);

    setup_eeprom();
    create_dir_if_not_exists("/cache");
    event.onStatusChange(publishStatusChangeHandler);

    attachInterrupt(pinCartridgeDetected, detector_changed_interrupt, CHANGE);

    if (startI2C())
    {
        Log.info("I2C bus started");
    }
    else
    {
        Log.info("Could not start I2C bus");
    }
    init_spectrophotometer_switch();
    power_off_all_spectrophotometers();

    Log.info("Size of eeprom: %d", sizeof(Particle_EEPROM));

    Log.info("Resetting stage");
    reset_stage(true);

    Log.info("Testing LEDs");
    // turn_on_all_LEDs(LED_DEFAULT_POWER);
    turn_on_laser_for_duration('A', 100);
    delay(100);
    turn_on_laser_for_duration('B', 100);
    delay(100);
    turn_on_laser_for_duration('C', 100);

    Log.info("Buzzing");
    turn_on_buzzer_for_duration(250, 330);

    reset_globals();

    clear_state();

    bool test_interrupted = eeprom.running_test_uuid[0] != '\0';

    Log.info("device id: %s", device_id.c_str());
    Log.info("Firmware version: %d", eeprom.firmware_version);
    Log.info("Data format version: %d", eeprom.data_format_version);
    Log.info("Lifetime stress test cycles: %d", eeprom.lifetime_stress_test_cycles);
    Log.info("Stress test cycles since reset: %d", eeprom.stress_test_cycles_since_reset);
    Log.info("Last stress test cycles: %d", eeprom.stress_test_cycles);
    Log.info("Interrupted test ? %c", test_interrupted ? 'Y' : 'N');
    Log.info("Cached test ? %c", cached_filename[0] == '\0' ? 'N' : 'Y');

    if (test_interrupted)
    {
        test_cancel_mode = true;
    }

    Log.info("Setup complete");
}

/////////////////////////////////////////////////////////////
//                                                         //
//                   DEVICE INDICATORS                     //
//                                                         //
/////////////////////////////////////////////////////////////

bool heater_debounced()
{
    if (previous_heater_ready != heater_ready)
    {
        heater_debouncing_in_progress = true;
        heater_debounce_time = millis() + HEATER_READY_DEBOUNCE_DELAY;
    }
    else if (heater_debouncing_in_progress)
    {
        heater_debouncing_in_progress = millis() < heater_debounce_time;
    }
    else if (heater_ready)
    {
        return true;
    }
    return false;
}

void set_device_indicators()
{
    previous_heater_ready = heater_ready;
    heater_ready = (heater.target_C_10X - heater.temp_C_10X) < HEATER_READY_TEMP_DELTA;

    if (stress_test_mode)
    {
        turn_on_dont_touch_LED();
    }
    else if (!heater_debounced())
    {
        if (detector_on)
        {
            turn_on_remove_cartridge_LED();
        }
        else
        {
            turn_on_dont_touch_LED();
        }
    }
    else if (barcode_scan_mode || cartridge_validation_mode || test_start_mode || test_underway || test_cancel_mode || test_upload_mode || magnet_validation_mode)
    {
        turn_on_dont_touch_LED();
    }
    else if (cartridge_validated && (cartridge_inserted || magnetometer_inserted || stress_test_cartridge_inserted))
    {
        turn_on_dont_touch_LED();
        if (test_completed || test_cancelled)
        {
            turn_on_buzzer_alert();
        }
    }
    else if (barcode_invalid)
    {
        turn_on_remove_cartridge_LED();
        turn_on_buzzer_alert();
    }
    else if (detector_on)
    {
        turn_on_remove_cartridge_LED();
        turn_off_buzzer_timer();
    }
    else
    {
        turn_on_insert_cartridge_LED();
        turn_off_buzzer_timer();
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void barcode_scan_loop()
{
    int max_cycles;
    if (detector_on)
    {
        if (barcode_scan_mode)
        {
            barcode_scan_in_progress = true;
            cartridge_inserted = cartridge_validation_mode = false;
            magnetometer_inserted = magnet_validation_mode = false;
            switch (scan_barcode())
            {
            case BARCODE_TYPE_CARTRIDGE:
                cartridge_inserted = true;
                cartridge_validation_mode = true;
                Log.info("Cartridge inserted");
                break;
            case BARCODE_TYPE_MAGNETOMETER:
                magnetometer_inserted = true;
                magnet_validation_mode = true;
                Log.info("Magnetometer inserted");
                break;
            case BARCODE_TYPE_STRESS_TEST:
                stress_test_cartridge_inserted = true;
                max_cycles = atoi(&barcode_uuid[12]);
                start_stress_test(max_cycles, LED_DEFAULT_POWER);
                Log.info("Stress test started, max_cycles = %d", max_cycles);
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

void stress_test_loop()
{
    if (stress_test_stop_flag)
    {
        stop_stress_test();
    }
    else
    {
        SINGLE_THREADED_BLOCK()
        {
            do_stress_test_step(stress_test_step);
        }
        stress_test_step++;
    }
}

void magnet_validation_loop()
{
    if (validate_magnets())
    {
        publish_upload_magnet_validation();
    }
    else
    {
        magnet_validation_in_progress = false;
        magnet_validation_mode = false;
    }
}

void cartridge_validation_loop()
{
    if (cartridge_validated)
    {
        return;
    }
    else if (!cartridge_validation_in_progress)
    {
        publish_validate_cartridge();
    }
}

void test_start_loop()
{
    if (test_underway)
    {
        return;
    }
    else if (!test_start_in_progress)
    {
        publish_start_test();
    }
}

void test_upload_loop()
{
    if (!test_upload_in_progress)
    {
        if (test_in_cache())
        {
            publish_upload_test();
        }
    }
}

void cancel_test_loop()
{
    if (!test_cancel_in_progress)
    {
        publish_cancel_test();
    }
}

void hardware_loop()
{
    if (detector_debouncing)
    {
        if (millis() > detector_debouncing_time)
        {
            detector_debouncing_time = 0;
            detector_debouncing = false;
            detector_changed = false;
            detector_on = digitalRead(pinCartridgeDetected) == LOW;
            Log.info("%s detected", detector_on ? "Insertion" : "Removal");
            if (detector_on)
            {
                reset_stage(false);
                move_stage_to_test_start_position();
                sleep_motor();
                if (heater_ready)
                {
                    turn_on_buzzer_for_duration(BUZZER_INSERT_DURATION, BUZZER_INSERT_FREQUENCY);
                    barcode_scan_mode = true;
                    barcode_invalid = false;
                }
                else
                {
                    barcode_invalid = true;
                }
            }
            else
            {
                turn_off_buzzer_timer();
                clear_state();
                reset_stage(true);
            }
        }
    }
    else if (detector_changed)
    {
        detector_debouncing = true;
        detector_debouncing_time = millis() + DETECTOR_DEBOUNCE_DELAY;
    }

    set_device_indicators();

    if (temperature_control_on && !spectrophotometer_read_in_progress)
    {
        set_heater_power(pid_controller());
    }

    if (start_problem_buzzer)
    {
        start_problem_buzzer = false;
        turn_on_buzzer_for_duration(BUZZER_PROBLEM_DURATION, BUZZER_PROBLEM_FREQUENCY);
    }
    else if (start_alert_buzzer)
    {
        start_alert_buzzer = false;
        turn_on_buzzer_for_duration(BUZZER_ALERT_DURATION, BUZZER_ALERT_FREQUENCY);
    }
}

void process_serial_port()
{
    if (Serial.available())
    {
        char c = Serial.read();
        serial_buffer[serial_buffer_index] = c;
        serial_buffer_index++;
        serial_buffer_index %= SERIAL_COMMAND_BUFFER_SIZE;
        if (c == '\n')
        {
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

    if (stress_test_mode)
    {
        stress_test_loop();
    }
    else if (Particle.connected())
    {
        particle_connect_timeout = 0;
        if (test_cancel_mode)
        {
            cancel_test_loop();
        }
        else if (test_upload_mode)
        {
            test_upload_loop();
        }
        else if (heater_debounced())
        {
            if (test_start_mode)
            {
                test_start_loop();
            }
            else if (cartridge_validation_mode)
            {
                cartridge_validation_loop();
            }
            else if (magnet_validation_mode)
            {
                magnet_validation_loop();
            }
            else if (barcode_scan_mode)
            {
                barcode_scan_loop();
            }
        }
    }
    else if (millis() > particle_connect_timeout)
    {
        particle_connect_timeout = millis() + PARTICLE_CLOUD_DELAY;
        Particle.connect();
    }
}
