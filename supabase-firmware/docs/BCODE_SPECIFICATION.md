# BCODE Specification

## Overview

BCODE (Brevitest CODE) is a custom instruction set used to control test execution on Brevitest diagnostic devices. It defines the sequence of hardware operations required for each assay type, including stage movement, spectrophotometer readings, timing, and control flow.

## Format

BCODE is stored as a character string with the following structure:

```
<opcode>:<param1>,<param2>,...|<opcode>:<param1>,...|...
```

### Delimiters

| Character | Name | Usage |
|-----------|------|-------|
| `\|` | ITEM_DELIM | Separates instructions |
| `:` | ATTR_DELIM | Separates opcode from parameters |
| `,` | ARG_DELIM | Separates parameters |
| `#` | END_DELIM | Optional end-of-BCODE marker |

### Example

```
0:|1:1000|2:5000,300|11:5|20:3|2:1000,300|21:|14:10|99:
```

This BCODE:
1. Starts the test
2. Delays 1000ms
3. Moves stage 5000 microns at 300µs step delay
4. Takes 5 baseline scans
5. Repeats 3 times: move 1000 microns
6. Takes 10 test scans
7. Ends the test

## Opcodes

### Test Lifecycle

| Opcode | Name | Parameters | Description |
|--------|------|------------|-------------|
| 0 | START_TEST | none | Initialize test - must be first instruction |
| 99 | END_TEST | none | Signal test completion |

### Timing

| Opcode | Name | Parameters | Description |
|--------|------|------------|-------------|
| 1 | DELAY | milliseconds | Delay for specified time |

**DELAY Behavior:**
- Executes in 1000ms chunks to allow heater control
- Calls delay loop callback between chunks
- Cancels if callback returns false (cartridge removed)

### Stage Control

| Opcode | Name | Parameters | Description |
|--------|------|------------|-------------|
| 2 | MOVE_MICRONS | microns, step_delay_us | Move stage relative to current position |
| 3 | OSCILLATE | microns, step_delay_us, cycles | Oscillate stage back and forth |

**MOVE_MICRONS Parameters:**
- `microns`: Distance to move (positive = forward, negative = reverse)
- `step_delay_us`: Delay between steps in microseconds (250-600 typical)

**OSCILLATE Parameters:**
- `microns`: Amplitude of oscillation
- `step_delay_us`: Step timing
- `cycles`: Number of complete oscillations

### Spectrophotometer Configuration

| Opcode | Name | Parameters | Description |
|--------|------|------------|-------------|
| 10 | SET_SENSOR_PARAMS | gain, astep, atime | Configure AS7341 sensor parameters |

**Parameters:**
- `gain`: AGAIN value (0-10, controls sensitivity)
- `astep`: ASTEP value (integration time factor)
- `atime`: ATIME value (integration time multiplier)

**Typical Values:**
- Default: gain=7, astep=999, atime=49 (~100ms integration)

### Spectrophotometer Readings

| Opcode | Name | Parameters | Description |
|--------|------|------------|-------------|
| 11 | BASELINE_SCANS | num_scans | Take baseline reference scans |
| 14 | TEST_SCANS | num_scans | Take test measurement scans |
| 15 | SENSOR_READING | channel, gain, astep, atime | Take single configured reading |
| 16 | CONTINUOUS_SCANS | is_baseline, start_pos, distance, step_delay | Scan while moving |

**BASELINE_SCANS / TEST_SCANS:**
- Each scan takes 3 readings (channels A, B, C)
- Readings stored in test record array
- Position and temperature recorded per reading

**SENSOR_READING Channel Values:**
| Value | Channel |
|-------|---------|
| 0 | All channels (A, B, C) |
| 1 | Channel A only |
| 2 | Channel B only |
| 3 | Channel C only |

**CONTINUOUS_SCANS:**
- Takes readings while stage moves continuously
- `is_baseline`: 1 for baseline, 0 for test
- `start_pos`: Starting stage position
- `distance`: Distance to scan
- `step_delay`: Step timing during scan

### Control Flow

| Opcode | Name | Parameters | Description |
|--------|------|------------|-------------|
| 20 | REPEAT_BEGIN | iterations | Start repeat block |
| 21 | REPEAT_END | none | End repeat block |

**Repeat Block Rules:**
- Maximum nesting depth: 4 levels
- REPEAT_END decrements iteration counter
- Block re-executes from REPEAT_BEGIN until counter reaches 0
- All instructions between BEGIN and END are repeated

## Execution Model

### State Machine

```
IDLE -> READY (load BCODE)
READY -> RUNNING (start execution)
RUNNING -> COMPLETED (opcode 99 or end of BCODE)
RUNNING -> CANCELLED (cartridge removal or manual stop)
RUNNING -> ERROR (invalid opcode or execution failure)
```

### Delay Loop

During DELAY instructions and other blocking operations, the interpreter:
1. Calls the delay loop callback every ~1000ms
2. Callback performs heater control (PID loop)
3. Callback checks cartridge presence
4. Returns false to cancel execution

### Error Handling

| Error Code | Condition |
|------------|-----------|
| ERR_BCODE_INVALID | Invalid opcode or missing parameters |
| ERR_BCODE_TIMEOUT | Operation timed out |
| ERR_TEST_CANCELLED | Cartridge removed or manual cancel |
| ERR_TEST_FAILED | Hardware operation failed |

## Hardware Integration

### Motor Control
- Stage movement via callback to MotorController
- Step delays enforced by BCODE (250-600µs typical)
- Position tracking maintained by motor module

### Temperature Control
- Heater PID runs during all delays
- Temperature recorded per spectrophotometer reading
- Test aborts if temperature out of range

### Spectrophotometer
- AS7341 10-channel sensor
- Readings stored in 32-byte structures
- Maximum 300 readings per test

## Example BCODEs

### Basic Test
```
0:|1:2000|11:5|1:60000|14:20|99:
```
1. Start test
2. Wait 2 seconds
3. 5 baseline scans
4. Wait 60 seconds (incubation)
5. 20 test scans
6. End

### Scanning Test
```
0:|2:7860,300|10:7,999,49|11:10|1:120000|14:50|2:-7860,300|99:
```
1. Start test
2. Move to test position (7860 microns)
3. Configure sensor (default params)
4. 10 baseline scans
5. 2-minute incubation
6. 50 test scans
7. Return home
8. End

### Stress Test Pattern
```
0:|20:100|2:5000,300|3:1000,350,10|2:-5000,300|21:|99:
```
1. Start
2. Repeat 100 times:
   - Move forward 5000µm
   - Oscillate 1000µm for 10 cycles
   - Return
3. End

## Version History

| Version | Changes |
|---------|---------|
| 1.0 | Initial specification (legacy firmware) |
| 2.0 | Supabase firmware port - identical opcodes |

## References

- Legacy firmware: `legacy-firmware/firmware/src/brevitest-firmware.ino`
- Test engine: `supabase-firmware/src/BCODEInterpreter.h`
- AS7341 datasheet: https://wiki.dfrobot.com/SKU_SEN0364_AS7341
