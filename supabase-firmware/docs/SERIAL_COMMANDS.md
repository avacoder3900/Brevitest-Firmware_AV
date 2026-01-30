# Serial Commands Documentation

Command reference for the Brevitest Supabase firmware serial interface.

## Command Format

Commands are sent as: `<command_id> [param1] [param2] ...`

- Command IDs are numeric (0-9999)
- Parameters are space-separated integers
- Commands are terminated by newline (\n)
- Responses: `OK:<result>` for success, `ERR:<code>:<message>` for errors

## System Commands (1-9)

| ID | Command | Parameters | Description |
|----|---------|------------|-------------|
| 1 | System Reset | none | Restarts the device |
| 2 | EEPROM Reset | none | Clears EEPROM to defaults |
| 3 | Check Cache | none | Returns 1 if test data cached, 0 otherwise |
| 4 | Read Digital Pin | pin | Reads state of specified digital pin |
| 5 | Limit Switch State | none | Returns cartridge limit switch state |
| 6 | Set Digital Pin | pin, state | Sets pin to HIGH (1) or LOW (0) |
| 7 | Toggle Pin | pin, count, delay | Toggles pin count times with delay (ms) |
| 8 | Read Analog Pin | pin | Reads analog value of specified pin |
| 9 | Clear WiFi | none | Clears stored WiFi credentials |

## Serial Port Messaging (10-12)

| ID | Command | Parameters | Description |
|----|---------|------------|-------------|
| 10 | Enable Messaging | none | Turns on serial output messages |
| 11 | Disable Messaging | none | Turns off serial output messages |
| 12 | Display Device ID | none | Prints device unique identifier |

## Stage Motion (20-29)

| ID | Command | Parameters | Description |
|----|---------|------------|-------------|
| 20 | Reset Stage | none | Homes stage and sleeps motor |
| 21 | Wake Motor | none | Activates stepper motor |
| 22 | Move Microns | distance, [delay] | Moves stage by distance in microns |
| 23 | Move to Position | position | Moves to absolute position in microns |
| 24 | Move to Test Start | none | Moves to test start position (7860um) |
| 25 | Move to Optical Read | none | Moves to optical read position (21000um) |
| 26 | Oscillate Stage | cycles, [amplitude] | Oscillates stage for mixing |
| 27 | Move to Shipping | none | Moves to shipping bolt position |
| 28 | Move to Zero | [delay] | Homes stage to position 0 |
| 29 | Sleep Motor | none | Deactivates stepper motor |

## Laser Diodes (30-33)

| ID | Command | Parameters | Description |
|----|---------|------------|-------------|
| 30 | Laser A | [duration_ms] | Activates laser A (red) for duration |
| 31 | Laser B | [duration_ms] | Activates laser B (green) for duration |
| 32 | Laser C | [duration_ms] | Activates laser C (blue) for duration |
| 33 | All Lasers | [duration_ms] | Activates all lasers for duration |

**Note:** Lasers require cartridge to be inserted (safety interlock).

## Buzzer (40-43)

| ID | Command | Parameters | Description |
|----|---------|------------|-------------|
| 40 | Buzzer On | [freq], [duration] | Activates buzzer at frequency/duration |
| 41 | Alert Buzzer | none | Plays ready/cartridge alert pattern |
| 42 | Problem Buzzer | none | Plays error buzzer pattern |
| 43 | Buzzer Off | none | Stops buzzer immediately |

## Heater (50-55)

| ID | Command | Parameters | Description |
|----|---------|------------|-------------|
| 50 | Read Temperature | none | Returns current temperature (10x format) |
| 51 | Heater On | [power] | Activates heater at power level (0-255) |
| 52 | Heater Off | none | Deactivates heater |
| 53 | Set Target Temp | temp | Sets PID target temperature (10x format) |
| 54 | Start Temp Control | none | Enables PID temperature control |
| 55 | Stop Temp Control | none | Disables PID temperature control |

**Note:** Temperature in 10x Celsius format (375 = 37.5°C).

## Barcode Scanner (60)

| ID | Command | Parameters | Description |
|----|---------|------------|-------------|
| 60 | Scan Barcode | none | Reads cartridge barcode, returns UUID |

## Spectrophotometer (301-311)

| ID | Command | Parameters | Description |
|----|---------|------------|-------------|
| 301 | Set Params | gain, time | Sets sensor gain (0-10) and integration time |
| 303 | Channel On | channel | Powers on spectro channel (1-3) |
| 304 | Channel Off | none | Powers off all spectro channels |
| 305 | Baseline Scan | channel | Performs baseline optical scan |
| 306 | Test Scan | channel | Performs test optical scan |
| 307 | Set Pulse Params | on_us, cycle_us | Sets LED pulse timing |
| 308 | Take Readings | count | Captures specified number of readings |
| 309 | Output Readings | none | Outputs raw test data to serial |
| 310 | Readings No Laser | count | Takes readings without activating lasers |
| 311 | Baseline Continuous | none | Continuous baseline across stage movement |

## Cloud Functions (400-406)

| ID | Command | Parameters | Description |
|----|---------|------------|-------------|
| 400 | Check Cache | none | Shows test and cloud cache counts |
| 401 | Clear Cache | none | Clears all cached test data |
| 402 | Load Cached | none | Displays next cached test filename |
| 403 | Upload Test | none | Uploads cached test to cloud |
| 404 | Check Assays | none | Lists cached assay files |
| 405 | Clear Assays | none | Deletes all cached assay files |
| 406 | Output Assay | [index] | Displays assay storage info |

## State Management (9000-9020)

| ID | Command | Parameters | Description |
|----|---------|------------|-------------|
| 9000 | State Help | none | Shows state command reference |
| 9001 | Current State | none | Shows mode, test state, cartridge state |
| 9002 | Detailed State | none | Returns full state as JSON |
| 9003 | Diagnose | none | Diagnoses transition issues |
| 9010 | Show Transitions | none | Displays transition history |
| 9011 | Clear History | none | Clears transition history buffer |
| 9020 | Force State | state | Forces transition to specified state |

### Device States (for command 9020)

| Value | State | Description |
|-------|-------|-------------|
| 0 | INITIALIZING | Device starting up |
| 1 | IDLE | Ready, waiting for cartridge |
| 2 | HEATING | Warming to operating temperature |
| 3 | BARCODE_SCANNING | Reading cartridge barcode |
| 4 | VALIDATING_CARTRIDGE | Validating with cloud |
| 5 | VALIDATING_MAGNETOMETER | Checking magnets |
| 6 | RUNNING_TEST | Test in progress |
| 7 | UPLOADING_RESULTS | Sending results to cloud |
| 8 | RESETTING_CARTRIDGE | Resetting cartridge state |
| 9 | STRESS_TESTING | Stress test mode |
| 10 | ERROR_STATE | Error condition |

## Error Codes

| Code | Meaning |
|------|---------|
| -1 | Hardware error or invalid parameter |
| -2 | Operation failed |
| -3 | Unknown command |
| -4 | Invalid state transition |
| -5 | Cloud operation failed |

## Examples

```
# Reset the device
1
OK:1

# Read temperature
50
Temperature: 375
OK:375

# Move stage to test start position
24
Moved to TEST_START
OK:7860

# Enable laser A for 100ms
30 100
Laser A on for 100 ms
OK:100

# Show current device state
9001
Mode: IDLE
Test: NOT_STARTED
Cartridge: NOT_INSERTED
OK:1

# Force transition to HEATING state
9020 2
State forced to HEATING
OK:2
```
