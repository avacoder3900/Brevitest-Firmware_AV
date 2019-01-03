#include "LSM6DS3_L3.h"

//
// GLOBAL VARIABLES AND DEFINES

// general constants
#define FIRMWARE_VERSION 5
#define DATA_FORMAT_VERSION 11
#define ASSAY_UUID_LENGTH 8
#define TEST_UUID_LENGTH 24
#define DEVICE_ID_LENGTH 24
#define CARTRIDGE_UUID_LENGTH 24
#define TAB_DELIM "\t"
#define RETURN_DELIM "\n"
#define COMMA_DELIM ","
#define BCODE_END "99"
#define MAX_ANALOG_READ 4095
#define THERMISTOR_SCALE 100000

// device open and cartridge validation
#define DEVICE_OPEN_UUID "FFFFFFFFFFFFFFFFFFFFFFFF"
#define NO_CARTRIDGE_UUID "DDDDDDDDDDDDDDDDDDDDDDDD"
#define CARTRIDGE_ERROR_UUID "EEEEEEEEEEEEEEEEEEEEEEEE"
#define SUCCESS "SUCCESS"

// serial number
#define SERIAL_NUMBER_LENGTH 19

// optical sensors
#define OPTICAL_SENSOR_NUMBER_OF_SAMPLES 3
#define OPTICAL_SENSOR_DEFAULT_PARAM 0xAB

// lasers
#define LASER_WARMUP_DELAY_MS 1000

// assay
#define ASSAY_BCODE_CAPACITY 2000

// params
#define PARAM_NUMBER_INDEX 2
#define PARAM_VALUE_INDEX 6
#define PARAM_NUMBER_OF_PARAMS 7

// caches
#define TEST_CACHE_SIZE 3
#define TEST_MAXIMUM_NUMBER_OF_READINGS 10

// tables
#define TABLE_PELTIER 0
#define TABLE_ASSAY 1
#define TABLE_CONTROL 2
#define TABLE_PELTIER_LENGTH 21

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
#define BARCODE_READ_TIMEOUT 2000
#define VALIDATE_CARTRIDGE_TIMEOUT 20000

// stepper
#define CUMULATIVE_STEP_LIMIT 11000
#define LIMIT_SWITCH_RELEASE_LENGTH 250
  //Well #1
  //#define STEPS_TO_MICROBEAD_WELL 1300

  //Well #2  - steps to the proximal edge of microbead well
#define STEPS_TO_MICROBEAD_WELL 2500
// #define STEPS_TO_FINAL_READ_POSITION 4400

// controller sensor readings
#define CONTROLLER_SENSORS_READ_INTERVAL 5000

// peltier
#define PELTIER_PIN_VALUE 160
#define PELTIER_PWM_FREQUENCY 256
#define PELTIER_OFF_THRESHOLD 100
#define PELTIER_ON_THRESHOLD 98
#define PELTIER_THERMISTOR_BALANCE_RESISTANCE 5000
#define PELTIER_THERMISTOR_BETA 3988
#define PELTIER_THERMISTOR_BASE_RESISTANCE 10000

// fan
#define FAN_ON_OFF_THRESHOLD 102

// solenoid
#define SOLENOID_PWM_FREQUENCY 1047

// upload
#define UPLOAD_INTERVAL 20000

// state
#define CARTRIDGE_LOADED_OPTICAL_RED_THRESHOLD 16500

// timeouts
#define TIMEOUT_VALIDATION 10000
#define TIMEOUT_START 20000
#define TIMEOUT_CANCEL 10000
#define TIMEOUT_FINISH 10000
#define TIMEOUT_UPLOAD 20000

// application watchdog
void watchdog(void);
ApplicationWatchdog wd(30000, watchdog);

// pin definitions

// ELECTRON PIN MAPPINGS

int pinAssaySensor_Ready = A0;
int pinPeltierThermistor = A1;
int pinBarcode_Trigger = A2;
int pinLaserAssay = A3;
int pinSolenoid = A4;
int pinHeater = A5;
int pinLaserControl = DAC1;
int pinBarcode_Success = WKP;
int pinBuzzer = B0;
int pinFan = B1;
int pinControlSensor_Ready = B2;
int pinControlThermistor = B3;
int pinAssayThermistor = B4;
int pinDoorOpen = B5;
// int pinUnused = C0;
// int pinUnused = C1;
int pinGPS_RX = C2;
int pinGPS_TX = C3;
int pinControllerSensor_SDA = C4;
int pinControllerSensor_SCL = C5;
int pinOpticalSensor_SDA = D0;
int pinOpticalSensor_SCL = D1;
int pinInteriorLED = D2;
// int pinUnused = D3;
int pinLimitSwitch = D4;
int pinStepper_Sleep = D5;
int pinStepper_Dir = D6;
int pinStepper_Step = D7;
int pinBarcode_RX = RX;
// int pinUnused = TX;

// global variables
int cumulative_steps = CUMULATIVE_STEP_LIMIT;
int power_status = 0;
unsigned long next_upload;
unsigned long validation_timeout;
unsigned long start_timeout;
unsigned long cancel_timeout;
unsigned long finish_timeout;
unsigned long upload_timeout;

// device state
volatile bool door_state_changed = false;
bool door_open = false;
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

bool cartridge_is_heated;
unsigned long next_optical_sensor_reading_time = 0;

// controller sensors read timer
void read_all_controller_sensors(void);
Timer read_all_controller_sensors_timer(CONTROLLER_SENSORS_READ_INTERVAL, read_all_controller_sensors);

// IMU
LSM6DS3 myIMU;

// temperature control system
int imu_temp_C_10X;
int peltier_temp_C_10X;
int assay_temp_C_10X;
int control_temp_C_10X;
bool heater_on = false;
bool fan_on = false;

// optical sensors
unsigned long last_optical_sensor_reading_time = 0;
bool read_optical_sensors_command_flag = false;
int read_optical_sensors_command_param;

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
char current_event[30];
char current_data[60];
int current_event_tries = 0;

// particle messaging
char particle_register[PARTICLE_REGISTER_SIZE + 1];
char particle_status[STATUS_LENGTH + 1];

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
    solenoid_power = 0xFF80;    // surge = 255, sustain = 128
    solenoid_surge_period_ms = 100;
    start_test_heat_red_threshold = 7180;
  }
};

struct BrevitestOpticalSensorRecord {  // 14 bytes
    char channel;
    uint8_t samples;
    unsigned long time_ms;
    uint16_t red;
    uint16_t green;
    uint16_t blue;
    uint16_t temperature;
} optical_state_reading;

struct BrevitestTestRecord {    // 74 bytes
    int start_time;
    int finish_time;
    char test_uuid[TEST_UUID_LENGTH + 1];    // 26 bytes w padding
    uint8_t number_of_readings;
    uint16_t reserved;
    BrevitestOpticalSensorRecord reading[TEST_MAXIMUM_NUMBER_OF_READINGS];
} test_record;

struct BrevitestAssayRecord {
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
