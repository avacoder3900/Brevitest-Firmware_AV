# Test Execution Guide
**Brevitest Firmware Unit Test Suite**
**ISO 13485:2016 Compliant**

## Document Control
- **Document Version**: 1.0
- **Firmware Version**: 43
- **Last Updated**: November 10, 2025
- **Purpose**: Provide instructions for executing unit tests

## Prerequisites

### Software Requirements
- GCC compiler (version 9.0 or later)
- Make build system
- Git (for version control)
- Text editor or IDE

### Hardware Requirements (Optional)
- Particle M-SoM device (for hardware-in-the-loop testing)
- USB cable for device connection
- Serial terminal software (e.g., PuTTY, screen)

### Environment Setup
1. Clone the repository
2. Navigate to the firmware directory
3. Ensure test directory structure is intact

## Test Suite Organization

### Directory Structure
```
firmware/
├── src/
│   ├── brevitest-firmware.ino
│   ├── brevitest-firmware.h
│   ├── DeviceState.h
│   └── DeviceState.cpp
├── test/
│   ├── README_TEST_SUITE.md
│   ├── TEST_TRACEABILITY_MATRIX.md
│   ├── TEST_EXECUTION_GUIDE.md (this file)
│   ├── test_framework.h
│   ├── test_framework.cpp
│   ├── test_serial_commands.h
│   ├── test_serial_commands.cpp
│   ├── test_state_transitions.h
│   ├── test_state_transitions.cpp
│   ├── test_main.cpp
│   ├── Makefile
│   └── run_tests.sh
```

## Building Tests

### Compile All Tests
```bash
cd firmware/test
make clean
make all
```

### Compile Specific Test Suite
```bash
make test_serial_commands
make test_state_transitions
```

## Running Tests

### Run All Test Suites
```bash
./run_tests.sh
```

### Run Specific Test Suite
```bash
./run_tests.sh serial_commands
./run_tests.sh state_transitions
```

### Run Individual Test Executable
```bash
./test_serial_commands
./test_state_transitions
```

## Test Execution Workflow

### 1. Pre-Test Checklist
- [ ] Firmware version verified (43)
- [ ] Test environment clean
- [ ] All dependencies installed
- [ ] Previous test results archived

### 2. Execute Tests
- [ ] Run all test suites
- [ ] Review console output
- [ ] Check for any failures
- [ ] Note any warnings

### 3. Post-Test Activities
- [ ] Generate test report
- [ ] Archive test results
- [ ] Update traceability matrix if needed
- [ ] Document any failures

## Test Output Format

### Console Output
Tests produce formatted console output with:
- Test suite name
- Individual test results (✅ PASS, ❌ FAIL, ⏭ SKIP)
- Test metadata (ID, requirement, risk level)
- Summary statistics
- Pass/fail percentages

### Example Output
```
╔═══════════════════════════════════════════════════════════════╗
║  Brevitest Firmware Unit Test Suite                          ║
║  ISO 13485:2016 Compliant Testing Framework                  ║
╚═══════════════════════════════════════════════════════════════╝

Firmware Version: 43
Data Format Version: 39
Test Execution Date: Mon Nov 10 2025

═══════════════════════════════════════════════════════════════
Test Suite: serial_commands
═══════════════════════════════════════════════════════════════

▶ Running: Reset EEPROM [TC-CMD-002]
  Requirement: REQ-CMD-002 | Risk: RISK-LOW-001 | Category: Low Level
  ✅ PASS: Reset EEPROM

▶ Running: Check Cache [TC-CMD-003]
  Requirement: REQ-CMD-003 | Risk: RISK-MED-001 | Category: Low Level
  ✅ PASS: Check Cache

...

═══════════════════════════════════════════════════════════════
║  Test Execution Summary                                       ║
╚═══════════════════════════════════════════════════════════════╝

Total Tests:   122
✅ Passed:     122 (100.0%)
❌ Failed:     0 (0.0%)
⏭  Skipped:    0 (0.0%)

═══════════════════════════════════════════════════════════════
                    ✨ ALL TESTS PASSED ✨
═══════════════════════════════════════════════════════════════
```

## Test Report Generation

### Generate Test Report
```bash
make report
```

This generates `test_report_YYYYMMDD_HHMMSS.txt` with:
- Test execution date and time
- Firmware version information
- Complete test results
- Pass/fail statistics
- Verification status
- Signature blocks for approval

### Report Location
Reports are saved to: `firmware/test/reports/`

## Test Categories and Priorities

### Priority 1 (P1) - Critical Tests
**Run Frequency**: Every build
**Test Count**: 22 tests

Critical tests include:
- Safety-critical commands (temperature limits, stage limits)
- State transition validation gates
- Emergency stop functionality
- Hardware failure handling

### Priority 2 (P2) - High-Risk Tests
**Run Frequency**: Before each release
**Test Count**: 48 tests

High-risk tests include:
- Complex state transitions
- Integration workflows
- Cloud operation handling
- Error recovery procedures

