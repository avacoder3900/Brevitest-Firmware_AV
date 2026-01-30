# BCODE Instruction Set Reference

**Version:** 1.0
**Date:** 2026-01-30
**Firmware:** v57.0.0-supabase

## Overview

BCODE is the custom instruction set that controls Brevitest test execution. It provides commands for stage movement, spectrophotometer readings, timing, and control flow.

### Format

BCODE instructions are text-based with the following delimiters:
- `|` (pipe) - Separates instructions
- `:` (colon) - Separates opcode from parameters
- `,` (comma) - Separates multiple parameters

**General Format:** `opcode:param1,param2,...|`

**Example:** `0:|1:1000|2:5000,300|99:`

---

## Instruction Reference

### Test Lifecycle

#### 0 - START_TEST
Initializes test execution. Must be the first instruction.

| Parameter | Description |
|-----------|-------------|
| (none) | No parameters required |

```
Format: 0:
Example: 0:|
```

**Actions:**
- Resets test record
- Initializes reading counters
- Records start timestamp
- Sets test state to RUNNING

---

#### 99 - END_TEST
Signals test completion. Must be the last instruction.

| Parameter | Description |
|-----------|-------------|
| (none) | No parameters required |

```
Format: 99:
Example: 99:|
```

**Actions:**
- Finalizes test record
- Calculates checksum
- Sets test state to COMPLETED
- Triggers result upload

---

### Timing

#### 1 - DELAY
Pauses execution for specified duration.

| Parameter | Description |
|-----------|-------------|
| milliseconds | Delay duration in milliseconds |

```
Format: 1:milliseconds
Example: 1:5000 (delay 5 seconds)
```

**Timing Behavior:**
- Heater control continues during delay
- Cartridge removal is detected (cancels test)
- Uses non-blocking delay with periodic callbacks

---

### Stage Control

#### 2 - MOVE_MICRONS
Moves the stage by a relative distance.

| Parameter | Description |
|-----------|-------------|
| microns | Distance in microns (negative = backward) |
| step_delay_us | Delay between motor steps (microseconds) |

```
Format: 2:microns,step_delay_us
Example: 2:7860,300 (move 7860 microns at 300us/step)
Example: 2:-7860,300 (move backward 7860 microns)
```

**Timing:**
- Step delay 300us = fast movement
- Step delay 600us = slow/precise movement
- Minimum step delay: 250us

---

#### 3 - OSCILLATE
Oscillates the stage back and forth.

| Parameter | Description |
|-----------|-------------|
| microns | Amplitude in microns |
| step_delay_us | Delay between steps |
| cycles | Number of complete oscillations |

```
Format: 3:microns,step_delay_us,cycles
Example: 3:1000,300,5 (oscillate 1000 microns, 5 cycles)
```

**Timing:**
- Each cycle = forward + backward movement
- Total time = 2 * cycles * (microns / step_size) * step_delay_us
- Cartridge removal detection between cycles

---

### Spectrophotometer

#### 10 - SET_SENSOR_PARAMS
Configures the AS7341 spectrophotometer.

| Parameter | Description | Default |
|-----------|-------------|---------|
| gain | AGAIN value (0-10) | 7 |
| astep | ASTEP value (0-65535) | 999 |
| atime | ATIME value (0-255) | 49 |

```
Format: 10:gain,astep,atime
Example: 10:7,999,49 (default settings)
```

**Gain Values:**
| Value | Gain |
|-------|------|
| 0 | 0.5x |
| 1 | 1x |
| 2 | 2x |
| 3 | 4x |
| 4 | 8x |
| 5 | 16x |
| 6 | 32x |
| 7 | 64x |
| 8 | 128x |
| 9 | 256x |
| 10 | 512x |

---

#### 11 - BASELINE_SCANS
Takes baseline spectrophotometer readings.

| Parameter | Description |
|-----------|-------------|
| num_scans | Number of baseline scans to take |

```
Format: 11:num_scans
Example: 11:5 (take 5 baseline scans)
```

**Actions:**
- Reads all 3 channels (A, B, C) per scan
- Total readings = num_scans * 3
- Enables/disables laser per channel
- Records readings to test record

---

#### 14 - TEST_SCANS
Takes test spectrophotometer readings.

| Parameter | Description |
|-----------|-------------|
| num_scans | Number of test scans to take |

```
Format: 14:num_scans
Example: 14:10 (take 10 test scans)
```

**Actions:**
- Same as baseline but flagged as test readings
- Used after sample processing

---

#### 15 - SENSOR_READING
Takes a single reading with specific parameters.

| Parameter | Description |
|-----------|-------------|
| channel | Channel letter ('A', 'B', 'C', or 0 for all) |
| gain | AGAIN value |
| astep | ASTEP value |
| atime | ATIME value |

```
Format: 15:channel,gain,astep,atime
Example: 15:A,7,999,49 (read channel A with settings)
Example: 15:0,7,999,49 (read all channels)
```

---

#### 16 - CONTINUOUS_SCANS
Takes continuous readings while moving the stage.

