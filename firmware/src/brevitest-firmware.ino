/*
 * Project brevitest_v1_0
 * Description: firmware for Acuity™ Sample Processing Unit, part of the Brevitest™ Platform
 * Author: Leo Linbeck III
 * Date: December 2025
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
    if (params.length() == 0)
    {
        Log.error("Clearing WiFi credentials");
        WiFi.clearCredentials();
        WiFi.disconnect();
        return -1;
    }
    else
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
    struct stat statbuf;
    event.data((char *)&test, sizeof(test), ContentType::BINARY);
    String filename = "/cache/" + String(test.cartridge_id);
    event.saveData(filename);
    stat(filename, &statbuf);
    Log.info("write_test_to_file, test size: %d, event data size: %d, file size: %ld", sizeof test, event.data().size(), statbuf.st_size);
}

void clear_cache()
{
    int tries = 10;
    while (test_in_cache() && --tries > 0)
    {
        if (unlink(cached_filename) == 0)
        {
            cached_filename[0] = '\0';
            Log.info("Cache cleared: %s", cached_filename);
        }
        else
        {
            Log.error("Failed to clear cache: %s, errno: %d", cached_filename, errno);
        }
    }
}

void load_cached_test(char *filename)
{
    struct stat statbuf;
    if (filename == NULL)
    {
        Log.error("load_cached_test: filename is NULL");
        return;
    }
    event.loadData(filename);
    BrevitestTestRecord *t = (BrevitestTestRecord *)event.data().data();
    stat(filename, &statbuf);
    Log.info("load_cached_test from %s, test size: %d, event data size: %d, file size: %ld", filename, sizeof *t, event.data().size(), statbuf.st_size);
    Log.info("Test loaded, cartridge: %s, assay: %s, readings: %d", t->cartridge_id, t->assay_id, t->number_of_readings);
}

bool test_in_cache()
{
    DIR *cache = opendir("/cache");
    int tries = 50;
    bool test_found = false;
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
            Log.info("test_in_cache, found file: %s", cached_filename);
            test_found = true;
        }
    } while (cache_entry != NULL && --tries > 0);
    closedir(cache);
    return test_found;
}

void output_cache()
{
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
            event.loadData(cached_filename);
            output_test_readings((BrevitestTestRecord *)event.data().data());
        }
    } while (cache_entry != NULL && --tries > 0);
    closedir(cache);
}

bool save_assay_to_file()
{
    struct stat statbuf;

    if (assay.id[0] == '\0')
    {
        Log.error("save_assay_to_file, assay.id is empty");
        return false;
    }

    String filename = "/assay/" + String(assay.id);
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND);
    if (fd < 0)
    {
        Log.error("save_assay_to_file, failed to open file %s, errno: %d", filename.c_str(), errno);
        return false;
    }
    String assay_data = String(assay.id) + "\n" +
                        String(assay.duration) + "\n" +
                        String(assay.BCODE) + "\n";
    write(fd, assay_data.c_str(), assay_data.length());
    close(fd);

    stat(filename, &statbuf);
    Log.info("save_assay_to_file, assay data size: %d, file size: %ld", assay_data.length(), statbuf.st_size);
    return true;
}

bool load_assay_from_file(String assay_id, int BCODE_checksum = 0)
{
    int bytes_read;
    int index = 0;
    char *mark;
    int checksum_value;

    if (assay_id.length() == 0)
    {
        Log.error("load_assay_from_file, assay.id is empty");
        return false;
    }

    String filename = "/assay/" + assay_id;
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        Log.error("load_assay_from_file, failed to open file %s, errno: %d", filename.c_str(), errno);
        return false;
    }

    bytes_read = read(fd, assay_buffer, sizeof assay_buffer - 1);
    close(fd);

    assay_buffer[bytes_read] = '\0'; // null-terminate the string

    index = 8;
    if (strncmp(assay_id, assay_buffer, index) != 0)
    {
        assay_buffer[index] = '\0'; // null-terminate the assay id
        Log.error("load_assay_from_file, assay id doesn't match file data - assay id: %s, filename: %s", assay_id.c_str(), assay_buffer);
        assay.id[0] = '\0'; // reset assay id
        return false;
    }
    else
    {
        strcpy(assay.id, assay_id.c_str());
    }
    mark = &assay_buffer[index + 1];
    index = strcspn(mark, "\n");
    assay.duration = extract_int_from_string(mark, 0, index);
    mark += index + 1;
    index = strcspn(mark, "\n");
    strncpy(assay.BCODE, mark, index);
    assay.BCODE[index] = '\0'; // null-terminate the BCODE string
    checksum_value = abs((int)checksum(assay.BCODE, strlen(assay.BCODE)));
    if (BCODE_checksum != 0 && checksum_value != BCODE_checksum)
    {
        Log.error("load_assay_from_file, BCODE checksum mismatch - expected: %d, actual: %d", BCODE_checksum, checksum_value);
        assay.id[0] = '\0'; // reset assay id
        return false;
    }
    return true;
}

int list_assay_files()
{
    DIR *assay_dir = opendir("/assay");
    int count = 0;
    Log.info("Assay file directory");
    do
    {
        assay_entry = readdir(assay_dir);
        if (assay_entry == NULL)
        {
            break;
        }
        if (assay_entry->d_type != DT_REG)
        {
            continue;
        }
        count++;
        Log.info("  %s", assay_entry->d_name);
    } while (assay_entry != NULL && count <= ASSAY_MAX_FILES);
    closedir(assay_dir);
    return count;
}

int clear_assay_files()
{
    DIR *assay_dir = opendir("/assay");
    int count = 0;
    Log.info("Clearing assay files");
    do
    {
        assay_entry = readdir(assay_dir);
        if (assay_entry == NULL)
        {
            break;
        }
        if (assay_entry->d_type != DT_REG)
        {
            continue;
        }
        String filename = "/assay/" + String(assay_entry->d_name);
        if (unlink(filename) == 0)
        {
            count++;
            Log.info("  %s", assay_entry->d_name);
        }
        else
        {
            Log.error("Failed to delete %s", assay_entry->d_name);
        }
    } while (assay_entry != NULL && count <= ASSAY_MAX_FILES);
    closedir(assay_dir);
    return count;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                 COMMUNICATIONS STATUS                   //
//                                                         //
/////////////////////////////////////////////////////////////

void disableAllRadios()
{
    WiFi.disconnect();
    WiFi.off();
    Cellular.disconnect();
    Cellular.off();
    BLE.off();

    radios.wifi = false;
    radios.cellular = false;
    radios.bluetooth = false;
}

void enableAllRadios()
{
    WiFi.on();
    Cellular.on();
    BLE.on();

    radios.wifi = true;
    radios.cellular = true;
    radios.bluetooth = true;
}

void testWiFiOnly()
{
    Serial.println("➜ TEST MODE: WiFi Only");
    disableAllRadios();
    delay(500);
    WiFi.on();
    radios.wifi = true;
    Serial.println("✓ WiFi ON, All others OFF");
}

void testCellularOnly()
{
    Serial.println("➜ TEST MODE: Cellular Only");
    disableAllRadios();
    delay(500);
    Cellular.on();
    radios.cellular = true;
    Serial.println("✓ Cellular ON, All others OFF");
}

void testBluetoothOnly()
{
    Serial.println("➜ TEST MODE: Bluetooth Only");
    disableAllRadios();
    delay(500);
    BLE.on();
    radios.bluetooth = true;
    Serial.println("✓ Bluetooth ON, All others OFF");
}

void emissionCheck()
{
    Serial.println("\n╔════════════════════════════════╗");
    Serial.println("║     EMISSION CHECK REPORT      ║");
    Serial.println("╠════════════════════════════════╣");

    if (!radios.wifi && !radios.cellular && !radios.bluetooth)
    {
        Serial.println("║  🔇 RADIO SILENCE CONFIRMED    ║");
        Serial.println("║  No emissions detected         ║");
    }
    else
    {
        Serial.println("║  📡 EMISSIONS ACTIVE:          ║");
        if (radios.wifi)
            Serial.println("║     • WiFi transmitting        ║");
        if (radios.cellular)
            Serial.println("║     • Cellular transmitting    ║");
        if (radios.bluetooth)
            Serial.println("║     • Bluetooth transmitting   ║");
    }

    Serial.println("╚════════════════════════════════╝");
}

void displayStatus()
{
    Serial.println("\n┌─────────────────────────┐");
    Serial.println("│     RADIO STATUS        │");
    Serial.println("├─────────────────────────┤");
    Serial.print("│ WiFi:      ");
    Serial.print(radios.wifi ? "ON " : "OFF");
    Serial.println("         │");
    Serial.print("│ Cellular:  ");
    Serial.print(radios.cellular ? "ON " : "OFF");
    Serial.println("         │");
    Serial.print("│ Bluetooth: ");
    Serial.print(radios.bluetooth ? "ON " : "OFF");
    Serial.println("         │");
    Serial.println("└─────────────────────────┘");
}

void displayHelp()
{
    Serial.println("\n╔═══════════════════════════════════════╗");
    Serial.println("║    8000 SERIES COMMAND REFERENCE      ║");
    Serial.println("╠═══════════════════════════════════════╣");
    Serial.println("║ GENERAL:                              ║");
    Serial.println("║   8000 - Display this help            ║");
    Serial.println("║   8001 - Show radio status            ║");
    Serial.println("║                                       ║");
    Serial.println("║ WIFI CONTROL:                         ║");
    Serial.println("║   8100 - WiFi OFF                     ║");
    Serial.println("║   8101 - WiFi ON                      ║");
    Serial.println("║                                       ║");
    Serial.println("║ CELLULAR CONTROL:                     ║");
    Serial.println("║   8200 - Cellular OFF                 ║");
    Serial.println("║   8201 - Cellular ON                  ║");
    Serial.println("║                                       ║");
    Serial.println("║ BLUETOOTH CONTROL:                    ║");
    Serial.println("║   8300 - Bluetooth OFF                ║");
    Serial.println("║   8301 - Bluetooth ON                 ║");
    Serial.println("║                                       ║");
    Serial.println("║ TEST MODES:                           ║");
    Serial.println("║   8500 - Test WiFi only               ║");
    Serial.println("║   8501 - Test Cellular only           ║");
    Serial.println("║   8502 - Test Bluetooth only          ║");
    Serial.println("║                                       ║");
    Serial.println("║ MASTER CONTROL:                       ║");
    Serial.println("║   8900 - ALL radios OFF               ║");
    Serial.println("║   8901 - ALL radios ON                ║");
    Serial.println("║   8999 - Emission check report        ║");
    Serial.println("╚═══════════════════════════════════════╝");
    Serial.println("\nType command number and press Enter");
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

void move_stage_to_magnetometer_start_position()
{
    move_stage_to_position(STAGE_MICRONS_TO_MAGNETOMETER_START_POSITION, MOTOR_SLOW_STEP_DELAY);
}

void move_stage_to_position(int position, int step_delay)
{
    int move_distance = position - stage_position;
    if (move_distance != 0)
    {
        move_stage(move_distance, step_delay);
    }
}

void BCODE_loop(void);
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
        if (device_state.test_state == TestState::CANCELLED)
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
        delayMicroseconds(BARCODE_DELAY_US);
    };
    success = digitalRead(pinBarcodeReady) == HIGH; // successful if read is completed before timeout
    digitalWrite(pinBarcodeTrigger, HIGH);          // stop read by setting trigger pin back to high

    if (success)
    {
        while (!Serial1.available() && millis() < timeout)
        { // if data buffer is empty, wait and check again
            delayMicroseconds(BARCODE_DELAY_US);
        };
        delayMicroseconds(BARCODE_DELAY_US); // allow barcode buffer to fill before reading
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
    tone(pinBuzzer, frequency, duration);
    delay(duration);
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
    if (buzzer_alert_running)
        return;
    buzzer_problem_running = false;
    start_problem_buzzer = false;
    buzzer_alert_running = true;
    start_alert_buzzer = true;
    buzzer_timer.changePeriod(BUZZER_ALERT_PERIOD);
    buzzer_timer.reset();
}

void turn_on_buzzer_problem()
{
    if (buzzer_problem_running)
        return;
    buzzer_problem_running = true;
    start_problem_buzzer = true;
    buzzer_alert_running = false;
    start_alert_buzzer = false;
    buzzer_timer.changePeriod(BUZZER_PROBLEM_PERIOD);
    buzzer_timer.reset();
}

void turn_off_buzzer_timer()
{
    buzzer_timer.stop();
    if (buzzer_alert_running)
    {
        buzzer_alert_running = false;
        start_alert_buzzer = false;
    }
    if (buzzer_problem_running)
    {
        buzzer_problem_running = false;
        start_problem_buzzer = false;
    }
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

void turn_off_heater()
{
    analogWrite(heater.heater_pin, 0, HEATER_PWM_FREQUENCY);
    delayMicroseconds(HEATER_STABILIZATION_TIME_US);
    heater.heater_on = false;
    heater.power = 0;
}

void turn_on_heater(int power)
{
    power = limit(power, HEATER_MAX_POWER, 0);
    if (heater.temp_C_10X > HEATER_MAX_TEMPERATURE)
    {
        Log.info("Heater is too hot, turning off");
        turn_off_heater();
        return;
    }
    else if (power == 0)
    {
        turn_off_heater();
        return;
    }
    else
    {
        analogWrite(heater.heater_pin, power, HEATER_PWM_FREQUENCY);
        heater.power = power;
        heater.heater_on = true;
    }
}

int set_heater_power(int power)
{
    unsigned long start = millis();
    if (power != 0)
    {
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

int load_latest_magnet_validation(bool serial_output = true)
{
    int latest = 0;
    DIR *validation = opendir("/validation");
    int count = 0;
    int bytes_read = 0;
    Log.info("Validation file directory");
    do
    {
        validation_entry = readdir(validation);
        if (validation_entry == NULL)
        {
            break;
        }
        if (validation_entry->d_type != DT_REG)
        {
            continue;
        }
        count++;
        String filename = String(validation_entry->d_name);
        int timestamp = filename.substring(7, filename.length() - 4).toInt();
        if (timestamp > latest)
        {
            latest = timestamp;
        }
    } while (validation_entry != NULL && count <= MAGNETOMETER_MAX_FILES);
    closedir(validation);
    if (latest > 0)
    {
        magnet_validation_filename = magnet_file_path + String(latest) + ".txt";
        int fd = open(magnet_validation_filename, O_RDONLY);
        if (fd < 0)
        {
            Log.error("Failed to open latest validation file: %s, errno: %d", magnet_validation_filename.c_str(), errno);
            return -1;
        }
        bytes_read = read(fd, magnet_validation_data, MAGNETOMETER_BUFFER_SIZE);
        close(fd);
        Log.info("Latest validation file: %s, bytes: %d", magnet_validation_filename.c_str(), bytes_read);
        if (serial_output)
        {
            Serial.println(magnet_validation_data);
        }
    }
    else
    {
        Log.info("No validation files found");
    }
    return bytes_read;
}

int list_validation_files()
{
    DIR *validation = opendir("/validation");
    int count = 0;
    Log.info("Validation file directory");
    do
    {
        validation_entry = readdir(validation);
        if (validation_entry == NULL)
        {
            break;
        }
        if (validation_entry->d_type != DT_REG)
        {
            continue;
        }
        count++;
        Log.info("  %s", validation_entry->d_name);
    } while (validation_entry != NULL && count <= MAGNETOMETER_MAX_FILES);
    closedir(validation);
    return count;
}

int clear_validation_files()
{
    DIR *validation = opendir("/validation");
    int count = 0;
    Log.info("Clearing validation files");
    do
    {
        validation_entry = readdir(validation);
        if (validation_entry == NULL)
        {
            break;
        }
        if (validation_entry->d_type != DT_REG)
        {
            continue;
        }
        String filename = "/validation/" + String(validation_entry->d_name);
        if (unlink(filename) == 0)
        {
            count++;
            Log.info("  %s", validation_entry->d_name);
        }
        else
        {
            Log.error("Failed to delete %s", validation_entry->d_name);
        }
    } while (validation_entry != NULL && count <= MAGNETOMETER_MAX_FILES);
    closedir(validation);
    magnet_validation_data[0] = '\0';
    return count;
}

int create_magnet_validation_file()
{
    magnet_validation_filename = magnet_file_path + String(Time.now()) + ".txt";
    int fd = open(magnet_validation_filename, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND);
    String magnet_title_0 = magnet_validation_filename + "\r\n";
    write(fd, magnet_title_0.c_str(), magnet_title_0.length());
    return fd;
}

void close_magnet_validation_file(int fd)
{
    struct stat statbuf;

    close(fd);
    stat(magnet_validation_filename, &statbuf);
    Log.info("write_magnet_validation_to_file, test size: %d, event data size: %d, file size: %ld", sizeof test, event.data().size(), statbuf.st_size);
}

void check_magnets_in_one_well(int well, int fd)
{
    BleCharacteristic characteristic;
    String result, out_str;

    move_stage(well_move[well], MOTOR_SLOW_STEP_DELAY);
    if (magnetometer.getCharacteristicByUUID(characteristic, bleCharUuid[well]))
    {
        characteristic.getValue(result);
    }
    else
    {
        result = "Could not find magnetometer data";
    }
    out_str = String::format("%d\t%s\r\n", (well + 1), result.c_str());
    Serial.print(out_str.c_str());
    write(fd, out_str.c_str(), out_str.length());
}

int validate_magnets()
{
    int tries = 10;
    if (device_state.detector_on)
    {
        magnetometer_found = false;
        BLE_scan();
        if (magnetometer_found)
        {
            Log.info("Magnetometer found, connecting...");
            magnetometer = BLE.connect(magnetometer_address);
            Log.info("Connecting to magnetometer %s", barcode_uuid);
            while (tries-- > 0 && !magnetometer.connected())
            {
                delay(MAGNETOMETER_CONNECT_DELAY);
            }
            if (magnetometer.connected())
            {
                Log.info("Connected to magnetometer");
                reset_stage(false);
                move_stage_to_magnetometer_start_position();
                int fd = create_magnet_validation_file();
                write(fd, magnet_title_1.c_str(), magnet_title_1.length());
                write(fd, magnet_title_2.c_str(), magnet_title_2.length());
                Serial.print(magnet_title_1.c_str());
                Serial.print(magnet_title_2.c_str());
                for (int i = 0; i < MAGNETOMETER_NUMBER_OF_WELLS; i++)
                {
                    check_magnets_in_one_well(i, fd);
                }
                close_magnet_validation_file(fd);
                magnetometer.disconnect();
                reset_stage(true);
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
    else
    {
        Log.info("Magnetometer check cancelled");
        device_state.transition_to(DeviceMode::IDLE);
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

    digitalWrite(laser->power_pin, HIGH);
    laser->power_on = true;
}

void turn_off_laser(char channel)
{
    Laser *laser = get_laser(channel);

    digitalWrite(laser->power_pin, LOW);
    laser->power_on = false;
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
    delayMicroseconds(10000);
}

void set_spectrophotometer_power(byte code)
{
    int count = 5;
    do
    {
        Wire.beginTransmission(SPECTRO_SWITCH_ADDR);
        Wire.write(SPECTRO_SWITCH_OUTPUT_COMMAND);
        Wire.write(code);
        byte result = Wire.endTransmission();
        if (result == 0)
        {
            break;
        }
        Log.info("Error setting spectrophotometer power: %X", result);
        Wire.reset();
        delayMicroseconds(10000);
    } while (count-- > 0);
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
    delayMicroseconds(5000); // Wait for sensor to power on.
    return true;
}

void power_off_all_spectrophotometers()
{
    // Turn off all sensors.
    set_spectrophotometer_power(SPECTRO_SWITCH_TURN_OFF_ALL);
    delayMicroseconds(5000); // Wait for sensors to power off.
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
        delayMicroseconds(100000);
    }
    delayMicroseconds(2000);
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
    if (as7341->measureComplete())
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
    Serial.println("number\tchannel\tposition\ttemp C\ttime ms\tlaser power\tF1(405-425nm)\tF2(435-455nm)\tF3(470-490nm)\tF4(505-525nm)\tF5(545-565nm)\tF6(580-600nm)\tF7(620-640nm)\tF8(670-690nm)\t\tClear\t\tNIR");
}

void single_reading(uint8_t number, char channel, bool lasers_on, bool log)
{
    DFRobot_AS7341 as7341(&Wire);
    if (power_on_spectrophotometer(channel))
    {
        if (init_spectrophotometer(channel, &as7341))
        {
            BrevitestSpectrophotometerReading *reading = &(test.reading[test.number_of_readings]);
            reading->number = number;
            reading->channel = channel;
            reading->temperature = heater.temp_C_10X;
            reading->position = stage_position;
            if (lasers_on)
            {
                turn_on_laser(channel);
                delayMicroseconds(LASER_PWM_ON_US);
            }
            reading->laser_output = analogRead(get_laser(channel)->value_pin);
            take_spectrophotometer_reading(channel, &as7341, reading);
            turn_off_all_lasers();
            test.number_of_readings++;
            if (log)
            {
                Serial.printlnf("%d\t%c\t\t%d\t\t%d\t%lu\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d", reading->number, reading->channel, reading->position, reading->temperature, reading->msec, reading->laser_output, reading->f1, reading->f2, reading->f3, reading->f4, reading->f5, reading->f6, reading->f7, reading->f8, reading->clear, reading->nir);
            }
        }
    }
    power_off_all_spectrophotometers();
}

void take_one_reading(uint8_t number, int chan_num, bool lasers_on, bool log)
{
    char channel;

    analogWrite(heater.heater_pin, 0, HEATER_PWM_FREQUENCY);
    delayMicroseconds(HEATER_STABILIZATION_TIME_US);

    if (chan_num == 0)
    {
        for (int i = 0; i < 3; i++)
        {
            single_reading(number, channels[i], lasers_on, log);
        }
    }
    else
    {
        channel = channels[(limit(chan_num, 3, 1) - 1)];
        single_reading(number, channel, lasers_on, log);
    }

    analogWrite(heater.heater_pin, heater.power, HEATER_PWM_FREQUENCY);
}

void spectrophotometer_reading(bool baseline, int scans, bool log)
{
    if (baseline)
    {
        test.baseline_scans = scans;
    }
    else
    {
        test.test_scans = scans;
    }

    for (int i = 0; i < scans; i++)
    {
        move_stage_to_position(SPECTRO_STARTING_STAGE_POSITION + i * (SPECTRO_WELL_LENGTH / (SPECTRO_NUMBER_OF_READINGS - 1)), MOTOR_SLOW_STEP_DELAY);
        take_one_reading(i, 0, true, log);
    }
}

void stress_test_read_spectrophotometer()
{
    print_spectrophotometer_heading();
    spectrophotometer_reading(true, 10, true);
    spectrophotometer_reading(false, 10, true);
}

/////////////////////////////////////////////////////////////
//                                                         //
//               TEMPERATURE CONTROL SYSTEM                //
//                                                         //
/////////////////////////////////////////////////////////////

#define HEATER_READINGS 20
#define HEATER_BAND_PASS_TAIL 4
int heater_reads[HEATER_READINGS];
int get_heater_temperature()
{
    int raw = 0;

    analogWrite(heater.heater_pin, 0, HEATER_PWM_FREQUENCY);
    delayMicroseconds(HEATER_STABILIZATION_TIME_US);
    for (int i = 0; i < HEATER_READINGS; i++)
    {
        heater_reads[i] = analogRead(heater.thermistor_pin);
    }

    std::sort(heater_reads, heater_reads + HEATER_READINGS);
    for (int i = HEATER_BAND_PASS_TAIL; i < HEATER_READINGS - HEATER_BAND_PASS_TAIL; i++)
    {
        if (heater_reads[i] == 0)
        {
            Log.info("Thermistor read error");
            stop_temperature_control();
            heater.temp_C_10X = 0;
            current_temperature = 0;
            heater.temp_F_10X = 0;
            return 0;
        }
        else
        {
            raw += heater_reads[i];
        }
    }

    raw /= HEATER_READINGS - 2 * HEATER_BAND_PASS_TAIL;
    heater.temp_C_10X = raw_table_lookup(raw);
    current_temperature = heater.temp_C_10X;
    heater.temp_F_10X = ((heater.temp_C_10X * 9) / 5) + 320;
    if (heater.temp_C_10X > HEATER_MAX_TEMPERATURE)
    {
        Log.info("Heater temperature too high: %d.%d˚C", heater.temp_C_10X / 10, heater.temp_C_10X % 10);
        stop_temperature_control();
        raw = 0;
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
        if (device_state.is_cartridge_validated())
        {
            return;
        }
        
        lastPublish = millis();
        device_state.start_cloud_operation();

        clear_payload_buffer();
        event.name("validate-cartridge");
        event.contentType(ContentType::STRUCTURED);
        data.set("uuid", barcode_uuid);
        event.data(data);
        if (event.canPublish(event.size()))
        {
            Log.info("Publishing validate cartridge, %s", barcode_uuid);
            Particle.publish(event);
        }
    }
}

/**
 * @brief Handle cartridge validation response from cloud
 * 
 * This function processes the cloud server's response to cartridge
 * validation requests. It updates the state machine based on whether
 * the cartridge is valid or invalid, and loads the appropriate assay.
 * 
 * @param cancel_event The cloud event containing the validation response
 */
