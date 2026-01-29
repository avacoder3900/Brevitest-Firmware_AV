# Brevitest Supabase Firmware - Progress Tracker

**Project**: Complete firmware rewrite from Particle/CouchDB to Supabase/PostgreSQL
**Started**: 2026-01-29
**Last Updated**: 2026-01-29

---

## Executive Summary

| Phase | Status | Progress |
|-------|--------|----------|
| Analysis | Complete | 100% |
| PRD Creation | Complete | 100% |
| Module Development | In Progress | 20% |
| Integration | Not Started | 0% |
| Validation | Not Started | 0% |

---

## Phase 1: Legacy Firmware Analysis

### Completed Analysis

#### 1. Repository Structure
- [x] Identified all source files
- [x] Mapped file dependencies
- [x] Documented folder organization
- [x] Reorganized into legacy-firmware/ and supabase-firmware/

#### 2. Platform & Tooling
- [x] **Platform**: Particle Boron (NRF52840-based)
- [x] **Particle OS Version**: 6.3.3
- [x] **Language**: C++ (Arduino-compatible)
- [x] **Build System**: Particle CLI with CMake
- [x] **Firmware Version**: 56
- [x] **Data Format Version**: 39

#### 3. Hardware Configuration
- [x] **Pin Mapping Documented**:
  - A0: Buzzer
  - A2-A4: Spectrophotometer channels
  - A6: Heater thermistor
  - D3: Cartridge detection (interrupt)
  - D4: Heater PWM
  - D5-D7: Lasers A/B/C
  - D8, D11-D13: Motor control
  - D22-D23: Barcode scanner
  - D26: Stage limit switch
  - I2C: AS7341 spectrophotometer, PCA9536 mux

#### 4. State Machine Architecture
- [x] **DeviceMode States**: 11 states identified
  - IDLE, INITIALIZING, HEATING, BARCODE_SCANNING
  - VALIDATING_CARTRIDGE, VALIDATING_MAGNETOMETER
  - RUNNING_TEST, UPLOADING_RESULTS, RESETTING_CARTRIDGE
  - STRESS_TESTING, ERROR_STATE
- [x] **TestState Sub-machine**: 6 states
- [x] **CartridgeState Sub-machine**: 5 states
- [x] **Transition validation rules documented**

#### 5. Cloud Integration Pattern
- [x] **Communication**: Particle Pub/Sub + Webhooks (NOT direct DB)
- [x] **Events**: validate-cartridge, load-assay, upload-test, reset-cartridge
- [x] **Response Handling**: Subscription callbacks with timeouts
- [x] **Retry Logic**: 3 attempts, 5s base delay, 45s timeout

#### 6. Data Structures
- [x] **BrevitestTestRecord**: 9668 bytes total
  - Header: 68 bytes (format, IDs, timing, config)
  - Readings: 300 x 32 bytes = 9600 bytes
- [x] **BrevitestSpectrophotometerReading**: 32 bytes each
  - Includes all 10 AS7341 channels + metadata
- [x] **BrevitestAssay**: Variable size (up to 5000 byte BCODE)
- [x] **EEPROM structure**: Version, cycle counts, running test info

#### 7. Serial Command Interface
- [x] **Baud Rate**: 115200
- [x] **Command Categories**: 100+ commands across 10 categories
  - 1-9: System commands
  - 10-12: Serial/device info
  - 20-29: Stage control
  - 30-33: Laser control
  - 40-43: Buzzer control
  - 50-55: Heater/temperature
  - 60: Barcode scanning
  - 70-73: Magnetometer
  - 90-93: Stress testing
  - 200+: BLE, spectrophotometer, cloud cache
  - 9000+: State management

### Pending Analysis

- [ ] Complete BCODE instruction set documentation
- [ ] Map all Particle cloud function signatures
- [ ] Document webhook payload formats
- [ ] Analyze magnetometer BLE protocol
- [ ] Review stress test automation logic

---

## Phase 2: PRD Documents

### Created PRDs

| PRD | Status | User Stories |
|-----|--------|--------------|
| prd-firmware-data-structures.json | Created | 6 |
| prd-firmware-hal.json | Created | 7 |
| prd-firmware-state-machine.json | Created | 8 |
| prd-firmware-spectrophotometer.json | Created | 7 |
| prd-firmware-motor.json | Created | 8 |
| prd-firmware-heater.json | Created | 7 |
| prd-firmware-laser.json | Created | 6 |
| prd-firmware-barcode.json | Created | 6 |
| prd-firmware-buzzer.json | Created | 6 |
| prd-firmware-cloud.json | Created | 9 |
| prd-firmware-serial.json | Created | 10 |
| prd-firmware-test-engine.json | Created | 8 |
| prd-firmware-storage.json | Created | 6 |
| prd-firmware-led.json | Created | 6 |
| prd-firmware-main.json | Created | 12 |
| prd-firmware-index.json | Created | (Index) |

**Total User Stories: 103**

### PRD Design Principles Applied