### Priority 3 (P3) - Standard Tests
**Run Frequency**: Weekly or on-demand
**Test Count**: 52 tests

Standard tests include:
- Basic command validation
- Query functions
- Low-risk transitions
- Boundary conditions

## Interpreting Test Results

### All Tests Pass
- **Action**: Generate test report
- **Status**: Ready for next phase (integration, validation)
- **Documentation**: Archive results with sign-off

### Some Tests Fail
- **Action**: Investigate root cause
- **Status**: Not ready for release
- **Documentation**: 
  - Log failure details
  - Perform root cause analysis
  - Create defect report
  - Plan corrective action

### Test Infrastructure Failure
- **Action**: Debug test framework
- **Status**: Test results invalid
- **Documentation**: Note infrastructure issue

## Troubleshooting

### Common Issues

#### Issue: Tests Won't Compile
**Symptoms**: Compiler errors
**Solutions**:
1. Verify GCC version (gcc --version)
2. Check all header files present
3. Ensure proper include paths
4. Verify Makefile settings

#### Issue: Tests Fail Unexpectedly
**Symptoms**: Previously passing tests now fail
**Solutions**:
1. Check firmware version matches
2. Verify no code changes without test updates
3. Check for environment changes
4. Review git history for changes

#### Issue: Mock Hardware Not Responding
**Symptoms**: Hardware-dependent tests fail
**Solutions**:
1. Verify mock state initialization
2. Check mock function implementations
3. Ensure proper reset between tests

## Continuous Integration

### CI/CD Integration
Tests can be integrated into CI/CD pipelines:

```yaml
# Example GitHub Actions workflow
name: Run Unit Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: sudo apt-get install -y gcc make
      - name: Build tests
        run: cd firmware/test && make all
      - name: Run tests
        run: cd firmware/test && ./run_tests.sh
      - name: Generate report
        run: cd firmware/test && make report
      - name: Archive results
        uses: actions/upload-artifact@v2
        with:
          name: test-results
          path: firmware/test/reports/
```

## Test Maintenance

### Adding New Tests
1. Identify requirement to test
2. Create test function in appropriate file
3. Add test case to suite runner
4. Update traceability matrix
5. Run tests to verify
6. Commit changes with clear message

### Modifying Existing Tests
1. Document reason for modification
2. Update test implementation
3. Update traceability if needed
4. Run full test suite
5. Update documentation
6. Get review and approval

### Removing Tests
1. Document justification (usually requirement change)
2. Update traceability matrix
3. Remove test code
4. Update coverage metrics
5. Get approval from quality team

## Regulatory Compliance

### ISO 13485:2016 Requirements

#### Design Verification (7.3.5)
- Execute all tests before release
- Document test results
- Maintain traceability
- Review and approve results

#### Change Control (7.3.9)
- Regression testing after changes
- Impact analysis
- Test updates as needed
- Re-verification

#### Record Keeping (4.2.4)
- Maintain test records
- Archive test reports
- Version control test code
- Traceability documentation

## Test Metrics

### Key Metrics Tracked
- **Test Coverage**: % of requirements with tests
- **Pass Rate**: % of tests passing
- **Defect Detection**: Number of bugs found
- **Test Execution Time**: Duration of test runs
- **Code Coverage**: % of code exercised by tests

### Target Metrics
- Statement Coverage: >95%
- Branch Coverage: >90%
- Function Coverage: 100%
- Requirements Coverage: 100%
- Pass Rate: 100%

## Contact Information

### Support
For questions or issues with the test suite:
- **Technical Issues**: firmware-dev@brevitest.com
- **Quality Issues**: quality@brevitest.com
- **General Questions**: support@brevitest.com

## Appendix A: Test Case Quick Reference

### Serial Command Tests (26 tests)
- Low Level Commands: 5 tests
- Messaging: 2 tests
- Stage Motion: 2 tests
- Lasers: 2 tests
- Heater: 6 tests
- Spectrophotometer: 1 test
- Integration: 2 tests
- Safety: 2 tests
- Boundary: 1 test

### State Transition Tests (73 tests)
- DeviceMode: 14 tests
- Invalid Transitions: 5 tests
- Cartridge Flow: 4 tests
- TestState: 8 tests
- CartridgeState: 8 tests
- Compound: 5 tests
- Cloud Operations: 4 tests
- Error Handling: 4 tests
- State Query: 6 tests
- Concurrent: 3 tests
- Safety: 4 tests
- Persistence: 2 tests

### Total: 122 Tests

## Appendix B: Risk-Based Test Prioritization

### Critical Risk Tests (22 tests)
Must pass for release. Never skip.

### High Risk Tests (48 tests)
Should pass for release. Skip only with approval.

### Medium Risk Tests (31 tests)
Important but not blocking. Can defer if time-constrained.

### Low Risk Tests (21 tests)
Nice to have. Can skip in emergency releases.

## Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2025-11-10 | Test Team | Initial test execution guide |

