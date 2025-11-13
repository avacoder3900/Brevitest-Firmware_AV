/*!
 * @file test_main.cpp
 * @brief Main test runner for Brevitest firmware unit tests
 * @details Executes all test suites and generates reports
 * 
 * This is the entry point for the test suite. It coordinates:
 * - Test framework initialization
 * - Test suite execution
 * - Result reporting
 * - Report generation
 * 
 * Compliance: ISO 13485:2016 Section 7.3.5
 */

#include "test_framework.h"
#include "test_serial_commands.h"
#include "test_state_transitions.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <cstdlib>

// Test suite runners (declared in respective files)
extern void run_serial_commands(void);
extern void run_state_transitions(void);

// Command line arguments
typedef struct {
    bool run_all;
    bool run_serial_commands;
    bool run_state_transitions;
    bool generate_report;
    bool verbose;
    char report_filename[256];
} TestOptions;

void print_usage(const char* program_name) {
    printf("Usage: %s [options]\n", program_name);
    printf("\nOptions:\n");
    printf("  -a, --all              Run all test suites (default)\n");
    printf("  -s, --serial           Run serial command tests only\n");
    printf("  -t, --state            Run state transition tests only\n");
    printf("  -r, --report [file]    Generate test report (default: test_report_<timestamp>.txt)\n");
    printf("  -v, --verbose          Verbose output\n");
    printf("  -h, --help             Display this help message\n");
    printf("\nExamples:\n");
    printf("  %s                     # Run all tests\n", program_name);
    printf("  %s -s                  # Run serial command tests only\n", program_name);
    printf("  %s -t -r               # Run state tests and generate report\n", program_name);
    printf("  %s -a -r my_report.txt # Run all tests, save to my_report.txt\n", program_name);
    printf("\n");
}

void parse_arguments(int argc, char* argv[], TestOptions* options) {
    // Default options
    options->run_all = true;
    options->run_serial_commands = false;
    options->run_state_transitions = false;
    options->generate_report = false;
    options->verbose = false;
    options->report_filename[0] = '\0';
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0) {
            options->run_all = true;
            options->run_serial_commands = false;
            options->run_state_transitions = false;
        }
        else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--serial") == 0) {
            options->run_all = false;
            options->run_serial_commands = true;
        }
        else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--state") == 0) {
            options->run_all = false;
            options->run_state_transitions = true;
        }
        else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--report") == 0) {
            options->generate_report = true;
            // Check if next argument is a filename
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                strncpy(options->report_filename, argv[i + 1], sizeof(options->report_filename) - 1);
                i++; // Skip the filename argument
            }
        }
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            options->verbose = true;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        }
        else {
            printf("Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            exit(1);
        }
    }
}

void generate_default_report_filename(char* filename, size_t size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    snprintf(filename, size, "test_report_%04d%02d%02d_%02d%02d%02d.txt",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
}

int main(int argc, char* argv[]) {
    TestOptions options;
    
    // Parse command line arguments
    parse_arguments(argc, argv, &options);
    
    // Initialize test framework
    initialize_test_framework();
    
    // Run selected test suites
    if (options.run_all || options.run_serial_commands) {
        run_serial_commands();
    }
    
    if (options.run_all || options.run_state_transitions) {
        run_state_transitions();
    }
    
    // Print summary
    print_test_summary();
    
    // Generate report if requested
    if (options.generate_report) {
        char report_filename[256];
        
        if (options.report_filename[0] == '\0') {
            // Generate default filename
            generate_default_report_filename(report_filename, sizeof(report_filename));
        } else {
            strncpy(report_filename, options.report_filename, sizeof(report_filename));
        }
        
        generate_test_report(report_filename);
    }
    
    // Return exit code based on test results
    if (g_test_results.failed_tests > 0) {
        printf("\n⚠️  WARNING: %d test(s) failed\n", g_test_results.failed_tests);
        printf("Review the output above for details.\n");
        printf("This build is NOT verified for release.\n\n");
        return 1; // Failure exit code
    }
    
    printf("\n✨ SUCCESS: All tests passed\n");
    printf("Firmware is verified and ready for next phase.\n\n");
    return 0; // Success exit code
}

