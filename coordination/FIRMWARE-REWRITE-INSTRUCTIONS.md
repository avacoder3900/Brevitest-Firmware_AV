# Brevitest Firmware Rewrite - Terminal Instructions

## Overview

This project rewrites the Brevitest firmware to use Supabase as the cloud backend while maintaining **exact behavioral compatibility** with the legacy Particle-based firmware.

**CRITICAL REQUIREMENT**: The new firmware must behave identically to the legacy firmware. Every state transition, timing delay, and hardware interaction must match the legacy behavior.

---

## Terminal Assignments

| Terminal | PRD File | Focus Area | Key Files |
|----------|----------|------------|-----------|
| **Alpha** | `prd-fw-alpha-statemachine.json` | State Machine & Core Loop | StateMachine.*, main.ino |
| **Beta** | `prd-fw-beta-hardware.json` | Hardware Controllers | Motor, Heater, Laser, Spectro, Barcode, Buzzer, LED |
| **Gamma** | `prd-fw-gamma-cloud.json` | Cloud & Storage | SupabaseClient.*, StorageManager.* |
| **Delta** | `prd-fw-delta-serial.json` | Serial Commands & BCODE | SerialCommands.*, BCODEInterpreter.*, TestRunner.* |

---

## Getting Started (For Each Terminal)

### 1. Register Your Session

```bash
# Read current state
cat coordination/firmware-rewrite-session-state.json

# Update activeSessions with your terminal name
# Set status: "starting", currentTask: null
```

### 2. Read Your PRD

```bash
# Example for Alpha terminal:
cat coordination/prd-fw-alpha-statemachine.json
```

### 3. Read the Legacy Code

**MANDATORY**: Before implementing ANY user story, you MUST read the corresponding legacy code and document your understanding.

Legacy firmware location: `legacy-firmware/firmware/src/`

Key files:
- `brevitest-firmware.ino` - Main firmware (6200+ lines)
- `brevitest-firmware.h` - Constants and structs
- `DeviceState.cpp` / `DeviceState.h` - State machine

### 4. Pick Highest Priority Story

Select the highest priority user story where `passes: false` and no dependencies are blocking.

### 5. Implement the Story

Follow the Ralph loop method:
1. Read the legacy code reference
2. Document your understanding
3. Implement matching behavior
4. Verify against legacy
5. Commit with proper message
6. Update PRD

---

## Quality Requirements

### Code Standards

```cpp
// GOOD - Clear, purposeful code
void HeaterController::update() {
    int16_t raw = HAL::readAnalog(PIN_THERMISTOR);
    _currentTemp = rawToTemperature(raw);

    if (_currentTemp > HEATER_MAX_TEMP) {
        disable();
        Log.error("Heater over-temperature: %d", _currentTemp);
        return;
    }

    if (_controlEnabled) {
        int16_t power = calculatePID();
        setPower(power);
    }
}

// BAD - AI slop
void HeaterController::update() {
    // This function updates the heater controller by reading
    // the temperature and applying PID control if enabled.
    // First we read the raw analog value from the thermistor.
    int16_t raw = HAL::readAnalog(PIN_THERMISTOR);
    // Then we convert it to temperature.
    _currentTemp = rawToTemperature(raw);
    // Check if we're over temperature
    // ... etc
}
```

### Documentation Standards

Every state transition must be documented:

```cpp
/**
 * @brief Transition from HEATING to BARCODE_SCANNING
 *
 * LEGACY BEHAVIOR (brevitest-firmware.ino:5321-5328):
 * - Triggered when cartridge inserted during HEATING mode
 * - Always scans barcode for early detection
 * - Does NOT validate if heater not ready
 * - Logs: "Cartridge detected during heating"
 *
 * TRIGGER: detector_changed interrupt with detector_on = true
 * GUARD: device_state.mode == DeviceMode::HEATING
 * ACTION:
 *   1. Set cartridge_state = DETECTED
 *   2. Log "Cartridge detected during heating - transitioning to BARCODE_SCANNING"
 *   3. Call transition_to(DeviceMode::BARCODE_SCANNING)
 */
```