1. **No File Overlap**: Each PRD owns specific files exclusively
2. **Clear Interfaces**: Module boundaries defined via abstract interfaces
3. **Independent Testing**: Each module testable without others
4. **Parallel Development**: All PRDs can be worked on simultaneously

---

## Phase 3: Module Development

### Module Status

| Module | Status | Version | Tests |
|--------|--------|---------|-------|
| Data Structures | Complete | 1.0 | Pending |
| HAL | Complete | 1.0 | 25 tests |
| State Machine | Not Started | - | - |
| Spectrophotometer | Not Started | - | - |
| Motor Controller | Not Started | - | - |
| Heater Controller | Not Started | - | - |
| Laser Controller | Not Started | - | - |
| Barcode Scanner | Not Started | - | - |
| Buzzer Controller | Not Started | - | - |
| Cloud Client | **Complete** | 1.0 | 30 tests |
| Serial Commands | Not Started | - | - |
| Test Engine | **Complete** | 1.0 | 18 tests |
| Storage Manager | Not Started | - | - |
| LED Controller | Not Started | - | - |
| Main Integration | Not Started | - | - |

---

## Phase 4: Integration

### Integration Milestones

- [ ] All modules compile independently
- [ ] HAL integration with hardware modules
- [ ] State machine drives all operations
- [ ] Cloud communication working
- [ ] Full test cycle (insert → scan → validate → test → upload)
- [ ] Serial command interface complete
- [ ] Stress test mode operational

---

## Phase 5: Validation

### Validation Tests

| Test | Legacy | Supabase | Status |
|------|--------|----------|--------|
| State transitions match | - | - | Pending |
| Test record format compatible | - | - | Pending |
| Cloud upload successful | - | - | Pending |
| Serial commands equivalent | - | - | Pending |
| Temperature control stable | - | - | Pending |
| Spectro readings accurate | - | - | Pending |
| Motor positioning correct | - | - | Pending |

---

## Technical Decisions Log

### 2026-01-29: Cloud Architecture

**Decision**: Replace Particle Pub/Sub with direct HTTPS to Supabase Edge Functions

**Rationale**:
- Eliminates Particle Cloud dependency
- Direct PostgreSQL access via Supabase
- Better error handling with synchronous responses
- Simpler retry logic

**Trade-offs**:
- Requires WiFi connectivity management
- Need to handle HTTPS certificate validation
- Larger code footprint for HTTP client

### 2026-01-29: Module Architecture

**Decision**: Strict module separation with interface contracts

**Rationale**:
- Enables parallel development
- Easier testing and validation
- Clear ownership boundaries

**Trade-offs**:
- More boilerplate code
- Interface overhead
- Must maintain interface compatibility

---

## Known Issues & Blockers

| Issue | Severity | Status | Notes |
|-------|----------|--------|-------|
| Supabase MCP 401 error | Low | Investigating | May need manual auth |

---

## Questions for Resolution

1. **Particle Cloud Functions**: Do we keep any Particle integration for device management, or move everything to Supabase?

2. **Data Migration**: How do we handle existing test data in CouchDB? Export/import to PostgreSQL?

3. **Firmware OTA**: Will we use Particle OTA updates or implement custom solution?

4. **Device Authentication**: How will devices authenticate to Supabase? API keys, JWT, device certificates?

---

## Session Log

### Session 3: 2026-01-29

**Agent**: DELTA - Cloud Communication Module

**Accomplished**:
- Created `CloudProtocol.h` with all request/response structures (CLOUD-001)
- Created `SupabaseClient.h` with complete interface definition
- Created `SupabaseClient.cpp` with full implementation:
  - Cartridge validation with retry logic (CLOUD-002)
  - Assay loading with checksum verification (CLOUD-003)
  - Test upload with 9668-byte record handling (CLOUD-004)
  - Cartridge reset functionality (CLOUD-005)
  - Offline caching system with 50-entry limit (CLOUD-006)
- Created 4 Supabase Edge Functions (CLOUD-007):
  - `validate-cartridge/index.ts` - Cartridge validation
  - `load-assay/index.ts` - Assay/BCODE download
  - `upload-test/index.ts` - Test result upload with binary handling
  - `reset-cartridge/index.ts` - Cartridge status reset
- Verified database schema already exists (CLOUD-008)
- Created `test_cloud.cpp` with 30 unit tests (CLOUD-009)

**Files Created**:
- `supabase-firmware/src/CloudProtocol.h` (10KB, 380+ lines)
- `supabase-firmware/src/SupabaseClient.h` (12KB, 400+ lines)
- `supabase-firmware/src/SupabaseClient.cpp` (32KB, 1000+ lines)
- `supabase-firmware/backend/functions/validate-cartridge/index.ts` (8KB, 280+ lines)
- `supabase-firmware/backend/functions/load-assay/index.ts` (6KB, 200+ lines)
- `supabase-firmware/backend/functions/upload-test/index.ts` (10KB, 340+ lines)
- `supabase-firmware/backend/functions/reset-cartridge/index.ts` (6KB, 200+ lines)
- `supabase-firmware/tests/unit/test_cloud.cpp` (14KB, 450+ lines)

