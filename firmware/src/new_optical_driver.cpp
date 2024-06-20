/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/leo3/github/brevitest-device/firmware/src/new_optical_driver.ino"
#include "new_optical_driver.h"

// ---------- Private ---------- //
/**
 * Given a channel finds the associated optical sensor address.
 * @todo Add logic here to interface with hardware that will determine the sensor's address.
 * @param channel The channel of the sensor.
 * @return The I2C address of the sensor on the given channel.
 */
byte sensor_addr(char channel);
void sensor_set_reg_bank(byte chip_addr, byte reg_addr);
byte sensor_request_bytes(byte chip_addr, byte reg_addr, size_t num_bytes);
byte sensor_write_byte(byte chip_addr, byte reg_addr, byte data);
bool set_ATIME(byte addr, uint8_t ATIME);
bool set_ASTEP(byte addr, uint16_t ASTEP);
bool enable_optical_sensor(byte addr);
bool disable_optical_sensor(byte addr);
bool enable_measurement_mode(byte addr);
bool disable_measurement_mode(byte addr);
bool disable_interrupts(byte addr);
bool config_optical_sensor(char channel);
bool optical_results_ready(byte addr);
void config_switch();
void power_off_all_spectrophotometers();
void power_on_spectrophotometer(char channel);
bool get_single_spectrophotometer_reading(char channel, int frequency, byte* buffer);
#line 10 "/Users/leo3/github/brevitest-device/firmware/src/new_optical_driver.ino"
byte sensor_addr(char channel) 
{
    return DEFAULT_OPTICAL_ADDR;
}

/**
 * Sets the appropriate register bank for the given register adddress in
 * the optical sensor.
 * 
 * @param chip_addr The I2C address of the optical sensor.
 * @param reg_addr The address to check the register bank of.
 */
void sensor_set_reg_bank(byte chip_addr, byte reg_addr) 
{
    if (reg_addr >= REG_BANK_LOW_BEGIN && reg_addr <= REG_BANK_LOW_END) {
        Wire.beginTransmission(chip_addr);
        Wire.write(reg_addr);    
        Wire.write(REG_BANK_LOW);
        Wire.endTransmission();
    } else if (reg_addr >= REG_BANK_HIGH_BEGIN) {
        Wire.beginTransmission(chip_addr);
        Wire.write(reg_addr);     
        Wire.write(REG_BANK_HIGH);
        Wire.endTransmission();
    }
}

/**
 * Request bytes starting at the given register.
 * @param chip_addr The I2C address of the optical sensor.
 * @param reg_addr The address of the register to write to.
 * @return The status register set, 0 if successful.
 */
byte sensor_request_bytes(byte chip_addr, byte reg_addr, size_t num_bytes)
{
    sensor_set_reg_bank(chip_addr, reg_addr);
    Wire.beginTransmission(chip_addr);
    Wire.write(reg_addr);
    byte result = Wire.endTransmission(false); // Don't drop the bus.
    Wire.requestFrom(chip_addr, num_bytes);
    return result;
}

/**
 * Writes the given byte data to the given register on the given sensor.
 * Releases the I2C bus after writing.
 * 
 * @param chip_addr The I2C address of the optical sensor.
 * @param reg_addr The address of the register to write to.
 * @param data The data to write to the register.
 * @return The status of the transmission. 0 if successful.
 */
byte sensor_write_byte(byte chip_addr, byte reg_addr, byte data) 
{
    sensor_set_reg_bank(chip_addr, reg_addr);
    Wire.beginTransmission(chip_addr);
    Wire.write(reg_addr);  
    Wire.write(data);
    return Wire.endTransmission(true);  // Drop the bus.
}

// ---------- Public ---------- //
/**
 * Sets the sensor's ATIME value.
 * 
 * Assumes the sensor is in IDLE mode.
 * 
 * @param addr The I2C address of the sensor.
 * @param ATIME The value to write to the ATIME register.
 * @return True if the write succeded, false otherwise.
 */
bool set_ATIME(byte addr, uint8_t ATIME) 
{
    byte result = sensor_write_byte(addr, REG_ATIME, ATIME);
    if (result != 0) 
        Log.info("Config optics: addr %d, set atime failed, result: %d", addr, result);

    return result == 0;
}

/**
 * Sets the sensor's ASTEP value.
 * 
 * Assumes the sensor is in IDLE mode.
 * 
 * @param addr The I2C address of the sensor.
 * @param ASTEP The value to write to the ASTEP register.
 * @return True if the write succeded, false otherwise.
 */
