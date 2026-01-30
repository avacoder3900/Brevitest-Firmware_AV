# Brevitest Serial Command Reference

**Version:** 1.0
**Date:** 2026-01-30
**Firmware:** v57.0.0-supabase

## Overview

The Brevitest firmware accepts serial commands at **115200 baud**. Commands are integer-based with optional comma-separated parameters.

**Format:** `command_number` or `command_number,param1,param2,...`

**Response Format:** Commands output results via `Serial.printf()` and `Log.info()`.

---

## Command Categories

| Category | Range | Description |
|----------|-------|-------------|
| System | 1-9 | Reset, EEPROM, pin control |
| Serial Port | 10-12 | Messaging control, device ID |
| Stage Motion | 20-29 | Motor and stage control |
| Laser | 30-33 | Laser diode control |
| Buzzer | 40-43 | Audio feedback |
| Heater | 50-55 | Temperature control |
| Barcode | 60 | Barcode scanning |
| Magnetometer | 70-73 | Validation file management |
| Stress Test | 90-93 | Automated stress testing |
| Spectrophotometer | 301-311 | Optical sensor control |
| Cloud/Cache | 400-406 | Cloud operations and caching |
| Radio Control | 8000-8999 | WiFi, Cellular, Bluetooth |
| State Management | 9000-9032 | Device state control |

---

## System Commands (1-9)

### 1 - System Reset
Resets the device to IDLE state and homes the stage.
```
Input:  1
Output: Device reset to IDLE
```

### 2 - EEPROM Reset
Resets EEPROM to factory defaults (preserves lifetime counters).
```
Input:  2
Output: EEPROM reset complete
```

### 3 - Check Cache
Checks if there are cached test results pending upload.
```
Input:  3
Output: Cache: X files pending
```

### 4 - Read Digital Pin
Reads the state of a digital pin.
```
Input:  4,pin_number
Output: Pin X: HIGH/LOW
Example: 4,D2 -> Pin D2: HIGH
```

### 5 - Limit Switch State
Reads the current state of limit switches.
```
Input:  5
Output: Limit switches - Home: X, Far: Y
```

### 6 - Set Digital Pin
Sets a digital pin to HIGH or LOW.
```
Input:  6,pin_number,state (0=LOW, 1=HIGH)
Output: Pin X set to HIGH/LOW
Example: 6,D2,1 -> Pin D2 set to HIGH
```

### 7 - Toggle Pin
Toggles a digital pin state.
```
Input:  7,pin_number
Output: Pin X toggled
```

### 8 - Read Analog Pin
Reads the value of an analog pin.
```
Input:  8,pin_number
Output: Analog X: value (0-4095)
Example: 8,A0 -> Analog A0: 2048
```

### 9 - Clear WiFi Credentials
Clears stored WiFi credentials.
```
Input:  9
Output: WiFi credentials cleared
```

---

## Serial Port Commands (10-12)

### 10 - Enable Messaging
Enables verbose serial output.
```
Input:  10
Output: Messaging enabled
```

### 11 - Disable Messaging
Disables verbose serial output.
```
Input:  11
Output: Messaging disabled
```

### 12 - Display Device ID
Shows the Particle device ID.
```
Input:  12
Output: Device ID: e00fce68xxxxyyyyzzzz
```

---

## Stage Motion Commands (20-29)

### 20 - Reset Stage (Home)
Homes the stage to the zero position.
```
Input:  20
Output: Stage homed
```

### 21 - Wake Motor
Wakes the motor driver from sleep mode.
```
Input:  21
Output: Motor awake
```

### 22 - Move Microns
Moves the stage by specified microns.
```
Input:  22,microns,step_delay_us
Output: Moved X microns
Example: 22,5000,300 -> Move 5000 microns at 300us step delay
```

### 23 - Move To Position
Moves the stage to an absolute position.
```
Input:  23,position_microns,step_delay_us
Output: Moved to position X
Example: 23,7860,300 -> Move to 7860 microns
```

### 24 - Move To Test Start
Moves the stage to the test start position (7860 microns).
```
Input:  24
Output: Stage at test start position
```

### 25 - Move To Optical Read
Moves to the optical reading position.
```
Input:  25
Output: Stage at optical read position
```

### 26 - Oscillate Stage
Oscillates the stage back and forth.
```
Input:  26,amplitude_microns,step_delay_us,cycles
Output: Oscillation complete
Example: 26,1000,300,5 -> Oscillate 1000 microns, 5 cycles
```

### 27 - Move To Shipping Bolt
Moves to position for shipping bolt installation.
```
Input:  27
Output: Stage at shipping bolt position
```

### 28 - Move To Zero
Moves stage to position zero.
```
Input:  28
Output: Stage at zero
```

### 29 - Sleep Motor
Puts the motor driver to sleep.
```
Input:  29
Output: Motor sleeping
```

---

## Laser Commands (30-33)

### 30 - Laser A Control
Controls laser channel A.
```
Input:  30,state (0=off, 1=on)
Output: Laser A ON/OFF
```

### 31 - Laser B Control
Controls laser channel B.
```
Input:  31,state (0=off, 1=on)
Output: Laser B ON/OFF
```

