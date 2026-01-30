# Serial Commands Documentation

One-sentence descriptions for all serial commands available in the firmware.

## Low-Level Commands (1-9)

- **1**: System reset - restarts the device.
- **2**: Reset EEPROM - clears all EEPROM data to defaults.
- **3**: Check cache - returns 1 if test data is cached, 0 otherwise.
- **4**: Check digital pin state - reads the state of a specified digital pin.
- **5**: Limit switch state - returns the current state of the stage limit switch.
- **6**: Set digital pin state - sets a digital pin to HIGH or LOW.
- **7**: Toggle pin repeatedly - toggles a digital pin state multiple times with specified delay.
- **8**: Check analog pin state - reads the analog value of a specified pin.
- **9**: Clear WiFi credentials - removes all stored WiFi network credentials.

## Serial Port Messaging (10-12)

- **10**: Enable serial messaging - turns on serial port message output.
- **11**: Disable serial messaging - turns off serial port message output.
- **12**: Display device ID - prints the device's unique identifier to serial.

## Stage Motion (20-29)

- **20**: Reset stage - moves stage to home position and sleeps motor.
- **21**: Wake motor - activates the stepper motor.
- **22**: Move stage microns - moves stage a specified distance in microns at given step delay.
- **23**: Move to position - moves stage to a specific absolute position.
- **24**: Move to test start position - resets stage and moves to test start position.
- **25**: Move to optical read position - resets stage and moves to spectrophotometer read position.
- **26**: Oscillate stage - oscillates stage back and forth for specified cycles.
- **27**: Move to shipping bolt location - moves stage to position for shipping bolt insertion.
- **28**: Move to position zero - moves stage to home position (0 microns) at specified step delay.
- **29**: Sleep motor - deactivates the stepper motor to save power.

## Laser Diodes (30-33)

- **30**: Turn on laser A - activates laser A for specified duration in milliseconds.
- **31**: Turn on laser B - activates laser B for specified duration in milliseconds.
- **32**: Turn on laser C - activates laser C for specified duration in milliseconds.
- **33**: Turn on all lasers - activates all three lasers simultaneously for specified duration.

## Buzzer (40-43)

- **40**: Turn on buzzer - activates buzzer at specified frequency and duration.
- **41**: Turn on alert buzzer - plays alert buzzer pattern (ready/cartridge insertion).
- **42**: Turn on problem buzzer - plays problem buzzer pattern (error state).
- **43**: Turn off buzzer - stops buzzer immediately.

## Heater (50-55)

- **50**: Read heater temperature - displays current heater temperature in degrees Celsius.
- **51**: Turn on heater - activates heater at specified power level.
- **52**: Turn off heater - deactivates heater immediately.
- **53**: Set heater target temperature - sets target temperature for PID control.
- **54**: Start temperature control - enables PID temperature control system.
- **55**: Stop temperature control - disables PID temperature control system.

## Barcode Scanner (60)

- **60**: Scan barcode - attempts to read barcode from cartridge and returns result code.

## Magnetometer (70-73)

- **70**: Validate magnets - performs magnetometer validation sequence.
- **71**: List magnet validation files - displays all magnet validation data files.
- **72**: Clear magnet validation files - deletes all magnet validation data files.
- **73**: Load latest magnet validation - loads the most recent magnet validation data.

## Stress Test (90-93)

- **90**: Reset stress test counter - resets cycle counter and disables WiFi for stress test.
- **91**: Disable WiFi for stress test - turns off WiFi to prepare for stress test.
- **92**: Start stress test - begins stress test sequence with specified cycle limit and LED power.
- **93**: Stop stress test - terminates stress test and returns device to IDLE state.

## Bluetooth LE (200)

- **200**: Scan BLE devices - scans for nearby Bluetooth Low Energy devices and returns count.

## Spectrophotometer (301-311)

- **301**: Set spectrophotometer params - configures sensor gain, step, and integration time.
- **303**: Power on spectrophotometer channel - activates specified spectrophotometer channel (1-3).
- **304**: Power off all spectrophotometers - deactivates all spectrophotometer channels.
- **305**: Baseline scan - performs baseline optical scan with specified channel.
- **306**: Test scan - performs test optical scan with specified channel.
- **307**: Set pulse params - configures LED pulse parameters for spectrophotometer readings.
- **308**: Take readings - captures specified number of sensor readings with all channels.
- **309**: Output raw readings - displays raw test data readings to serial output.
- **310**: Take readings without lasers - captures sensor readings without activating lasers.
- **311**: Baseline continuous scan - performs continuous baseline scan across stage movement.

## Cloud Functions (400-406)

- **400**: Check cache - returns 1 if cached test data exists, 0 otherwise.
- **401**: Clear cache - deletes all cached test data files.
- **402**: Load cached record - displays cached test data to serial output.
- **403**: Upload test - initiates upload of test results to cloud server.
- **404**: Check assay files - lists all assay files stored on device.
- **405**: Clear assay files - deletes all assay files from device storage.
- **406**: Output assay file - displays contents of specified assay file by number.

## Communication Commands (8000-8999)

### General (8000-8001)

- **8000**: Display help - shows command reference for 8000-series commands.
- **8001**: Show radio status - displays current status of WiFi, Cellular, and Bluetooth radios.

### WiFi Control (8100-8101)

- **8100**: WiFi OFF - disconnects and disables WiFi radio.
- **8101**: WiFi ON - enables WiFi radio.

### Cellular Control (8200-8201)

- **8200**: Cellular OFF - disconnects and disables cellular radio.
- **8201**: Cellular ON - enables cellular radio.

### Bluetooth Control (8300-8301)

- **8300**: Bluetooth OFF - disables Bluetooth radio.
- **8301**: Bluetooth ON - enables Bluetooth radio.

### Test Modes (8500-8503)

- **8500**: Test WiFi only - enables WiFi while disabling other radios for testing.
- **8501**: Test Cellular only - enables Cellular while disabling other radios for testing.
- **8502**: Test Bluetooth only - enables Bluetooth while disabling other radios for testing.
- **8503**: FCC compliance test - enables all radios simultaneously for FCC compliance testing.

### Master Control (8900-8901, 8999)

- **8900**: All radios OFF - disables WiFi, Cellular, and Bluetooth simultaneously.
- **8901**: All radios ON - enables WiFi, Cellular, and Bluetooth simultaneously.
- **8999**: Emission check - displays current radio emission status report.

## State Management Commands (9000-9032)

### State Information (9000-9003)

- **9000**: Display state help - shows state management command reference.
- **9001**: Show current device state - displays current mode, test state, and cartridge state.
- **9002**: Show detailed state information - displays comprehensive device state including hardware and cloud status.
- **9003**: Diagnose test transition issues - provides diagnostic information about why device isn't transitioning to test state.

### Transition History (9010-9013)

- **9010**: Show all state transitions - displays complete state transition history.
- **9011**: Show last 10 transitions - displays the 10 most recent state transitions.
- **9012**: Show last 20 transitions - displays the 20 most recent state transitions.
- **9013**: Clear transition history - resets the state transition history log.

### State Queries (9020-9021)

- **9020**: Get transition count - returns total number of state transitions logged.
- **9021**: Get compact history string - displays transition history in compact Particle Cloud format.

### Barcode Tracking (9030-9032)

- **9030**: Show barcode transition history - displays state transitions filtered by barcode UUID.
- **9031**: Show barcode history for current barcode - displays transition history for currently active barcode.
- **9032**: Clear barcode history - clears all barcode-specific transition history.