void response_validate_cartridge(CloudEvent cancel_event)
{
    // === END CLOUD OPERATION TRACKING ===
    device_state.end_cloud_operation();

    // === PARSE CLOUD RESPONSE ===
    Variant json = Variant::fromJSON(cancel_event.dataString());
    if (json.get("status").toString() == "SUCCESS")
    {
        // === CARTRIDGE VALIDATION SUCCESSFUL ===
        String assay_id = json.get("assayId").toString();
        int checksum_value = json.get("checksum").toInt();
        Log.info("Cartridge validation successful, assay ID: %s, checksum: %d", assay_id.c_str(), checksum_value);
        
        // === LOAD ASSAY FROM FILE ===
        if (load_assay_from_file(assay_id, checksum_value))
        {
            // === ASSAY LOADED SUCCESSFULLY ===
            device_state.cartridge_state = CartridgeState::VALIDATED;
            device_state.test_state = TestState::NOT_STARTED;
            strcpy(test.cartridge_id, json.get("cartridgeId").toString().c_str());
            device_state.transition_to(DeviceMode::RUNNING_TEST);
            run_test();
        }
        else
        {
            // === ASSAY LOAD FAILED ===
            device_state.cartridge_state = CartridgeState::INVALID;
            device_state.set_error("Could not load assay from file due to checksum mismatch");
            Log.info("Cartridge validation failed: could not load assay from file due to checksum mismatch");
            memcpy(reset_uuid, barcode_uuid, BARCODE_UUID_LENGTH + 1);
            device_state.transition_to(DeviceMode::RESETTING_CARTRIDGE);
        }
    }
    else
    {
        // === CARTRIDGE VALIDATION FAILED ===
        device_state.cartridge_state = CartridgeState::INVALID;
        device_state.set_error("Cartridge validation failed");
        Log.info("Cartridge validation failed");
        
        // === GET ERROR MESSAGE IF AVAILABLE ===
        String errorMessage = json.get("errorMessage").toString();
        if (errorMessage.length() > 0)
        {
            Log.info("Cartridge validation error: %s", errorMessage.c_str());
        }
    }
}