### 32 - Laser C Control
Controls laser channel C.
```
Input:  32,state (0=off, 1=on)
Output: Laser C ON/OFF
```

### 33 - All Lasers Control
Controls all laser channels.
```
Input:  33,state (0=off, 1=on)
Output: All lasers ON/OFF
```

---

## Buzzer Commands (40-43)

### 40 - Buzzer On
Plays a tone at specified frequency and duration.
```
Input:  40,frequency_hz,duration_ms
Output: Buzzer playing
Example: 40,600,1000 -> Play 600Hz for 1 second
```

### 41 - Alert Buzzer
Plays the alert buzzer pattern.
```
Input:  41
Output: Alert buzzer activated
```

### 42 - Problem Buzzer
Plays the problem/error buzzer pattern.
```
Input:  42
Output: Problem buzzer activated
```

### 43 - Buzzer Off
Stops any playing buzzer.
```
Input:  43
Output: Buzzer stopped
```

---

## Heater Commands (50-55)

### 50 - Read Temperature
Reads the current temperature.
```
Input:  50
Output: Temperature: XX.X C
```

### 51 - Heater On
Turns the heater on at full power.
```
Input:  51
Output: Heater ON
```

### 52 - Heater Off
Turns the heater off.
```
Input:  52
Output: Heater OFF
```

### 53 - Set Target Temperature
Sets the target temperature (in 10x Celsius).
```
Input:  53,temp_10x
Output: Target temperature: XX.X C
Example: 53,450 -> Target 45.0 C
```

### 54 - Start Temperature Control
Enables PID temperature control.
```
Input:  54
Output: Temperature control started
```

### 55 - Stop Temperature Control
Disables PID temperature control.
```
Input:  55
Output: Temperature control stopped
```

---

## Barcode Commands (60)

### 60 - Scan Barcode
Triggers a barcode scan.
```
Input:  60
Output: Barcode: <uuid> or Scan failed
```

---

## Magnetometer Commands (70-73)

### 70 - Start Magnetometer Validation
Starts magnetometer validation mode.
```
Input:  70
Output: Magnetometer validation started
```

### 71 - List Validation Files
Lists all magnetometer validation files.
```
Input:  71
Output: Validation file directory:
        magnet_1706620800.txt
        magnet_1706620900.txt
        Total validation files: 2
```

### 72 - Load Latest Validation
Loads the most recent validation file.
```
Input:  72
Output: Loaded validation data (XXX bytes)
```

### 73 - Clear Validation Files
Deletes all validation files.
```
Input:  73
Output: Cleared X validation files
```

---

## Stress Test Commands (90-93)

### 90 - Start Stress Test
Starts automated stress testing.
```
Input:  90,num_cycles
Output: Stress test started: X cycles
Example: 90,100 -> Run 100 stress test cycles
```

### 91 - Stop Stress Test
Stops the current stress test.
```
Input:  91
Output: Stress test stopped
```

### 92 - Get Stress Test Status
Shows stress test progress.
```
Input:  92
Output: Stress test: X/Y cycles, Z readings
```

### 93 - Reset Stress Test Counters
Resets stress test counters (since reset only).
```
Input:  93
Output: Stress test counters reset
```

---

## Spectrophotometer Commands (301-311)

### 301 - Set Sensor Parameters
Sets AS7341 sensor parameters.
```
Input:  301,gain,astep,atime
Output: Spectro params: gain=X, astep=Y, atime=Z
Example: 301,7,999,49 -> AGAIN=7, ASTEP=999, ATIME=49
```

### 303 - Enable Channel
Enables a specific spectrophotometer channel with laser.
```
Input:  303,channel ('A','B','C')
Output: Channel X enabled
```

### 304 - Disable Channel
Disables a specific channel.
```
Input:  304,channel
Output: Channel X disabled
```

### 305 - Baseline Scan
Takes baseline spectrophotometer readings.
```
Input:  305,num_scans
Output: Baseline complete: X readings
```

### 306 - Test Scan
Takes test spectrophotometer readings.
```
Input:  306,num_scans
Output: Test scan complete: X readings
```

### 307 - Set Pulse Parameters
Sets laser pulse parameters.
```
Input:  307,pulse_on_us,pulse_off_us
Output: Pulse params set
```

### 308 - Take Readings
Takes spectrophotometer readings with laser enabled.
```
Input:  308,channel,num_readings
Output: F1=X F2=X F3=X F4=X F5=X F6=X F7=X F8=X CLR=X NIR=X
```

### 309 - Output Readings
Outputs all stored readings.
```
Input:  309
Output: [readings in CSV format]
```

### 310 - Take Readings No Laser
Takes readings without laser enabled.
```
Input:  310,num_readings
Output: [readings data]
```

### 311 - Baseline Continuous
Takes continuous baseline readings while moving.
```
Input:  311,start_position,distance,step_delay
Output: Continuous scan complete
```

---

## Cloud/Cache Commands (400-406)

### 400 - Check Cache
Checks for cached test results.
```
Input:  400
Output: Cached tests: X files
```

