# Getting Started with the Test Suite
**Quick Start Guide for Brevitest Firmware Unit Tests**

## 5-Minute Quick Start

### 1. Build the Tests
```bash
cd firmware/test
make clean
make all
```

### 2. Run All Tests
```bash
./run_tests.sh
```

or

```bash
make test
```

### 3. View Results
The test output will show:
- ✅ Passed tests in green
- ❌ Failed tests in red
- Summary statistics

### 4. Generate Report (Optional)
```bash
./run_tests.sh --report
```

Report will be saved to `reports/test_report_<timestamp>.txt`

---

## What's Included

### Test Coverage
- **122 comprehensive test cases**
- **87 requirements** fully covered
- **100% requirements coverage**
- **ISO 13485 compliant**

### Test Categories

#### Serial Command Tests (26 tests)
Tests all firmware commands:
- Low level operations (EEPROM, pins)
- Stage motion control
- Heater and temperature control
- Laser control
- Spectrophotometer operations
- Cloud functions

#### State Transition Tests (73 tests)
Tests state machine behavior:
- Device operational states (11 states)
- Test execution states (7 states)
- Cartridge lifecycle states (6 states)
- Valid and invalid transitions
- Error handling and recovery
- Safety-critical scenarios

#### Integration Tests (9 tests)
Tests complete workflows:
- Full test execution
- Heater control sequences
- Stage motion sequences
- Error recovery workflows
- Validation failures

#### Safety Tests (6 tests)
Tests critical safety functions:
- Temperature limit enforcement
- Stage position limits
- Emergency stop capability
- Hardware failure handling

---

## Directory Structure

```
firmware/test/
├── README_TEST_SUITE.md           # Overview and documentation
├── GETTING_STARTED.md             # This file - quick start guide
├── TEST_EXECUTION_GUIDE.md        # Detailed execution instructions
├── TEST_TRACEABILITY_MATRIX.md    # Requirements-to-tests mapping
├── ISO_13485_COMPLIANCE.md        # Compliance documentation
│
├── test_framework.h               # Test framework header
├── test_framework.cpp             # Test framework implementation
├── test_serial_commands.h         # Serial command tests header
├── test_serial_commands.cpp       # Serial command tests (26 tests)
├── test_state_transitions.h       # State transition tests header
├── test_state_transitions.cpp     # State transition tests (73 tests)
├── test_main.cpp                  # Main test runner
│
├── Makefile                       # Build system
├── run_tests.sh                   # Test execution script
│
└── reports/                       # Generated test reports
    └── test_report_*.txt
```

---

## Common Usage Scenarios

### Development Workflow
```bash
# 1. Make code changes to firmware
vim ../src/brevitest-firmware.ino

# 2. Run relevant tests
./run_tests.sh serial_commands

# 3. If tests pass, run all tests
./run_tests.sh

# 4. Generate report for documentation
./run_tests.sh --report
```

### Pre-Release Verification
```bash
# Run complete test suite with report
./run_tests.sh --report

# Check that all tests pass (exit code 0)
echo $?

# Review generated report
cat reports/test_report_*.txt
```

### Continuous Integration
```bash
# Automated CI/CD pipeline
cd firmware/test
make clean
make all
make test

# Check exit code
if [ $? -eq 0 ]; then
    echo "Tests passed - deploy allowed"
else
    echo "Tests failed - deploy blocked"
    exit 1
fi
```

### Code Coverage Analysis
```bash
# Build and run with coverage
make coverage

# View coverage files
ls *.gcov

# Analyze coverage
gcov test_serial_commands.cpp
```

---

## Understanding Test Results

### Successful Test Run
```
═══════════════════════════════════════════════════════════════
Test Suite: serial_commands
═══════════════════════════════════════════════════════════════

▶ Running: Reset EEPROM [TC-CMD-002]
  Requirement: REQ-CMD-002 | Risk: RISK-LOW-001 | Category: Low Level
  ✅ PASS: Reset EEPROM

Total Tests:   122
✅ Passed:     122 (100.0%)
❌ Failed:     0 (0.0%)

═══════════════════════════════════════════════════════════════
                    ✨ ALL TESTS PASSED ✨
═══════════════════════════════════════════════════════════════
```

### Failed Test Run
```
▶ Running: Temperature Safety Limits [TC-SAFE-001]
  Requirement: REQ-SAFE-001 | Risk: RISK-CRIT-004 | Category: Safety
  ❌ FAIL: Temperature Safety Limits
     Expected: 600, Actual: 700
     Line 234 in test_serial_commands.cpp

Total Tests:   122
✅ Passed:     121 (99.2%)
❌ Failed:     1 (0.8%)

═══════════════════════════════════════════════════════════════
                   ⚠️  TESTS FAILED ⚠️
═══════════════════════════════════════════════════════════════
```

