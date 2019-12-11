/*#include "LSM6DS3_L3.h"
#include "BaroSensor.h"*/
#include "main.h"
#include "Serial4/Serial4.h"

SYSTEM_THREAD(ENABLED);
PRODUCT_ID(4347);
PRODUCT_VERSION(FIRMWARE_VERSION);



/////////////////////////////////////////////////////////////
//                                                         //
//                        TABLES                           //
//                                                         //
/////////////////////////////////////////////////////////////

//  temperature is 10x to get one decimal place of accuracy
static int table_temperature[] = { 1000, 950, 900, 850, 800, 750, 700, 650, 600, 550, 500, 450, 400, 350, 300, 250, 200, 150, 100, 50, 0 };
// new thermistor
static int table_raw[] = { 3606, 3540, 3464, 3377, 3279, 3168, 3043, 2903, 2748, 2578, 2394, 2199, 1994, 1784, 1573, 1365, 1166, 979, 809, 657, 525 };
// old thermistor
/*static int table_raw[] = { 1707, 1566, 1426, 1288, 1154, 1026, 904, 790, 684, 587, 499, 421, 352, 291, 239, 194, 156, 125, 98, 77, 59 };*/
/*static int table_ratio[] = { 6975, 8054, 9336, 10867, 12703, 14917, 17598, 20864, 24862, 29784, 35882, 43481, 53015, 65055, 80371, 100000, 125353, 158371, 201746, 259246, 336206 };*/

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