bool set_ASTEP(byte addr, uint16_t ASTEP) 
{
    // Set the least significant byte of the ASTEP register.
    byte set_lsb = sensor_write_byte(addr, REG_ASTEP_1, LSB(ASTEP));
    if (set_lsb != 0) 
        Log.info("Config optics: addr %d, set astep lsb failed, result: %d", addr, set_lsb);

    // Set the most significant byte of the ASTEP register.
    byte set_msb = sensor_write_byte(addr, REG_ASTEP_2, MSB(ASTEP));
    if (set_lsb != 0) 
        Log.info("Config optics: addr %d, set astep msb failed, result: %d", addr, set_lsb);

    return set_lsb == 0 && set_msb == 0;
}

/**
 * Enables the optical sensor.
 * 
 * Assumes the sensor is in SLEEP mode.
 * Moves it from SLEEP mode to IDLE mode.
 * 
 * @param addr The I2C address of the sensor.
 * @return True if the write succeded, false otherwise.
 */
bool enable_optical_sensor(byte addr) 
{
    byte result = sensor_write_byte(addr, REG_ENABLE, ENABLE_OPTICAL_SENSOR);
    if (result != 0)
        Log.info("Config optics: addr %d, enable sensor failed, result: %d", addr, result);

    return result == 0;
}

/**
 * Disables the optical sensor.
 * 
 * Assumes the sensor is in IDLE mode.
 * Moves it from IDLE mode to SLEEP mode.
 * 
 * @param addr The I2C address of the sensor.
 * @return True if the write succeded, false otherwise.
 */
bool disable_optical_sensor(byte addr)
{
    byte result = sensor_write_byte(addr, REG_ENABLE, DISABLE_OPTICAL_SENSOR);
    if (result != 0)
        Log.info("Config optics: addr %d, disable sensor failed, result: %d", addr, result);

    return result == 0;
}

/**
 * Enables and begins spectral measurement.
 * 
 * Assumes the sensor is in IDLE mode.
 * Moves sensor from IDLE mode to ACTIVE mode.
 * 
 * @param addr The I2C address of the sensor.
 * @return True if the write succeded, false otherwise.
 */
bool enable_measurement_mode(byte addr) 
{
    byte result = sensor_write_byte(addr, REG_ENABLE, ENABLE_SPM);
    if (result != 0)
        Log.info("Config optics: addr %d, enable measurement failed, result: %d", addr, result);

    return result == 0;
}

/**
 * Disables spectral measurement.
 * 
 * Assumes the sensor is in ACTIVE mode.
 * Moves it from ACTIVE mode to IDLE mode.
 * 
 * @param addr The I2C address of the sensor.
 * @return True if the write succeded, false otherwise.
 */
bool disable_measurement_mode(byte addr) 
{
    byte result = sensor_write_byte(addr, REG_ENABLE, DISABLE_SPM);
    if (result != 0)
        Log.info("Config optics: addr %d, disable measurement failed, result: %d", addr, result);

    return result == 0;
}

/**
 * Disables interrupts.
 * @param addr The I2C address of the sensor.
 * @return True if the write succeded, false otherwise.
 */
bool disable_interrupts(byte addr) 
{
    byte result = sensor_write_byte(addr, REG_INTENAB, DISABLE_INTR);
    if (result != 0)
        Log.info("Config optics: addr %d, disable interrupts failed, result: %d", addr, result);

    return result == 0;
}

/**
 * Enables and then configures the optical sensor at the given channel.
 * 
 * Assumes the sensor is in SLEEP mode. Moves to IDLE.
 * 
 * @param channel The channel of the sensor.
 * @return True if config succeded, false otherwise.
 */
bool config_optical_sensor(char channel) 
{
    bool success;
    byte chip_addr = sensor_addr(channel);

    // Enable the sensor, now in IDLE mode.
    success = enable_optical_sensor(chip_addr);
    // Config integration time.
    success = success && set_ATIME(chip_addr, DEFAULT_ATIME);
    success = success && set_ASTEP(chip_addr, DEFAULT_ASTEP);
    // Disable interrupts.
    success = success && disable_interrupts(chip_addr);

    return success;
}

/**
 * Checks if the sensor is in measurement mode and the measurement results are ready.
 * 
 * @param addr The I2C address of the sensor.
 * @return True if the optical sensor results are ready to be read.
 */
bool optical_results_ready(byte addr) 
{
    byte result;

    // Check if the sensor is in measurement mode.
    result = sensor_request_bytes(addr, REG_ENABLE, 1);
    if(result != 0)
        Log.info("Ready optics: addr %d, request bytes failed, result: %d", addr, result);
    bool active = (Wire.read() >> 1) & 0x01; // If the measurement mode bit is set.
    Wire.endTransmission();

    // Check if the sensor results are ready.
    result = sensor_request_bytes(addr, REG_STAT, 1);
    if(result != 0)
        Log.info("Ready optics: addr %d, request bytes failed, result: %d", addr, result);

    bool ready = Wire.read() & RESULTS_ARE_READY; // If the ready bit is set.
    Wire.endTransmission();

    return active && ready;
}