/////////////////////////////////////////////////////
//                RESET CARTRIDGE                  //
/////////////////////////////////////////////////////

/**
 * @brief Publish cartridge reset request to cloud
 * 
 * This function sends a reset request to the cloud server for the
 * specified cartridge UUID. It uses the state machine to track
 * the cloud operation and prevent duplicate requests.
 */
void publish_reset_cartridge()
{
    particle::Variant data;

    // === CHECK IF WE CAN PUBLISH ===
    if (!event.isSending() && ((lastPublish == 0) || (millis() - lastPublish >= publishPeriod.count())))
    {
        lastPublish = millis();
        device_state.start_cloud_operation();

        // === PREPARE CLOUD EVENT ===
        event.name("reset-cartridge");
        event.contentType(ContentType::STRUCTURED);
        data.set("uuid", reset_uuid);
        event.data(data);
        
        // === PUBLISH IF POSSIBLE ===
        if (event.canPublish(event.size()))
        {
            Log.info("Publishing reset cartridge, %s", reset_uuid);
            Particle.publish(event);
        }
    }
}

/**
 * @brief Handle cartridge reset response from cloud
 * 
 * This function processes the cloud server's response to cartridge
 * reset requests. It updates the state machine based on whether
 * the reset was successful or failed.
 * 
 * @param reset_event The cloud event containing the reset response
 */
