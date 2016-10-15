#include "TCS34725.h"

// GLOBAL VARIABLES AND DEFINES

// general constants
#define FIRMWARE_VERSION 6
#define DATA_FORMAT_VERSION 2
#define ASSAY_UUID_LENGTH 8
#define TEST_UUID_LENGTH 24
#define DEVICE_ID_LENGTH 24
#define CARTRIDGE_UUID_LENGTH 24
#define ERROR_MESSAGE(err) Serial.println(err)
#define CANCELLABLE(x) if (!cancel_process) {x}
#define TAB_DELIM "\t"
#define RETURN_DELIM "\n"
#define COMMA_DELIM ","

// device open and cartridge validation
#define DEVICE_OPEN_UUID "FFFFFFFFFFFFFFFFFFFFFFFF"
#define NO_CARTRIDGE_UUID "DDDDDDDDDDDDDDDDDDDDDDDD"
#define CARTRIDGE_ERROR_UUID "EEEEEEEEEEEEEEEEEEEEEEEE"
#define SUCCESS "SUCCESS"

// bluetooth
#define BLUETOOTH_TIMEOUT 2000
#define BLUETOOTH_ADD_SERVICE_STRING "AT+GATTADDSERVICE=UUID128=0f-6f-41-0d-89-47-48-70-98-4f-7d-c8-43-47-fa-ab"
#define BLUETOOTH_ADD_DEVICE_ID_CHARACTERISTIC_STRING "AT+GATTADDCHAR=UUID=0x0001,PROPERTIES=0x12,MIN_LEN=24,MAX_LEN=24,VALUE="
#define BLUETOOTH_ADD_CARTRIDGE_ID_CHARACTERISTIC_STRING "AT+GATTADDCHAR=UUID=0x0002,PROPERTIES=0x12,MIN_LEN=24,MAX_LEN=24,VALUE=000000000000000000000000"
#define BLUETOOTH_UPDATE_CHARACTERISTIC_STRING "AT+GATTCHAR="

int bluetooth_battery_service_index;
int bluetooth_battery_level_char_index;
int bluetooth_test_service_index;
int bluetooth_test_code_char_index;
int bluetooth_test_progress_char_index;

// serial number
#define SERIAL_NUMBER_LENGTH 19

// sensors
#define SENSOR_NUMBER_OF_SAMPLES 6
#define SENSOR_LED_WARMUP_DELAY_MS 3000
#define SENSOR_LED_TRANSITION_HIGH_DELAY_MS 800
#define SENSOR_LED_TRANSITION_LOW_DELAY_MS 400
#define SENSOR_NUMBER_ASSAY 1
#define SENSOR_NUMBER_CONTROL 0
#define SENSOR_LED_ASSAY 255
#define SENSOR_LED_CONTROL 229
#define SENSOR_DEVICE_OPEN_THRESHOLD 35
#define SENSOR_DEVICE_OPEN_CHECK_PERIOD 1000
#define SENSOR_CHECK_CARD_LED_POWER 200
#define SENSOR_CHECK_CARD_LED_DELAY 200
#define SENSOR_CARD_CHECK_THRESHOLD 25000

// assay
#define ASSAY_BCODE_CAPACITY 1000

// params
#define PARAM_NUMBER_INDEX 2
#define PARAM_VALUE_INDEX 6
#define PARAM_NUMBER_OF_PARAMS 7

// caches
#define TEST_CACHE_SIZE 6
#define TEST_MAXIMUM_NUMBER_OF_READINGS 10

// particle
#define PARTICLE_REGISTER_SIZE 622
#define PARTICLE_ARG_SIZE 63
#define PARTICLE_PUBLISH_INTERVAL 1000
#define MOVE_STEPS_BETWEEN_PARTICLE_PROCESS 10

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
#define QR_READ_TIMEOUT 1000
#define VALIDATE_CARTRIDGE_TIMEOUT 10000

// stepper
#define CUMULATIVE_STEP_LIMIT 9800
#define LIMIT_SWITCH_RELEASE_LENGTH 250

