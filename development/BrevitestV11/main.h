#include "TCS34725.h"

// GLOBAL VARIABLES AND DEFINES

// general constants
#define FIRMWARE_VERSION 2
#define DATA_FORMAT_VERSION 10
#define ASSAY_UUID_LENGTH 8
#define TEST_UUID_LENGTH 24
#define DEVICE_ID_LENGTH 24
#define CARTRIDGE_UUID_LENGTH 24
#define TAB_DELIM "\t"
#define RETURN_DELIM "\n"
#define COMMA_DELIM ","
#define BCODE_END "99"

// device open and cartridge validation
#define DEVICE_OPEN_UUID "FFFFFFFFFFFFFFFFFFFFFFFF"
#define NO_CARTRIDGE_UUID "DDDDDDDDDDDDDDDDDDDDDDDD"
#define CARTRIDGE_ERROR_UUID "EEEEEEEEEEEEEEEEEEEEEEEE"
#define SUCCESS "SUCCESS"

// serial number
#define SERIAL_NUMBER_LENGTH 19

// sensors
#define SENSOR_NUMBER_OF_SAMPLES 3
#define SENSOR_WARMUP_DELAY_MS 1000
#define SENSOR_NUMBER_ASSAY 1
#define SENSOR_NUMBER_CONTROL 0
#define SENSOR_DEFAULT_INTEGRATION_TIME TCS34725_INTEGRATIONTIME_50MS
#define SENSOR_DEFAULT_IT_DELAY 50
#define SENSOR_DEFAULT_GAIN TCS34725_GAIN_16X

// lasers
#define LASER_WARMUP_DELAY_MS 1000

// assay
#define ASSAY_BCODE_CAPACITY 2000

// timers
#define DEVICE_STATE_CHECK_PERIOD 1000
#define BATTERY_CHECK_PERIOD 2000

// params
#define PARAM_NUMBER_INDEX 2
#define PARAM_VALUE_INDEX 6
#define PARAM_NUMBER_OF_PARAMS 7

// caches
#define TEST_CACHE_SIZE 3
#define TEST_MAXIMUM_NUMBER_OF_READINGS 10

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

// device LED
#define DEVICE_LED_BLINK_DELAY_DEFAULT 500
#define DEVICE_LED_BLINK_DELAY_CLAIMED 100
#define DEVICE_LED_BLINK_NO_TIMEOUT 0

// qr scanner
#define QR_DELAY_AFTER_POWER_ON_MS 1000
#define QR_DELAY_AFTER_TRIGGER_MS 50
#define QR_READ_TIMEOUT 2000
#define VALIDATE_CARTRIDGE_TIMEOUT 10000

// stepper
#define CUMULATIVE_STEP_LIMIT 10000
#define LIMIT_SWITCH_RELEASE_LENGTH 250
  //Well #1
  //#define STEPS_TO_MICROBEAD_WELL 1300

  //Well #2  - steps to the proximal edge of microbead well
#define STEPS_TO_MICROBEAD_WELL 2300
// #define STEPS_TO_FINAL_READ_POSITION 4400

// battery
#define BATTERY_CONVERSION_FACTOR 34

// cartridge heater
#define CARTRIDGE_HEATER_TEST_START_RED_THRESHOLD 8600
#define CARTRIDGE_HEATER_TEST_START_CHECK_PERIOD 2000

// solenoid
#define SOLENOID_PWM_FREQUENCY 1047

// upload
#define UPLOAD_INTERVAL 20000

// state
#define STATE_SENSOR_STARTUP_DELAY 200
#define STATE_SENSOR_LED_POWER 200
#define STATE_SENSOR_LED_DELAY 200
#define STATE_DEVICE_OPEN_THRESHOLD 10
// #define STATE_DEVICE_CARTRIDGE_CLEAR_THRESHOLD 16500  - white sensor body
#define STATE_DEVICE_CARTRIDGE_CLEAR_THRESHOLD 500

// timeouts
#define TIMEOUT_VALIDATION 10000
#define TIMEOUT_START 10000
#define TIMEOUT_CANCEL 10000
#define TIMEOUT_FINISH 10000
#define TIMEOUT_UPLOAD 20000

// application watchdog
void watchdog(void);
ApplicationWatchdog wd(30000, watchdog);

// pin definitions

// ELECTRON PIN MAPPINGS
int pinBatteryLED = A0;
int pinBatteryAin = A1;
int pinDCinDetect = A2;
int pinQRTrigger = A3;
int pinSensorLED = A4;
int pinSolenoid = A5;
int pinDeviceLEDRed = B0;
int pinDeviceLEDGreen = B1;
int pinDeviceLEDBlue = B2;
// int pinBluetoothMode = B3;
int pinAssayLaser = C0;
int pinControlLaser = C1;
int pinCartridgeHeaterLED = C2;
int pinCartridgeHeater = C3;
int pinAssaySDA = C4;
int pinAssaySCL = C5;
int pinControlSDA = D0;
int pinControlSCL = D1;
int pinLimitSwitch = D2;
int pinStepperSleep = D3;
int pinStepperDir = D4;
int pinStepperStep = D5;
int pinQRDecoderRX = RX;
int pinQRDecoderTX = TX;

