#include "spectrophotometer_driver.h"

//
//
//  Spectrophotometer switch
//
//

#define SWITCH_ADDR 0xA0     // I2C address of MCP23008.
#define SWITCH_IO_REGISTER 0x00     // I/O direction register address.
#define SWITCH_IO_CONFIG 0xE6     // Value to configure the IO register to output on GP0, GP2, and GP4.
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
    int result = Wire.endTransmission();
    delay(5);  // Wait for sensor to power on.
    if (result == 0) {
        Log.info("I2C device found at address %X", SWITCH_ADDR);
        config_spectrophotometer(channel);
    } else {
        Log.info("No I2C device found at address %X", SWITCH_ADDR);
    }

    byte addr = sensor_addr(channel);
    Wire.beginTransmission(addr);
    Wire.write(0x00);
    if (Wire.endTransmission() == 0) {
        Log.info("AS7341 found at address %X", addr);
    } else {
        Log.info("AS7341 device found at address %X", addr);
    }
}

// ---------- Private ---------- //
/**
 * Given a channel finds the associated spectrophotometer address.
 * @todo Add logic here to interface with hardware that will determine the sensor's address.
 * @param channel The channel of the sensor.
 * @return The I2C address of the sensor on the given channel.
 */
byte sensor_addr(char channel) 
{
    return DEFAULT_SPECTROPHOTOMETER_ADDR;
}

/**
 * Request bytes starting at the given register.
 * @param chip_addr The I2C address of the spectrophotometer.
 * @param reg_addr The address of the register to write to.
 * @param num_bytes The number of bytes to read.
 * @param buffer The address of the register to write to.
 * @return The status register set, 0 if successful.
 */
byte sensor_read_bytes(byte chip_addr, byte reg_addr, int num_bytes, byte* buffer)
{
    Wire.beginTransmission(chip_addr);
    Wire.write(reg_addr);
    byte result = Wire.endTransmission(false); // Don't drop the bus.
    Wire.requestFrom(chip_addr, num_bytes);
    if(result != 0)
        Log.info("Ready optics: addr %X, request bytes failed, result: %X", chip_addr, result);

    int i = 0;
    while(Wire.available() && i < num_bytes) {
        buffer[i++] = Wire.read();
    }
    return Wire.endTransmission(true);  // Drop the bus.
}

/**
 * Request bytes starting at the given register.
 * @param chip_addr The I2C address of the spectrophotometer.
 * @param reg_addr The address of the register to write to.
 * @param num_bytes The number of bytes to read.
 * @param buffer The address of the register to write to.
 * @return The status register set, 0 if successful.
 */
byte sensor_read_control_byte(byte chip_addr, byte reg_addr)
{
    Wire.beginTransmission(chip_addr);
    Wire.write(reg_addr);
    byte result = Wire.endTransmission(false); // Don't drop the bus.
    Wire.requestFrom(chip_addr, 1);
    if(result != 0)
        Log.info("Ready optics: addr %X, request control byte failed, result: %X", chip_addr, result);

    result = byte(Wire.read() & 0xFF);
    Wire.endTransmission(true);  // Drop the bus.
    return result;
}

/**
 * Writes the given byte data to the given register on the given sensor.
 * Releases the I2C bus after writing.
 * 
 * @param chip_addr The I2C address of the spectrophotometer.
 * @param reg_addr The address of the register to write to.
 * @param data The data to write to the register.
 * @return The status of the transmission. 0 if successful.
 */
