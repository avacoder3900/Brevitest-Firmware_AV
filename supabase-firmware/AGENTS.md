# Brevitest Supabase Firmware - Agent Instructions

## Project Overview

This is a complete firmware rewrite for the Brevitest medical testing device. The new firmware migrates from the legacy Particle Cloud + CouchDB architecture to a modern Supabase PostgreSQL-based system while maintaining **exact behavioral compatibility** with the legacy firmware.

**Target Platform**: Particle Boron (NRF52840)
**Language**: C++ (Arduino-compatible)
**Cloud Backend**: Supabase (PostgreSQL + Edge Functions)

**CRITICAL REQUIREMENT**: The new firmware must behave identically to the legacy firmware. Every state transition, timing delay, and hardware interaction must match the legacy behavior exactly.

---

## PRD-Based Development (Ralph Loop Method)

This project uses the Ralph loop method with 4 parallel terminals:

| Terminal | PRD File | Focus Area | Stories |
|----------|----------|------------|---------|
| **Alpha** | `prd-fw-alpha-statemachine.json` | State Machine & Core Loop | 17 |
| **Beta** | `prd-fw-beta-hardware.json` | Hardware Controllers | 20 |
| **Gamma** | `prd-fw-gamma-cloud.json` | Cloud & Storage | 16 |
| **Delta** | `prd-fw-delta-serial.json` | Serial Commands & BCODE | 28 |

**Total: 81 User Stories**

### Coordination Files
- `coordination/firmware-rewrite-session-state.json` - Active session tracking
- `coordination/FIRMWARE-REWRITE-INSTRUCTIONS.md` - Detailed terminal instructions
- `coordination/firmware-rewrite-progress.txt` - Progress log and patterns
- `coordination/docs/STATE_TRANSITIONS_LEGACY_ANALYSIS.md` - State machine reference

---

## Repository Structure

```
brevitest-device/
├── coordination/           # Multi-agent coordination protocol
│   ├── prd-fw-alpha-statemachine.json  # State machine PRD
│   ├── prd-fw-beta-hardware.json       # Hardware PRD
│   ├── prd-fw-gamma-cloud.json         # Cloud PRD
│   ├── prd-fw-delta-serial.json        # Serial/BCODE PRD
│   ├── firmware-rewrite-session-state.json  # Session coordination
│   ├── FIRMWARE-REWRITE-INSTRUCTIONS.md     # Terminal instructions
│   └── docs/               # Analysis documentation
├── ralph-main/             # Orchestration framework (DO NOT MODIFY)
├── legacy-firmware/        # Original firmware (REFERENCE ONLY - no edits)
│   └── firmware/src/       # Main firmware source (6200+ lines)
├── supabase-firmware/      # NEW FIRMWARE (all development here)
│   ├── src/                # Source code modules
│   ├── docs/               # Module documentation
│   ├── tests/              # Unit and integration tests
│   └── backend/            # Supabase Edge Functions
└── scripts/                # Build and deployment scripts
```

---

## Critical Rules

### 1. File Access Rules
- **REFERENCE ONLY**: `legacy-firmware/` - Read to understand patterns, NEVER modify
- **DEVELOPMENT**: `supabase-firmware/` - All new code goes here
- **COORDINATION**: `coordination/` - Update session state and progress files

### 2. Before Implementing ANY Story
1. **Read the legacy code reference** listed in the user story
2. **Document your understanding** of the legacy behavior
3. **Match the behavior exactly** - not "similar to" or "inspired by"
4. **Verify against legacy** before marking complete

### 3. Code Quality Standards - NO AI SLOP
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

