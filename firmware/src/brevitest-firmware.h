#include "application.h"

//
// GLOBAL VARIABLES AND DEFINES

// general constants
// PRODUCT_NUMBER to 14974 for production, 11170 for development
#define PRODUCT_NUMBER 14974
// FIRMWARE_VERSION to 13 for production, 40 for development
#define FIRMWARE_VERSION 13
#define DATA_FORMAT_VERSION 21

#define TEST_DATA_FORMAT_CODE 'D'
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

// optical sensors
#define OPTICAL_SENSOR_NUMBER_OF_SAMPLES 4
#define OPTICAL_SENSOR_DEFAULT_PARAM 0x45
#define OPTICAL_MAXIMUM_NUMBER_OF_READINGS 21
#define OPTICAL_TEST_DEFAULT_READINGS 5
#define OPTICAL_TEST_DEFAULT_DISTANCE 2000
#define OPTICAL_TARGET_L_VALUE 20000
#define OPTICAL_SEARCH_THRESHOLD 50
#define OPTICAL_ERROR_THRESHOLD 75
#define OPTICAL_FAILURE_THRESHOLD 200

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
#define STAGE_OPTICAL_SENSOR_READ_POSITION 20900
#define STAGE_MICRONS_TO_INITIAL_POSITION 250
#define STAGE_MICRONS_TO_TEST_START_POSITION 12800
#define STAGE_SHIPPING_BOLT_LOCATION 28000

// heater
#define HEATER_MAX_POWER 255
#define HEATER_DEFAULT_POWER 128
#define HEATER_PWM_FREQUENCY 50
#define HEATER_MAX_TEMPERATURE 600
#define HEATER_CONTROL_INTERVAL 1000
#define HEATER_PULSE_DURATION 800
#define HEATER_DEFAULT_TEMP_TARGET 450
#define HEATER_READY_TEMP_DELTA 10
#define HEATER_READY_DEBOUNCE_DELAY 5000

// thermistors
#define THERMISTOR_SCALE 10000
#define TERMISTOR_TABLE_LENGTH 21

// pubsub
#define PUBSUB_EVENT_NAME "brevitest-production"
#define PUBSUB_EVENT_MAX_LENGTH 32
#define PUBSUB_STATUS_MAX_LENGTH 16
#define PUBSUB_CALLBACK_BUFFER_SIZE 5000
#define PUBSUB_VERIFY_DEVICE 10
#define PUBSUB_VALIDATE_CARTRIDGE 20
#define PUBSUB_START_TEST 30
#define PUBSUB_UPLOAD_TEST 40
#define PUBSUB_VALIDATE_MAGNETS 50
#define PUBSUB_VALIDATE_OPTICS 60

// magnetometer
#define MAGNETOMETER_HEATING_DELAY 3000
#define MAGNETOMETER_INITIAL_HEATING_DELAY 60000

// stress test
#define STRESS_TEST_MAXIMUM_RECORDS 15

// pin definitions
int pinLEDControl2 = A0;
int pinLEDControl1 = A1;
int pinLEDAssay = A2;
int pinHeaterThermistor = A3;
int pinStageLimit = SCK;
int pinMotorDir = MOSI;
int pinCartridgeDetected = MISO;
int pinRX = RX;
int pinTX = TX;
// int pinSDA = SDA;
// int pinSCL = SCL;
int pinBarcodeTrigger = D2;
int pinBuzzer = D3;
int pinBarcodeReady = D4;
int pinMotorStep = D5;
int pinMotorSleep = D6;
int pinHeater = D7;
int pinMotorReset = D8;

// global variables
bool new_device = true;
int stage_position = 0;
int microns_error = 0;
int serial_buffer_index = 0;
char serial_buffer[SERIAL_COMMAND_BUFFER_SIZE];
bool serial_messaging_on = false;
bool optical_read_in_progress = false;
bool motor_awake = false;

