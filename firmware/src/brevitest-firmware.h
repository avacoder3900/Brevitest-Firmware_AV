#include "application.h"

//
// GLOBAL VARIABLES AND DEFINES

// general constants
#define FIRMWARE_VERSION 3
#define DATA_FORMAT_VERSION 33

#define TEST_DATA_FORMAT_CODE 'F'
#define ASSAY_UUID_LENGTH 8
#define BARCODE_UUID_LENGTH 36
#define CARTRIDGE_UUID_LENGTH 24
#define MAGNETOMETER_UUID_LENGTH 32
#define SHIPPING_BOLT_UUID_LENGTH 13
#define OPTICAL_UUID_LENGTH 17
#define STRESS_TEST_UUID_LENGTH 16
#define DEVICE_UUID_LENGTH 24

#define ARG_DELIM ','
#define ATTR_DELIM ':'
#define ITEM_DELIM '|'
#define END_DELIM '#'
#define MAX_ANALOG_READ 4095
#define SERIAL_COMMAND_BUFFER_SIZE 40

// barcode and callback strings
#define BARCODE_ERROR_MESSAGE "---BARCODE READ ERROR---"
#define BARCODE_ERROR_MESSAGE_LENGTH 24
#define SUCCESS "SUCCESS"
#define INVALID "INVALID"
#define BARCODE_PREFIX_LENGTH 4
#define MAGNETOMETER_PREFIX "MAG-"
#define OPTICAL_PREFIX "OPT-"
#define STRESS_TEST_PREFIX "STRESS-TEST-"
#define STRESS_TEST_PREFIX_LENGTH 12
#define SHIPPING_BOLT_BARCODE "SHIPPING BOLT"

// spectrophotometer
#define SPECTRO_ASTEP_DEFAULT 599
#define SPECTRO_ATIME_DEFAULT 39
#define SPECTRO_AGAIN_DEFAULT 7
#define SPECTRO_MAXIMUM_NUMBER_OF_SAMPLES 6    // maximum number of samples in a scan
#define SPECTRO_WELL_LENGTH 6000                // sixth well length in microns
#define SPECTRO_STARTING_STAGE_POSITION 22000

#define SPECTRO_SWITCH_ADDR 0x41                // I2C address of PCA9536.
#define SPECTRO_SWITCH_CONFIG_COMMAND 0x03      // Configure command.
#define SPECTRO_SWITCH_SET_PORTS 0xF0           // Set all ports to output.
#define SPECTRO_SWITCH_INPUT_COMMAND 0x00      // Input command.
#define SPECTRO_SWITCH_OUTPUT_COMMAND 0x01      // Output command.
#define SPECTRO_SWITCH_FLIP_COMMAND 0x02      // Polarity inversion command.
#define SPECTRO_SWITCH_TURN_OFF_ALL 0x00        // Turn off all spectrophotometers.
#define SPECTRO_SWITCH_TURN_ON_A 0x01           // Turn on spectrophotometer A.
#define SPECTRO_SWITCH_TURN_ON_B 0x02           // Turn on spectrophotometer B.
#define SPECTRO_SWITCH_TURN_ON_C 0x04           // Turn on spectrophotometer C.

// async commands
#define ASYNC_COMMAND_DEFAULT_INTERVAL 5000

// LEDs
#define LED_DEFAULT_POWER 115
#define LED_WARMUP_DELAY_MS 1000
#define LED_DURATION 500

// buzzer
#define BUZZER_FREQUENCY 600
#define BUZZER_DURATION 1000
#define BUZZER_INSERT_FREQUENCY 620
#define BUZZER_INSERT_DURATION 200
#define BUZZER_REMOVE_FREQUENCY 620
#define BUZZER_REMOVE_DURATION 500
#define BUZZER_ALERT_FREQUENCY 850
#define BUZZER_ALERT_DURATION 500
#define BUZZER_ALERT_PERIOD 4000
#define BUZZER_PROBLEM_FREQUENCY 620
#define BUZZER_PROBLEM_DURATION 200
#define BUZZER_PROBLEM_PERIOD 400

// BCODE
#define BCODE_CAPACITY 2000
#define BCODE_MAX_DELAY 500

// particle
#define PARTICLE_REGISTER_SIZE 622
#define PARTICLE_ARG_SIZE 63 
#define PARTICLE_CLOUD_DELAY 4000