#define SWITCH_ADDR 0xA0     // I2C address of MCP23008.
#define SWITCH_IO_REGISTER 0x00     // I/O direction register address.
#define SWITCH_IO_CONFIG 0x15     // Value to configure the IO register to output on GP0, GP2, and GP4.
#define SWITCH_GPIO_REGISTER 0x09     // GPIO register address.
#define SWITCH_TURN_OFF_ALL 0x00     // Turn off all spectrophotometers.
#define SWITCH_TURN_ON_A 0x01     // Turn on spectrophotometer A.
#define SWITCH_TURN_ON_B 0x04     // Turn on spectrophotometer B.
#define SWITCH_TURN_ON_C 0x10     // Turn on spectrophotometer C.

void config_switch() 
{
    // Configure spectrophotometer switch.
    Wire.beginTransmission(SWITCH_ADDR);
    Wire.write(SWITCH_IO_REGISTER);    
    Wire.write(SWITCH_IO_CONFIG);
    Wire.endTransmission();
    Log.info("Config optics: spectrophotometer switch configured");
}


void power_off_all_spectrophotometers() 
{
    // Turn off all sensors.
    Wire.beginTransmission(SWITCH_ADDR);
    Wire.write(SWITCH_GPIO_REGISTER);    
    Wire.write(SWITCH_TURN_OFF_ALL);
    Wire.endTransmission();
    Log.info("Config optics: all spectrophotometers off");
}

void power_on_spectrophotometer(char channel) 
{
    // Turn on sensor
    Wire.beginTransmission(SWITCH_ADDR);
    Wire.write(SWITCH_GPIO_REGISTER);
    switch (channel) {
        case 'A':
            Wire.write(SWITCH_TURN_ON_A);
            Log.info("Config optics: spectrophotometer A on");
            break;
        case 'B':
            Wire.write(SWITCH_TURN_ON_B);
            Log.info("Config optics: spectrophotometer B on");
            break;
        case 'C':
            Wire.write(SWITCH_TURN_ON_C);
            Log.info("Config optics: spectrophotometer C on");
            break;
        default:
            Wire.write(SWITCH_TURN_OFF_ALL);
            Log.info("Config optics: spectrophotometers off by default");
            break;
    }
    Wire.endTransmission();
    delay(20);  // Wait for sensor to power on.
}

/**
 * Begins a measurement if the sensor is not already in measurement mode.
 * Waits for the results to be ready, then reads them into the buffer.
 * 
 * The first byte will contain the spectral gain and saturation status, 
 * the subsequent bytes will contain the spectral data.
 * 
 * Does not exit measurement mode, sensor will keep making measurements 
 * unless explicilty stopped with `disable_measurement_mode()`.
 * 
 * @param channel The channel of the sensor.
 * @param buffer The buffer to write the results to.
 * @returns False if the results could not be read within the timeout.
 */
bool get_single_spectrophotometer_reading(char channel, int frequency, byte* buffer) 
{
    // Turn on sensor
    power_on_spectrophotometer(channel);

    byte addr = sensor_addr(channel);
    Wire.beginTransmission(addr);
    Wire.write(0x00);
    int result = Wire.endTransmission();
    if (result == 0) {
        Log.info("I2C device found at address %X", addr);
    } else {
        Log.info("No I2C device found at address %X", addr);
    }

    // Begin an optical measurement.
    enable_measurement_mode(addr);

    unsigned long timeout = millis() + RESULTS_READY_TIMEOUT;   // Maximum time to wait for results.
    bool ready = optical_results_ready(addr);                   // Are the results ready?
    // Wait for the results to be ready or for us to timeout.
    while (!ready && millis() < timeout) {
        delayMicroseconds(RESULTS_READY_CHECK_INTERVAL);
        ready = optical_results_ready(addr);
    }

    // Return early if the results are not ready within the timeout.
    if (!ready) {
        Log.info("Config optics: addr %d, results not ready", addr);
        return false;
    }

    // The results are ready, read them.
    sensor_request_bytes(addr, REG_ASTATUS, MEASUREMENT_RESULTS_LENGTH);

    // Read the results into the buffer.
    for (int i = 0; i < MEASUREMENT_RESULTS_LENGTH; i++) {
        buffer[i] = Wire.read();
    }

    power_off_all_spectrophotometers();

    return true;
}