---

## Commit Message Format

```
feat: [STORY-ID] - [Story Title]

Legacy reference: [file:lines]
Behavior verified against: [specific legacy function]

Co-authored-by: Claude <noreply@anthropic.com>
```

Example:
```
feat: ALPHA-004 - Implement state transition validation matrix

Legacy reference: DeviceState.cpp:22-105
Behavior verified against: can_transition_to()

- Implements all 11 state transition rules
- Matches legacy validation exactly
- Unit tests pass for all valid/invalid transitions

Co-authored-by: Claude <noreply@anthropic.com>
```

---

## Progress Logging

After each story, append to `coordination/firmware-rewrite-progress.txt`:

```
## [Date] - [STORY-ID]: [Story Title]
Terminal: [alpha/beta/gamma/delta]

### Legacy Reference
- File: [filename:lines]
- Function: [function name]

### Understanding of Legacy Behavior
[Detailed explanation of what the legacy code does]

### Implementation Notes
[What was implemented and any differences from legacy]

### Files Changed
- [file1]
- [file2]

### Learnings
- [Pattern discovered]
- [Gotcha encountered]

---
```

---

## Coordination Protocol

### Before Editing a File

1. Check `firmware-rewrite-session-state.json` for reservations
2. If file is reserved by another terminal, skip
3. Add your reservation before editing
4. Release reservation when done

### Communication

Use the messageLog in session state to communicate:
- When starting a major change
- When discovering issues that affect other terminals
- When completing a milestone

### Conflict Resolution

If you discover a conflict:
1. Stop and document in messageLog
2. Do not overwrite another terminal's work
3. Wait for orchestrator resolution

---

## Critical Constants (Must Match Legacy)

### Timing
```cpp
#define DETECTOR_DEBOUNCE_DELAY_MS       10
#define HEATER_READY_DEBOUNCE_DELAY_MS   5000
#define VALIDATION_TIMEOUT_MS            45000
#define VALIDATION_MAX_RETRIES           3
#define VALIDATION_RETRY_BACKOFF_BASE_MS 5000
#define RECENT_TEST_COOLDOWN_MS          30000
```

### Hardware
```cpp
#define STAGE_POSITION_LIMIT_MICRONS     45000
#define STAGE_TEST_START_POSITION        7860
#define MOTOR_MICRONS_PER_EIGHTH_STEP    25
#define MOTOR_MINIMUM_STEP_DELAY_US      250
#define HEATER_TARGET_TEMP_C_10X         4500
#define HEATER_READY_TEMP_DELTA_C_10X    100
```

### Communication
```cpp
#define SERIAL_BAUD_RATE                 115200
#define BARCODE_BAUD_RATE                9600
#define BARCODE_UUID_LENGTH              36
```

---

## State Transition Quick Reference

```
INITIALIZING → IDLE, HEATING
IDLE → BARCODE_SCANNING, STRESS_TESTING, HEATING, UPLOADING_RESULTS
HEATING → IDLE, BARCODE_SCANNING, STRESS_TESTING
BARCODE_SCANNING → VALIDATING_CARTRIDGE, VALIDATING_MAGNETOMETER, STRESS_TESTING, IDLE
VALIDATING_CARTRIDGE → RUNNING_TEST, RESETTING_CARTRIDGE, IDLE
VALIDATING_MAGNETOMETER → IDLE
RUNNING_TEST → UPLOADING_RESULTS, IDLE
UPLOADING_RESULTS → IDLE
RESETTING_CARTRIDGE → IDLE
STRESS_TESTING → IDLE
ERROR_STATE → IDLE

ALWAYS ALLOWED: Any State → ERROR_STATE
ALWAYS ALLOWED: Any State → IDLE
```

---

## Questions?

If you're unsure about legacy behavior:
1. Read the legacy source code
2. Document your understanding
3. Ask in messageLog before implementing
4. Never guess - the firmware controls medical device hardware