### 401 - Clear Cache
Deletes all cached test results.
```
Input:  401
Output: Cache cleared
```

### 402 - Load Cached Test
Loads a specific cached test file.
```
Input:  402,filename
Output: Loaded test: <cartridge_id>
```

### 403 - Upload Test
Uploads the current test record.
```
Input:  403
Output: Upload started/completed/failed
```

### 404 - Check Assays
Lists cached assay files.
```
Input:  404
Output: Assay file directory:
        ASSAY001
        ASSAY002
```

### 405 - Clear Assays
Deletes all cached assay files.
```
Input:  405
Output: Assay cache cleared
```

### 406 - Output Assay
Outputs a specific assay's BCODE.
```
Input:  406,assay_id
Output: Assay: <id>, Duration: Xms, BCODE: ...
```

---

## Radio Control Commands (8000-8999)

### 8000 - Radio Help
Shows radio control command help.
```
Input:  8000
Output: [Radio command list]
```

### 8001 - Radio Status
Shows current radio states.
```
Input:  8001
Output: WiFi: ON/OFF, Cellular: ON/OFF, BLE: ON/OFF
```

### 8100 - WiFi Off
Turns WiFi off.
```
Input:  8100
Output: WiFi disabled
```

### 8101 - WiFi On
Turns WiFi on.
```
Input:  8101
Output: WiFi enabled
```

### 8200 - Cellular Off
Turns Cellular off.
```
Input:  8200
Output: Cellular disabled
```

### 8201 - Cellular On
Turns Cellular on.
```
Input:  8201
Output: Cellular enabled
```

### 8300 - Bluetooth Off
Turns Bluetooth off.
```
Input:  8300
Output: Bluetooth disabled
```

### 8301 - Bluetooth On
Turns Bluetooth on.
```
Input:  8301
Output: Bluetooth enabled
```

### 8500 - WiFi Only Mode
Enables only WiFi.
```
Input:  8500
Output: WiFi-only mode
```

### 8501 - Cellular Only Mode
Enables only Cellular.
```
Input:  8501
Output: Cellular-only mode
```

### 8502 - Bluetooth Only Mode
Enables only Bluetooth.
```
Input:  8502
Output: Bluetooth-only mode
```

### 8503 - FCC Test Mode
Special mode for FCC testing.
```
Input:  8503
Output: FCC test mode enabled
```

### 8900 - All Radios Off
Turns all radios off.
```
Input:  8900
Output: All radios disabled
```

### 8901 - All Radios On
Turns all radios on.
```
Input:  8901
Output: All radios enabled
```

### 8999 - Emission Check
Outputs radio emission status report.
```
Input:  8999
Output: [Detailed radio status report]
```

---

## State Management Commands (9000-9032)

### 9000 - State Help
Shows state management command help.
```
Input:  9000
Output: [State command list]
```

### 9001 - Show Current State
Shows the current device mode.
```
Input:  9001
Output: Current state: IDLE
```

### 9002 - Show Detailed State
Shows detailed state information.
```
Input:  9002
Output: Mode: IDLE
        Test state: NOT_STARTED
        Cartridge: NOT_INSERTED
        Cloud pending: false
        Temperature: XX.X C
```

### 9003 - Diagnose Transition
Checks if a transition to specified state is valid.
```
Input:  9003,target_mode
Output: Transition to X: VALID/INVALID
Example: 9003,6 -> Transition to RUNNING_TEST: INVALID (no cartridge)
```

### 9010 - Show Transitions
Shows recent state transition history.
```
Input:  9010
Output: [Last 10 state transitions with timestamps]
```

### 9011 - Clear History
Clears state transition history.
```
Input:  9011
Output: History cleared
```

### 9020 - Force State
Forces a state transition (bypasses validation).
```
Input:  9020,mode_number
Output: Forced to state: X
Example: 9020,0 -> Forced to IDLE
```

---

## Device Mode Reference

| Number | Mode | Description |
|--------|------|-------------|
| 0 | IDLE | Ready for cartridge |
| 1 | INITIALIZING | Device startup |
| 2 | HEATING | Waiting for heater |
| 3 | BARCODE_SCANNING | Reading barcode |
| 4 | VALIDATING_CARTRIDGE | Cloud validation |
| 5 | VALIDATING_MAGNETOMETER | BLE validation |
| 6 | RUNNING_TEST | Test execution |
| 7 | UPLOADING_RESULTS | Cloud upload |
| 8 | RESETTING_CARTRIDGE | Cloud reset |
| 9 | STRESS_TESTING | Stress test mode |
| 10 | ERROR_STATE | Error condition |

---

## Examples

### Basic Status Check
```
10        # Enable messaging
12        # Get device ID
50        # Read temperature
9002      # Show detailed state
```

### Manual Test Run
```
20        # Home stage
51        # Heater on
54        # Start temp control
24        # Move to test start
301,7,999,49  # Set spectro params
305,5     # Take 5 baseline scans
306,10    # Take 10 test scans
309       # Output readings
```

### Stress Test
```
90,100    # Start 100 cycle stress test
92        # Check progress
91        # Stop if needed
93        # Reset counters
```
