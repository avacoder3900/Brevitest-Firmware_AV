#ifndef NEW_OPTICAL_DRIVER_H
#define NEW_OPTICAL_DRIVER_H

bool set_ATIME(byte addr, uint8_t ATIME);
bool set_ASTEP(byte addr, uint16_t ASTEP);
bool enable_optical_sensor(byte addr);
bool disable_optical_sensor(byte addr);
bool enable_measurement_mode(byte addr);
bool disable_measurement_mode(byte addr);
bool disable_interrupts(byte addr);
bool config_optical_sensor(char channel);
bool optical_results_ready(byte addr);
bool get_single_optical_measurement(char channel, byte* buffer);

#endif // NEW_OPTICAL_DRIVER_H