void response_reset_cartridge(CloudEvent reset_event)
{
    // === END CLOUD OPERATION TRACKING ===
    device_state.end_cloud_operation();

    // === PARSE CLOUD RESPONSE ===
    Variant json = Variant::fromJSON(reset_event.dataString());
    if (json.get("status").toString() == "SUCCESS")
    {
        // === CARTRIDGE RESET SUCCESSFUL ===
        String cartridge_id = json.get("cartridgeId").toString();
        Log.info("Cartridge reset successful: %s", cartridge_id.c_str());
        memset(reset_uuid, 0, BARCODE_UUID_LENGTH + 1);
        device_state.transition_to(DeviceMode::IDLE);
    }
    else
    {
        // === CARTRIDGE RESET FAILED ===
        Log.info("Cartridge reset failed");
        String errorMessage = json.get("errorMessage").toString();
        if (errorMessage.length() > 0)
        {
            Log.info("Cartridge reset error: %s", errorMessage.c_str());
        }
        device_state.set_error("Cartridge reset failed");
    }
}

/////////////////////////////////////////////////////
//                   LOAD ASSAY                    //
/////////////////////////////////////////////////////

void publish_load_assay(String assay_to_load)
{
    particle::Variant data;

    if (!event.isSending() && ((lastPublish == 0) || (millis() - lastPublish >= publishPeriod.count())))
    {
        Variant data;

        lastPublish = millis();

        clear_payload_buffer();
        event.name("load-assay");
        event.contentType(ContentType::STRUCTURED);
        data.set("assay_id", assay_to_load);
        event.data(data);
        if (event.canPublish(event.size()))
        {
            Log.info("Publishing load assay, %s", assay_to_load.c_str());
            Particle.publish(event);
        }
        assay_to_load = ""; // reset assay_to_load after publishing
    }
}

void clear_payload_buffer()
{
    for (int i = 0; i < PARTICLE_PAYLOAD_BUFFER_SIZE; i++)
    {
        payload_buffer[i] = "";
    }
}

void output_payload_buffer()
{
    for (int i = 0; i < PARTICLE_PAYLOAD_BUFFER_SIZE; i++)
    {
        Log.info("Payload buffer[%d]: %s", i, payload_buffer[i].c_str());
    }
}

bool all_payloads_received()
{
    bool all = true;
    int last = -1;
    for (int i = 0; i < PARTICLE_PAYLOAD_BUFFER_SIZE; i++)
    {
        int len = payload_buffer[i].length();
        if (len > 0 && len < 512)
        {
            last = i;
            break;
        }
    }
    if (last == -1)
    {
        return false;
    }
    for (int i = 0; i <= last; i++)
    {
        all = all && (payload_buffer[i].length() > 0);
    }
    return all;
}

void response_load_assay(CloudEvent load_assay_event)
{
    Log.info("response_load_assay event: name=%s, size=%d, content type=%d", load_assay_event.name(), load_assay_event.data().size(), (int)load_assay_event.contentType());
    String final_result;
    String name = load_assay_event.name();

    int index = limit(name.substring(name.length() - 1).toInt(), PARTICLE_PAYLOAD_BUFFER_SIZE - 1, 0);
    Log.info("response_load_assay data size: %d, index: %d", load_assay_event.data().size(), index);

    payload_buffer[index] = load_assay_event.dataString();
    if (!all_payloads_received())
        return;

    for (int ii = 0; ii < PARTICLE_PAYLOAD_BUFFER_SIZE; ii++)
    {
        if (payload_buffer[ii].length() > 0)
            final_result += payload_buffer[ii];
    }
    Log.info("response_load_assay: %s", final_result.c_str());

    Variant json = Variant::fromJSON(final_result);
    if (json.get("status").toString() == "SUCCESS")
    {
        strcpy(assay.id, json.get("assayId").toString().c_str());
        strcpy(test.assay_id, assay.id);
        Log.info("Assay ID: %s", assay.id);

        int crc_loaded = json.get("checksum").toUInt();
        Log.info("checksum: %d", crc_loaded);

        strcpy(assay.BCODE, json.get("bcode").toString().c_str());
        assay.BCODE_length = strlen(assay.BCODE);
        Log.info("BCODE : %s", assay.BCODE);

        assay.duration = json.get("duration").toInt();
        Log.info("duration: %d", assay.duration);

        int crc_calculated = abs((int)checksum(assay.BCODE, strlen(assay.BCODE)));
        Log.info("crc_loaded: %d, crc_calculated: %d", crc_loaded, crc_calculated);
        if (crc_loaded == crc_calculated)
        {
            if (save_assay_to_file())
            {
                Log.info("Assay %s %s", assay.id, "loaded successfully");
            }
            else
            {
                Log.info("Assay %s %s", assay.id, "saved to file failed");
            }
        }
        else
        {
            Log.info("Assay %s %s", assay.id, "loaded but checksum mismatch");
            Log.info("CRC loaded: %d, CRC calculated: %d", crc_loaded, crc_calculated);
        }
    }
    else
    {
        Log.info("Assay %s %s", assay.id, "loading failed");
    }
}

/////////////////////////////////////////////////////
//                  UPLOAD TEST                    //
/////////////////////////////////////////////////////

/**
 * @brief Publish test upload request to cloud
 * 
 * This function sends a test result file to the cloud server for
 * processing. It uses the state machine to track the cloud operation
 * and prevent duplicate requests.
 */
void publish_upload_test()
{
    particle::Variant data;

    // === CHECK IF WE CAN PUBLISH ===
    if (!event.isSending() && ((lastPublish == 0) || (millis() - lastPublish >= publishPeriod.count())))
    {
        // === CHECK IF TEST IS IN CACHE ===
        if (!test_in_cache())
        {
            return;
        }
        
        // === PREPARE CLOUD EVENT ===
        lastPublish = millis();
        device_state.start_cloud_operation();

        event.name("upload-test");
        event.contentType(ContentType::BINARY);
        event.loadData(cached_filename);
        
        // === PUBLISH IF POSSIBLE ===
        if (event.canPublish(event.size()))
        {
            Log.info("Publishing upload test, %s (%d bytes)", cached_filename, event.size());
            Particle.publish(event);
        }
    }
}

/**
 * @brief Handle test upload response from cloud
 * 
 * This function processes the cloud server's response to test
 * upload requests. It updates the state machine based on whether
 * the upload was successful or failed, and manages cache cleanup.
 * 
 * @param upload_event The cloud event containing the upload response
 */
