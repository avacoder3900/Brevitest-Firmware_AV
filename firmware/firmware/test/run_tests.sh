#!/bin/bash
# Brevitest Firmware Test Runner Script
# ISO 13485:2016 Compliant

# Script configuration
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
TEST_RUNNER="$SCRIPT_DIR/test_runner"
REPORT_DIR="$SCRIPT_DIR/reports"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}ℹ${NC} $1"
}

print_success() {
    echo -e "${GREEN}✓${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}⚠${NC} $1"
}

print_error() {
    echo -e "${RED}✗${NC} $1"
}

# Function to check if test runner exists
check_test_runner() {
    if [ ! -f "$TEST_RUNNER" ]; then
        print_error "Test runner not found. Building..."
        cd "$SCRIPT_DIR"
        make clean
        make all
        if [ $? -ne 0 ]; then
            print_error "Failed to build test runner"
            exit 1
        fi
        print_success "Test runner built successfully"
    fi
}

# Function to create report directory
create_report_dir() {
    if [ ! -d "$REPORT_DIR" ]; then
        mkdir -p "$REPORT_DIR"
        print_info "Created report directory: $REPORT_DIR"
    fi
}

# Function to display help
show_help() {
    echo "Brevitest Firmware Test Runner"
    echo "==============================="
    echo ""
    echo "Usage: $0 [options] [test_suite]"
    echo ""
    echo "Test Suites:"
    echo "  all                Run all test suites (default)"
    echo "  serial_commands    Run serial command tests only"
    echo "  state_transitions  Run state transition tests only"
    echo ""
    echo "Options:"
    echo "  -r, --report       Generate test report"
    echo "  -v, --verbose      Verbose output"
    echo "  -c, --coverage     Run with code coverage analysis"
    echo "  -h, --help         Display this help message"
    echo ""
    echo "Examples:"
    echo "  $0                           # Run all tests"
    echo "  $0 serial_commands           # Run serial command tests"
    echo "  $0 -r                        # Run all tests and generate report"
    echo "  $0 -r serial_commands        # Run serial tests with report"
    echo "  $0 --coverage                # Run all tests with coverage"
    echo ""
}

# Parse command line arguments
GENERATE_REPORT=false
VERBOSE=false
COVERAGE=false
TEST_SUITE="all"

while [[ $# -gt 0 ]]; do
    case $1 in
        -r|--report)
            GENERATE_REPORT=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -c|--coverage)
            COVERAGE=true
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        all|serial_commands|state_transitions)
            TEST_SUITE=$1
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Main execution
main() {
    print_info "Brevitest Firmware Unit Test Suite"
    print_info "ISO 13485:2016 Compliant"
    echo ""
    
    # Check prerequisites
    check_test_runner
    create_report_dir
    
    # Build coverage version if requested
    if [ "$COVERAGE" = true ]; then
        print_info "Building with coverage support..."
        cd "$SCRIPT_DIR"
        make coverage
        if [ $? -ne 0 ]; then
            print_error "Coverage build failed"
            exit 1
        fi
        print_success "Coverage build complete"
        echo ""
    fi
    
    # Prepare test runner arguments
    TEST_ARGS=""
    
    case $TEST_SUITE in
        serial_commands)
            TEST_ARGS="-s"
            print_info "Running serial command tests..."
            ;;
        state_transitions)
            TEST_ARGS="-t"
            print_info "Running state transition tests..."
            ;;
        all)
            TEST_ARGS="-a"
            print_info "Running all test suites..."
            ;;
    esac
    
    if [ "$GENERATE_REPORT" = true ]; then
        TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
        REPORT_FILE="$REPORT_DIR/test_report_$TIMESTAMP.txt"
        TEST_ARGS="$TEST_ARGS -r $REPORT_FILE"
        print_info "Report will be saved to: $REPORT_FILE"
    fi
    
    if [ "$VERBOSE" = true ]; then
        TEST_ARGS="$TEST_ARGS -v"
    fi
    
    echo ""
    
    # Run tests
    cd "$SCRIPT_DIR"
    $TEST_RUNNER $TEST_ARGS
    TEST_EXIT_CODE=$?
    
    echo ""
    
    # Check results
    if [ $TEST_EXIT_CODE -eq 0 ]; then
        print_success "All tests passed!"
        
        if [ "$GENERATE_REPORT" = true ]; then
            print_info "Test report generated: $REPORT_FILE"
        fi
        
        if [ "$COVERAGE" = true ]; then
            print_info "Coverage data available in .gcov files"
        fi
        
        echo ""
        print_success "Firmware verification complete - PASSED"
        exit 0
    else
        print_error "Some tests failed!"
        
        if [ "$GENERATE_REPORT" = true ]; then
            print_info "Test report with failures: $REPORT_FILE"
        fi
        
        echo ""
        print_error "Firmware verification complete - FAILED"
        exit 1
    fi
}

# Run main function
main