// barcode scanner
#define BARCODE_DELAY_AFTER_POWER_ON_MS 1000
#define BARCODE_DELAY_AFTER_TRIGGER_MS 50
#define BARCODE_READ_TIMEOUT 5000
#define BARCODE_TYPE_CARTRIDGE 1
#define BARCODE_TYPE_MAGNETOMETER 2
#define BARCODE_TYPE_OPTICAL 3
#define BARCODE_TYPE_STRESS_TEST 4
#define BARCODE_TYPE_SHIPPING 5
#define BARCODE_TYPE_GENERAL_ERROR -1
#define BARCODE_TYPE_VALIDATION_ERROR -2
#define BARCODE_TYPE_OPTICAL_ERROR -3

// motor
#define MOTOR_MICRONS_PER_EIGHTH_STEP 25
#define MOTOR_MOVE_DURATION_UNIT 25000
#define MOTOR_MINIMUM_STEP_DELAY 250
#define MOTOR_RESET_STEP_DELAY 290
#define MOTOR_BOUNCE_STEP_DELAY 350
#define MOTOR_FAST_STEP_DELAY 290
#define MOTOR_SLOW_STEP_DELAY 600
#define MOTOR_OSCILLATION_STEP_DELAY 350

// stage
#define STAGE_RESET_STEPS -60000
#define STAGE_POSITION_LIMIT 45000
#define STAGE_MICRONS_TO_INITIAL_POSITION -300
#define STAGE_MICRONS_TO_TEST_START_POSITION 9800
#define STAGE_SHIPPING_BOLT_LOCATION 28000

// heater
#define HEATER_MAX_POWER 128
#define HEATER_DEFAULT_POWER 64
#define HEATER_PWM_FREQUENCY 10
#define HEATER_MAX_TEMPERATURE 600
#define HEATER_CONTROL_INTERVAL 1000
#define HEATER_PULSE_DURATION 800
#define HEATER_DEFAULT_TEMP_TARGET 350
#define HEATER_READY_TEMP_DELTA 10
#define HEATER_READY_DEBOUNCE_DELAY 5000

// laser
#define LASER_MAX_POWER 255
#define LASER_DEFAULT_POWER 128
#define LASER_PWM_FREQUENCY 500

// thermistors
#define THERMISTOR_SCALE 10000
#define TERMISTOR_TABLE_LENGTH 21

// PID controller
#define PHOTO_SCALE 10000
#define PID_MAX_POWER 255
#define PID_DEFAULT_POWER 128
#define PID_PWM_FREQUENCY 50
#define PID_MAX_VALUE 600
#define PID_CONTROL_INTERVAL 1000
#define PID_DEFAULT_PULSE_DURATION 800
#define PID_DEFAULT_TARGET_VALUE 450
#define PID_STABLE_VALUE_DELTA 10

// pubsub
#define PUBSUB_EVENT_MAX_LENGTH 32
#define PUBSUB_RETRY_DELAY 22000

// magnetometer
#define MAGNETOMETER_HEATING_DELAY 3000
#define MAGNETOMETER_INITIAL_HEATING_DELAY 60000

// stress test
#define STRESS_TEST_MAXIMUM_RECORDS 15

// test status codes
#define TEST_STATUS_UNDERWAY 0x00
#define TEST_STATUS_SUCCESS 0x01
#define TEST_STATUS_CANCELLED 0x02
#define TEST_STATUS_INVALID_CARTRIDGE 0x03
#define TEST_STATUS_VALIDATION_CANCELLED 0x04
#define TEST_STATUS_FAILED_TO_START 0x05
#define TEST_STATUS_START_CANCELLED 0x06
#define TEST_STATUS_ERROR 0xFF

// pin definitions
int pinBuzzer = A0;
int pinHeaterThermistor = A1;
int pinPhotoA = A2;
int pinPhotoB = A3;
int pinPhotoC = A4;
int pinStageLimit = D2;
int pinCartridgeDetected = D3;
int pinHeater = D4;
int pinLaserA = D5;
int pinLaserB = D6;
int pinLaserC = D7;
int pinMotorDir = D8;
int pinMotorReset = D11;
int pinMotorSleep = D12;
int pinMotorStep = D13;
int pinBarcodeTrigger = D22;
int pinBarcodeReady = D23;

// global variables
bool new_device = true;
int stage_position = 0;
int microns_error = 0;
int serial_buffer_index = 0;
char serial_buffer[SERIAL_COMMAND_BUFFER_SIZE];
bool serial_messaging_on = false;
bool spectrophotometer_read_in_progress = false;
bool motor_awake = false;