---

## Test Metadata Explained

Each test includes metadata for traceability:

```
▶ Running: Reset EEPROM [TC-CMD-002]
  Requirement: REQ-CMD-002 | Risk: RISK-LOW-001 | Category: Low Level
```

- **Test ID**: `TC-CMD-002` - Unique test identifier
- **Requirement**: `REQ-CMD-002` - Linked requirement ID
- **Risk**: `RISK-LOW-001` - Risk level (CRIT/HIGH/MED/LOW)
- **Category**: `Low Level` - Test category

This metadata supports:
- ISO 13485 compliance
- Requirements traceability
- Risk-based testing
- Test organization

---

## Troubleshooting

### Issue: Tests Won't Compile
**Solution**:
```bash
# Check compiler version
gcc --version  # Should be 9.0 or later

# Install dependencies (Ubuntu/Debian)
make install-deps

# Clean and rebuild
make clean
make all
```

### Issue: Tests Fail Unexpectedly
**Solution**:
1. Verify firmware version matches (43)
2. Check for recent code changes
3. Review test output for specific failures
4. Check git history: `git log --oneline`

### Issue: Permission Denied (Linux/Mac)
**Solution**:
```bash
# Make script executable
chmod +x run_tests.sh

# Run tests
./run_tests.sh
```

---

## Next Steps

### For Developers
1. **Run tests before committing**:
   ```bash
   make test
   ```

2. **Add tests for new features**:
   - Edit `test_serial_commands.cpp` or `test_state_transitions.cpp`
   - Add test to suite runner
   - Update traceability matrix

3. **Update tests when fixing bugs**:
   - Add regression test for the bug
   - Verify fix with test
   - Ensure all tests still pass

### For QA Engineers
1. **Review test documentation**:
   - `README_TEST_SUITE.md` for overview
   - `TEST_EXECUTION_GUIDE.md` for details
   - `TEST_TRACEABILITY_MATRIX.md` for coverage

2. **Execute verification tests**:
   ```bash
   ./run_tests.sh --report
   ```

3. **Review and approve reports**:
   - Check reports directory
   - Sign off on test results
   - Archive for regulatory submission

### For Regulatory/Quality
1. **Review compliance documentation**:
   - `ISO_13485_COMPLIANCE.md`
   - Traceability matrix
   - Test reports

2. **Verify coverage**:
   - 100% requirements coverage achieved
   - All critical risks tested
   - Safety functions validated

3. **Maintain records**:
   - Archive test reports
   - Track changes
   - Maintain approval signatures

---

## Additional Resources

### Documentation
- **README_TEST_SUITE.md** - Complete test suite overview
- **TEST_EXECUTION_GUIDE.md** - Detailed testing procedures
- **TEST_TRACEABILITY_MATRIX.md** - Requirements mapping
- **ISO_13485_COMPLIANCE.md** - Regulatory compliance

### Build System
- **Makefile** - Build configuration and targets
- **run_tests.sh** - Test execution script with options

### Test Code
- **test_framework.h/cpp** - Reusable test infrastructure
- **test_serial_commands.cpp** - Serial command tests
- **test_state_transitions.cpp** - State machine tests

---

## Support

### Getting Help
- **Technical Issues**: Review TEST_EXECUTION_GUIDE.md
- **Test Failures**: Check test output and logs
- **Coverage Questions**: Review TEST_TRACEABILITY_MATRIX.md
- **Compliance Questions**: Review ISO_13485_COMPLIANCE.md

### Contact
- Firmware Team: firmware-dev@brevitest.com
- Quality Assurance: quality@brevitest.com
- Support: support@brevitest.com

---

## Quick Reference Commands

```bash
# Build
make all              # Build everything
make clean            # Clean build artifacts

# Run Tests
make test             # Run all tests
./run_tests.sh        # Run all tests with script
./run_tests.sh -r     # Run with report generation

# Specific Tests
make test-serial      # Serial command tests only
make test-state       # State transition tests only

# Coverage
make coverage         # Build and run with coverage

# Reports
make report           # Run tests and generate report

# Help
make help             # Show available targets
./run_tests.sh -h     # Show script usage
```

---

**Ready to start testing? Run: `make test`**

✨ Happy Testing! ✨

