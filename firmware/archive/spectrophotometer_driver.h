#ifndef NEW_OPTICAL_DRIVER_H
#define NEW_OPTICAL_DRIVER_H

// ---------- Constants ---------- //
// The I2C slave address as defined by the sensor's datasheet.
#define DEFAULT_SPECTROPHOTOMETER_ADDR 0x39

// Spectrophotometer register addresses.
#define REG_ENABLE 0x80     // Sensor mode.
#define REG_CONFIG 0x70     // Integration mode.
#define REG_CFG0 0xA9       // Set register bank.
#define REG_ATIME 0x81      // Intergration time.
#define REG_ASTEP_1 0xCA    // Integration time.
#define REG_ASTEP_2 0xCB    // Integration time.
#define REG_STAT 0x71       // Sensor results status.
#define REG_ASTATUS 0x94    // Read results.
#define REG_SMUX_DATA 0x95  // First address of LOW register bank.
#define REG_SMUX_CONFIG 0xAF    // First address of HIGH register bank.

// Spectrophotometer commands.
#define ENABLE_SPECTROPHOTOMETER 0x01      // Enables the sensor.
#define DISABLE_SPECTROPHOTOMETER 0x00     // Disables the sensor.
#define ENABLE_SMUX 0x13                // Enters specteral measurement mode.
#define DISABLE_SMUX 0x01               // Leaves spectraal measurement mode.
#define SET_SMUX_CONFIG 0x02            // Sets SMUX config mode to write.

// The default ATIME and ASTEP values recommended by the sensor's datasheet.
// Used to determine integration time
#define DEFAULT_ATIME 29
#define DEFAULT_ASTEP 599

#define MEASUREMENT_RESULTS_LENGTH 12   // The number of bytes to read from the sensor.
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
bool enable_spectrophotometer(char channel);
bool disable_spectrophotometer(char channel);
bool start_SMUX_measurement(char channel);
bool disable_SMUX_measurement(char channel);
bool config_spectrophotometer(char channel);
bool spectrophotometer_results_ready(char channel);
void config_switch(void);
void power_on_spectrophotometer(char channel);
void power_off_all_spectrophotometers();
bool get_single_spectrophotometer_reading(char channel, byte* buffer);

#endif // NEW_OPTICAL_DRIVER_H