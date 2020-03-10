#include "application.h"

//
// GLOBAL VARIABLES AND DEFINES

// general constants
#define FIRMWARE_VERSION 6
#define DATA_FORMAT_VERSION 13
#define ASSAY_UUID_LENGTH 8
#define TEST_UUID_LENGTH 24
#define DEVICE_ID_LENGTH 24
#define CARTRIDGE_UUID_LENGTH 24
#define TAB_DELIM String("\t")
#define RETURN_DELIM String("\n")
#define COMMA_DELIM String(",")
#define BCODE_END String("99")
#define MAX_ANALOG_READ 4095
#define SERIAL_COMMAND_BUFFER_SIZE 40

// device open and cartridge validation
#define DEVICE_OPEN_UUID "FFFFFFFFFFFFFFFFFFFFFFFF"
#define NO_CARTRIDGE_UUID "DDDDDDDDDDDDDDDDDDDDDDDD"
#define CARTRIDGE_ERROR_UUID "EEEEEEEEEEEEEEEEEEEEEEEE"
#define SUCCESS "SUCCESS"

// serial number
#define SERIAL_NUMBER_LENGTH 19

// optical sensors
#define OPTICAL_SENSOR_NUMBER_OF_SAMPLES 5
#define OPTICAL_SENSOR_DEFAULT_PARAM 0xB6
#define OPTICAL_SENSORS_TEST_INTERVAL 5000
#define OPTICAL_BASELINE_THRESHOLD 100
#define OPTICAL_BASELINE_MAX_READINGS 50

// LEDs
#define LED_DEFAULT_POWER 255
#define LED_WARMUP_DELAY_MS 1000
#define LED_DURATION 500

// buzzer
#define BUZZER_FREQUENCY 600
#define BUZZER_DURATION 1000

// assay
#define ASSAY_BCODE_CAPACITY 2000

// params
#define PARAM_NUMBER_INDEX 2
#define PARAM_VALUE_INDEX 6
#define PARAM_NUMBER_OF_PARAMS 7

// caches
#define TEST_CACHE_SIZE 4
#define TEST_MAXIMUM_NUMBER_OF_READINGS 15

// particle
#define PARTICLE_REGISTER_SIZE 622
#define PARTICLE_ARG_SIZE 63
#define PARTICLE_PUBLISH_INTERVAL 1000
#define MOVE_STEPS_BETWEEN_PARTICLE_PROCESS 200
#define PARTICLE_CLOUD_DELAY 2000

// status
#define STATUS_LENGTH 622
#define STATUS(...) snprintf(particle_status, STATUS_LENGTH, __VA_ARGS__)
#define TEST_DURATION_LENGTH 6

// barcode scanner
#define BARCODE_DELAY_AFTER_POWER_ON_MS 1000
#define BARCODE_DELAY_AFTER_TRIGGER_MS 50
#define BARCODE_READ_TIMEOUT 5000
#define VALIDATE_CARTRIDGE_TIMEOUT 20000

// motor
#define MICRONS_PER_FULL_STEP 200
#define MICRONS_PER_EIGHTH_STEP 25
#define STAGE_POSITION_LIMIT 41000
#define FAST_STEP_DELAY 150
#define SLOW_STEP_DELAY 1000
#define OPTICAL_SENSOR_READ_POSITION 18000
#define MICRONS_TO_INITIAL_POSITION 10800
#define MICRONS_TO_TEST_START_POSITION 10800
#define OSCILLATION_STEP_DELAY 250

// heater
#define HEATER_MAX_POWER 255
#define HEATER_DEFAULT_POWER 128
#define HEATER_PWM_FREQUENCY 20000
#define HEATER_MAX_TEMPERATURE 600
#define HEATER_CONTROL_INTERVAL 1000
#define HEATER_PULSE_DURATION 800
#define HEATER_DEFAULT_TEMP_TARGET 370

// thermistors
#define THERMISTOR_SCALE 10000
#define TERMISTOR_TABLE_LENGTH 21

// upload
#define UPLOAD_INTERVAL 60000

// timeouts
#define TIMEOUT_VALIDATION 10000
#define TIMEOUT_START 20000
#define TIMEOUT_CANCEL 10000
#define TIMEOUT_FINISH 10000
#define TIMEOUT_UPLOAD 20000

// application watchdog
void watchdog(void);
ApplicationWatchdog wd(60000, watchdog);

// pin definitions

// BORON PIN MAPPINGS

