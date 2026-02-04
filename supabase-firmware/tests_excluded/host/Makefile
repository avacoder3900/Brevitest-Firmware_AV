# Makefile for Brevitest host-based unit tests
# Works on Linux, macOS, and Windows with MinGW

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -I../../src

SRC_DIR = ../../src
TEST_SRCS = test_host.cpp
FIRMWARE_SRCS = $(SRC_DIR)/StateMachine.cpp $(SRC_DIR)/DataTypes.cpp

TARGET = test_host

# Default target
all: $(TARGET)

$(TARGET): $(TEST_SRCS) $(FIRMWARE_SRCS)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Run tests
test: $(TARGET)
	./$(TARGET)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TARGET).exe *.o *.obj

# Verbose build
verbose: CXXFLAGS += -v
verbose: $(TARGET)

.PHONY: all test clean verbose