| Parameter | Description |
|-----------|-------------|
| is_baseline | 1 for baseline, 0 for test |
| start_position | Starting position in microns |
| distance | Scan distance in microns |
| step_delay_us | Step delay for movement |

```
Format: 16:is_baseline,start_position,distance,step_delay_us
Example: 16:1,0,5000,600 (baseline scan over 5000 microns)
```

---

### Control Flow

#### 20 - REPEAT_BEGIN
Starts a repeat block.

| Parameter | Description |
|-----------|-------------|
| iterations | Number of times to repeat |

```
Format: 20:iterations
Example: 20:3 (repeat following block 3 times)
```

**Nesting:**
- Maximum 4 levels of nesting supported
- Each level tracked on repeat stack

---

#### 21 - REPEAT_END
Ends a repeat block.

| Parameter | Description |
|-----------|-------------|
| (none) | No parameters required |

```
Format: 21:
Example: 21:|
```

**Behavior:**
- Decrements iteration counter
- If iterations remaining, jumps back to REPEAT_BEGIN
- If complete, continues to next instruction

---

## Complete BCODE Examples

### Simple Test
```
0:|10:7,999,49|2:7860,300|11:5|1:5000|14:10|2:-7860,300|99:
```
Breakdown:
1. `0:` - Start test
2. `10:7,999,49` - Set spectro params
3. `2:7860,300` - Move to test position
4. `11:5` - Take 5 baseline scans
5. `1:5000` - Wait 5 seconds
6. `14:10` - Take 10 test scans
7. `2:-7860,300` - Return to home
8. `99:` - End test

### Test with Oscillation
```
0:|10:7,999,49|2:7860,300|11:5|3:500,300,10|1:10000|14:10|2:-7860,300|99:
```
Breakdown:
1. `0:` - Start test
2. `10:7,999,49` - Set spectro params
3. `2:7860,300` - Move to test position
4. `11:5` - Take 5 baseline scans
5. `3:500,300,10` - Oscillate (mixing) for 10 cycles
6. `1:10000` - Wait 10 seconds (incubation)
7. `14:10` - Take 10 test scans
8. `2:-7860,300` - Return to home
9. `99:` - End test

### Test with Repeat Loop
```
0:|10:7,999,49|2:7860,300|11:5|20:3|1:5000|14:5|21:|2:-7860,300|99:
```
Breakdown:
1. `0:` - Start test
2. `10:7,999,49` - Set spectro params
3. `2:7860,300` - Move to test position
4. `11:5` - Take 5 baseline scans
5. `20:3` - Start repeat (3 times)
6. `1:5000` - Wait 5 seconds
7. `14:5` - Take 5 test scans
8. `21:` - End repeat (jumps back to 20: if iterations remain)
9. `2:-7860,300` - Return to home
10. `99:` - End test

### Nested Repeat Example
```
0:|20:2|20:3|1:1000|21:|21:|99:
```
Breakdown:
- Outer loop runs 2 times
- Inner loop runs 3 times per outer iteration
- Total delays: 2 * 3 = 6 delays of 1 second each

---

## Timing Reference

### Movement Timing
| Step Delay | Speed | Use Case |
|------------|-------|----------|
| 250us | Maximum | Not recommended |
| 290us | Fast | Normal movement |
| 300us | Fast | Standard fast |
| 600us | Slow | Precise positioning |

### Integration Time
Integration time = (ATIME + 1) * (ASTEP + 1) * 2.78us

| ATIME | ASTEP | Integration Time |
|-------|-------|------------------|
| 49 | 999 | ~139ms |
| 29 | 599 | ~50ms |
| 9 | 199 | ~5.5ms |

---

## Error Handling

### Cancellation
- Test is cancelled if cartridge is removed
- Delay loops check cartridge presence
- Movement commands check cartridge presence between steps

### Error Codes
| Code | Description |
|------|-------------|
| ERR_BCODE_INVALID | Invalid opcode or parameters |
| ERR_BCODE_TIMEOUT | Instruction timeout exceeded |
| ERR_TEST_CANCELLED | Test cancelled (cartridge removed) |

### Recovery
- Interrupted tests are saved to EEPROM
- On restart, interrupted test is cached for upload
- Recovery data cleared after caching

---

## Implementation Notes

### Callbacks
The BCODE interpreter uses callbacks for hardware operations:
- `DelayLoopCallback` - Called during delays for heater control
- `StageMoveCallback` - Handles stage movement
- `StageOscillateCallback` - Handles oscillation
- `SpectroConfigCallback` - Configures spectrophotometer
- `SpectroScanCallback` - Takes readings

### Memory
- BCODE buffer: 5000 bytes maximum
- Repeat stack: 4 levels maximum
- Readings storage: 300 readings maximum (9600 bytes)

### State Machine
During test execution:
- Device mode: `RUNNING_TEST`
- Test state: `RUNNING`
- Transitions to `COMPLETED` on `99:` instruction
- Transitions to `CANCELLED` on cartridge removal