int pinLEDControl2 = A0;
int pinLEDControl1 = A1;
int pinLEDAssay = A2;
int pinHeaterThermistor = A3;
int pinIRThermistor = A4;
int pinIRThermopile = A5;
int pinStageLimit = SCK;
int pinMotorSleep = MOSI;
int pinCartridgeLoaded = MISO;
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
int stage_position = 0;
int microns_error = 0;
unsigned long periodic_event;
bool periodic_event_flag = false;
unsigned long next_upload;
unsigned long validation_timeout;
unsigned long start_timeout;
unsigned long cancel_timeout;
unsigned long finish_timeout;
unsigned long upload_timeout;
int serial_buffer_index = 0;
char serial_buffer[SERIAL_COMMAND_BUFFER_SIZE];
bool serial_messaging_on = false;

// device LED
LEDStatus ledProblem(RGB_COLOR_RED, LED_PATTERN_BLINK, LED_SPEED_FAST);
LEDStatus ledCartridgeLoaded(RGB_COLOR_GREEN, LED_PATTERN_BLINK, LED_SPEED_SLOW);

// device state
volatile bool cartridge_state_changed = false;
bool cartridge_state_debounce = false;
bool cartridge_loaded = false;
bool ready_to_scan_barcode = false;
bool barcode_being_scanned = false;

bool test_startup_successful = false;
bool test_in_progress = false;
bool reading_optical_sensors = false;

bool cartridge_validated = false;
bool waiting_for_validation = false;
bool starting_test = false;
bool waiting_for_start_confirmation = false;
bool cancelling_test = false;
bool waiting_for_cancel_confirmation = false;
bool finishing_test = false;
bool waiting_for_finish_confirmation = false;
bool uploading_test = false;
bool waiting_for_upload_confirmation = false;

unsigned long next_optical_sensor_reading_time = 0;

// optical sensors read timer
void test_optical_sensors(void);
Timer test_optical_sensors_timer(OPTICAL_SENSORS_TEST_INTERVAL, test_optical_sensors);

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
unsigned long control_heater_temperature_flag = false;

// optical sensors
unsigned long last_optical_sensor_reading_time = 0;
bool read_optical_sensors_command_flag = false;
int read_optical_sensors_command_param;
int read_optical_sensors_command_led_power;
bool read_optical_sensor_baselines_command_flag = false;
int read_optical_sensor_baselines_command_param;
int read_optical_sensor_baselines_command_led_power;

// progress
int test_progress;
int test_percent_complete;
unsigned long test_last_progress_update;

// uuids
char cartridge_uuid[CARTRIDGE_UUID_LENGTH + 1];
char barcode_uuid[CARTRIDGE_UUID_LENGTH + 1];
char device_id[DEVICE_ID_LENGTH + 1];
String device_id_string;

// publish and subscribe callback
#define CALLBACK_BUFFER_SIZE 2500
char callback_buffer[CALLBACK_BUFFER_SIZE];
bool callback_complete;
char callback_event[30];
char callback_status[40];
char callback_target[40];
String current_event;
char current_data[60];
int current_event_tries = 0;

// particle messaging
char particle_register[PARTICLE_REGISTER_SIZE + 1];
char particle_status[STATUS_LENGTH + 1];

struct BrevitestOpticalSensorRecord
{ // 14 bytes
    char channel;
    uint8_t samples;
    unsigned long time_ms;
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint16_t temperature;
} reading_assay, reading_control_1, reading_control_2;

struct BrevitestTestRecord
{ // 550 bytes
    int start_time;
    int finish_time;
    char test_uuid[TEST_UUID_LENGTH + 1]; // 27 bytes
    uint8_t number_of_readings;
    uint16_t reserved;
    BrevitestOpticalSensorRecord reading[TEST_MAXIMUM_NUMBER_OF_READINGS]; // 504 bytes
} test_record;

struct BrevitestAssayRecord
{
    char uuid[ASSAY_UUID_LENGTH + 1];
    int duration;
    int optical_sensor_integration_time;
    int optical_sensor_gain;
    int reserved;
    int delay_between_optical_sensor_readings_ms;
    uint16_t BCODE_length;
    uint8_t BCODE_version;
    char BCODE[ASSAY_BCODE_CAPACITY];
} assay;

struct Particle_EEPROM
{
    uint8_t firmware_version; // 8 bytes
    uint8_t data_format_version;
    uint8_t most_recent_test;
    char serial_number[SERIAL_NUMBER_LENGTH + 1];    // 20 bytes, includes trailing \0
    BrevitestTestRecord test_cache[TEST_CACHE_SIZE]; // up to 4 test results cached
    Particle_EEPROM()
    {
        firmware_version = FIRMWARE_VERSION;
        data_format_version = DATA_FORMAT_VERSION;
        most_recent_test = 255;
    }
} eeprom;
