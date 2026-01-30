# Terminal Launch Code - Phase 4 & 5

Copy and paste the following prompt into a new Claude Code terminal:

---

```
I am joining the Brevitest Supabase Firmware Rewrite project as a parallel worker terminal.

## Project Status
- 12/15 modules COMPLETE (Phase 1-3 done)
- 3 modules remaining: Test Engine, Serial Interface, Main Integration
- All source files in: supabase-firmware/src/
- All PRDs in: coordination/prd-firmware-*.json

## My Assignment
I will work on the remaining Phase 4 & 5 modules. Please read:
1. coordination/prd-firmware-index.json - Project overview
2. supabase-firmware/AGENTS.md - Development guidelines
3. supabase-firmware/PROGRESS.md - Current progress

## Available Modules to Work On

### Phase 4 (can run in parallel):
1. **Test Engine** (prd-firmware-test-engine.json)
   - BCODE interpreter for test instructions
   - Test data collection (300 readings max)
   - Test record generation (9668 bytes)
   - Files: TestRunner.h/.cpp, BCODEInterpreter.h/.cpp

2. **Serial Interface** (prd-firmware-serial.json)
   - 100+ debug commands
   - Command parser
   - All command categories (system, motor, laser, heater, spectro, etc.)
   - Files: SerialCommands.h/.cpp, CommandParser.h/.cpp

### Phase 5 (after Phase 4):
3. **Main Integration** (prd-firmware-main.json)
   - setup() and loop() functions
   - State handlers for all 11 DeviceModes
   - Interrupt handlers
   - Files: main.ino, main.h

## Completed Modules Available for Reference
- DataTypes.h/.cpp (structures, enums)
- HAL.h/.cpp (hardware abstraction)
- StateMachine.h/.cpp (state management)
- StorageManager.h/.cpp (EEPROM, cache)
- MotorController.h/.cpp (stage control)
- HeaterController.h/.cpp (PID temperature)
- LaserController.h/.cpp (3-channel PWM)
- BarcodeScanner.h/.cpp (UUID scanning)
- BuzzerController.h/.cpp (audio alerts)
- LEDController.h/.cpp (RGB indicators)
- Spectrophotometer.h/.cpp + drivers/AS7341.h/.cpp (sensor)
- SupabaseClient.h/.cpp + CloudProtocol.h (cloud comm)

## Instructions
1. Read the PRD index and AGENTS.md first
2. Pick either Test Engine OR Serial Interface to start
3. Create the header and implementation files
4. Follow the patterns in existing modules
5. Include unit tests

Start by reading the project files to understand the current state.
```

---

## Quick Copy Version (Single Line)

```
Read coordination/prd-firmware-index.json, supabase-firmware/AGENTS.md, and supabase-firmware/PROGRESS.md. I'm joining to help with Phase 4-5: Test Engine (prd-firmware-test-engine.json), Serial Interface (prd-firmware-serial.json), and Main Integration (prd-firmware-main.json). Pick one module and implement it following existing patterns in supabase-firmware/src/.
```
