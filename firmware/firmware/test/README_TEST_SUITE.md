# Brevitest Firmware Unit Test Suite
**ISO 13485:2016 Compliant Testing Framework**

## Overview
This test suite provides comprehensive unit testing for the Brevitest™ Sample Processing Unit firmware, specifically designed to meet ISO 13485:2016 medical device quality management requirements.

## Document Control
- **Document Version**: 1.0
- **Firmware Version**: 43
- **Data Format Version**: 39
- **Last Updated**: November 10, 2025
- **Compliance Standard**: ISO 13485:2016

## Scope of Testing
This test suite covers:
1. **Serial Command Testing** - All 80+ serial commands validated
2. **State Transition Testing** - Complete state machine validation
3. **Integration Testing** - Cross-functional testing scenarios
4. **Risk-Based Testing** - Critical path and safety-related functions

## Test Categories

### 1. Serial Commands Testing
Tests all serial commands categorized by function:
- **Low Level Commands** (1-9): System control, EEPROM, pin management
- **Serial Port Messaging** (10-12): Communication control
- **Stage Motion** (20-29): Motor and stage control
- **Laser Diodes** (30-33): Optical system control
- **Buzzer** (40-43): User feedback system
- **Heater** (50-55): Temperature control system
- **Barcode Scanner** (60): Cartridge identification
- **Magnetometer** (70-73): Magnet validation
- **Stress Test** (90-93): Device reliability testing
- **Bluetooth LE** (200): Wireless communication
- **Spectrophotometer** (301-311): Optical measurements
- **Cloud Functions** (400-405): Data management
- **Communication** (8000-8999): Radio and diagnostic commands

### 2. State Transition Testing
Tests the DeviceStateMachine with three state hierarchies:
- **DeviceMode**: Primary operational states (11 states)
- **TestState**: Test execution tracking (7 states)
- **CartridgeState**: Cartridge lifecycle (6 states)

### 3. Integration Testing
Tests complex workflows combining multiple commands and state transitions.

## Test Execution

### Prerequisites
- Arduino/Particle testing framework installed
- Serial port access for communication testing
- Mock hardware interfaces for hardware-dependent tests

### Running Tests
```bash
# Run all tests
./run_tests.sh

# Run specific test suite
./run_tests.sh serial_commands
./run_tests.sh state_transitions
./run_tests.sh integration

# Generate test report
./generate_test_report.sh
```

## ISO 13485 Compliance

### Design Controls (7.3)
- Tests provide verification of design outputs
- Traceability matrix links requirements to tests
- Risk-based testing prioritizes critical functions

### Verification (7.3.5)
- Unit tests verify individual functions
- Integration tests verify combined functionality
- State transition tests verify system behavior

### Documentation (4.2.4)
- Test cases documented with unique identifiers
- Test results recorded with pass/fail criteria
- Traceability maintained throughout

### Risk Management (ISO 14971)
- High-risk functions have comprehensive test coverage
- Safety-related commands tested with boundary conditions
- Failure modes tested and documented

## Test Traceability
All tests are traceable to:
- Software requirements
- Risk analysis items
- Design specifications
- Verification activities

See `TEST_TRACEABILITY_MATRIX.md` for detailed mapping.

## Test Coverage Goals
- **Statement Coverage**: >95%
- **Branch Coverage**: >90%
- **Function Coverage**: 100%
- **State Coverage**: 100%
- **Command Coverage**: 100%

## Defect Management
- All test failures documented
- Root cause analysis performed
- Corrective actions tracked
- Regression tests added for fixed defects

## Test Environment
- **Target Hardware**: Particle M-SoM
- **Firmware Platform**: Particle Device OS 6.3.3
- **Test Framework**: Custom C++ unit test framework
- **Mock Framework**: Hardware abstraction layer mocks

## Revision History
| Version | Date | Author | Description |
|---------|------|--------|-------------|
| 1.0 | 2025-11-10 | Test Team | Initial test suite creation |

## Contact
For questions regarding this test suite, contact the firmware development team.