**Architecture Changes**:
- Replaced Particle.publish/subscribe with HTTPS POST requests
- Implemented synchronous request/response pattern
- Added request ID correlation for tracking
- Implemented automatic retry with exponential backoff
- Added offline caching to filesystem

**Key Features**:
- 45-second default timeout
- 3 retry attempts with 5-second base delay
- Base64-encoded binary test records in JSON
- CRC32 checksum verification
- Device authentication via API key

---

### Session 2: 2026-01-29

**Agent**: BETA - Hardware Abstraction Layer

**Accomplished**:
- Created `HardwareConfig.h` with all pin definitions and constants (HAL-001, HAL-002)
- Created `HAL.h` with complete abstraction interface
- Created `HAL.cpp` with full implementation (HAL-003, HAL-004, HAL-005, HAL-006)
- Created `test_hal.cpp` with 25+ unit tests (HAL-007)
- Implemented 21-point thermistor lookup table (identical to legacy)
- Added heater safety features (2000ms failsafe)
- Implemented I2C helpers with error handling
- Implemented spectrophotometer mux control

**Files Created**:
- `supabase-firmware/src/HardwareConfig.h` (17KB, 400+ lines)
- `supabase-firmware/src/HAL.h` (13KB, 350+ lines)
- `supabase-firmware/src/HAL.cpp` (17KB, 500+ lines)
- `supabase-firmware/tests/unit/test_hal.cpp` (17KB, 450+ lines)

**Verified**:
- All pin numbers match legacy firmware exactly
- Thermistor lookup table matches legacy (21 points)
- Heater failsafe timing = 2000ms as specified
- All hardware connections documented

---

### Session 1: 2026-01-29

**Agent**: Initial analysis and setup

**Accomplished**:
- Complete codebase exploration
- Repository reorganization (legacy-firmware/, supabase-firmware/)
- Created AGENTS.md with development guidelines
- Created this PROGRESS.md
- Analyzed Supabase MCP capabilities
- Documented state machine architecture
- Documented hardware configuration
- Documented data structures

**Next Steps**:
- Create all PRD documents
- Begin module development
- Set up Supabase project schema

### Session 4: 2026-01-29

**Agent**: Test Engine Module Implementation

**Accomplished**:
- Created `BCODEInterpreter.h` with complete interface (TEST-002)
  - All 12 BCODE opcodes defined as enum
  - Instruction parsing and execution state machine
  - Callback system for hardware integration
  - Support for nested repeat blocks (4 levels)
- Created `BCODEInterpreter.cpp` with full implementation
  - Token parsing following legacy format (|:,# delimiters)
  - Delay execution with chunked 1000ms intervals for heater control
  - Repeat block tracking with start index and iteration counter
- Created `TestRunner.h` with complete interface (TEST-001)
  - Test lifecycle management (start, stop, update)
  - Data collection infrastructure (300 readings max)
  - Stress test mode support (TEST-006)
  - Callback registration for hardware modules
- Created `TestRunner.cpp` with full implementation
  - Test record initialization and finalization (TEST-004)
  - Spectrophotometer reading storage (TEST-003)
  - Cartridge removal detection (TEST-005)
  - Singleton pattern for static callback access
- Created `BCODE_SPECIFICATION.md` (TEST-007)
  - Complete documentation of all opcodes
  - Parameter formats and examples
  - Execution model and error handling
- Created `test_runner.cpp` with 18+ unit tests (TEST-008)
  - BCODE loading and parsing tests
  - Opcode execution tests
  - Repeat block tests
  - Cartridge removal simulation
  - Test record management tests

**Files Created**:
- `supabase-firmware/src/BCODEInterpreter.h` (9KB, 320+ lines)
- `supabase-firmware/src/BCODEInterpreter.cpp` (12KB, 420+ lines)
- `supabase-firmware/src/TestRunner.h` (13KB, 450+ lines)
- `supabase-firmware/src/TestRunner.cpp` (14KB, 480+ lines)
- `supabase-firmware/docs/BCODE_SPECIFICATION.md` (6KB, 250+ lines)
- `supabase-firmware/tests/unit/test_runner.cpp` (10KB, 350+ lines)

**Key Features**:
- Binary-compatible with legacy BCODE format
- All 12 legacy opcodes supported (0, 1, 2, 3, 10, 11, 14, 15, 16, 20, 21, 99)
- Heater control loop integration during delays
- Automatic test cancellation on cartridge removal
- Stress test mode with configurable LED power variation

---

## References

### Legacy Documentation
- `legacy-firmware/firmware/STATE_MANAGEMENT_GUIDE.md`
- `legacy-firmware/firmware/SERIAL_COMMANDS.md`
- `legacy-firmware/firmware/PARTICLE_CONSOLE_QUICK_START.md`

### External Resources
- [Supabase MCP](https://supabase.com/docs/guides/getting-started/mcp)
- [Supabase Edge Functions](https://supabase.com/docs/guides/functions)
- [Particle Device OS API](https://docs.particle.io/reference/device-os/api/)