// global variables
int cumulative_steps = CUMULATIVE_STEP_LIMIT;
int power_status = 0;
bool update_battery_life = false;
unsigned long next_upload;
unsigned long validation_timeout;
unsigned long start_timeout;
unsigned long cancel_timeout;
unsigned long finish_timeout;
unsigned long upload_timeout;

// power management unit - used to turn off blinking red light
PMIC _pmic;

// device state
bool device_open = false;
bool cartridge_loaded = false;
bool ready_to_scan_qr_code = false;
bool qr_code_being_scanned = false;

bool test_startup_successful = false;
bool test_in_progress = false;
bool reading_sensors = false;

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

bool cartridge_is_heated;
unsigned long next_sensor_reading_time = 0;

// device LED
void update_blinking_device_LED(void);
Timer device_LED_timer(DEVICE_LED_BLINK_DELAY_DEFAULT, update_blinking_device_LED);
struct DeviceLED {
    bool blinking;
    bool currently_on;
    unsigned int blink_rate;
    unsigned long blink_timeout;
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    DeviceLED() {
      blinking = false;
      currently_on = false;
      blink_rate = DEVICE_LED_BLINK_DELAY_DEFAULT;
      blink_timeout = 0;
      red = 0;
      green = 0;
      blue = 0;
    }
} device_LED;

// timers
void set_update_battery_life_flag(void);
Timer battery_check_timer(BATTERY_CHECK_PERIOD, set_update_battery_life_flag);

// sensors
TCS34725 tcsAssay;
TCS34725 tcsControl;
unsigned long last_sensor_reading_time = 0;
bool read_sensors_command_flag = false;
int read_sensors_command_integration_time;
int read_sensors_command_gain;
int read_sensors_samples;

// progress
int test_progress;
int test_percent_complete;
unsigned long test_last_progress_update;

// uuids
char cartridge_uuid[CARTRIDGE_UUID_LENGTH + 1];
char qr_uuid[CARTRIDGE_UUID_LENGTH + 1];
char device_id[DEVICE_ID_LENGTH + 1];
String device_id_string;

// publish and subscribe callback
#define CALLBACK_BUFFER_SIZE 2500
char callback_buffer[CALLBACK_BUFFER_SIZE];
bool callback_complete;
char callback_event[30];
char callback_status[40];
char callback_target[40];
char current_event[30];
char current_data[60];
int current_event_tries = 0;

// particle messaging
char particle_register[PARTICLE_REGISTER_SIZE + 1];
char particle_status[STATUS_LENGTH + 1];

BrevitestSensorSampleRecord assay_buffer[SENSOR_NUMBER_OF_SAMPLES];
BrevitestSensorSampleRecord control_buffer[SENSOR_NUMBER_OF_SAMPLES];

struct Param {      // 32 bytes
  uint16_t reset_steps;
  uint16_t step_delay_us;
  uint16_t steps_to_calibration_point;
  uint16_t stepper_wake_delay_ms;
  uint16_t solenoid_power;  // surge << 8 + sustain
  uint16_t solenoid_surge_period_ms;
  uint16_t start_test_heat_red_threshold;
  uint16_t reserved[10];
  Param() {
    reset_steps = 14000;
    step_delay_us = 800;
    steps_to_calibration_point = 720;  // added to constant STEPS_TO_MICROBEAD_WELL on reset_stage
    stepper_wake_delay_ms = 5;
    solenoid_power = 0xFF6C;    // surge = 255, sustain = 108
    solenoid_surge_period_ms = 150;
    start_test_heat_red_threshold = 7180;
  }
};

BrevitestSensorRecord sensor_reading, sensor_state;

struct BrevitestTestRecord {    // 74 bytes
    int start_time;
    int finish_time;
    char test_uuid[TEST_UUID_LENGTH + 1];    // 26 bytes w padding
    uint8_t number_of_readings;
    uint16_t reserved;
    BrevitestSensorRecord reading[TEST_MAXIMUM_NUMBER_OF_READINGS];
} test_record;

struct BrevitestAssayRecord {
    char uuid[ASSAY_UUID_LENGTH + 1];
    int duration;
    int sensor_integration_time;
    int sensor_gain;
    int reserved;
    int delay_between_sensor_readings_ms;
    uint16_t BCODE_length;
    uint8_t BCODE_version;
    char BCODE[ASSAY_BCODE_CAPACITY];
} assay;

struct Particle_EEPROM {
  uint8_t firmware_version;     // 8 bytes
  uint8_t data_format_version;
  uint8_t most_recent_test;
  uint8_t reserved2[5];
  char serial_number[SERIAL_NUMBER_LENGTH + 1]; // 20 bytes, includes trailing \0
  Param param;  // 32 bytes
  BrevitestTestRecord test_cache[TEST_CACHE_SIZE];  // up to 6 test results cached
  Particle_EEPROM() {
      firmware_version = FIRMWARE_VERSION;
      data_format_version = DATA_FORMAT_VERSION;
  }
} eeprom;