// device LED
LEDStatus indicatorProblem(RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_CRITICAL);
LEDStatus indicatorBusy(RGB_COLOR_BLUE, LED_PATTERN_SOLID, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
LEDStatus indicatorAvailable(RGB_COLOR_GREEN, LED_PATTERN_FADE, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);

// logging
SerialLogHandler logHandler;

//
//    DEVICE STATE
//

// detector
volatile bool detector_changed = false;
bool detector_debouncing = false;
bool detector_on = false;

// verify device
bool device_verified = false;
bool device_verification_in_progress = false;

// scan barcode
bool barcode_scan_mode = false;
bool barcode_scan_in_progress = false;
bool barcode_read_cartridge = false;
bool barcode_read_validate_device = false;
bool barcode_read_invalid = false;
bool barcode_read_error = false;
bool barcode_invalid = false;

// validate cartridge
bool cartridge_inserted = false;
bool cartridge_validation_mode = false;
bool cartridge_validation_in_progress = false;
bool cartridge_validated = false;

// start test
bool test_start_mode = false;
bool test_start_in_progress = false;
bool test_underway = false;
bool test_completed = false;
bool test_cancelled = false;
bool test_invalid = false;

// upload test
bool test_upload_mode = false;
bool test_upload_in_progress = false;

// magnet validation
bool magnetometer_inserted = false;
int magnetometer_initial_heating_delay = MAGNETOMETER_INITIAL_HEATING_DELAY;
int magnetometer_heating_delay = MAGNETOMETER_HEATING_DELAY;
bool magnet_validation_mode = false;
bool magnet_validation_in_progress = false;

// stress test
bool stress_test_cartridge_inserted = false;
bool stress_test_mode = false;
bool stress_test_stop_flag = false;
int stress_test_step = 0;
int stress_test_limit = 0;
int stress_test_LED_power = 0;

// shipping bolt
bool shipping_bolt_cartridge_inserted = false;

// pubsub callback timeouts and retries
unsigned long callback_timeout = 0;
bool publish_in_progress = false;

// temperature control system
struct HeatingElement
{
    int heater_pin;
    int thermistor_pin;
    bool heater_on;
    int power;
    int pulse_duration;
    int previous_error;
    int integral;
    unsigned long read_time;
    int temp_C_10X;
    int target_C_10X;
    int temp_F_10X;
    int k_p_num;
    int k_p_den;
    int k_i_num;
    int k_i_den;
    int k_d_num;
    int k_d_den;
    HeatingElement()
    {
        heater_pin = pinHeater;
        thermistor_pin = pinHeaterThermistor;
        power = 0;
        heater_on = false;
        pulse_duration = HEATER_PULSE_DURATION;
        previous_error = 0;
        integral = 0;
        target_C_10X = HEATER_DEFAULT_TEMP_TARGET;
        k_p_num = 100;
        k_p_den = 5;
        k_i_num = 1;
        k_i_den = 5000;
        k_d_num = 1;
        k_d_den = 5;
    }
} heater;

void control_heater_temperature(void);
Timer control_heater_temperature_timer(HEATER_CONTROL_INTERVAL, control_heater_temperature);
bool control_heater_temperature_flag = false;
bool heater_debouncing_in_progress = false;
unsigned long heater_debounce_time;
bool heater_ready = false;
bool previous_heater_ready = false;

// lasers
struct Laser
{
    int power_pin;
    int value_pin;
    bool power_on;
    int power;
    Laser()
    {
        power = 0;
        power_on = false;
    }
} laserA, laserB, laserC;

// buzzer
void check_buzzer(void);
Timer buzzer_timer(BUZZER_ALERT_PERIOD, check_buzzer);
bool start_alert_buzzer = false;
bool buzzer_alert_running = false;
bool start_problem_buzzer = false;
bool buzzer_problem_running = false;

// spectrophotometer
int spectro_astep = SPECTRO_ASTEP_DEFAULT;
int spectro_atime = SPECTRO_ATIME_DEFAULT;
int spectro_again = SPECTRO_AGAIN_DEFAULT;
int spectro_read_index = 0;

// progress
int test_progress;
int test_percent_complete;

// uuids
char barcode_uuid[BARCODE_UUID_LENGTH + 1];
char assay_uuid[ASSAY_UUID_LENGTH + 1];
String device_id;

// publish and subscribe callback
char current_event[PUBSUB_EVENT_MAX_LENGTH + 1];

// particle messaging
char particle_register[PARTICLE_REGISTER_SIZE + 1];

// spectrophotometer data structure

char channels[3] = { 'A', 'B', 'C' };

struct BrevitestSpectrophotometerReading
{ // 24 bytes
    uint16_t f1;
    uint16_t f2;
    uint16_t f3;
    uint16_t f4;
    uint16_t f5;
    uint16_t f6;
    uint16_t f7;
    uint16_t f8;
    uint16_t clear;
    uint16_t nir;
    uint16_t temperature;
    uint16_t laser_strength;
};

struct BrevitestSpectrophotometerData
{ // 80 bytes
    uint16_t position;
    uint16_t reserved;
    unsigned long msec;
    BrevitestSpectrophotometerReading channel_a; // 24 bytes
    BrevitestSpectrophotometerReading channel_b; // 24 bytes
    BrevitestSpectrophotometerReading channel_c; // 24 bytes
} stress_spectro_data;

struct BrevitestScanRecord
{ // 966 bytes for 6 samples max
    uint8_t number_of_samples;
    BrevitestSpectrophotometerData sample[SPECTRO_MAXIMUM_NUMBER_OF_SAMPLES];
} scan;

struct BrevitestTestRecord
{ // 992 bytes for 6 samples max
    char cartridge_uuid[CARTRIDGE_UUID_LENGTH + 1]; // 25 bytes
    uint8_t number_of_samples; // 4 MSB = number of baselines, 4 LSB = number of tests
    uint16_t test_status_code = TEST_STATUS_UNDERWAY;
    uint16_t duration;
    uint16_t reserved;
    BrevitestSpectrophotometerData baseline[SPECTRO_MAXIMUM_NUMBER_OF_SAMPLES];
    BrevitestSpectrophotometerData test[SPECTRO_MAXIMUM_NUMBER_OF_SAMPLES];
} test;

struct BrevitestAssay
{
    char uuid[ASSAY_UUID_LENGTH + 1];
    uint8_t BCODE_version;
    int duration;
    uint16_t BCODE_length;
    char BCODE[BCODE_CAPACITY];
} assay;

struct Particle_EEPROM
{
    uint8_t firmware_version = FIRMWARE_VERSION; // 8 bytes
    uint8_t data_format_version = DATA_FORMAT_VERSION;
    int lifetime_stress_test_cycles = 0;
    int stress_test_cycles_since_reset = 0;
    int stress_test_cycles = 0;
    int reserved[4];
    char running_test_uuid[CARTRIDGE_UUID_LENGTH + 1];
    BrevitestTestRecord cache;
    int stress_test_reading_count = 0;
} eeprom;

#define BLE_TYPE BleCharacteristicProperty::READ
BleAdvertisingData advertData, scanResponse;

BleUuid magnetometerService("4d2b2311-bb00-43e3-a284-5c73b737c369");

BleUuid well1uuid("b1c14499-8e1d-41b2-b1bc-c89faa88d62a");
BleUuid well2uuid("2216cfb5-38a7-46a3-9509-ad7f287a569a");
BleUuid well3uuid("2230e907-583b-4328-84a4-8f9023a681c1");
BleUuid well4uuid("8c58309c-7c2d-4805-b71f-8137eb4a01f8");
BleUuid well5uuid("b9dc1dd4-a0da-4328-8003-6c72a526a12b");
BleUuid bleCharUuid[5] = { well1uuid, well2uuid, well3uuid, well4uuid, well5uuid };

BleCharacteristic wellChar1("well_1", BLE_TYPE, well1uuid, magnetometerService);
BleCharacteristic wellChar2("well_2", BLE_TYPE, well2uuid, magnetometerService);
BleCharacteristic wellChar3("well_3", BLE_TYPE, well3uuid, magnetometerService);
BleCharacteristic wellChar4("well_4", BLE_TYPE, well4uuid, magnetometerService);
BleCharacteristic wellChar5("well_5", BLE_TYPE, well5uuid, magnetometerService);
BleCharacteristic bleWell[5] = { wellChar1, wellChar2, wellChar3, wellChar4, wellChar5 };

int well_move[5] = { -8000, 8000, 8000, 8000, 8000 };

BleAddress magnetometer_address;
BlePeerDevice magnetometer;
bool magnetometer_found = false;

