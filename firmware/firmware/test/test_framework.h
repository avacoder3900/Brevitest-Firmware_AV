/*!
 * @file test_framework.h
 * @brief ISO 13485 compliant unit test framework for Brevitest firmware
 * @details Provides testing infrastructure with full traceability and documentation
 * 
 * This framework supports:
 * - Test case management with unique identifiers
 * - Pass/fail criteria documentation
 * - Test execution tracking
 * - Mock hardware interfaces
 * - Test result reporting
 * 
 * Compliance: ISO 13485:2016 Section 7.3.5 (Design Verification)
 */

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Test result tracking
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int skipped_tests;
} TestResults;

// Global test results
extern TestResults g_test_results;

// Test case metadata for traceability
typedef struct {
    const char* test_id;        // Unique test identifier (e.g., "TC-CMD-001")
    const char* test_name;      // Descriptive test name
    const char* requirement_id; // Linked requirement (e.g., "REQ-CMD-001")
    const char* risk_id;        // Linked risk analysis item (e.g., "RISK-001")
    const char* category;       // Test category for organization
} TestMetadata;

// Test assertion macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("  ❌ FAIL: %s\n", message); \
            printf("     Line %d in %s\n", __LINE__, __FILE__); \
            g_test_results.failed_tests++; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_INT(expected, actual, message) \
    do { \
        if ((expected) != (actual)) { \
            printf("  ❌ FAIL: %s\n", message); \
            printf("     Expected: %d, Actual: %d\n", (expected), (actual)); \
            printf("     Line %d in %s\n", __LINE__, __FILE__); \
            g_test_results.failed_tests++; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual, message) \
    do { \
        if (strcmp((expected), (actual)) != 0) { \
            printf("  ❌ FAIL: %s\n", message); \
            printf("     Expected: %s, Actual: %s\n", (expected), (actual)); \
            printf("     Line %d in %s\n", __LINE__, __FILE__); \
            g_test_results.failed_tests++; \
            return false; \
        } \
    } while(0)

#define TEST_ASSERT_TRUE(condition, message) \
    TEST_ASSERT((condition), message)

#define TEST_ASSERT_FALSE(condition, message) \
    TEST_ASSERT(!(condition), message)

#define TEST_ASSERT_NULL(pointer, message) \
    TEST_ASSERT((pointer) == NULL, message)

#define TEST_ASSERT_NOT_NULL(pointer, message) \
    TEST_ASSERT((pointer) != NULL, message)

// Test case definition macro with metadata
#define TEST_CASE(test_func, metadata) \
    do { \
        printf("\n▶ Running: %s [%s]\n", metadata.test_name, metadata.test_id); \
        printf("  Requirement: %s | Risk: %s | Category: %s\n", \
               metadata.requirement_id, metadata.risk_id, metadata.category); \
        g_test_results.total_tests++; \
        bool result = test_func(); \
        if (result) { \
            printf("  ✅ PASS: %s\n", metadata.test_name); \
            g_test_results.passed_tests++; \
        } \
    } while(0)

#define TEST_SKIP(test_name, reason) \
    do { \
        printf("\n▶ Skipping: %s\n", test_name); \
        printf("  ⏭  SKIP: %s\n", reason); \
        g_test_results.total_tests++; \
        g_test_results.skipped_tests++; \
    } while(0)

// Test suite management
#define BEGIN_TEST_SUITE(suite_name) \
    void run_##suite_name() { \
        printf("\n═══════════════════════════════════════════════════════════════\n"); \
        printf("Test Suite: %s\n", #suite_name); \
        printf("═══════════════════════════════════════════════════════════════\n");

#define END_TEST_SUITE() \
        printf("\n═══════════════════════════════════════════════════════════════\n"); \
    }

// Test setup and teardown
typedef void (*TestSetupFunc)(void);
typedef void (*TestTeardownFunc)(void);

// Mock framework support
typedef struct {
    bool mock_enabled;
    int mock_return_value;
    int call_count;
} MockFunction;

// Mock helpers
#define INIT_MOCK(mock) \
    do { \
        (mock).mock_enabled = false; \
        (mock).mock_return_value = 0; \
        (mock).call_count = 0; \
    } while(0)

#define ENABLE_MOCK(mock, return_val) \
    do { \
        (mock).mock_enabled = true; \
        (mock).mock_return_value = return_val; \
    } while(0)

#define DISABLE_MOCK(mock) \
    do { \
        (mock).mock_enabled = false; \
    } while(0)

#define MOCK_CALLED(mock) \
    ((mock).call_count)

// Test reporting
void print_test_summary(void);
void initialize_test_framework(void);
void generate_test_report(const char* filename);

// Risk levels for risk-based testing
typedef enum {
    RISK_CRITICAL,  // Safety-related, could cause harm
    RISK_HIGH,      // Major functionality impact
    RISK_MEDIUM,    // Moderate functionality impact
    RISK_LOW        // Minor functionality impact
} RiskLevel;

// Test priority based on risk
typedef enum {
    PRIORITY_P1,    // Must run every build (critical/high risk)
    PRIORITY_P2,    // Run before release (medium risk)
    PRIORITY_P3     // Run periodically (low risk)
} TestPriority;

#endif // TEST_FRAMEWORK_H

