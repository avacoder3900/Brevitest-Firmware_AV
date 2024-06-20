#ifndef NEW_OPTICAL_DRIVER_H
#define NEW_OPTICAL_DRIVER_H

// ---------- Constants ---------- //
// The I2C slave address as defined by the sensor's datasheet.
#define DEFAULT_OPTICAL_ADDR 0x39

// Optical sensor register addresses.
#define REG_ENABLE 0x80     // Sensor mode.
#define REG_CONFIG 0x70     // Integration mode.
#define REG_CFG0 0xA9       // Set register bank.
#define REG_INTENAB 0xF9    // Set interrupts.
#define REG_ATIME 0x81      // Intergration time.
#define REG_ASTEP_1 0xCA    // Integration time.
#define REG_ASTEP_2 0xCB    // Integration time.
#define REG_STAT 0x71       // Sensor results status.
#define REG_ASTATUS 0x94    // Read results.

// Optical sensor register values.
#define REG_BANK_LOW_BEGIN 0x60     // First address of LOW register bank.
#define REG_BANK_LOW_END 0x74       // Last address of the LOW register bank.
#define REG_BANK_HIGH_BEGIN 0x80    // First address of the HIGH register bank.
#define REG_BANK_LOW 0x10           // Set LOW register bank.
#define REG_BANK_HIGH 0x00          // Set HIGH register bank.

#define ENABLE_OPTICAL_SENSOR 0x01  // Enables the sensor.
#define DISABLE_OPTICAL_SENSOR 0x00 // Disables the sensor.
#define ENABLE_SPM 0x03             // Enters specteral measurement mode.
#define DISABLE_SPM 0x01            // Leaves spectraal measurement mode.
#define DISABLE_INTR 0x00           // Disable interrupts.

// The default ATIME and ASTEP values recommended by the sensor's datasheet.
// Used to determine integration time
#define DEFAULT_ATIME 29
#define DEFAULT_ASTEP 599

#define MEASUREMENT_RESULTS_LENGTH 13   // The number of bytes to read from the sensor.
#define RESULTS_ARE_READY 1             // The sensor has results ready to be read.

// @todo Tune these values. Make sure that the check interval is less than the 
// integration time. so as not to accidently overlap with any subsequent measurements.
#define RESULTS_READY_TIMEOUT 5000          // The maximum time to wait for results to be ready.
#define RESULTS_READY_CHECK_INTERVAL 50     // The time between checks for results.

// Most and least significant bytes of a 16bit integer.
#define LSB(number) ((number) & 0xff)
#define MSB(number) (((number) >> 8) & 0xff)

// Integration time in microseconds.
#define INTEGRATION_TIME(atime, astep) ((int) (((atime) + 1) * ((astep) + 1) * (2.78)))

bool set_ATIME(byte addr, uint8_t ATIME);
bool set_ASTEP(byte addr, uint16_t ASTEP);
bool enable_optical_sensor(byte addr);
bool disable_optical_sensor(byte addr);
bool enable_measurement_mode(byte addr);
bool disable_measurement_mode(byte addr);
bool disable_interrupts(byte addr);
bool config_optical_sensor(char channel);
bool optical_results_ready(byte addr);
bool get_single_spectrophotometer_reading(char channel, int frequency, byte* buffer);
void config_switch(void);

#endif // NEW_OPTICAL_DRIVER_H