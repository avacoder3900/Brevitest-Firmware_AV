/**
 * @file Particle.h
 * @brief Mock Particle SDK header for host-side unit testing
 *
 * Provides stub implementations of Particle APIs used by firmware modules.
 * This allows compiling and testing pure logic on a desktop without hardware.
 */

#ifndef PARTICLE_H
#define PARTICLE_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Mark that we're in a mock/test environment
// (Note: some firmware files check #ifdef PARTICLE — we define it so
// those code paths get tested too, but with mock implementations)

// ============================================================================
// LOGGING MOCK
// ============================================================================

class MockLogger {
public:
    MockLogger() {}
    MockLogger(const char*) {}

    void info(const char* fmt, ...) { (void)fmt; }
    void warn(const char* fmt, ...) { (void)fmt; }
    void error(const char* fmt, ...) { (void)fmt; }
    void trace(const char* fmt, ...) { (void)fmt; }
};

// Global Log object (used by firmware as Log.info(), etc.)
static MockLogger Log;

// Logger class used by SupabaseClient
typedef MockLogger Logger;

// ============================================================================
// TIME MOCK
// ============================================================================

static uint32_t _mock_millis = 0;
static uint32_t _mock_time = 1706500000;

inline uint32_t millis() { return _mock_millis; }
inline void delay(uint32_t ms) { _mock_millis += ms; }
inline void delayMicroseconds(uint32_t us) { (void)us; }

// Mock Time class
class MockTime {
public:
    bool isValid() const { return true; }
    uint32_t now() const { return _mock_time; }
};

static MockTime Time;

// ============================================================================
// PIN/GPIO MOCK
// ============================================================================

#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define INPUT_PULLDOWN 3
#define HIGH 1
#define LOW 0

inline void pinMode(uint16_t, uint16_t) {}
inline void digitalWrite(uint16_t, uint8_t) {}
inline int digitalRead(uint16_t) { return LOW; }
inline int analogRead(uint16_t) { return 0; }

// Pin definitions (M-SoM)
#define D0  0
#define D1  1
#define D2  2
#define D3  3
#define D4  4
#define D5  5
#define D6  6
#define D7  7
#define D8  8
#define D9  9
#define D10 10
#define D11 11
#define D12 12
#define D13 13
#define D19 19
#define D20 20
#define D21 21
#define D22 22
#define D23 23
#define D24 24
#define D25 25
#define D26 26
#define A0  30
#define A1  31
#define A2  32
#define A5  35

// ============================================================================
// STRING CLASS MOCK (minimal)
// ============================================================================

class String {
public:
    String() : _buf(nullptr), _len(0) {}
    String(const char* s) {
        _len = strlen(s);
        _buf = (char*)malloc(_len + 1);
        strcpy(_buf, s);
    }
    ~String() { free(_buf); }

    const char* c_str() const { return _buf ? _buf : ""; }
    size_t length() const { return _len; }

private:
    char* _buf;
    size_t _len;
};

// ============================================================================
// WIRE (I2C) MOCK
// ============================================================================

class MockWire {
public:
    void begin() {}
    void beginTransmission(uint8_t) {}
    uint8_t endTransmission() { return 0; }
    uint8_t requestFrom(uint8_t, uint8_t) { return 0; }
    size_t write(uint8_t) { return 1; }
    size_t write(const uint8_t*, size_t len) { return len; }
    int available() { return 0; }
    int read() { return 0; }
};

static MockWire Wire;

// ============================================================================
// SERIAL MOCK
// ============================================================================

class MockSerial {
public:
    void begin(unsigned long) {}
    int available() { return 0; }
    int read() { return -1; }
    size_t print(const char*) { return 0; }
    size_t println(const char*) { return 0; }
    size_t printf(const char*, ...) { return 0; }
    operator bool() { return true; }
};

static MockSerial Serial;
static MockSerial Serial1;

#endif // PARTICLE_H