byte sensor_write_byte(byte chip_addr, byte reg_addr, byte data) 
{
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
bool set_ATIME(char channel, uint8_t ATIME) 
{
    byte addr = sensor_addr(channel);
    byte result = sensor_write_byte(addr, REG_ATIME, ATIME);
    if (result != 0) 
        Log.info("Config optics: addr %X, set atime failed, result: %X", addr, result);

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
bool set_ASTEP(char channel, uint16_t ASTEP) 
{
    byte addr = sensor_addr(channel);
    // Set the least significant byte of the ASTEP register.
    byte set_lsb = sensor_write_byte(addr, REG_ASTEP_1, LSB(ASTEP));
    if (set_lsb != 0) 
        Log.info("Config optics: addr %X, set astep lsb failed, result: %X", addr, set_lsb);

    // Set the most significant byte of the ASTEP register.
    byte set_msb = sensor_write_byte(addr, REG_ASTEP_2, MSB(ASTEP));
    if (set_lsb != 0) 
        Log.info("Config optics: addr %X, set astep msb failed, result: %X", addr, set_lsb);

    return set_lsb == 0 && set_msb == 0;
}

/**
 * Disable spectral measurement.
 * 
 * Assumes the sensor is in IDLE mode.
 * Moves sensor to IDLE mode.
 * 
 * @param addr The I2C address of the sensor.
 * @return True if the write succeded, false otherwise.
 */
bool disable_SMUX_measurement(char channel) 
{
    byte addr = sensor_addr(channel);
    byte result = 0;

    result = sensor_write_byte(addr, REG_ENABLE, DISABLE_SMUX);
    if (result != 0)
        Log.info("Config optics: addr %X, enable measurement failed, result: %X", addr, result);

    result = sensor_read_control_byte(addr, REG_ENABLE);
    return result == 0x01;
}

/**
 * Sets the sensor's SMUX multiplexing state ().
 * 
 * Assumes the sensor is in IDLE mode.
 * 
 * @param addr The I2C address of the sensor.
 * @param low True if low frequencies, false if high frequencies.
 */
void set_SMUXFrequencies(char channel, bool low) 
{
    byte addr = sensor_addr(channel);
    if(!disable_SMUX_measurement(channel)) {
        Log.info("Config optics: addr %X, disable measurement failed", addr);
        return;
    };

    if (low) {
        // SMUX Config for F1,F2,F3,F4,NIR,Clear
        sensor_write_byte(addr, byte(0x00), byte(0x30)); // F3 left set to ADC2
        sensor_write_byte(addr, byte(0x01), byte(0x01)); // F1 left set to ADC0
        sensor_write_byte(addr, byte(0x02), byte(0x00)); // Reserved or disabled
        sensor_write_byte(addr, byte(0x03), byte(0x00)); // F8 left disabled
        sensor_write_byte(addr, byte(0x04), byte(0x00)); // F6 left disabled
        sensor_write_byte(addr, byte(0x05), byte(0x42)); // F4 left connected to ADC3/f2 left connected to ADC1
        sensor_write_byte(addr, byte(0x06), byte(0x00)); // F5 left disbled
        sensor_write_byte(addr, byte(0x07), byte(0x00)); // F7 left disbled
        sensor_write_byte(addr, byte(0x08), byte(0x50)); // CLEAR connected to ADC4
        sensor_write_byte(addr, byte(0x09), byte(0x00)); // F5 right disabled
        sensor_write_byte(addr, byte(0x0A), byte(0x00)); // F7 right disabled
        sensor_write_byte(addr, byte(0x0B), byte(0x00)); // Reserved or disabled
        sensor_write_byte(addr, byte(0x0C), byte(0x20)); // F2 right connected to ADC1
        sensor_write_byte(addr, byte(0x0D), byte(0x04)); // F4 right connected to ADC3
        sensor_write_byte(addr, byte(0x0E), byte(0x00)); // F6/F8 right disabled
        sensor_write_byte(addr, byte(0x0F), byte(0x30)); // F3 right connected to AD2
        sensor_write_byte(addr, byte(0x10), byte(0x01)); // F1 right connected to AD0
        sensor_write_byte(addr, byte(0x11), byte(0x50)); // CLEAR right connected to AD4
        sensor_write_byte(addr, byte(0x12), byte(0x00)); // Reserved or disabled
        sensor_write_byte(addr, byte(0x13), byte(0x06)); // NIR connected to ADC5
    } else {
        // SMUX Config for F5,F6,F7,F8,NIR,Clear
        sensor_write_byte(addr, byte(0x00), byte(0x00)); // F3 left disable
        sensor_write_byte(addr, byte(0x01), byte(0x00)); // F1 left disable
        sensor_write_byte(addr, byte(0x02), byte(0x00)); // reserved/disable
        sensor_write_byte(addr, byte(0x03), byte(0x40)); // F8 left connected to ADC3
        sensor_write_byte(addr, byte(0x04), byte(0x02)); // F6 left connected to ADC1
        sensor_write_byte(addr, byte(0x05), byte(0x00)); // F4/ F2 disabled
        sensor_write_byte(addr, byte(0x06), byte(0x10)); // F5 left connected to ADC0
        sensor_write_byte(addr, byte(0x07), byte(0x03)); // F7 left connected to ADC2
        sensor_write_byte(addr, byte(0x08), byte(0x50)); // CLEAR Connected to ADC4
        sensor_write_byte(addr, byte(0x09), byte(0x10)); // F5 right connected to ADC0
        sensor_write_byte(addr, byte(0x0A), byte(0x03)); // F7 right connected to ADC2
        sensor_write_byte(addr, byte(0x0B), byte(0x00)); // Reserved or disabled
        sensor_write_byte(addr, byte(0x0C), byte(0x00)); // F2 right disabled
        sensor_write_byte(addr, byte(0x0D), byte(0x00)); // F4 right disabled
        sensor_write_byte(addr, byte(0x0E), byte(0x24)); // F8 right connected to ADC2/ F6 right connected to ADC1
        sensor_write_byte(addr, byte(0x0F), byte(0x00)); // F3 right disabled
        sensor_write_byte(addr, byte(0x10), byte(0x00)); // F1 right disabled
        sensor_write_byte(addr, byte(0x11), byte(0x50)); // CLEAR right connected to AD4
        sensor_write_byte(addr, byte(0x12), byte(0x00)); // Reserved or disabled
        sensor_write_byte(addr, byte(0x13), byte(0x06)); // NIR connected to ADC5
    }
    start_SMUX_measurement(channel);
    return;
}

/**
 * Enables the spectrophotometer.
 * 
 * Assumes the sensor is in SLEEP mode.
 * Moves it from SLEEP mode to IDLE mode.
 * 
 * @param addr The I2C address of the sensor.
 * @return True if the write succeded, false otherwise.
 */
bool enable_spectrophotometer(char channel) 
{
    byte addr = sensor_addr(channel);
    byte result = sensor_write_byte(addr, REG_ENABLE, ENABLE_SPECTROPHOTOMETER);
    if (result != 0)
        Log.info("Config optics: addr %X, enable sensor failed, result: %X", addr, result);

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
bool start_SMUX_measurement(char channel) 
{
    byte addr = sensor_addr(channel);
    byte result = 0;

    result = sensor_write_byte(addr, REG_ENABLE, ENABLE_SMUX);
    if (result != 0)
        Log.info("Config optics: addr %X, enable measurement failed, result: %X", addr, result);

    delay(5);
    byte state = sensor_read_control_byte(addr, REG_ENABLE);
    Log.info("Config optics: addr %X, start SMUX register, value: %X", addr, state);
    return result == 0;
}

/**
 * Enables and then configures the spectrophotometer at the given channel.
 * 
 * Assumes the sensor is in SLEEP mode. Moves to IDLE.
 * 
 * @param channel The channel of the sensor.
 * @return True if config succeded, false otherwise.
 */
bool config_spectrophotometer(char channel) 
{
    bool success;

    // Enable the sensor, now in IDLE mode.
    success = enable_spectrophotometer(channel);
    // Config integration time.
    success = success && set_ATIME(channel, DEFAULT_ATIME);
    success = success && set_ASTEP(channel, DEFAULT_ASTEP);

    return success;
}

/**
 * Checks if the sensor is in measurement mode and the measurement results are ready.
 * 
 * @param addr The I2C address of the sensor.
 * @return True if the spectrophotometer results are ready to be read.
 */
bool spectrophotometer_results_ready(char channel) 
{
    byte addr = sensor_addr(channel);
    byte state = sensor_read_control_byte(addr, REG_ENABLE);
    bool active = (state >> 1) & 0x01; // If the measurement mode bit is set.
    byte ready = sensor_read_control_byte(addr, REG_STAT) & 0x01;
    return active && ready;
}

bool spectrophotometer_wait_for_results(char channel) 
{
    unsigned long timeout = millis() + RESULTS_READY_TIMEOUT;   // Maximum time to wait for results.
    bool ready = spectrophotometer_results_ready(channel);                   // Are the results ready?
    // Wait for the results to be ready or for us to timeout.
    while (!ready && millis() < timeout) {
        delayMicroseconds(RESULTS_READY_CHECK_INTERVAL);
        ready = spectrophotometer_results_ready(channel);
    }
    if (!ready) {
        Log.info("Config optics: channel %c, no results found", channel);
    }
    return ready;
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
bool get_single_spectrophotometer_reading(char channel, byte* buffer) 
{
    // Turn on sensor
    power_on_spectrophotometer(channel);

    // set multiplexing to low channels and start measurement
    set_SMUXFrequencies(channel, true);   
    if (spectrophotometer_wait_for_results(channel)) {
        // The results are ready, read them.
        sensor_read_bytes(channel, REG_SMUX_DATA, 6, buffer);
        Log.info("Config optics: channel %c, low results %X %X %X %X %X %X", channel, buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
    }

    // set multiplexing to high channels
    set_SMUXFrequencies(channel, false);   
    if (spectrophotometer_wait_for_results(channel)) {
        // The results are ready, read them.
        sensor_read_bytes(channel, REG_SMUX_DATA, 6, &buffer[6]);
        Log.info("Config optics: channel %c, high results %X %X %X %X %X %X", channel, buffer[6], buffer[7], buffer[8], buffer[9], buffer[10], buffer[11]);
    }

    power_off_all_spectrophotometers();

    return true;
}