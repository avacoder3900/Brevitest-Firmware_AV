#include "application.h"

//
// GLOBAL VARIABLES AND DEFINES

// general constants
#define FIRMWARE_VERSION 5
#define DATA_FORMAT_VERSION 17
#define TEST_DATA_FORMAT_CODE 'B'
#define ASSAY_UUID_LENGTH 8
#define BARCODE_UUID_LENGTH 36
#define CARTRIDGE_UUID_LENGTH 24
#define VALIDATION_UUID_LENGTH 32
#define DEVICE_UUID_LENGTH 24

#define ARG_DELIM ','
#define ATTR_DELIM ':'
#define ITEM_DELIM '|'
#define END_DELIM '#'
#define MAX_ANALOG_READ 4095
#define SERIAL_COMMAND_BUFFER_SIZE 40

// barcode and callback strings
#define BARCODE_ERROR_MESSAGE "---BARCODE READ ERROR---"
#define SUCCESS "SUCCESS"
#define INVALID "INVALID"
#define VALIDATION_PREFIX_LENGTH 8
#define MAGNETOMETER_PREFIX "MAG-001-"
#define TEMPERATURE_PREFIX "TMP-001-"
#define OPTICAL_PREFIX "OPT-001-"

// optical sensors
#define OPTICAL_SENSOR_NUMBER_OF_SAMPLES 5
#define OPTICAL_SENSOR_DEFAULT_PARAM 0xB6
#define OPTICAL_SENSORS_TEST_INTERVAL 5000
#define OPTICAL_MAXIMUM_NUMBER_OF_READINGS 21

// LEDs
#define LED_DEFAULT_POWER 255
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
#define BARCODE_TYPE_TEMPERATURE 3
#define BARCODE_TYPE_OPTICAL 4
#define BARCODE_TYPE_ERROR -1

// motor
#define MOTOR_MICRONS_PER_EIGHTH_STEP 25
#define MOTOR_MOVE_DURATION_UNIT 25000
#define MOTOR_MINIMUM_STEP_DELAY 150
#define MOTOR_FAST_STEP_DELAY 200
#define MOTOR_SLOW_STEP_DELAY 1000
#define MOTOR_OSCILLATION_STEP_DELAY 250

// stage
#define STAGE_RESET_STEPS -60000
#define STAGE_POSITION_LIMIT 39500
#define STAGE_OPTICAL_SENSOR_READ_POSITION 20825
#define STAGE_MICRONS_TO_INITIAL_POSITION 12300
#define STAGE_MICRONS_TO_TEST_START_POSITION STAGE_MICRONS_TO_INITIAL_POSITION

// heater
#define HEATER_MAX_POWER 255
#define HEATER_DEFAULT_POWER 128
#define HEATER_PWM_FREQUENCY 15000
#define HEATER_MAX_TEMPERATURE 600
#define HEATER_CONTROL_INTERVAL 1000
#define HEATER_PULSE_DURATION 800
#define HEATER_DEFAULT_TEMP_TARGET 500
#define HEATER_READY_TEMP_DELTA 10
#define HEATER_READY_DEBOUNCE_DELAY 5000

// thermistors
#define THERMISTOR_SCALE 10000
#define TERMISTOR_TABLE_LENGTH 21

// magnetometer test
#define MAGNETOMETER_TEST_DEFAULT_STEP_DISTANCE 200
#define MAGNETOMETER_TEST_CONFIRM_TIMEOUT 20000

// pubsub
#define PUBSUB_EVENT_NAME "brevitest-production"
#define PUBSUB_EVENT_MAX_LENGTH 32
#define PUBSUB_STATUS_MAX_LENGTH 16
#define PUBSUB_CALLBACK_BUFFER_SIZE 5000
#define PUBSUB_CALLBACK_TIMEOUT 5000
#define PUBSUB_REGISTER_DEVICE 10
#define PUBSUB_VALIDATE_CARTRIDGE 20
#define PUBSUB_START_TEST 30
#define PUBSUB_UPLOAD_TEST 40

// retry intervals - added to PUBSUB_CALLBACK_TIMEOUT
#define RETRY_DEVICE_REGISTRATION 15000
#define RETRY_CARTRIDGE_VALIDATION 5000
#define RETRY_TEST_START 5000
#define RETRY_TEST_UPLOAD 30000

// application watchdog
// void watchdog(void);
// ApplicationWatchdog wd(60000, watchdog);

// pin definitions

// PIN MAPPINGS

int pinLEDControl2 = A0;
int pinLEDControl1 = A1;
int pinLEDAssay = A2;
int pinHeaterThermistor = A3;
int pinIRThermistor = A4;
int pinIRThermopile = A5;
int pinStageLimit = SCK;
int pinMotorSleep = MOSI;
int pinCartridgeDetected = MISO;
int pinRX = RX;
int pinTX = TX;