// BAD - AI slop with unnecessary comments
void HeaterController::update() {
    // This function updates the heater controller by reading
    // the temperature and applying PID control if enabled.
    // First we read the raw analog value from the thermistor.
    int16_t raw = HAL::readAnalog(PIN_THERMISTOR);
    // Then we convert it to temperature...
}
```

### 4. Coordination Protocol
- Check `firmware-rewrite-session-state.json` before editing files
- Reserve files you're working on
- Release reservations when done
- Log progress to `firmware-rewrite-progress.txt`

---

## Module Architecture

### Core Modules (Independent Development)

| Module | PRD File | Primary Files | Dependencies |
|--------|----------|---------------|--------------|
| State Machine | prd-state-machine.json | DeviceState.h/.cpp | None |
| Hardware Abstraction | prd-hal.json | HardwareConfig.h | None |
| Spectrophotometer | prd-spectro.json | Spectrophotometer.h/.cpp | HAL |
| Motor Control | prd-motor.json | MotorController.h/.cpp | HAL |
| Heater/Temperature | prd-heater.json | HeaterController.h/.cpp | HAL |
| Laser Control | prd-laser.json | LaserController.h/.cpp | HAL |
| Barcode Scanner | prd-barcode.json | BarcodeScanner.h/.cpp | HAL |
| Buzzer/Alerts | prd-buzzer.json | BuzzerController.h/.cpp | HAL |
| Cloud Communication | prd-cloud.json | SupabaseClient.h/.cpp | None |
| Serial Interface | prd-serial.json | SerialCommands.h/.cpp | All modules |
| Test Engine | prd-test-engine.json | TestRunner.h/.cpp | All hardware |
| Data Structures | prd-data.json | DataTypes.h | None |
| Storage Manager | prd-storage.json | StorageManager.h/.cpp | None |
| LED Indicators | prd-led.json | LEDController.h/.cpp | HAL |
| Main Integration | prd-main.json | main.ino | All modules |

### Dependency Graph

```
┌─────────────────┐
│  Data Structures│ ←── Shared types, no dependencies
└────────┬────────┘
         │
┌────────▼────────┐
│       HAL       │ ←── Hardware pin definitions
└────────┬────────┘
         │
    ┌────┴────┬────────┬────────┬────────┬────────┐
    │         │        │        │        │        │
┌───▼──┐ ┌───▼──┐ ┌───▼──┐ ┌───▼──┐ ┌───▼──┐ ┌───▼──┐
│Motor │ │Heater│ │Laser │ │Spectro│ │Barcode│ │Buzzer│
└───┬──┘ └───┬──┘ └───┬──┘ └───┬──┘ └───┬──┘ └───┬──┘
    │        │        │        │        │        │
    └────────┴────────┴────┬───┴────────┴────────┘
                          │
              ┌───────────▼───────────┐
              │     State Machine     │
              └───────────┬───────────┘
                          │
         ┌────────────────┼────────────────┐
         │                │                │
    ┌────▼────┐    ┌─────▼─────┐    ┌─────▼─────┐
    │  Cloud  │    │Test Engine│    │  Serial   │
    └────┬────┘    └─────┬─────┘    └─────┬─────┘
         │               │                │
         └───────────────┼────────────────┘
                         │
                  ┌──────▼──────┐
                  │    Main     │
                  └─────────────┘
```

---

## Interface Contracts

All modules must implement the following interface pattern:

```cpp
// Module header template
class ModuleName {
public:
    // Lifecycle
    bool init();           // Initialize hardware/resources
    void shutdown();       // Clean shutdown

    // Main loop integration
    void update();         // Called every loop iteration

    // Status
    bool isReady() const;  // Module ready for operation
    String getStatus() const; // Human-readable status