// battery
#define BATTERY_CONVERSION_FACTOR 34

// upload
#define UPLOAD_INTERVAL 60000

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
int pinBluetoothMode = B3;
int pinBluetoothRX = C2;
int pinBluetoothTX = C3;
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
bool test_in_progress;
bool start_test;
bool cancel_process;
bool calibrate;
bool stress_test;
int cumulative_steps = CUMULATIVE_STEP_LIMIT;
int power_status;
unsigned long last_upload;
bool qr_code_being_scanned = false;

// device open check
Timer device_open_timer(SENSOR_DEVICE_OPEN_CHECK_PERIOD, set_check_device_status_flag);
bool device_open_state = true;
bool check_device_status_flag = true;
bool cartridge_validated = false;

// device LED
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

void update_blinking_device_LED(void);
Timer device_LED_timer(DEVICE_LED_BLINK_DELAY_DEFAULT, update_blinking_device_LED);

// sensors
TCS34725 tcsAssay;
TCS34725 tcsControl;

// progress
int test_progress;
int test_percent_complete;
unsigned long test_last_progress_update;

// uuids
char cartridge_uuid[CARTRIDGE_UUID_LENGTH + 1];
char qr_uuid[CARTRIDGE_UUID_LENGTH + 1];
char device_id[DEVICE_ID_LENGTH + 1];

// bluetooth
#define BLUETOOTH_BUFFER_SIZE 500
char bluetooth_buffer[BLUETOOTH_BUFFER_SIZE];
int bluetooth_buffer_count;
int bluetooth_buffer_line_count;
char delim_string[2];
struct GATT {
    int service;
    int device_id_characteristic;
    int cartridge_id_characteristic;
} gatt;
char newline[2];

// particle messaging
#define CALLBACK_BUFFER_SIZE 1200
char callback_buffer[CALLBACK_BUFFER_SIZE];
bool callback_complete;
char particle_register[PARTICLE_REGISTER_SIZE + 1];
char particle_status[STATUS_LENGTH + 1];

struct BrevitestSensorSampleRecord {        // 12 bytes
    int sample_time;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    uint16_t clear;
};
BrevitestSensorSampleRecord assay_buffer[SENSOR_NUMBER_OF_SAMPLES];
BrevitestSensorSampleRecord control_buffer[SENSOR_NUMBER_OF_SAMPLES];

struct Param {      // 32 bytes
  uint16_t reset_steps;
  uint16_t step_delay_us;
  uint16_t publish_interval_during_move;
  uint16_t stepper_wake_delay_ms;
  uint16_t solenoid_power;  // surge << 8 + sustain
  uint16_t solenoid_surge_period_ms;
  uint16_t calibration_steps;
  uint16_t reserved[9];
  Param() {
    reset_steps = 14000;
    step_delay_us = 800;
    publish_interval_during_move = 100;
    stepper_wake_delay_ms = 5;
    solenoid_power = 0xFFC0;    // surge = 255, sustain = 192
    solenoid_surge_period_ms = 150;
    calibration_steps = 250;
  }
};

struct BrevitestSensorRecord {  // 16 bytes
    char channel;
    uint8_t samples;
    int start_time;
    uint16_t red_mean;
    uint16_t green_mean;
    uint16_t blue_mean;
    uint16_t clear_mean;
    uint16_t clear_max;
    uint16_t clear_min;
} sensor_reading;

struct BrevitestTestRecord {    // 74 bytes
    int start_time;
    int finish_time;
    char test_uuid[TEST_UUID_LENGTH + 1];    // 26 bytes w padding
    uint8_t number_of_readings;
    BrevitestSensorRecord reading[TEST_MAXIMUM_NUMBER_OF_READINGS];
} test_record;

struct BrevitestAssayRecord {
    char uuid[ASSAY_UUID_LENGTH + 1];
    int duration;
    int sensor_integration_time;
    int sensor_gain;
    int led_power;
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
