# Makefile for Brevitest Firmware Unit Tests
# ISO 13485:2016 Compliant Testing Framework

# Compiler settings
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -std=c11 -g
CXXFLAGS = -Wall -Wextra -std=c++11 -g
INCLUDES = -I../src
LDFLAGS =

# Directories
SRC_DIR = .
BUILD_DIR = build
REPORT_DIR = reports

# Source files
FRAMEWORK_SRCS = test_framework.cpp
SERIAL_CMD_SRCS = test_serial_commands.cpp
STATE_TRANS_SRCS = test_state_transitions.cpp
MAIN_SRC = test_main.cpp

# Object files
FRAMEWORK_OBJS = $(BUILD_DIR)/test_framework.o
SERIAL_CMD_OBJS = $(BUILD_DIR)/test_serial_commands.o
STATE_TRANS_OBJS = $(BUILD_DIR)/test_state_transitions.o
MAIN_OBJ = $(BUILD_DIR)/test_main.o

ALL_OBJS = $(FRAMEWORK_OBJS) $(SERIAL_CMD_OBJS) $(STATE_TRANS_OBJS) $(MAIN_OBJ)

# Executables
TEST_RUNNER = test_runner

# Default target
all: directories $(TEST_RUNNER)

# Create necessary directories
directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(REPORT_DIR)

# Build test runner
$(TEST_RUNNER): $(ALL_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "✅ Test runner built successfully"

# Build object files
$(BUILD_DIR)/test_framework.o: test_framework.cpp test_framework.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_serial_commands.o: test_serial_commands.cpp test_serial_commands.h test_framework.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_state_transitions.o: test_state_transitions.cpp test_state_transitions.h test_framework.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/test_main.o: test_main.cpp test_framework.h test_serial_commands.h test_state_transitions.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Run all tests
test: $(TEST_RUNNER)
	@echo "Running all tests..."
	./$(TEST_RUNNER)

# Run specific test suites
test-serial: $(TEST_RUNNER)
	@echo "Running serial command tests..."
	./$(TEST_RUNNER) -s

test-state: $(TEST_RUNNER)
	@echo "Running state transition tests..."
	./$(TEST_RUNNER) -t

# Generate test report
report: $(TEST_RUNNER)
	@echo "Running tests and generating report..."
	./$(TEST_RUNNER) -r

# Run tests with coverage (requires gcov)
coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: CXXFLAGS += -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += -lgcov --coverage
coverage: clean all
	./$(TEST_RUNNER)
	@echo "Generating coverage report..."
	gcov $(FRAMEWORK_SRCS) $(SERIAL_CMD_SRCS) $(STATE_TRANS_SRCS)
	@echo "Coverage data generated in .gcov files"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TEST_RUNNER)
	rm -f *.gcov *.gcda *.gcno
	@echo "✅ Clean complete"

# Clean everything including reports
clean-all: clean
	rm -rf $(REPORT_DIR)
	@echo "✅ All clean"

# Install dependencies (Ubuntu/Debian)
install-deps:
	@echo "Installing dependencies..."
	sudo apt-get update
	sudo apt-get install -y build-essential gcc g++ make

# Help target
help:
	@echo "Brevitest Firmware Unit Test Suite - Make Targets"
	@echo "================================================="
	@echo ""
	@echo "Building:"
	@echo "  make                  Build test runner"
	@echo "  make all              Build test runner (same as make)"
	@echo ""
	@echo "Running Tests:"
	@echo "  make test             Run all tests"
	@echo "  make test-serial      Run serial command tests only"
	@echo "  make test-state       Run state transition tests only"
	@echo "  make report           Run tests and generate report"
	@echo ""
	@echo "Coverage:"
	@echo "  make coverage         Build with coverage and run tests"
	@echo ""
	@echo "Cleaning:"
	@echo "  make clean            Remove build artifacts"
	@echo "  make clean-all        Remove build artifacts and reports"
	@echo ""
	@echo "Other:"
	@echo "  make install-deps     Install required dependencies (Ubuntu/Debian)"
	@echo "  make help             Show this help message"
	@echo ""

# Phony targets
.PHONY: all directories test test-serial test-state report coverage clean clean-all install-deps help

# End of Makefile