// int pinSDA = SDA;
// int pinSCL = SCL;
int pinBarcodeTrigger = D2;
int pinMotorPFD = D3;
int pinBarcodeReady = D4;
int pinMotorDir = D5;
int pinMotorStep = D6;
int pinBuzzer = D7;
int pinHeater = D8;

// global variables
bool new_device = true;
int stage_position = 0;
int microns_error = 0;
int serial_buffer_index = 0;
char serial_buffer[SERIAL_COMMAND_BUFFER_SIZE];
bool serial_messaging_on = false;
int stress_test_count = 0;
int stress_test_step = 0;

// device LED
LEDStatus indicatorProblem(RGB_COLOR_RED, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_CRITICAL);
LEDStatus indicatorBusy(RGB_COLOR_RED, LED_PATTERN_SOLID, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
LEDStatus indicatorAvailable(RGB_COLOR_GREEN, LED_PATTERN_FADE, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
LEDStatus indicatorStressTest(RGB_COLOR_BLUE, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);
LEDStatus indicatorValidation(RGB_COLOR_YELLOW, LED_PATTERN_BLINK, LED_SPEED_NORMAL, LED_PRIORITY_IMPORTANT);

// logging
SerialLogHandler logHandler;

// device state
bool device_starting_up = false;
bool device_registration_in_progress = false;
bool device_registered = false;
bool detector_changed = false;
bool detector_debouncing = false;
bool detector_on = false;
bool barcode_start_scan = false;
bool barcode_scanning = false;
bool barcode_read_cartridge = false;
bool barcode_read_validate_device = false;
bool barcode_read_invalid = false;
bool barcode_read_error = false;
bool cartridge_inserted = false;
bool cartridge_validation_mode = false;
bool cartridge_validation_in_progress = false;
bool cartridge_invalid = false;
bool cartridge_validated = false;
bool test_start_mode = false;
bool test_start_in_progress = false;
bool test_underway = false;
bool test_completed = false;
bool test_cancelled = false;
bool test_invalid = false;
bool test_upload_mode = false;
bool test_upload_in_progress = false;
bool test_upload_finished = false;
bool stress_test_running = false;
bool optical_read_in_progress = false;
bool magnetometer_inserted = false;
bool magnetometer_validation_mode = false;
bool magnetometer_validation_in_progress = false;
bool magnetometer_validation_finished = false;
bool temperature_probe_inserted = false;
bool temperature_validation_mode = false;
bool temperature_validation_in_progress = false;
bool temperature_validation_finished = false;
bool optical_probe_inserted = false;
bool optical_validation_mode = false;
bool optical_validation_in_progress = false;
bool optical_validation_completed = false;
bool buzzer_problem_running = false;
bool buzzer_alert_running = false;
bool long_duration_process_running = false;

// pubsub callback timeouts and retries
unsigned long callback_timeout = 0;
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
bool heater_debounced = true;
unsigned long heater_debounce_time;

// optical sensors
void test_optical_sensors(void);
Timer test_optical_sensors_timer(OPTICAL_SENSORS_TEST_INTERVAL, test_optical_sensors);

unsigned long last_optical_sensor_reading_time = 0;
bool read_optical_sensors_command_flag = false;
int read_optical_sensors_command_param;
int read_optical_sensors_command_led_power;
int read_optical_sensors_command_count;
int read_optical_sensor_command_microns_to_move;

// buzzer
void alert_buzzer(void);
Timer alert_buzzer_timer(BUZZER_ALERT_PERIOD, alert_buzzer);
bool start_alert_buzzer = false;

void problem_buzzer(void);
Timer problem_buzzer_timer(BUZZER_PROBLEM_PERIOD, problem_buzzer);
bool start_problem_buzzer = false;

// magnetometer
unsigned long test_magnetometer_command_confirmation_timeout = 0;
bool test_magnetometer_command_flag = false;
int test_magnetometer_command_microns_to_move;
bool test_magnetometer_awaiting_confirmation = false;


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

struct BrevitestOpticalSensorRecord
{ // 14 bytes
    char channel;
    uint8_t samples;
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint16_t temperature;
};

struct BrevitestTestRecord
{ // 206 bytes
    char cartridge_uuid[CARTRIDGE_UUID_LENGTH + 1]; // 25 bytes
    uint8_t number_of_readings; // 0 = cancelled
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
    int stress_test_cycles;
    int maximum_stress_test_cycles;
    char running_test_uuid[CARTRIDGE_UUID_LENGTH + 1];
    BrevitestTestRecord cache;
    Particle_EEPROM()
    {
        firmware_version = FIRMWARE_VERSION;
        data_format_version = DATA_FORMAT_VERSION;
        stress_test_cycles = 0;
        maximum_stress_test_cycles = 0;
        memset(running_test_uuid, 0, CARTRIDGE_UUID_LENGTH);
    }
} eeprom;