    // Error handling
    int getLastError() const;
    String getErrorMessage(int code) const;
};
```

---

## Communication Patterns

### Legacy (Particle Cloud)
```
Device → Particle.publish() → Webhook → Backend → CouchDB
Backend → Particle.subscribe() → Device callback
```

### New (Supabase)
```
Device → HTTPS POST → Supabase Edge Function → PostgreSQL
Device ← HTTPS Response ← Supabase Edge Function
```

Key differences:
- Direct HTTPS calls instead of pub/sub
- Synchronous request/response (with async option)
- PostgreSQL instead of CouchDB document store
- Row Level Security (RLS) for data protection

---

## Testing Strategy

### Unit Tests (per module)
- Located in `supabase-firmware/tests/unit/`
- Test individual functions in isolation
- Use mock objects for hardware dependencies

### Integration Tests
- Located in `supabase-firmware/tests/integration/`
- Test module interactions
- Run on actual hardware when possible

### Validation Tests
- Compare behavior against legacy firmware
- Ensure data format compatibility
- Verify cloud communication protocols

---

## Discovered Patterns (Update as you work)

### CRITICAL Legacy Firmware Patterns

1. **hardware_loop() is called EVERY loop iteration**
   - Handles detector debouncing (10ms)
   - Handles heater ready debouncing (5000ms)
   - Calls set_device_indicators() for LED/buzzer
   - Runs PID temperature control
   - **This is why the new firmware doesn't work - hardware_loop() is missing**

2. **Heater Ready Debouncing (5000ms)**
   - Temperature must be in range for 5 continuous seconds
   - Prevents false "heater ready" triggers
   - Use `heater_debounced()` function pattern

3. **Cartridge Rejection During HEATING**
   - If cartridge inserted while heater NOT ready:
     - Scan barcode immediately (early detection)
     - Store as `pending_barcode_uuid`
     - Signal user to REMOVE cartridge (green blink)
     - DO NOT validate - reject the cartridge
   - **This prevents thermal damage to cartridges**

4. **Validation Retry Logic**
   ```
   Timeout: 45000ms
   Max Retries: 3
   Backoff: 5000ms * retry_count (exponential)
   ```

5. **Recently Tested Barcode Cooldown (30s)**
   - Same barcode within 30 seconds is ignored
   - Prevents immediate re-scanning after test completion

6. **LED Indicator Rules**
   - Red solid = DON'T TOUCH (scanning, validating, testing)
   - Green fade = INSERT CARTRIDGE (idle, heater ready)
   - Green blink = REMOVE CARTRIDGE (test complete, invalid)

7. **State Transition Validation**
   - ALWAYS call `can_transition_to()` before `setMode()`
   - Log format: "State transition: X -> Y"
   - Use `Time.now()` for timestamps, `millis()` for timeouts

8. **Cloud Operation Tracking**
   - Track `cloud_operation_pending` flag
   - Clear ALL tracking when cartridge removed
   - Validate checksums on downloaded data

---

## Critical Constants (MUST Match Legacy)

### Timing
```cpp
#define DETECTOR_DEBOUNCE_DELAY_MS       10
#define HEATER_READY_DEBOUNCE_DELAY_MS   5000
#define VALIDATION_TIMEOUT_MS            45000
#define VALIDATION_MAX_RETRIES           3
#define VALIDATION_RETRY_BACKOFF_BASE_MS 5000
#define RECENT_TEST_COOLDOWN_MS          30000
#define UPLOAD_TIMEOUT_MS                30000
#define RESET_TIMEOUT_MS                 30000
```

### Hardware
```cpp
#define STAGE_POSITION_LIMIT_MICRONS     45000
#define STAGE_TEST_START_POSITION        7860
#define MOTOR_MICRONS_PER_EIGHTH_STEP    25
#define MOTOR_MINIMUM_STEP_DELAY_US      250
#define HEATER_TARGET_TEMP_C_10X         4500  // 45.0°C
#define HEATER_READY_TEMP_DELTA_C_10X    100   // ±1.0°C
```

### Communication
```cpp
#define SERIAL_BAUD_RATE                 115200
#define BARCODE_BAUD_RATE                9600
#define BARCODE_UUID_LENGTH              36
```

---

## Validation Checklist

Before marking any story complete:

- [ ] Legacy code reference read and understood
- [ ] Behavior documented with legacy line numbers
- [ ] Implementation matches legacy EXACTLY
- [ ] All acceptance criteria from PRD met
- [ ] No compilation errors or warnings
- [ ] Progress logged in `firmware-rewrite-progress.txt`
- [ ] PRD updated with `passes: true`

---

## Resources

### Legacy Firmware Reference (READ BEFORE CODING)
- Main firmware: `legacy-firmware/firmware/src/brevitest-firmware.ino` (6200+ lines)
- State machine: `legacy-firmware/firmware/src/DeviceState.h/.cpp`
- Header/constants: `legacy-firmware/firmware/src/brevitest-firmware.h`
- Spectro driver: `legacy-firmware/firmware/src/DFRobot_AS7341.h/.cpp`

### Coordination Files
- PRDs: `coordination/prd-fw-*.json`
- Session state: `coordination/firmware-rewrite-session-state.json`
- Instructions: `coordination/FIRMWARE-REWRITE-INSTRUCTIONS.md`
- State transitions: `coordination/docs/STATE_TRANSITIONS_LEGACY_ANALYSIS.md`
- Progress log: `coordination/firmware-rewrite-progress.txt`

### Supabase Documentation
- MCP Server: https://supabase.com/docs/guides/getting-started/mcp
- Edge Functions: https://supabase.com/docs/guides/functions
- PostgreSQL: https://supabase.com/docs/guides/database

### Hardware Documentation
- Particle Boron: https://docs.particle.io/reference/device-os/api/
- AS7341 Spectrophotometer: https://wiki.dfrobot.com/SKU_SEN0364_AS7341

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

See `coordination/docs/STATE_TRANSITIONS_LEGACY_ANALYSIS.md` for complete details.
