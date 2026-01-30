# Brevitest Supabase Firmware - Agent Instructions

## Project Overview

This is a complete firmware rewrite for the Brevitest medical testing device. The new firmware migrates from the legacy Particle Cloud + CouchDB architecture to a modern Supabase PostgreSQL-based system while maintaining all existing functionality.

**Target Platform**: Particle Boron (NRF52840)
**Language**: C++ (Arduino-compatible)
**Cloud Backend**: Supabase (PostgreSQL + Edge Functions)

---

## Repository Structure

```
brevitest-device/
├── coordination/           # Multi-agent coordination protocol (DO NOT MODIFY)
├── ralph-main/             # Orchestration framework (DO NOT MODIFY)
├── legacy-firmware/        # Original firmware (REFERENCE ONLY - no edits)
│   ├── firmware/           # Main firmware source
│   ├── archive/            # Historical versions (v4-v21)
│   ├── magnetometer/       # Magnetometer module
│   └── testing/            # Test utilities
├── supabase-firmware/      # NEW FIRMWARE (all development here)
│   ├── src/                # Source code modules
│   ├── docs/               # Module documentation
│   ├── tests/              # Unit and integration tests
│   └── backend/            # Supabase Edge Functions
└── prd-*.json              # PRD documents for each module
```

---

## Critical Rules

### 1. File Access Rules
- **REFERENCE ONLY**: `legacy-firmware/` - Read to understand patterns, NEVER modify
- **DEVELOPMENT**: `supabase-firmware/` - All new code goes here
- **DO NOT TOUCH**: `coordination/`, `ralph-main/` - Orchestration infrastructure

### 2. Task Independence
Each PRD is designed to be **completely independent**. Tasks within a PRD:
- Do not share files with other PRDs
- Have clearly defined interfaces
- Can be developed and tested in isolation
- Communicate only through documented interfaces

### 3. Code Quality Standards
- Follow existing Particle C++ patterns from legacy firmware
- Document all public functions with Doxygen-style comments
- Include unit test stubs for all modules
- Use consistent naming conventions (snake_case for functions, PascalCase for types)

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

### From Legacy Firmware Analysis

1. **State Machine Pattern**: The DeviceMode enum drives all firmware behavior. Every hardware operation must check current state before executing.

2. **Interrupt Handling**: Cartridge detection (D3) uses hardware interrupt. All handlers must be non-blocking.

3. **Temperature Control**: PID loop runs continuously even during tests. Never block the heater control loop.

4. **Spectrophotometer Timing**: Reading sequence must not be interrupted. Uses critical sections for timing-sensitive operations.

5. **Cloud Communication Resilience**: All cloud operations have retry logic with exponential backoff. Request IDs correlate responses to requests.

6. **Data Format Stability**: BrevitestTestRecord (9668 bytes) format is critical for cloud compatibility. Any changes require backend coordination.

---

## Validation Checklist

Before marking any module complete:

- [ ] All acceptance criteria from PRD met
- [ ] Unit tests pass
- [ ] No compilation errors or warnings
- [ ] Behavior matches legacy firmware (where applicable)
- [ ] Documentation updated in PROGRESS.md
- [ ] Interface contract implemented correctly
- [ ] Error handling implemented
- [ ] Serial commands work (if applicable)

---

## Resources

### Legacy Firmware Reference
- Main firmware: `legacy-firmware/firmware/src/brevitest-firmware.ino`
- State machine: `legacy-firmware/firmware/src/DeviceState.h/.cpp`
- Spectro driver: `legacy-firmware/firmware/src/DFRobot_AS7341.h/.cpp`
- Documentation: `legacy-firmware/firmware/STATE_MANAGEMENT_GUIDE.md`

### Supabase Documentation
- MCP Server: https://supabase.com/docs/guides/getting-started/mcp
- Edge Functions: https://supabase.com/docs/guides/functions
- PostgreSQL: https://supabase.com/docs/guides/database

### Hardware Documentation
- Particle Boron: https://docs.particle.io/reference/device-os/api/
- AS7341 Spectrophotometer: https://wiki.dfrobot.com/SKU_SEN0364_AS7341