int raw_table_lookup(int raw) {
    int result, indx1, indx2;

    if (raw > table_raw[0]) {
        return 1000;
    }

    for (indx1 = 0, indx2 = 1; indx1 < (TERMISTOR_TABLE_LENGTH - 1); indx1++, indx2++) {
        if (raw <= table_raw[indx1] && raw > table_raw[indx2]) {
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

uint32_t checksum(char *buf, int size) {
	uint8_t *p;
    uint32_t crc = ~0U;

	p = (uint8_t *) buf;

	while (size--)
		crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);

	return crc ^ ~0U;
}

int integerSqrt(int n) {
    int shift, nShifted, result, candidateResult;

    if (n < 0) {
        return -1;
    }

    shift = 2;
    nShifted = n >> shift;
    while ((nShifted != 0) && (nShifted != n)) {
        shift += 2;
        nShifted = n >> shift;
    }
    shift -= 2;

    result = 0;
    while (shift >= 0) {
        result <<= 1;
        candidateResult = result + 1;
        if ((candidateResult * candidateResult) <= (n >> shift)) {
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

void erase_test_cache() {
        int *ptr;
        int i;

        Serial.println("Erasing test cache");
        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                ptr = &eeprom.test_cache[i].start_time;
                memset(ptr, '\0', sizeof(BrevitestTestRecord));
        }
				eeprom.most_recent_test = 255;
}

int reset_eeprom() {
        Particle_EEPROM e;

				reset_param();
        erase_test_cache();

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

        analogWrite(pinSolenoid, surge);
        delay(eeprom.param.solenoid_surge_period_ms);

        if (sustain_time) {
            analogWrite(pinSolenoid, sustain, SOLENOID_PWM_FREQUENCY);
            delay(sustain_time);
        }

        analogWrite(pinSolenoid, 0);

        /*Serial.printlnf("surge: %d, surge_time: %d, sustain: %d, sustain_time: %d", surge, eeprom.param.solenoid_surge_period_ms, sustain, sustain_time);*/
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        STEPPER                          //
//                                                         //
/////////////////////////////////////////////////////////////

void move_steps(int steps, int step_delay){
        // rotate a specific number of steps - negative for reverse movement
		// steps: positive => move proximally, negative => move distally
		// controller: dir = LOW => move proximally, dir = HIGH => move distally

		int abs_steps, dir, i;

        dir = (steps > 0) ? LOW : HIGH;
		digitalWrite(pinStepper_Dir, dir);
		/*Serial.printlnf("Stepping, dir = %c", dir == LOW ? 'L' : 'H');*/

        abs_steps = abs(steps);
        for(i = 0; i < abs_steps; i++) {
                /*if (cancelling_test) {
                        break;
                }*/

                /*if (Particle.connected() && (i % MOVE_STEPS_BETWEEN_PARTICLE_PROCESS) == 0) {
                        Particle.process();
                }*/

                if ((dir == LOW) && (digitalRead(pinLimitSwitch) == LOW)) {
                    delay(20);  // debounce
                    if (digitalRead(pinLimitSwitch) == LOW) {
						Serial.println("Carriage limit switch detected");
                        cumulative_steps = 0;
                        break;
                    }
                }

                if (dir == HIGH) {
                    if (cumulative_steps >= CUMULATIVE_STEP_LIMIT) {
						Serial.println("Carriage distal limit reached");
                        break;
                    }
                }

                digitalWrite(pinStepper_Step, HIGH);
                delayMicroseconds(step_delay);

                digitalWrite(pinStepper_Step, LOW);
                delayMicroseconds(step_delay);

                cumulative_steps += dir == LOW ? -1 : 1;
        }
		/*Serial.printlnf("Move complete, cumulative steps = %d, step limit = %d", cumulative_steps, CUMULATIVE_STEP_LIMIT);*/
        //sleep_stepper();
}

void wake_move_sleep_stepper(int steps, int step_delay) {
    wake_stepper();
    move_steps(steps, step_delay);
    sleep_stepper();
}

void sleep_stepper() {
    digitalWrite(pinStepper_Sleep, LOW);
}

void wake_stepper() {
    digitalWrite(pinStepper_Sleep, HIGH);
    /*delay(eeprom.param.stepper_wake_delay_ms);*/
    delay(10);
}

void reset_stage() {
        cumulative_steps = CUMULATIVE_STEP_LIMIT;
        wake_stepper();
        move_steps(14000, RESET_STEP_DELAY);
        move_steps(-STEPS_TO_STARTING_POSITION, RESET_STEP_DELAY);
        /*move_steps(-eeprom.param.reset_steps, eeprom.param.step_delay_us);
        move_steps(STEPS_TO_STARTING_POSITION + eeprom.param.steps_to_calibration_point, eeprom.param.step_delay_us);*/
        sleep_stepper();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                  BARCODE SCANNER                        //
//                                                         //
/////////////////////////////////////////////////////////////

int scan_barcode() {
        unsigned long timeout;
        int i = 0;
        bool read_success = false;
		int buf;

        barcode_being_scanned = true;

        /*Serial.println("Start scan_barcode");
        Serial1.begin(9600); // barcode scanner interface through RX/TX pins

        Serial.println("Trigger barcode reader");
        digitalWrite(pinBarcode_Trigger, LOW);

        timeout = millis() + BARCODE_READ_TIMEOUT;

        Serial.println("Wait for success");
        do {
            read_success = digitalRead(pinBarcode_Success) == HIGH;
			Serial.print('.');
            Particle.process();
            delay(200);
        } while (!read_success && millis() < timeout);

        Serial.printlnf("Stop triggering barcode reader, ready=%c", Serial1.available() ? 'Y' : 'N');
        digitalWrite(pinBarcode_Trigger, HIGH);

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
                        barcode_uuid[i++] = (char) buf;
                }
        } while (Serial1.available() && i < CARTRIDGE_UUID_LENGTH);
        Serial.println();
        Serial.printlnf("Barcode: %s, length: %d", barcode_uuid, i);

        if (i < CARTRIDGE_UUID_LENGTH) {
            memcpy(barcode_uuid, CARTRIDGE_ERROR_UUID, CARTRIDGE_UUID_LENGTH);
        }
        barcode_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
        Serial.printlnf("Barcode: %s, length: %d", barcode_uuid, i);

        Serial1.end();
        Serial.println("Finish reading barcode");*/

        barcode_being_scanned = false;

        return i;
}

/////////////////////////////////////////////////////////////
//                                                         //
//                        BUZZER                           //
//                                                         //
/////////////////////////////////////////////////////////////

void turn_on_buzzer_for_duration(int frequency, int duration) {
    if (serial_messaging_on) Serial.printlnf("Turning on buzzer at frequency %d for duration %d", frequency, duration);
    tone(pinBuzzer, frequency, duration);
}

void play_startup_tune() {
	turn_on_buzzer_for_duration(262, 250);
	delay(50);
	turn_on_buzzer_for_duration(294, 250);
	delay(50);
	turn_on_buzzer_for_duration(330, 250);
	delay(50);
	turn_on_buzzer_for_duration(349, 250);
	delay(50);
	turn_on_buzzer_for_duration(392, 250);
	delay(50);
	turn_on_buzzer_for_duration(440, 250);
	delay(50);
	turn_on_buzzer_for_duration(494, 250);
	delay(50);
	turn_on_buzzer_for_duration(523, 250);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                       HEATER                            //
//                                                         //
/////////////////////////////////////////////////////////////

void turn_on_heater(int power) {
	power = power > HEATER_MAX_POWER ? HEATER_MAX_POWER : (power < 0 ? 0 : power);
    analogWrite(heater.heater_pin, power, HEATER_PWM_FREQUENCY);
	heater.power = power;
    heater.heater_on = true;
}

void turn_off_heater() {
    analogWrite(heater.heater_pin, 0);
    heater.heater_on = false;
	heater.power = 0;
}

int limit(int value, int max, int min) {
	return value > max ? max : (value < min ? min : value);
}

int pulse_heater(int power) {
	unsigned long start = millis();
	power = limit(power, HEATER_MAX_POWER, 0);
	analogWrite(heater.heater_pin, power, HEATER_PWM_FREQUENCY);
	if (power > 0) {
		delay(pulse_duration);
		analogWrite(heater.heater_pin, 0);
	}
	heater.power = power;
	heater.heater_on = power != 0;
	if (heater.heater_on) {
		if (serial_messaging_on) Serial.printlnf("Heater set to power %d", power);
	}

	return (int) (millis() - start);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                         LEDS                            //
//                                                         //
/////////////////////////////////////////////////////////////

void turn_on_LED(char channel, int power) {
	if (channel == 'A') {
		turn_on_assay_LED(power);
	}
	else {
		turn_on_control_LED(power);
	}
}

void turn_off_LED(char channel) {
	if (channel == 'A') {
		turn_off_assay_LED();
	}
	else {
		turn_off_control_LED();
	}
}

void turn_on_both_LEDs(int power) {
	analogWrite(pinLEDAssay, power);
	analogWrite(pinLEDControl, power);
}

void turn_on_assay_LED(int power) {
    analogWrite(pinLEDAssay, power);
}

void turn_on_control_LED(int power) {
	analogWrite(pinLEDControl, power);
}

void turn_off_assay_LED() {
    analogWrite(pinLEDAssay, 0);
}

void turn_off_control_LED() {
    analogWrite(pinLEDControl, 0);
}

void turn_off_both_LEDs() {
	analogWrite(pinLEDAssay, 0);
	analogWrite(pinLEDControl, 0);
}

void turn_on_assay_LED_for_duration(int duration, int power) {
    turn_on_assay_LED(power);
    delay(duration);
    turn_off_assay_LED();
}

void turn_on_control_LED_for_duration(int duration, int power) {
    turn_on_control_LED(power);
    delay(duration);
    turn_off_control_LED();
}

void turn_on_both_LEDs_for_duration(int duration, int power) {
    turn_on_assay_LED(power);
    turn_on_control_LED(power);
    delay(duration);
    turn_off_assay_LED();
    turn_off_control_LED();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                INERTIAL MEASUREMENT UNIT                //
//                                                         //
/////////////////////////////////////////////////////////////

void imu_read() {
	int temp_F_10X;

	Serial.printlnf("IMU accelerometer: X = %d, Y = %d, Z = %d", myIMU.readRawAccelX(), myIMU.readRawAccelY(), myIMU.readRawAccelZ());
    Serial.printlnf("IMU gyroscope: X = %d, Y = %d, Z = %d", myIMU.readRawGyroX(), myIMU.readRawGyroY(), myIMU.readRawGyroZ());

    int error, tempC_raw;
    error = myIMU.begin();
    if (error) {
        Serial.printlnf("Error setting up IMU, code: %d", error);
    }
    else {
        tempC_raw = myIMU.readRawTemp() + 400;
        imu_temp_C_10X = ((tempC_raw >> 4) * 10) + (((tempC_raw & 0x0F) * 6250) / 10000);
		temp_F_10X = ((imu_temp_C_10X * 9) / 5) + 320;
        Serial.printlnf("IMU temperature: %d.%d˚C, %d.%d˚F", imu_temp_C_10X / 10, imu_temp_C_10X % 10, temp_F_10X / 10, temp_F_10X % 10);
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//               BAROMETRIC PRESSURE SENSOR                //
//                                                         //
/////////////////////////////////////////////////////////////

void pressure_read() {
	int temp, press;

    BaroSensor.begin();
    BaroSensor.getTempAndPressure(&temp, &press, CELSIUS, OSR_512);
	Serial.printlnf("Temperature: %d.%d˚C, Pressure: %d.%d mmHg", temp / 10, temp % 10, press / 10, press % 10);
}

/////////////////////////////////////////////////////////////
//                                                         //
//                    OPTICAL SENSORS                      //
//                                                         //
/////////////////////////////////////////////////////////////

void test_optical_sensors() {
	read_optical_sensors_command_flag = true;
}

void config_optical_sensors(char channel, int param, int addr) {
    int bytes_received, bytes_sent, reg, result;

    if (serial_messaging_on) Serial.printlnf("Configuring optical sensor %c", channel);
    Wire.beginTransmission(addr);
    bytes_sent = Wire.write(0x00);
    bytes_sent += Wire.write(0x02);	// enter configuration mode
    result = Wire.endTransmission(true);
    if (result != 0) {
        Serial.printlnf("Config optics %c, %d bytes, result: %d", channel, bytes_sent, result);
    }

    Wire.beginTransmission(addr);
    bytes_sent = Wire.write(0x06);
	bytes_sent += Wire.write((uint8_t) param);	// write param to CREG1
    result = Wire.endTransmission(true);
    if (result != 0) {
        Serial.printlnf("Config optics %c, %d bytes, result: %d", channel, bytes_sent, result);
    }

	Wire.beginTransmission(addr);
	bytes_sent = Wire.write(0x06);	// confirm register was written
	result = Wire.endTransmission(false);
	if (result != 0) {
		Serial.printlnf("Send error, %d bytes, result: %d", bytes_sent, result);
	}
	bytes_received = Wire.requestFrom(addr, 1);

	reg = Wire.read();
	if (serial_messaging_on) Serial.printlnf("param = %d, CREG1 = %d", param, reg);
}

bool optical_sensor_ready(uint8_t addr) {
	int bytes, reg, result;
	uint8_t osr, status, lsb, msb;

	Wire.beginTransmission(addr);
	bytes = Wire.write(0x00);
	result = Wire.endTransmission(false);
	if (result != 0) {
		Serial.printlnf("Send error, %d bytes, result: %d", bytes, result);
	}
	bytes = Wire.requestFrom(addr, 4);

	// status
	osr = Wire.read();
	status = Wire.read();
	if (serial_messaging_on) Serial.printlnf("Status = %d, OSR = %d", status, osr);

	return ((status & 0x04) == 0 && osr == 3);
}

void get_data_from_one_optical_sensor(char channel, int param, bool is_a_test) {
	int bytes, ready, ready_pin, result;
	uint8_t osr, status, addr, lsb, msb;
	uint16_t tempC, tempF, x, y, z, l_value;
	unsigned long timeout, duration;
	BrevitestOpticalSensorRecord *reading;

	if (is_a_test) {
	    reading = &(test_record.reading[test_record.number_of_readings % TEST_MAXIMUM_NUMBER_OF_READINGS]);
		test_record.number_of_readings++;
	}
	else {
		reading = channel == 'A' ? &reading_assay : &reading_control;
	}

	reading->x = reading->y = reading->z = reading->temperature = reading->time_ms = reading->samples = 0;

	if (channel == 'A') {
	    addr = 0x75;
	    ready_pin = pinAssaySensor_Ready;
	}
	else {
	    addr = 0x74;
	    ready_pin = pinControlSensor_Ready;
	}

	duration = millis();

	config_optical_sensors(channel, param, addr);

	Wire.beginTransmission(addr);
	bytes = Wire.write(0x00);
	bytes += Wire.write(0x83);
	result = Wire.endTransmission(true);
  	if (result != 0) {
      	Serial.printlnf("Config optics %c, %d bytes, result: %d", channel, bytes, result);
  	}

	timeout = millis() + 5000;
	ready = optical_sensor_ready(addr);
	while (!ready && millis() < timeout) {
		/*Serial.print('.');*/
		turn_on_LED('A', LED_POWER);
		turn_on_LED('C', LED_POWER);
		delay(1);
		turn_off_LED('A');
		turn_off_LED('C');
		delay(1);
		ready = optical_sensor_ready(addr);
	}

	duration = millis() - duration;

	if (!ready) {
		Serial.printlnf("Read failure: %c", channel);
		return;
	}

	if (serial_messaging_on) Serial.printlnf("Starting optical sensor data %c read", channel);

	Wire.beginTransmission(addr);
	bytes = Wire.write(0x00);
	result = Wire.endTransmission(false);
	if (result != 0) {
		Serial.printlnf("Send error, %d bytes, result: %d", bytes, result);
	}
	bytes = Wire.requestFrom(addr, 10);

	// status
	osr = Wire.read();
	status = Wire.read();
	if (serial_messaging_on) Serial.printlnf("Status = %d, OSR = %d", status, osr);

    // temperature
    lsb = Wire.read();
	if (serial_messaging_on) Serial.printf("%d ", lsb);
    msb = Wire.read();
	if (serial_messaging_on) Serial.printf("%d ", msb);
    tempC = ((((msb << 8) + lsb) * 5) / 100) - 67;
    tempF = ((tempC * 9) / 5) + 32;

	// light
    lsb = Wire.read();
	if (serial_messaging_on) Serial.printf("%d ", lsb);
    msb = Wire.read();
	if (serial_messaging_on) Serial.printf("%d ", msb);
    x = (msb << 8) + lsb;

    lsb = Wire.read();
	if (serial_messaging_on) Serial.printf("%d ", lsb);
    msb = Wire.read();
	if (serial_messaging_on) Serial.printf("%d ", msb);
    y = (msb << 8) + lsb;

    lsb = Wire.read();
	if (serial_messaging_on) Serial.printf("%d ", lsb);
    msb = Wire.read();
	if (serial_messaging_on) Serial.printlnf("%d ", msb);
    z = (msb << 8) + lsb;

    reading->channel = channel;
    reading->x = x;
    reading->y = y;
    reading->z = z;
    reading->temperature = tempC;
    reading->time_ms = millis();
    reading->samples = 1;
		l_value = integerSqrt((x * x) + (y * y) + (z * z));

    Serial.printlnf("S: %c %d %d %d %d => T = %d˚C %d˚F, X = %d, Y = %d, Z = %d, L = %d", channel, millis() % 1000000, duration, osr, status, tempC, tempF, x, y, z, l_value);
}

bool enable_optical_sensors(bool force_read) {
    if (Wire.isEnabled()) {
        return true;
    }

    if (Wire1.isEnabled()) {    // is other I2C bus busy?
        if (force_read) {    // kill controller sensor bus if forced read
            Wire1.end();
            pinMode(pinControllerSensor_SCL, INPUT);
            pinMode(pinControllerSensor_SDA, INPUT);
        }
        else {
            return false;
        }
    }
    /*Serial.println("Attempting to read optical sensors");*/
    Wire.setSpeed(CLOCK_SPEED_100KHZ);
    Wire.begin();
    delay(100);

    return Wire.isEnabled();
}

void disable_optical_sensors() {
    Wire.end();
    pinMode(pinOpticalSensor_SCL, INPUT);
    pinMode(pinOpticalSensor_SDA, INPUT);
}

void read_optical_sensors(int param, bool is_a_test) {
	int l_value, d_x, d_y, d_z;
	unsigned long elapsed = millis();

  if (enable_optical_sensors(true)) {
    get_data_from_one_optical_sensor('A', param, is_a_test);
    get_data_from_one_optical_sensor('C', param, is_a_test);
  }
  else {
  	Serial.println("Unable to start communication with optical sensors");
  }

  disable_optical_sensors();

  if (serial_messaging_on) Serial.printlnf("Elapsed time: %u", millis() - elapsed);
}

void read_optical_sensor_baselines(int param) {
	BrevitestOpticalSensorRecord *a, *c;
	int prev_x_assay, prev_x_control, d_a, d_c;
	int tries = OPTICAL_BASELINE_MAX_READINGS;
	bool converged = false;
	unsigned long elapsed = millis();

  	if (enable_optical_sensors(true)) {
			prev_x_assay = prev_x_control = 0;
	  	while (!converged && tries-- > 0) {
				test_record.number_of_readings = 0;
				get_data_from_one_optical_sensor('A', param, true);
				get_data_from_one_optical_sensor('C', param, true);
				a = &(test_record.reading[0]);
				c = &(test_record.reading[1]);
				d_a = abs(a->x - prev_x_assay);
				d_c = abs(c->x - prev_x_control);
				converged = d_a < OPTICAL_BASELINE_THRESHOLD && d_c < OPTICAL_BASELINE_THRESHOLD;
				if (converged) {
					Serial.printlnf("d_a=%d, d_c=%d, converged=%c, tries=%d", d_a, d_c, converged ? 'Y' : 'N', OPTICAL_BASELINE_MAX_READINGS - tries);
				} else {
					prev_x_assay = a->x;
					prev_x_control = c->x;
				}
				delay(OPTICAL_SENSORS_TEST_INTERVAL);
	  	}
  }
  else {
  	Serial.println("Unable to start communication with optical sensors");
  }

  disable_optical_sensors();

  if (serial_messaging_on) Serial.printlnf("Elapsed time: %u", millis() - elapsed);
}

/////////////////////////////////////////////////////////////
//                                                         //
//               CONTROLLER SENSOR STATUS                  //
//                                                         //
/////////////////////////////////////////////////////////////

bool enable_controller_sensors(bool force_read) {
    if (Wire1.isEnabled()) {
        return true;
    }

    if (Wire.isEnabled()) { // is other I2C bus busy?
        Serial.println("Optical sensors busy");
        if (force_read) {    // kill optical sensor bus if forced read
            Wire.end();
            pinMode(pinOpticalSensor_SCL, INPUT);
            pinMode(pinOpticalSensor_SDA, INPUT);
        }
        else {
            return false;
        }
    }

    Wire1.setSpeed(CLOCK_SPEED_100KHZ);
    Wire1.begin();
    delay(100);

    return Wire1.isEnabled();
}

void disable_controller_sensors() {
    Wire1.end();
    pinMode(pinControllerSensor_SCL, INPUT);
    pinMode(pinControllerSensor_SDA, INPUT);
}

void read_all_controller_sensors() {
    if (enable_controller_sensors(false)) {
        imu_read();
        heater_temperature_read();
        pressure_read();
        disable_controller_sensors();
		if (serial_messaging_on) Serial.println();
    }
    else {
        if (serial_messaging_on) Serial.printlnf("Controller I2C busy, read skipped");
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//               TEMPERATURE CONTROL SYSTEM                //
//                                                         //
/////////////////////////////////////////////////////////////

int get_heater_temperature() {
    int raw = analogRead(heater.thermistor_pin);
    if (raw == 0) {
		stop_temperature_control();
    }
	else {
		heater.temp_C_10X = raw_table_lookup(raw);
		heater.temp_F_10X = ((heater.temp_C_10X * 9) / 5) + 320;
		if (heater.temp_C_10X > HEATER_MAX_TEMPERATURE) {
			stop_temperature_control();
			raw = 0;
		}
	}
    return raw;
}

void heater_temperature_read() {
	get_heater_temperature();
	if (serial_messaging_on) Serial.printlnf("Temperature: %d.%d˚C, %d.%d˚F", heater.temp_C_10X / 10, heater.temp_C_10X % 10, heater.temp_F_10X / 10, heater.temp_F_10X % 10);
}

int pid_controller() {
    int dt, error, derivative, raw;
	int output = 0;
    unsigned long current_read_time, prev_read_time;

	current_read_time = millis();
	prev_read_time = heater.read_time;
	if ((current_read_time - prev_read_time) >= HEATER_CONTROL_INTERVAL) {
		raw = get_heater_temperature();
	    heater.read_time = current_read_time;
	    if (raw != 0) {
	        if (prev_read_time == 0) {
	            heater.previous_error = 0;
	            heater.integral = 0;
	        }
	        else {
	            dt = heater.read_time - prev_read_time;
	            error = heater.target_C_10X - heater.temp_C_10X;
	            heater.integral += (error * dt) / 1000;
	            derivative = (1000 * (error - heater.previous_error)) / dt;
	            output = (heater.k_p_num * error) / heater.k_p_den;
				output += (heater.k_i_num * heater.integral) / heater.k_i_den;
				output += (heater.k_d_num * derivative) / heater.k_d_den;

	            if (serial_messaging_on) Serial.printlnf("raw = %d, T = %d.%d˚C, target = %d.%d, dt = %d, error = %d, integral = %d, derivative = %d, output = %d", raw, heater.temp_C_10X / 10, heater.temp_C_10X % 10, heater.target_C_10X / 10, heater.target_C_10X % 10, dt, error, heater.integral, derivative, output);
	            heater.previous_error = error;
	        }
	    }
	}

	return output;
}

void control_heater_temperature() {
	control_heater_temperature_flag = true;
}

void start_temperature_control() {
	heater.read_time = 0;
	control_heater_temperature_timer.start();
    Serial.println("Temperature control system started");
}

void stop_temperature_control() {
    control_heater_temperature_timer.stop();
	pulse_heater(0);
    Serial.println("Temperature control system stopped");
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
        cartridge_validated = false;
        brevitest_publish("validate-cartridge", barcode_uuid, false);
    }
    else {
        Serial.println("Validation failed - not connected to the cloud");
        waiting_for_validation = false;
        cartridge_validated = false;
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
    assay.optical_sensor_integration_time = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
    assay.optical_sensor_gain = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
    assay.reserved = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
    assay.delay_between_optical_sensor_readings_ms = extract_int_from_delimited_string(assayString, &indx, TAB_DELIM);
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
    cartridge_validated = (strncmp(callback_status, SUCCESS, 7) == 0);
    Serial.printlnf("Cartridge validated? %c", cartridge_validated ? 'Y' : 'N');
    if (cartridge_validated) {    // cartridge found
        if (load_assay_record(cartridgeId, assayString)) {
            starting_test = true;
        }
        else {
            Serial.println("Failed to load assay record");
        }
    }
    else {
        Serial.printlnf("cartridgeId: %s, assayString: %s", cartridgeId, assayString);
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
      Serial.printlnf("callback_buffer: %s, callback_complete: %c", callback_buffer, callback_complete ? 'Y' : 'N');
}

/////////////////////////////////////////////////////////////
//                                                         //
//                   EEPROM TEST CACHE                     //
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

int append_test_reading(int start, BrevitestOpticalSensorRecord *reading) {
    return sprintf(&(particle_register[start]), "%c\t%11d\t%5d\t%5d\t%5d\t%5d\n", \
        reading->channel, reading->time_ms, reading->x, reading->y, reading->z, reading->temperature);
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
        if (serial_messaging_on) Serial.printlnf("Looking for test to upload");
        for (i = 0; i < TEST_CACHE_SIZE; i += 1) {
                if (eeprom.test_cache[i].test_uuid[0] != '\0') {
                        if (serial_messaging_on) Serial.printlnf("Test found: %s", eeprom.test_cache[i].test_uuid[0]);
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
		Serial.printlnf("%s, %d percent complete, temp = %d.%d", message, test_percent_complete, heater.temp_C_10X / 10, heater.temp_C_10X % 10);
}

void BCODE_delay(int target_duration) {
	int i, pulse_time, residual_duration;
	unsigned long total_duration, cycle_start;
	int cycles = 1;

	total_duration = millis();
	if (target_duration >= HEATER_CONTROL_INTERVAL) {
		cycles = target_duration / HEATER_CONTROL_INTERVAL;
		for (i = 0; i < cycles; i++) {
			cycle_start = millis();
			pulse_heater(pid_controller());
			residual_duration = HEATER_CONTROL_INTERVAL - (int) (millis() - cycle_start);
			if (residual_duration > 0) {
				delay(residual_duration);
			}
		}
	}
	total_duration = millis() - total_duration;
	residual_duration = target_duration - (int) total_duration;
	if (serial_messaging_on) Serial.printlnf("BCODE_delay: target_duration = %d, cycles = %d, total_duration = %d, residual_duration = %d", target_duration, cycles, total_duration, residual_duration);
	if (residual_duration > 0) {
		delay(target_duration - (int) total_duration);
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
				update_progress("Starting test", 6000);
				turn_on_buzzer_for_duration(2000, 600);
				BCODE_delay(1000);
                break;
        case 1: // Delay(milliseconds)
                index = get_BCODE_token(index, &param1);
                update_progress("Pausing", param1);
                BCODE_delay(param1);
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
        case 4: // Buzzer on(milliseconds, frequency)
				index = get_BCODE_token(index, &param1);    // duration_ms
				index = get_BCODE_token(index, &param2);    // frequency
				update_progress("Buzzing", param1);
				turn_on_buzzer_for_duration(param1, param2);
                break;
        case 5: // unused
                break;
        case 6: // unused
                break;
        case 7: // take initial sensor reading using default param
				steps = cumulative_steps - OPTICAL_SENSOR_READ_POSITION;
				update_progress("Reading optical sensor baselines", 6000);
				read_optical_sensor_baselines(OPTICAL_SENSOR_DEFAULT_PARAM);
				break;
        case 8: // take initial sensor reading with param = param1
				index = get_BCODE_token(index, &param1); // params
				update_progress("Reading optical sensor baselines", 6000);
				read_optical_sensor_baselines(param1);
                break;
        case 9: // Read optical sensors with default values
				steps = cumulative_steps - OPTICAL_SENSOR_READ_POSITION;
				update_progress("Moving magnets to prepare for reading", (abs(steps) * RESET_STEP_DELAY) / 1000);
				move_steps(steps, RESET_STEP_DELAY);	// move stage away from optical sensors
				update_progress("Reading optical sensors", 6000);
                read_optical_sensors(OPTICAL_SENSOR_DEFAULT_PARAM, true);
                break;
        case 10: // Read optical sensors with parameters
                index = get_BCODE_token(index, &param1); // params
				steps = cumulative_steps - OPTICAL_SENSOR_READ_POSITION;
				update_progress("Moving magnets to prepare for reading", (abs(steps) * RESET_STEP_DELAY) / 1000);
				move_steps(steps, RESET_STEP_DELAY);	// move stage away from optical sensors
				update_progress("Reading optical sensors", 6000);
                read_optical_sensors(param1, true);
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
		case 14: // unused
				break;
		case 15: // set heater target temperature
				index = get_BCODE_token(index, &param1);
				update_progress("Setting heater target temperature", 0);
				if (param1 > 0 && param1 < HEATER_MAX_TEMPERATURE) {
					heater.target_C_10X = param1;
					heater.read_time = 0;
				}
				break;
        case 17: // Raster well
                index = get_BCODE_token(index, &param1);    // total steps
                index = get_BCODE_token(index, &param2);    // step_delay_us
                index = get_BCODE_token(index, &param3);    // number of rasters
                if (serial_messaging_on) Serial.printlnf("Raster well - total steps: %d, rasters: %d", param1, param3);
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
							update_progress("Rastering magnets", param7 + param8);
                            move_solenoid(param7);
                            BCODE_delay(param8);
                        }
						update_progress("Moving magnets", (abs(steps) * param2) / 1000);
                        move_steps(steps, param2);
                }

                index = mark;
                for (j = 0; j < param4; j++) {
                    if (j < param6) {
                        index = get_BCODE_token(index, &param7);    // energize time
                        index = get_BCODE_token(index, &param8);    // delay time
                    }
					update_progress("Rastering magnets", param7 + param8);
                    move_solenoid(param7);
                    BCODE_delay(param8);
                }

                steps = param1 - steps * (param3 - 1);
                if (steps) {
					update_progress("Moving magnets", (abs(steps) * param2) / 1000);
                    move_steps(steps, param2);
                }

				update_progress("Pausing to gather microspheres", param5);
                BCODE_delay(param5);   // gather beads
                break;
        case 18: // Well transit
                index = get_BCODE_token(index, &param1);    // step_delay_us
                index = get_BCODE_token(index, &param2);    // gather_time_ms
                index = get_BCODE_token(index, &param3);    // number of segments
                if (serial_messaging_on) Serial.printlnf("Well transit - segments: %d", param3);
                for (i = 0; i < param3; i++) {
                        if (cancelling_test) {
                                break;
                        }
                        // read segment data
                        index = get_BCODE_token(index, &param4);    // number of iterations
                        index = get_BCODE_token(index, &param5);    // steps per iteration
                        index = get_BCODE_token(index, &param6);    // delay between steps, in milliseconds
                        for (j = 0; j < param4; j++) {
							update_progress("Moving magnets", param6 + (abs(param5) * param1) / 1000);
                            move_steps(param5, param1);
                            BCODE_delay(param6);
                        }
                }
				update_progress("Pausing to gather microspheres", param2);
                BCODE_delay(param2);   // gather beads
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
//                         COMMAND                         //
//                                                         //
/////////////////////////////////////////////////////////////

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
			Serial.printlnf("Move stepper %d steps, cumulative %d", param1, cumulative_steps);
            return cumulative_steps;
        case 4: // read optical sensors (param)
            read_optical_sensors_command_param = param1;
            read_optical_sensors_command_flag = true;
            return param1;
        case 5: // not used
            return 0;
        case 6: // not used
            return 0;
        case 7: // read heater temperature
			param1 = get_heater_temperature();
			Serial.printlnf("Heater: raw = %d, T = %d.%d˚C", param1, heater.temp_C_10X / 10, heater.temp_C_10X % 10);
			return param1;
        case 8: // reset params
            reset_eeprom();
            return (int) eeprom.data_format_version;
        case 9: // fire solenoid
            move_solenoid(param1);
            return param1;
        case 10: // turn on assay LED for param1 milliseconds at power param2
            if (param1 > 10000 || param1 < 0) {
                param1 = 2000;
            }
			if (param2 > 800 || param2 < 0) {
                param2 = LED_POWER;
            }
            turn_on_assay_LED_for_duration(param1, param2);
            return param1;
        case 11: // turn on control LED for param1 milliseconds at power param2
            if (param1 > 10000 || param1 < 0) {
                param1 = 2000;
            }
			if (param2 > 800 || param2 < 0) {
                param2 = LED_POWER;
            }
            turn_on_control_LED_for_duration(param1, param2);
            return param1;
        case 12: // turn on both LEDs for param1 milliseconds at power param2
            if (param1 > 10000 || param1 < 0) {
                param1 = 2000;
            }
			if (param2 > 800 || param2 < 0) {
                param2 = LED_POWER;
            }
            turn_on_both_LEDs_for_duration(param1, param2);
            return param1;
        case 13: // turn on buzzer param1 frequency param2 duration
            turn_on_buzzer_for_duration(param1, param2);
            return 1;
        case 14: // not used
            return 0;
		case 15: // set heater target temperature
			if (param1 > 0 && param1 < HEATER_MAX_TEMPERATURE) {
				heater.target_C_10X = param1;
				heater.read_time = 0;
			}
            return param1;
		case 16: // not used
            return 0;
        case 17: // not used
			return 0;
		case 18: // turn on heater at power param1
			turn_on_heater(param1);
            return param1;
        case 19: // turn off heater
			turn_off_heater();
			return 1;
		case 20: // start temperature control
			start_temperature_control();
			return 1;
		case 21: // stop temperature control
			stop_temperature_control();
			return 1;
		case 22: // turn on serial messaging
			serial_messaging_on = true;
			return 1;
		case 23: // turn off serial messaging
			serial_messaging_on = false;
			return 0;
		case 24: // start reading sensors
			read_all_controller_sensors_timer.start();
			return 1;
		case 25: // stop reading sensors
			read_all_controller_sensors_timer.stop();
			return 1;
		case 26: // start reading sensors
			controller_i2c_bus_scan();
			return 1;
		case 27: // stop reading sensors
			optical_i2c_bus_scan();
			return 1;
		case 28: // start optical sensor reading test, param = param1
			serial_messaging_on = false;
			read_optical_sensors_command_param = param1;
			read_optical_sensors_command_flag = true;
			test_optical_sensors_timer.start();
			return 1;
		case 29: // stop optical sensor reading test
			test_optical_sensors_timer.stop();
			return 1;
		case 30: // scan barcode
			scan_barcode();
			return 1;
		case 31: // determine sensor baselines
			read_optical_sensor_baselines_command_param = param1;
			read_optical_sensor_baselines_command_flag = true;
			return 1;
		case 32: // set solenoid power
			if (param1 < 256) {
				param1 = param1 * 256 + param1;
			}
			eeprom.param.solenoid_power = param1;
			store_eeprom();
			move_solenoid(1000);
	}

    return 0;

}

/////////////////////////////////////////////////////////////
//                                                         //
//                    DEVICE STATE                         //
//                                                         //
/////////////////////////////////////////////////////////////

void cartridge_loaded_interrupt() {
    cartridge_state_changed = true;
}

void check_device_state() {
    cartridge_loaded = digitalRead(pinCartridgeLoaded) == LOW;

			if (cartridge_loaded) {
				ledCartridgeLoaded.setActive(true);
				turn_on_buzzer_for_duration(600, 350);
			}
			else {
				ledCartridgeLoaded.setActive(false);
				turn_on_buzzer_for_duration(300, 200);
			}

    Serial.printlnf("Cartridge loaded? %c", cartridge_loaded ? 'Y' : 'N');
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
        reading_optical_sensors = false;

        test_progress = 0;
        test_percent_complete = 0;

        barcode_uuid[0] = '\0';
        barcode_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
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

    /*start_blinking_device_LED(0, 500, 0, 255, 0);*/
    brevitest_publish("test-start", test_record.test_uuid, false);
}

void cancel_test() {
    Serial.println("Test cancelled");

    waiting_for_cancel_confirmation = true;
    cancel_timeout = millis() + TIMEOUT_CANCEL;

    update_progress("Test cancelled", -1);
    brevitest_publish("test-cancel", test_record.test_uuid, false);

    sleep_stepper();
    /*stop_blinking_device_LED();
    set_device_LED_color(255, 0, 0);
    turn_on_device_LED();*/
}

void finish_test() {
    Serial.println("Test completed");

    waiting_for_finish_confirmation = true;
    finish_timeout = millis() + TIMEOUT_FINISH;

    update_progress("Test complete", -1);
    brevitest_publish("test-finish", test_record.test_uuid, false);

    sleep_stepper();
    /*stop_blinking_device_LED();
    set_device_LED_color(0, 255, 0);
    turn_on_device_LED();*/
}

void run_test() {
	test_in_progress = true;
    /*start_blinking_device_LED(0, 500, 0, 255, 0);*/
    Serial.println("Running test");

    test_last_progress_update = 0;
    update_progress("Running test", 0);

    pinMode(pinSolenoid, OUTPUT);
    analogWrite(pinSolenoid, 0);

    Particle.disconnect();
    delay(PARTICLE_CLOUD_DELAY);
    while(!Particle.disconnected()) {
        Serial.println("-");
        Particle.process();
        delay(PARTICLE_CLOUD_DELAY);
    }

	control_heater_temperature_timer.stop();

    SINGLE_THREADED_BLOCK() {
        process_BCODE(0);
    }

	control_heater_temperature_timer.start();

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

int particle_run_test(String arg) {
    if (arg.length() == CARTRIDGE_UUID_LENGTH) {
        arg.toCharArray(barcode_uuid, CARTRIDGE_UUID_LENGTH + 1);
        barcode_uuid[CARTRIDGE_UUID_LENGTH] = '\0';
        Serial.printlnf("Running test for cartridge %s", barcode_uuid);
        validate_cartridge();
        return 1;
    }
    else {
        return -1;
    }
}

/////////////////////////////////////////////////////////////
//                                                         //
//                          SETUP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void watchdog() {
    Serial.println("Watchdog!");
}

void init_analog_pin(uint16_t pin, PinMode mode, uint8_t value) {
    pinMode(pin, mode);
    if (mode == OUTPUT) {
        analogWrite(pin, value);
    }
}

void init_digital_pin(uint16_t pin, PinMode mode, uint8_t value) {
    pinMode(pin, mode);
    if (mode == OUTPUT) {
        digitalWrite(pin, value);
    }
}

void controller_i2c_bus_scan() {
    int addr, bytes_sent, result;

    if (enable_controller_sensors(true)) {
        Serial.println("Starting controller I2C bus scan");
        for (addr = 0; addr < 127; addr++) {
            Wire1.beginTransmission(addr);
            bytes_sent = Wire1.write(0x00);
            result = Wire1.endTransmission();
            if (result == 0) {
                Serial.printlnf("I2C device found at address %X", addr);
            }
        }
    }
    disable_controller_sensors();
}

void optical_i2c_bus_scan() {
    int addr, bytes_sent, result;

    if (enable_optical_sensors(true)) {
        Serial.println("Starting optical I2C bus scan");
        for (addr = 0; addr < 127; addr++) {
            Wire.beginTransmission(addr);
            bytes_sent = Wire.write(0x00);
            result = Wire.endTransmission();
            if (result == 0) {
                Serial.printlnf("I2C device found at address %X", addr);
            }
        }
    }
    disable_optical_sensors();
}

void setup() {
        Particle.variable("register", particle_register, STRING);
        Particle.function("command", particle_command);
        Particle.function("run_test", particle_run_test);

        device_id_string = System.deviceID();
        Particle.subscribe(String("hook-response/brevitest-" + device_id_string), brevitest_callback, MY_DEVICES);
        Particle.subscribe(String("hook-error/brevitest-" + device_id_string), brevitest_error, MY_DEVICES);
        device_id_string.toCharArray(device_id, DEVICE_ID_LENGTH + 1);
        device_id[DEVICE_ID_LENGTH] = '\0';

        init_digital_pin(pinLimitSwitch, INPUT_PULLUP, 0);
        init_digital_pin(pinCartridgeLoaded, INPUT_PULLUP, 0);

        init_digital_pin(pinBarcode_Trigger, OUTPUT, HIGH);
        init_digital_pin(pinBarcode_Success, INPUT_PULLDOWN, 0);

        init_analog_pin(pinLEDAssay, OUTPUT, 0);
        init_analog_pin(pinLEDControl, OUTPUT, 0);

        init_analog_pin(pinThermistor, INPUT, 0);

        init_digital_pin(pinAssaySensor_Ready, INPUT, 0);
        init_digital_pin(pinControlSensor_Ready, INPUT, 0);

		init_analog_pin(pinHeater, OUTPUT, 0);

    	init_analog_pin(pinSolenoid, OUTPUT, 0);
        init_analog_pin(pinBuzzer, OUTPUT, 0);

        init_digital_pin(pinStepper_Step, OUTPUT, LOW);
        init_digital_pin(pinStepper_Sleep, OUTPUT, LOW);
        init_digital_pin(pinStepper_Dir, OUTPUT, LOW);

        Serial.begin(115200); // standard serial port

        load_eeprom();
        if (eeprom.firmware_version != FIRMWARE_VERSION || eeprom.data_format_version != DATA_FORMAT_VERSION) {
            reset_eeprom();
        }

		controller_i2c_bus_scan();

        Serial.println("Resetting stage");
        reset_stage();

        Serial.println("Firing solenoid");
        move_solenoid(1000);

        Serial.println("Turning on LEDs");
		turn_on_assay_LED_for_duration(5, LED_POWER);
		delay(100);
		turn_on_control_LED_for_duration(5, LED_POWER);

		Serial.println("Playing startup tune");
        play_startup_tune();

        /*Serial.println("Scanning barcode");
        scan_barcode();
*/
        reset_globals();
        /*read_all_controller_sensors_timer.start();*/

        check_device_state();
        attachInterrupt(pinCartridgeLoaded, cartridge_loaded_interrupt, CHANGE);

        Serial.printlnf("device id: %s", device_id);
        Serial.printlnf("eeprom.firmware_version: %d, eeprom.data_format_version: %d, eeprom.most_recent_test: %d", eeprom.firmware_version, eeprom.data_format_version, eeprom.most_recent_test);

        start_temperature_control();
}

/////////////////////////////////////////////////////////////
//                                                         //
//                           LOOP                          //
//                                                         //
/////////////////////////////////////////////////////////////

void test_loop() {
	if (test_in_progress) {
        if (cancelling_test) {
            cancelling_test = false;
            cancel_test();
        }
        else if (finishing_test) {
            finishing_test = false;
            finish_test();
        }
        else if (waiting_for_cancel_confirmation && millis() > cancel_timeout) {
			Serial.println("Timeout waiting for cancel confirmation. Cancelling test again");
            cancel_test();
        }
        else if (waiting_for_finish_confirmation && millis() > finish_timeout) {
			Serial.println("Timeout waiting for finish confirmation. Finishing test again");
            finish_test();
        }
    }
    else if (waiting_for_start_confirmation && millis() > start_timeout) {
		Serial.println("Timeout waiting for start confirmation. Starting test again");
        start_test();
    }
    else if (test_startup_successful) {
        if (!cartridge_loaded) {
			Serial.println("Cartridge not loaded. Waiting...");
			turn_on_buzzer_for_duration(500, 700);
			delay(2000);
        }
        else {
			test_startup_successful = false;
            run_test();
        }
    }
	else if (starting_test) {
        starting_test = false;
        start_test();
    }
	else if (waiting_for_upload_confirmation) {
        if (millis() > upload_timeout) {
            upload_tests();
        }
    }
	else if (tests_to_upload()) {
        upload_tests();
    }
}

void cartridge_loop() {
	if (cartridge_state_changed) {
		if (cartridge_state_debounce) {
			cartridge_state_debounce = false;
			cartridge_state_changed = false;
	        Serial.println("Cartridge state changed");
	        check_device_state();
		}
		else {
			delay(50);
			cartridge_state_debounce = true;
		}
	}
	if (waiting_for_validation) {
	    if (millis() > validation_timeout) {
			Serial.println("Timeout waiting for cartridge validation. Validating cartridge again");
	        validate_cartridge();
	    }
	}
	else if (ready_to_scan_barcode) {
	    ready_to_scan_barcode = false;
	    if (cartridge_loaded) {
	        if (scan_barcode() == CARTRIDGE_UUID_LENGTH) {
	            validate_cartridge();
	        }
	        else {
	            cartridge_validated = false;
	        }
	    }
	}
}

void loop() {
	while (Serial.available()) {
		char c = Serial.read();
		serial_buffer[serial_buffer_index] = c;
		if (Serial.available()) {
			serial_buffer_index++;
			serial_buffer_index %= SERIAL_BUFFER_SIZE;
		}
		else {
			serial_buffer[serial_buffer_index] = '\0';
			Serial.println(serial_buffer);
			serial_buffer_index = 0;
			particle_command(String(serial_buffer));
		}
	}

  if (callback_complete) {
      Serial.println("Processing callback");
      process_callback_buffer();
      return;
  }

	cartridge_loop();
	test_loop();

	if (read_optical_sensors_command_flag) {
		read_optical_sensors_command_flag = false;
		read_optical_sensors(read_optical_sensors_command_param, false);
	}

	if (read_optical_sensor_baselines_command_flag) {
		read_optical_sensor_baselines_command_flag = false;
		read_optical_sensor_baselines(read_optical_sensor_baselines_command_param);
	}

	if (control_heater_temperature_flag) {
		control_heater_temperature_flag = false;
		pulse_heater(pid_controller());
	}
}