// device LED
LEDStatus indicatorProblem(RGB_COLOR_RED, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_CRITICAL);
LEDStatus indicatorBusy(RGB_COLOR_RED, LED_PATTERN_SOLID, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
LEDStatus indicatorAvailable(RGB_COLOR_GREEN, LED_PATTERN_FADE, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
LEDStatus indicatorAsync(RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
LEDStatus indicatorValidation(RGB_COLOR_YELLOW, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);

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

// optical validation
bool optical_probe_inserted = false;
bool optical_validation_mode = false;
bool optical_validation_in_progress = false;

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
int publish_retry_attempt = 0;
int publish_retry_max_index = 11;
unsigned long publish_retry_intervals[12] = {2000, 3000, 5000, 8000, 13000, 21000, 34000, 55000, 89000, 144000, 233000, 377000};
unsigned long next_optical_sensor_reading_time = 0;

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
        k_p_den = 1;
        k_i_num = 1;
        k_i_den = 1000;
        k_d_num = 1;
        k_d_den = 1;
    }
} heater;

void control_heater_temperature(void);
Timer control_heater_temperature_timer(HEATER_CONTROL_INTERVAL, control_heater_temperature);
bool control_heater_temperature_flag = false;
bool heater_debouncing_in_progress = false;
unsigned long heater_debounce_time;
bool heater_ready = false;
bool previous_heater_ready = false;

// async commands
void async_command(void);
Timer async_command_timer(ASYNC_COMMAND_DEFAULT_INTERVAL, async_command);
bool async_command_running = false;

bool async_command_magnet_running = false;
bool magnet_test_take_reading = false;
bool magnet_test_awaiting_confirmation = false;
int magnet_test_count = 0;
int magnet_test_readings = 0;
int magnet_test_move = 0;
unsigned long magnet_test_confirmation_timeout;

bool async_command_optical_running = false;
bool optical_test_take_reading = false;
int optical_test_count = 0;
int optical_test_readings = 0;
int optical_test_move = 0;

// buzzer
void check_buzzer(void);
Timer buzzer_timer(BUZZER_ALERT_PERIOD, check_buzzer);
bool start_alert_buzzer = false;
bool buzzer_alert_running = false;
bool start_problem_buzzer = false;
bool buzzer_problem_running = false;

// progress
int test_progress;
int test_percent_complete;

// uuids
char barcode_uuid[BARCODE_UUID_LENGTH + 1];
char assay_uuid[ASSAY_UUID_LENGTH + 1];
String device_id;

// publish and subscribe callback
char callback_buffer[PUBSUB_CALLBACK_BUFFER_SIZE + 1];
bool callback_complete;
char callback_event[PUBSUB_EVENT_MAX_LENGTH + 1];
char callback_status[PUBSUB_STATUS_MAX_LENGTH + 1];
char *callback_data;
char current_event[PUBSUB_EVENT_MAX_LENGTH + 1];
int current_event_code = 0;

// particle messaging
char particle_register[PARTICLE_REGISTER_SIZE + 1];

struct BrevitestOpticalBaselineChannel {
    uint8_t led_power;
    uint16_t l_value;
    int error;
};

struct BrevitestOpticalBaseline {
    uint8_t led_assay;
    uint8_t led_c1;
    uint8_t led_c2;
    BrevitestOpticalBaseline() {
        led_assay = LED_DEFAULT_POWER;
        led_c1 = LED_DEFAULT_POWER;
        led_c2 = LED_DEFAULT_POWER;
    }
} baseline;

struct BrevitestOpticalSensorRecord
{ // 12 bytes
    char channel;
    uint8_t samples;
    unsigned long msec;
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint16_t temperature;
};

struct BrevitestTestRecord
{ // 206 bytes
    char cartridge_uuid[CARTRIDGE_UUID_LENGTH + 1]; // 25 bytes
    uint8_t number_of_readings; // 0 = cancelled
    uint16_t duration;
    BrevitestOpticalSensorRecord reading[OPTICAL_MAXIMUM_NUMBER_OF_READINGS]; // 168 bytes
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
    uint8_t firmware_version; // 8 bytes
    uint8_t data_format_version;
    int lifetime_stress_test_cycles;
    int stress_test_cycles_since_reset;
    int stress_test_cycles;
    int reserved[4];
    char running_test_uuid[CARTRIDGE_UUID_LENGTH + 1];
    BrevitestTestRecord cache;
    int stress_test_reading_count;
    BrevitestOpticalSensorRecord stress_test_reading[STRESS_TEST_MAXIMUM_RECORDS];

    Particle_EEPROM()
    {
        firmware_version = FIRMWARE_VERSION;
        data_format_version = DATA_FORMAT_VERSION;
        lifetime_stress_test_cycles = 0;
        stress_test_cycles_since_reset = 0;
        stress_test_cycles = 0;
        memset(running_test_uuid, 0, CARTRIDGE_UUID_LENGTH + 1);
        memset(&cache, 0, sizeof(BrevitestTestRecord));
        stress_test_reading_count = 0;
        memset(&stress_test_reading, 0, STRESS_TEST_MAXIMUM_RECORDS * sizeof(BrevitestOpticalSensorRecord));
    }
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