void response_upload_test(CloudEvent upload_event)
{
    // === END CLOUD OPERATION TRACKING ===
    device_state.end_cloud_operation();

    // === PARSE CLOUD RESPONSE ===
    Variant json = Variant::fromJSON(upload_event.dataString());
    String cartridgeId = json.get("cartridgeId").toString();
    if (json.get("status").toString() == "SUCCESS")
    {
        // === TEST UPLOAD SUCCESSFUL ===
        if (cartridgeId.length() == BARCODE_UUID_LENGTH)
        {
            // === CLEAN UP CACHE ===
            unlink("/cache/" + cartridgeId);
            Log.info("Uploaded test successful, %s removed from cache", cartridgeId.c_str());
            device_state.test_state = TestState::UPLOADED;
        }
        else
        {
            Log.info("Uploaded test successful, but %s not removed from cache", cartridgeId.c_str());
        }
        device_state.transition_to(DeviceMode::IDLE);
    }
    else
    {
        // === TEST UPLOAD FAILED ===
        Log.info("Uploaded test invalid");
        device_state.set_error("Test upload failed");
    }
    // === GET ERROR MESSAGE IF AVAILABLE ===
    String errorMessage = json.get("errorMessage").toString();
    if (errorMessage.length() > 0)
    {
        Log.info("Cancel test error: %s", errorMessage.c_str());
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

    if (device_state.test_state == TestState::CANCELLED)
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

void BCODE_loop()
{
    set_heater_power(pid_controller());
    if (digitalRead(pinCartridgeDetected) == HIGH) // cartridge removal detected, debounce
    {
        delay(DETECTOR_DEBOUNCE_DELAY);
        if (digitalRead(pinCartridgeDetected) == HIGH)
        {
            device_state.test_state = TestState::CANCELLED;
        }
        if (device_state.test_state == TestState::CANCELLED)
        {
            Log.info("Cartridge removed, cancelling test");
        }
    }
}

#define BCODE_DELAY_UNIT 1000 // milliseconds
int process_BCODE(int);
int process_one_BCODE_command(int cmd, int index)
{
    int param1, param2, param3, param4, start_index, position, remainder;
    unsigned long start_time;
    uint8_t number;

    if (device_state.test_state == TestState::CANCELLED)
        return index;

    BCODE_loop();
    switch (cmd)
    {
    case 0: // Start test()
        Log.info("Start test");
        delay(1000);
        break;
    case 1: // Delay(milliseconds)
        index = get_BCODE_token(index, &param1);
        Log.info("Delay %d ms", param1);
        start_time = millis();
        for (int i = 0; i < (param1 / BCODE_DELAY_UNIT - 1); i++)
        {
            BCODE_loop();
            delay(BCODE_DELAY_UNIT);
        }
        remainder = param1 - (int)(millis() - start_time);
        if (remainder > 0)
        {
            BCODE_loop();
            delay(remainder < BCODE_DELAY_UNIT ? remainder : BCODE_DELAY_UNIT);
        }
        break;
    case 2:                                      // Move Microns(microns, microseconds)
        index = get_BCODE_token(index, &param1); // microns to move
        index = get_BCODE_token(index, &param2); // step_delay_us
        Log.info("Move %d microns, %d µs", param1, param2);
        move_stage(param1, param2);
        break;
    case 3:                                      // Oscillate Stage(microns, microseconds, cycles)
        index = get_BCODE_token(index, &param1); // microns to move
        index = get_BCODE_token(index, &param2); // step_delay_us
        index = get_BCODE_token(index, &param3); // number of cycles
        Log.info("Oscillate %d microns, %d µs, %d cycles", param1, param2, param3);
        oscillate_stage(param1, param2, param3, true);
        break;
    case 10:                                     // Set sensor params
        index = get_BCODE_token(index, &param1); // gain
        index = get_BCODE_token(index, &param2); // step
        index = get_BCODE_token(index, &param3); // integration time
        Log.info("Set sensor params: %X %X %X", param1, param2, param3);
        test.again = param1;
        test.astep = param2;
        test.atime = param3;
        break;
    case 11:                                     // Baseline scans
        index = get_BCODE_token(index, &param1); // number of scans (3 readings per scan)
        Log.info("Baseline scans: %d", param1);
        position = stage_position;
        noInterrupts();
        spectrophotometer_reading(true, param1, false);
        interrupts();
        move_stage_to_position(position, MOTOR_SLOW_STEP_DELAY);
        break;
    case 14:                                     // Test scans
        index = get_BCODE_token(index, &param1); // number of scans (3 readings per scan)
        Log.info("Test scans: %d", param1);
        position = stage_position;
        noInterrupts();
        spectrophotometer_reading(false, param1, false);
        interrupts();
        move_stage_to_position(position, MOTOR_SLOW_STEP_DELAY);
        break;
    case 15:                                     // take sensor readings
        index = get_BCODE_token(index, &param1); // channel (all = 0, A = 1, B = 2, C = 3)
        index = get_BCODE_token(index, &param2); // gain
        index = get_BCODE_token(index, &param3); // step
        index = get_BCODE_token(index, &param4); // integration
        Log.info("Take sensor readings: %d %d %d %d", param1, param2, param3, param4);
        test.again = param2;
        test.astep = param3;
        test.atime = param4;
        number = param1 == 0 ? test.number_of_readings / 3 : test.number_of_readings;
        noInterrupts();
        take_one_reading(number, param1, true, false);
        interrupts();
        break;
    case 20: // Repeat begin(number of iterations)
        index = get_BCODE_token(index, &param1);
        Log.info("Repeat start: %d", param1);
        start_index = index + 1;
        for (int i = 0; i < param1; i += 1)
        {
            if (device_state.test_state == TestState::CANCELLED)
                break;
            index = process_BCODE(start_index);
        }
        break;
    case 21: // Repeat end
        Log.info("Repeat end");
        return -index;
        break;
    case 99: // Finish test
        Log.info("Finish test");
        break;
    default:
        Log.info("Unknown command: %d  %d", cmd, index);
    }

    return index + 1;
}

int process_BCODE(int start_index)
{
    int cmd, index;

    index = get_BCODE_token(start_index, &cmd);
    if ((start_index == 0) && (cmd != 0))
    { // first command
        device_state.test_state = TestState::CANCELLED;
        return -1;
    }
    else
    {
        index = process_one_BCODE_command(cmd, index);
    }

    while ((cmd != 99) && (index > 0) && (device_state.test_state != TestState::CANCELLED))
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
    device_state.transition_to(DeviceMode::STRESS_TESTING);
    eeprom.stress_test_cycles = 0;
    eeprom.stress_test_reading_count = 0;
    EEPROM.put(0, eeprom);
    stress_test_limit = limit;
    stress_test_LED_power = led_power;
    stress_test_step = 0;
    Particle.disconnect();
    return 1;
}

void stop_stress_test()
{
    device_state.transition_to(DeviceMode::IDLE);
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
        if (device_state.mode != DeviceMode::STRESS_TESTING)
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
            device_state.transition_to(DeviceMode::IDLE);
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
    case 71: // list magnet validation files
        result = list_validation_files();
        break;
    case 72: // clear magnet validation files
        result = clear_validation_files();
        break;
    case 73: // load latest magnet validation file
        result = load_latest_magnet_validation();
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
        device_state.transition_to(DeviceMode::IDLE);
        result = 1;
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
        indx = get_next_command_param(arg, indx, &param1, 1);
        reset_stage(false);
        print_spectrophotometer_heading();
        spectrophotometer_reading(true, param1, true);
        sleep_motor();
        result = stage_position;
        break;
    case 306: // test scan
        indx = get_next_command_param(arg, indx, &param1, 1);
        reset_stage(false);
        print_spectrophotometer_heading();
        spectrophotometer_reading(false, param1, true);
        sleep_motor();
        result = stage_position;
        break;
    case 307: // set pulse params
        indx = get_next_command_param(arg, indx, &pulsesA, 10);
        indx = get_next_command_param(arg, indx, &pulsesB, 10);
        indx = get_next_command_param(arg, indx, &pulsesC, 10);
        result = stage_position;
        break;
    case 308:                                                 // take readings
        indx = get_next_command_param(arg, indx, &param1, 5); // count
        indx = get_next_command_param(arg, indx, &param2, 0); // channel (0 = all, 1 = A, 2 = B, 3 = C)
        indx = get_next_command_param(arg, indx, &param3, SPECTRO_ASTEP_DEFAULT);
        indx = get_next_command_param(arg, indx, &param4, SPECTRO_ATIME_DEFAULT);
        indx = get_next_command_param(arg, indx, &param5, SPECTRO_AGAIN_DEFAULT);
        param1 = limit(param1, param2 == 0 ? SPECTRO_RAW_MAX_CYCLES / 3 : SPECTRO_RAW_MAX_CYCLES, 1);
        test.astep = param3;
        test.atime = param4;
        test.again = param5;
        test.number_of_readings = 0;
        for (int i = 0; i < param1; i++)
        {
            take_one_reading(i, param2, true, false);
        }
        output_test_readings(&test);
        result = test.number_of_readings;
        break;
    case 309: // output raw readings
        output_test_readings(&test);
        result = test.number_of_readings;
        break;
    case 310:                                                 // take readings without lasers
        indx = get_next_command_param(arg, indx, &param1, 5); // count
        indx = get_next_command_param(arg, indx, &param2, 0); // channel (0 = all, 1 = A, 2 = B, 3 = C)
        indx = get_next_command_param(arg, indx, &param3, SPECTRO_ASTEP_DEFAULT);
        indx = get_next_command_param(arg, indx, &param4, SPECTRO_ATIME_DEFAULT);
        indx = get_next_command_param(arg, indx, &param5, SPECTRO_AGAIN_DEFAULT);
        param1 = limit(param1, param2 == 0 ? SPECTRO_RAW_MAX_CYCLES / 3 : SPECTRO_RAW_MAX_CYCLES, 1);
        test.astep = param3;
        test.atime = param4;
        test.again = param5;
        test.number_of_readings = 0;
        for (int i = 0; i < param1; i++)
        {
            take_one_reading(i, param2, false, false);
        }
        output_test_readings(&test);
        result = test.number_of_readings;
        break;
        //
        //  CLOUD FUNCTIONS
        //
    case 400: // check cache
        result = (int)test_in_cache();
        break;
    case 401: // clear cache
        clear_cache();
        result = 1;
        break;
    case 402: // load cached record
        output_cache();
        break;
    case 403: // upload test
        device_state.transition_to(DeviceMode::UPLOADING_RESULTS);
        result = 1;
        break;
    case 404: // check assay files
        result = list_assay_files();
        break;
    case 405: // clear assay files
        result = clear_assay_files();
        break;
    // ===== COMMUNICATION COMMANDS =====
    case 8000:
        displayHelp();
        break;
    case 8001:
        displayStatus();
        break;
    // ===== WIFI COMMANDS =====
    case 8100:
        Serial.println("➜ WiFi OFF");
        WiFi.disconnect();
        WiFi.off();
        radios.wifi = false;
        Serial.println("✓ WiFi DISABLED");
        break;

    case 8101:
        Serial.println("➜ WiFi ON");
        WiFi.on();
        radios.wifi = true;
        Serial.println("✓ WiFi ENABLED");
        break;

    // ===== CELLULAR COMMANDS =====
    case 8200:
        Serial.println("➜ Cellular OFF");
        Cellular.disconnect();
        Cellular.off();
        radios.cellular = false;
        Serial.println("✓ Cellular DISABLED");
        break;

    case 8201:
        Serial.println("➜ Cellular ON");
        Cellular.on();
        radios.cellular = true;
        Serial.println("✓ Cellular ENABLED");
        break;

    // ===== BLUETOOTH COMMANDS =====
    case 8300:
        Serial.println("➜ Bluetooth OFF");
        BLE.off();
        radios.bluetooth = false;
        Serial.println("✓ Bluetooth DISABLED");
        break;

    case 8301:
        Serial.println("➜ Bluetooth ON");
        BLE.on();
        radios.bluetooth = true;
        Serial.println("✓ Bluetooth ENABLED");
        break;

    // ===== MASTER COMMANDS =====
    case 8900:
        Serial.println("➜ ALL RADIOS OFF");
        disableAllRadios();
        Serial.println("✓ RADIO SILENCE ACTIVE");
        break;

    case 8901:
        Serial.println("➜ ALL RADIOS ON");
        enableAllRadios();
        Serial.println("✓ ALL RADIOS ENABLED");
        break;

    // ===== TEST MODES =====
    case 8500:
        testWiFiOnly();
        break;

    case 8501:
        testCellularOnly();
        break;

    case 8502:
        testBluetoothOnly();
        break;

    // ===== EMISSION CHECK =====
    case 8999:
        emissionCheck();
        break;

    default:
        result = 0;
    }

    Log.info("Completed command: %d, result: %d, p1: %d, p2: %d, p3: %d, p4: %d, p5: %d", cmd, result, param1, param2, param3, param4, param5);

    return result;
}

int test_runner(String cartridgeId)
{
    if (device_state.detector_on)
    {
        device_state.cartridge_state = CartridgeState::DETECTED;
        if (cartridgeId.length() == BARCODE_UUID_LENGTH)
        {
            strcpy(barcode_uuid, cartridgeId.c_str());
            device_state.transition_to(DeviceMode::VALIDATING_CARTRIDGE);
            return 0;
        }
        else
        {
            device_state.transition_to(DeviceMode::IDLE);
            Log.info("Invalid cartridge ID: %s", cartridgeId.c_str());
            return cartridgeId.length();
        }
    }
    else
    {
        device_state.cartridge_state = CartridgeState::NOT_INSERTED;
        Log.info("Cartridge not inserted");
        return -1;
    }
}

int load_assay(String assayId)
{
    if (assayId.length() == ASSAY_UUID_LENGTH)
    {
        publish_load_assay(assayId);
        return 0;
    }
    else
    {
        Log.info("Invalid assay ID: %s", assayId.c_str());
        return assayId.length();
    }
}

int reset_cartridge(String cartridgeId)
{
    if (cartridgeId.length() == BARCODE_UUID_LENGTH)
    {
        memcpy(reset_uuid, cartridgeId.c_str(), BARCODE_UUID_LENGTH + 1);
        publish_reset_cartridge();
        return 0;
    }
    else
    {
        Log.info("Invalid cartridge ID: %s", cartridgeId.c_str());
        return cartridgeId.length();
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           TESTS                         //
//                                                         //
/////////////////////////////////////////////////////////////

void reset_device_state()
{
    device_state.reset_to_idle();
    device_state.test_state = TestState::NOT_STARTED;
    device_state.cartridge_state = CartridgeState::NOT_INSERTED;
    device_state.cloud_operation_pending = false;
    device_state.last_error = "";
    
    // Clear UUIDs
    memset(barcode_uuid, 0, BARCODE_UUID_LENGTH + 1);
    memset(test.cartridge_id, 0, BARCODE_UUID_LENGTH + 1);
    memset(test.assay_id, 0, ASSAY_UUID_LENGTH + 1);
    memset(assay.id, 0, ASSAY_UUID_LENGTH + 1);
    
    Log.info("Device state reset to IDLE");
}

void disconnect_from_cloud()
{
    int tries = 5;
    Log.info("Disconnecting from cloud...");
    Particle.disconnect();
    while (Particle.connected() && tries-- > 0)
    {
        Particle.disconnect();
        delay(PARTICLE_CLOUD_DELAY);
    }
    if (!Particle.connected())
    {
        Log.info("Disconnected from the cloud");
    }
    else
    {
        Log.info("Failed to disconnect from the cloud");
    }
}

void connect_to_cloud()
{
    int tries = 5;
    Log.info("Connecting to cloud...");
    while (!Particle.connected() && tries-- > 0)
    {
        Particle.connect();
        delay(PARTICLE_CLOUD_DELAY);
    }
    if (Particle.connected())
    {
        Log.info("Connected to cloud");
    }
    else
    {
        Log.info("Failed to connect to cloud");
    }
}

void output_test_readings(BrevitestTestRecord *t)
{
    if (t->number_of_readings > 0)
    {
        Serial.printlnf("Test %s", t->cartridge_id);
        Serial.printlnf("Data format: %c, assay: %s, duration: %d, start_time: %lu", t->data_format_code, t->assay_id, t->duration, t->start_time);
        Serial.printlnf("astep: %d, atime: %d, again: %d, baseline_scans: %d, test_scans: %d, number_of_readings: %d", t->astep, t->atime, t->again, t->baseline_scans, t->test_scans, t->number_of_readings);
        print_spectrophotometer_heading();
        for (int i = 0; i < t->number_of_readings; i++)
        {
            BrevitestSpectrophotometerReading *r = &(t->reading[i]);
            Serial.printlnf("%d\t%c\t\t%d\t\t%d\t%lu\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d\t\t%d", r->number, r->channel, r->position, r->temperature, r->msec, r->laser_output, r->f1, r->f2, r->f3, r->f4, r->f5, r->f6, r->f7, r->f8, r->clear, r->nir);
        }
    }
    else
    {
        Log.info("No test readings found");
    }
}

/**
 * @brief Run the main test execution
 * 
 * This function initiates the test execution process. It sets up
 * the hardware, disconnects from cloud to avoid interference,
 * and starts the BCODE test execution.
 */
void run_test()
{
    // === SET TEST STATE ===
    device_state.test_state = TestState::RUNNING;
    
    // === DISCONNECT FROM CLOUD ===
    // Avoid cloud interference during test execution
    disconnect_from_cloud();

    // === SETUP HARDWARE ===
    turn_on_dont_touch_LED();
    reset_stage(false);
    move_stage_to_test_start_position();
    turn_on_buzzer_for_duration(1000, 600);
    turn_off_buzzer_timer();

    // === STOP TEMPERATURE CONTROL ===
    // Avoid interference during test execution
    stop_temperature_control();

    // === SAVE TEST INFO TO EEPROM ===
    memcpy(eeprom.running_test_uuid, test.cartridge_id, BARCODE_UUID_LENGTH + 1);
    memcpy(eeprom.running_assay_id, test.assay_id, ASSAY_UUID_LENGTH + 1);
    EEPROM.put(0, eeprom);

    // === INITIALIZE TEST DATA ===
    test.number_of_readings = 0;
    test.baseline_scans = 0;
    test.test_scans = 0;
    memset(test.reading, 0, sizeof(test.reading));

    // === EXECUTE TEST ===
    Log.info("Running test %s", test.cartridge_id);
    test.start_time = millis();
    process_BCODE(0);
    test.duration = (millis() - test.start_time) / 1000;
    Log.info("Test %s finished, duration: %d sec", test.cartridge_id, test.duration / 1000);

    // === RESTART TEMPERATURE CONTROL ===
    start_temperature_control();

    // === CHECK TEST COMPLETION STATUS ===
    if (device_state.test_state == TestState::CANCELLED)
    {
        Log.info("Test cancelled");
        test.number_of_readings = 0;
    }
    else
    {
        Log.info("Test completed successfully");
        device_state.test_state = TestState::COMPLETED;
    }

    // === SAVE TEST RESULTS ===
    write_test_to_file();
    output_test_readings(&test);
    
    // === CLEAR EEPROM TEST INFO ===
    memset(eeprom.running_test_uuid, 0, BARCODE_UUID_LENGTH + 1);
    memset(eeprom.running_assay_id, 0, ASSAY_UUID_LENGTH + 1);
    EEPROM.put(0, eeprom);

    // === TRANSITION TO UPLOAD MODE ===
    device_state.cartridge_state = CartridgeState::TEST_COMPLETE;
    device_state.test_state = TestState::UPLOAD_PENDING;
    device_state.transition_to(DeviceMode::UPLOADING_RESULTS);

    // === CLEANUP AND RECONNECT ===
    reset_stage(true);
    reset_device_state();
    connect_to_cloud();
    turn_on_buzzer_alert();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                          SETUP                          //
//                                                         //
/////////////////////////////////////////////////////////////

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
    delayMicroseconds(10000);

    return Wire.isEnabled();
}

/**
 * @brief Device setup and initialization
 * 
 * This function initializes the device hardware, sets up the state machine,
 * and prepares the device for operation. It replaces the previous scattered
 * initialization with a structured approach using the state machine.
 */
void setup()
{
    // === SERIAL COMMUNICATION SETUP ===
    Serial.begin(115200); // standard serial port
    waitFor(Serial.isConnected, 15000);
    delayMicroseconds(100000);
    Log.info("====== Serial Connected, Begin Setup ======");

    // === CARTRIDGE DETECTOR INITIALIZATION ===
    init_digital_pin(pinCartridgeDetected, INPUT_PULLUP);
    device_state.detector_on = digitalRead(pinCartridgeDetected) == LOW;
    if (device_state.detector_on)
    {
        device_state.cartridge_state = CartridgeState::DETECTED;
        turn_on_remove_cartridge_LED();
    }
    else
    {
        // === NO CARTRIDGE DETECTED ===
        device_state.cartridge_state = CartridgeState::NOT_INSERTED;
        turn_on_dont_touch_LED();
    }

    // === HARDWARE PIN INITIALIZATION ===
    init_analog_pin(pinBuzzer, OUTPUT, 0);
    init_digital_pin(pinStageLimit, INPUT_PULLUP);
    init_digital_pin(pinBarcodeTrigger, OUTPUT, HIGH);
    init_digital_pin(pinBarcodeReady, INPUT_PULLUP);

    // === LASER SYSTEM INITIALIZATION ===
    init_digital_pin(pinLaserA, OUTPUT, LOW);
    laserA.power_pin = pinLaserA;
    init_analog_pin(pinPhotoA, INPUT);
    laserA.value_pin = pinPhotoA;

    init_digital_pin(pinLaserB, OUTPUT, LOW);
    laserB.power_pin = pinLaserB;
    init_analog_pin(pinPhotoB, INPUT);
    laserB.value_pin = pinPhotoB;

    init_digital_pin(pinLaserC, OUTPUT, LOW);
    laserC.power_pin = pinLaserC;
    init_analog_pin(pinPhotoC, INPUT);
    laserC.value_pin = pinPhotoC;

    // === HEATER AND MOTOR INITIALIZATION ===
    init_digital_pin(pinHeater, OUTPUT, LOW);
    init_digital_pin(pinMotorReset, OUTPUT, HIGH);
    init_digital_pin(pinMotorSleep, OUTPUT, LOW);
    init_digital_pin(pinMotorStep, OUTPUT, LOW);
    init_digital_pin(pinMotorDir, OUTPUT, LOW);

    // === DEVICE ID AND LOGGING ===
    device_id = System.deviceID();
    Log.info("Device ID: %s", device_id.c_str());

    // === PARTICLE CLOUD VARIABLES ===
    Particle.variable("temperature", current_temperature);
    Particle.variable("magnet_validation", magnet_validation_data);

    // === PARTICLE CLOUD FUNCTIONS ===
    Particle.function("load_assay", load_assay);
    Particle.function("set_wifi_credentials", set_wifi_credentials);
    Particle.function("run_test", test_runner);
    Particle.function("reset_cartridge", reset_cartridge);

    // === PARTICLE CLOUD SUBSCRIPTIONS ===
    // Success responses
    Particle.subscribe(String(device_id + "/hook-response/load-assay/"), response_load_assay);
    Particle.subscribe(String(device_id + "/hook-response/validate-cartridge/"), response_validate_cartridge);
    Particle.subscribe(String(device_id + "/hook-response/reset-cartridge/"), response_reset_cartridge);
    Particle.subscribe(String(device_id + "/hook-response/upload-test/"), response_upload_test);

    // Error responses
    Particle.subscribe(String(device_id + "/hook-error/load-assay/"), response_error);
    Particle.subscribe(String(device_id + "/hook-error/validate-cartridge/"), response_error);
    Particle.subscribe(String(device_id + "/hook-error/reset-cartridge/"), response_error);
    Particle.subscribe(String(device_id + "/hook-error/upload-test/"), response_error);

    // === EEPROM SETUP ===
    setup_eeprom();
    Log.info("Size of eeprom: %d", sizeof(Particle_EEPROM));

    // === DIRECTORY CREATION ===
    create_dir_if_not_exists("/cache");
    create_dir_if_not_exists("/buffer");
    create_dir_if_not_exists("/validation");
    create_dir_if_not_exists("/assay");

    // === INTERRUPT SETUP ===
    attachInterrupt(pinCartridgeDetected, detector_changed_interrupt, CHANGE);

    // === I2C BUS INITIALIZATION ===
    if (startI2C())
    {
        Log.info("I2C bus started");
    }
    else
    {
        Log.info("Could not start I2C bus");
    }
    
    // === SPECTROPHOTOMETER INITIALIZATION ===
    init_spectrophotometer_switch();
    power_off_all_spectrophotometers();

    // === STAGE RESET ===
    Log.info("Resetting stage");
    reset_stage(true);

    // === LED TESTING ===
    Log.info("Testing LEDs");
    turn_on_laser_for_duration('A', 250);
    turn_on_laser_for_duration('B', 250);
    turn_on_laser_for_duration('C', 250);

    // === BUZZER TESTING ===
    Log.info("Buzzing");
    turn_on_buzzer_for_duration(250, 330);
    buzzer_timer.start();

    // === STATE MACHINE INITIALIZATION ===
    device_state.mode = DeviceMode::INITIALIZING;
    device_state.test_state = TestState::NOT_STARTED;
    device_state.cartridge_state = CartridgeState::NOT_INSERTED;
    device_state.cloud_operation_pending = false;

    // === CHECK FOR INTERRUPTED TEST ===
    bool test_interrupted = eeprom.running_test_uuid[0] != '\0';
    if (test_interrupted)
    {
        Log.info("Test interrupted: %s (Assay %s) - saving cancelled test to file", eeprom.running_test_uuid, eeprom.running_assay_id);
        memcpy(test.cartridge_id, eeprom.running_test_uuid, BARCODE_UUID_LENGTH + 1);
        memcpy(test.assay_id, eeprom.running_assay_id, ASSAY_UUID_LENGTH + 1);
        write_test_to_file();
        memset(eeprom.running_test_uuid, 0, BARCODE_UUID_LENGTH + 1);
        memset(eeprom.running_assay_id, 0, ASSAY_UUID_LENGTH + 1);
        EEPROM.put(0, eeprom);
    }

    // === FINAL STATE RESET ===
    reset_device_state();

    // === LOGGING ===
    Log.info("device id: %s", device_id.c_str());
    Log.info("Firmware version: %d", eeprom.firmware_version);
    Log.info("Data format version: %d", eeprom.data_format_version);
    Log.info("Lifetime stress test cycles: %d", eeprom.lifetime_stress_test_cycles);
    Log.info("Stress test cycles since reset: %d", eeprom.stress_test_cycles_since_reset);
    Log.info("Last stress test cycles: %d", eeprom.stress_test_cycles);
    Log.info("Interrupted test ? %c", test_interrupted ? 'Y' : 'N');
    Log.info("Cached test ? %c", test_in_cache() ? 'Y' : 'N');

    // === START TEMPERATURE CONTROL ===
    start_temperature_control();

    // === LOAD MAGNETOMETER VALIDATION ===
    load_latest_magnet_validation(false);

    // === TRANSITION TO IDLE STATE ===
    // Setup complete - device is ready for operation
    device_state.transition_to(DeviceMode::IDLE);
    
    Log.info("Setup complete");
}

/////////////////////////////////////////////////////////////
//                                                         //
//                   DEVICE INDICATORS                     //
//                                                         //
/////////////////////////////////////////////////////////////

bool heater_debounced()
{
    if (heater_debouncing_in_progress)
    {
        if (millis() > heater_debounce_time)
        {
            heater_debouncing_in_progress = false;
            heater_debounce_time = 0;
            return heater_ready;
        }
        else
        {
            return false;
        }
    }
    else if (previous_heater_ready != heater_ready)
    {
        heater_debouncing_in_progress = true;
        heater_debounce_time = millis() + HEATER_READY_DEBOUNCE_DELAY;
        return false;
    }
    return heater_ready;
}

/**
 * @brief Set device indicators based on state machine
 * 
 * This function replaces the previous complex boolean flag checking
 * with a clear switch statement based on the device state machine.
 * Each state has appropriate LED and buzzer indicators.
 */
void set_device_indicators()
{
    switch (device_state.mode) {
        case DeviceMode::STRESS_TESTING:
        case DeviceMode::RUNNING_TEST:
        case DeviceMode::BARCODE_SCANNING:
        case DeviceMode::VALIDATING_CARTRIDGE:
        case DeviceMode::VALIDATING_MAGNETOMETER:
            // === ACTIVE OPERATION STATES ===
            // Device is busy - don't touch
            turn_on_dont_touch_LED();
            break;
            
        case DeviceMode::ERROR_STATE:
            // === ERROR STATE ===
            if (device_state.is_cartridge_invalid()) {
                // Invalid cartridge - remove it
                turn_on_remove_cartridge_LED();
                turn_on_buzzer_problem();
            } else {
                // Other error - don't touch
                turn_on_dont_touch_LED();
            }
            break;
            
        case DeviceMode::HEATING:
            // === HEATING STATE ===
            if (device_state.detector_on) {
                // Cartridge inserted while heating - remove it
                turn_on_remove_cartridge_LED();
            } else {
                // Heating up - don't touch
                turn_on_dont_touch_LED();
            }
            break;
            
        case DeviceMode::IDLE:
            // === IDLE STATE ===
            if (device_state.detector_on) {
                // Cartridge present but not processed - remove it
                turn_on_remove_cartridge_LED();
                turn_on_buzzer_alert();
            } else {
                // Ready for cartridge insertion
                turn_on_insert_cartridge_LED();
            }
            break;
            
        case DeviceMode::UPLOADING_RESULTS:
        case DeviceMode::RESETTING_CARTRIDGE:
            // === CLOUD OPERATION STATES ===
            // Device is communicating with cloud - don't touch
            turn_on_dont_touch_LED();
            break;
            
        case DeviceMode::INITIALIZING:
            // === INITIALIZATION STATE ===
            // Device is starting up - don't touch
            turn_on_dont_touch_LED();
            break;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

/**
 * @brief Barcode scanning loop - processes scanned barcodes
 * 
 * This function handles barcode scanning when the device is in
 * BARCODE_SCANNING mode. It determines the type of cartridge/magnetometer
 * inserted and transitions to the appropriate validation state.
 */
void barcode_scan_loop()
{
    int max_cycles;
    
    // Only scan if cartridge is detected and we're in barcode scanning mode
    if (device_state.detector_on && device_state.mode == DeviceMode::BARCODE_SCANNING)
    {
        switch (scan_barcode())
        {
        case BARCODE_TYPE_CARTRIDGE:
            // === REGULAR CARTRIDGE DETECTED ===
            device_state.cartridge_state = CartridgeState::BARCODE_READ;
            device_state.transition_to(DeviceMode::VALIDATING_CARTRIDGE);
            Log.info("Cartridge inserted");
            break;
            
        case BARCODE_TYPE_MAGNETOMETER:
            // === MAGNETOMETER DETECTED ===
            device_state.cartridge_state = CartridgeState::BARCODE_READ;
            device_state.transition_to(DeviceMode::VALIDATING_MAGNETOMETER);
            Log.info("Magnetometer inserted");
            break;
            
        case BARCODE_TYPE_STRESS_TEST:
            // === STRESS TEST CARTRIDGE DETECTED ===
            device_state.cartridge_state = CartridgeState::BARCODE_READ;
            max_cycles = atoi(&barcode_uuid[12]);
            start_stress_test(max_cycles, LED_DEFAULT_POWER);
            device_state.transition_to(DeviceMode::STRESS_TESTING);
            Log.info("Stress test started, max_cycles = %d", max_cycles);
            break;
            
        default:
            // === UNKNOWN BARCODE FORMAT ===
            device_state.cartridge_state = CartridgeState::INVALID;
            device_state.set_error("Unknown barcode format");
            Log.info("Unknown barcode format");
        }
    }
}

void stress_test_loop()
{
    if (device_state.mode != DeviceMode::STRESS_TESTING)
    {
        stop_stress_test();
    }
    else
    {
        do_stress_test_step(stress_test_step);
        stress_test_step++;
    }
}

void magnet_validation_loop()
{
    if (validate_magnets())
    {
        device_state.transition_to(DeviceMode::IDLE);
    }
}

/**
 * @brief Hardware loop - handles physical hardware state changes
 * 
 * This function manages the physical hardware state and coordinates
 * with the state machine for cartridge insertion/removal detection.
 * It replaces the previous scattered boolean flag management with
 * centralized state machine transitions.
 */
void hardware_loop()
{
    // === HEATER TEMPERATURE MONITORING ===
    previous_heater_ready = heater_ready;
    heater_ready = (heater.target_C_10X - heater.temp_C_10X) < HEATER_READY_TEMP_DELTA;
    device_state.heater_ready = heater_ready;

    // === CARTRIDGE DETECTION DEBOUNCING ===
    if (detector_debouncing)
    {
        if (millis() > detector_debouncing_time)
        {
            // Debouncing period complete - process the state change
            detector_debouncing_time = 0;
            detector_debouncing = false;
            detector_changed = false;
            bool new_detector_state = digitalRead(pinCartridgeDetected) == LOW;
            device_state.detector_on = new_detector_state;
            
            Log.info("%s detected", new_detector_state ? "Insertion" : "Removal");
            
            if (new_detector_state)
            {
                // === CARTRIDGE INSERTED ===
                reset_stage(false);
                move_stage_to_test_start_position();
                sleep_motor();
                
                // Check if we can start barcode scanning (heater ready + valid transition)
                if (heater_ready && device_state.can_transition_to(DeviceMode::BARCODE_SCANNING))
                {
                    device_state.cartridge_state = CartridgeState::DETECTED;
                    device_state.transition_to(DeviceMode::BARCODE_SCANNING);
                    turn_on_buzzer_for_duration(BUZZER_INSERT_DURATION, BUZZER_INSERT_FREQUENCY);
                }
                else
                {
                    // Heater not ready - set error state
                    device_state.cartridge_state = CartridgeState::DETECTED;
                    device_state.set_error("Heater not ready for cartridge insertion");
                }
            }
            else
            {
                // === CARTRIDGE REMOVED ===
                // Reset everything to IDLE state
                reset_stage(true);
                turn_off_buzzer_timer();
                device_state.reset_to_idle();
            }
        }
    }
    else if (detector_changed)
    {
        // Start debouncing period to avoid false triggers
        detector_debouncing = true;
        detector_debouncing_time = millis() + (DETECTOR_DEBOUNCE_DELAY);
    }

    // === UPDATE DEVICE INDICATORS ===
    // LED and buzzer states are now determined by the state machine
    set_device_indicators();

    // === TEMPERATURE CONTROL ===
    if (temperature_control_on)
    {
        set_heater_power(pid_controller());
    }

    // === BUZZER MANAGEMENT ===
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

/**
 * @brief Main loop - state machine driven operation
 * 
 * This is the core of the state machine implementation. Instead of
 * scattered boolean flags and complex if-else chains, the main loop
 * now uses a clear switch statement based on the current device mode.
 * Each state has specific actions that are appropriate for that mode.
 */
void loop()
{
    // === ALWAYS RUN THESE ===
    process_serial_port();  // Handle serial commands
    hardware_loop();        // Handle hardware state changes

    // === STATE-DRIVEN OPERATION ===
    // Each device mode has specific actions that are appropriate for that state
    switch (device_state.mode) {
        case DeviceMode::STRESS_TESTING:
            // === STRESS TEST MODE ===
            stress_test_loop();
            break;
            
        case DeviceMode::RESETTING_CARTRIDGE:
            // === CARTRIDGE RESET MODE ===
            // Only publish if not already waiting for response
            if (!device_state.cloud_operation_pending) {
                publish_reset_cartridge();
            }
            // Check for timeout if we're waiting for a response
            else if (device_state.cloud_operation_pending && device_state.is_cloud_operation_timeout(30000)) {
                Log.error("Cartridge reset timeout - no response from cloud");
                device_state.set_error("Cartridge reset timeout");
            }
            break;
            
        case DeviceMode::UPLOADING_RESULTS:
            // === TEST UPLOAD MODE ===
            // Only publish if not already waiting for response
            if (!device_state.cloud_operation_pending) {
                publish_upload_test();
            }
            // Check for timeout if we're waiting for a response
            else if (device_state.cloud_operation_pending && device_state.is_cloud_operation_timeout(30000)) {
                Log.error("Test upload timeout - no response from cloud");
                device_state.set_error("Test upload timeout");
            }
            break;
            
        case DeviceMode::BARCODE_SCANNING:
            // === BARCODE SCANNING MODE ===
            // Only scan if heater is ready (debounced)
            if (heater_debounced()) {
                barcode_scan_loop();
            }
            break;
            
        case DeviceMode::VALIDATING_CARTRIDGE:
            // === CARTRIDGE VALIDATION MODE ===
            // Check cloud connection first
            if (!Particle.connected()) {
                Log.error("Not connected to Particle cloud - cannot validate cartridge");
                device_state.set_error("No cloud connection");
                break;
            }
            // Only validate if heater ready and not already waiting for response
            if (heater_debounced() && !device_state.cloud_operation_pending) {
                publish_validate_cartridge();
            }
            // Check for timeout if we're waiting for a response
            else if (device_state.cloud_operation_pending && device_state.is_cloud_operation_timeout(30000)) {
                Log.error("Cartridge validation timeout - no response from cloud");
                device_state.set_error("Cartridge validation timeout");
            }
            break;
            
        case DeviceMode::VALIDATING_MAGNETOMETER:
            // === MAGNETOMETER VALIDATION MODE ===
            // Only validate if heater ready
            if (heater_debounced()) {
                magnet_validation_loop();
            }
            break;
            
        case DeviceMode::IDLE:
        case DeviceMode::HEATING:
            // === IDLE/HEATING MODES ===
            // Wait for state changes from hardware_loop
            // No specific actions needed - hardware_loop handles transitions
            break;
            
        case DeviceMode::ERROR_STATE:
            // === ERROR STATE ===
            // Could add recovery logic here in the future
            // For now, error state is handled by set_device_indicators()
            break;
            
        case DeviceMode::RUNNING_TEST:
            // === TEST EXECUTION MODE ===
            // Test is running in BCODE - no additional action needed in main loop
            // BCODE execution is handled by process_BCODE() function
            break;
            
        case DeviceMode::INITIALIZING:
            // === INITIALIZATION MODE ===
            // Device startup - handled in setup()
            // Should not reach here in normal operation
            break;
    }
}
