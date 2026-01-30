# Hardware Timing Requirements

This document specifies all timing-critical operations in the Brevitest firmware.
All timings are derived from legacy firmware analysis and hardware specifications.

## Motor Controller

| Operation | Timing | Constant | Notes |
|-----------|--------|----------|-------|
| Minimum step delay | 250us | `MOTOR_MINIMUM_STEP_DELAY` | Fastest safe motor speed |
| Reset step delay | 290us | `MOTOR_RESET_STEP_DELAY` | Homing/reset movement |
| Fast step delay | 290us | `MOTOR_FAST_STEP_DELAY` | Fast positioning |
| Bounce step delay | 350us | `MOTOR_BOUNCE_STEP_DELAY` | Bounce movement |
| Oscillation step delay | 350us | `MOTOR_OSCILLATION_STEP_DELAY` | Mixing oscillation |
| Slow step delay | 600us | `MOTOR_SLOW_STEP_DELAY` | Precise/slow movement |
| Sensor step delay | 1000us | `MOTOR_SENSOR_STEP_DELAY` | During spectro readings |
| Motor wake delay | 10000us | N/A | After `motorWake()` HIGH, wait before stepping |
| Limit switch debounce | 10000us | N/A | Double-check limit after initial trigger |

### Motor Step Timing Details
- Each step consists of: HIGH -> delay -> LOW -> delay
- Total step duration = 2 * step_delay
- Step pulse must be HIGH for minimum 1us (A4988 specification)

### Position Tracking
- Distance per 1/8 step: 25 microns (`MOTOR_MICRONS_PER_EIGHTH_STEP`)
- Maximum position: 45000 microns (`STAGE_POSITION_LIMIT`)
- Reset position moves -60000 steps (overshoots to ensure limit hit)

## Heater Controller

| Operation | Timing | Constant | Notes |
|-----------|--------|----------|-------|
| ADC stabilization | 5000us | `HEATER_STABILIZATION_TIME_US` | Wait after heater off before reading thermistor |
| PWM frequency | 150Hz | `HEATER_PWM_FREQUENCY` | Heater PWM base frequency |
| Control interval | 1000ms | `HEATER_CONTROL_INTERVAL` | PID update interval |
| Pulse duration | 800ms | `HEATER_PULSE_DURATION` | Default heater pulse |
| Ready debounce | 5000ms | `HEATER_READY_DEBOUNCE_DELAY` | Temperature stable confirmation |
| Failsafe timing | 2000ms | `HEATER_FAILSAFE_TIMING` | Maximum continuous heater ON |

### Temperature Safety
- Maximum temperature: 60.0C (600 in 10x format)
- Ready tolerance: +/- 1.0C (`HEATER_READY_TEMP_DELTA = 10`)
- Valid ADC range: 550-890 raw values

## Barcode Scanner

| Operation | Timing | Constant | Notes |
|-----------|--------|----------|-------|
| Trigger delay | 100000us | `BARCODE_DELAY_US` | Wait between scan attempts |
| Read timeout | 1000ms | `BARCODE_READ_TIMEOUT` | Maximum wait for scan complete |
| Serial baud rate | 9600 | `BARCODE_SERIAL_BAUD` | Serial1 communication |
| Serial read delay | 1000us | `BARCODE_SERIAL_DELAY_US` | Inter-character delay |

### Scan Protocol
1. Open Serial1 at 9600 baud
2. Set trigger LOW (active)
3. Wait for ready HIGH or timeout
4. Read characters from Serial1
5. Set trigger HIGH (inactive)
6. Close Serial1

## Buzzer Controller

| Operation | Timing | Constant | Notes |
|-----------|--------|----------|-------|
| Standard beep | 1000ms | `BUZZER_DURATION` | Default beep at 600Hz |
| Insert notification | 200ms | `BUZZER_INSERT_DURATION` | Cartridge insert at 620Hz |
| Remove notification | 500ms | `BUZZER_REMOVE_DURATION` | Cartridge remove at 620Hz |
| Alert duration | 500ms | `BUZZER_ALERT_DURATION` | Warning at 850Hz |
| Alert period | 4000ms | `BUZZER_ALERT_PERIOD` | Repeat interval for alerts |
| Problem duration | 100ms | `BUZZER_PROBLEM_DURATION` | Problem at 620Hz |
| Problem period | 777ms | `BUZZER_PROBLEM_PERIOD` | Repeat interval for problems |

### Alert Patterns
- Periodic alerts use Particle Timer for non-blocking operation
- Alert frequencies: 600Hz (standard), 620Hz (insert/remove/problem), 850Hz (alert)

## LED Controller

| Operation | Timing | Constant | Notes |
|-----------|--------|----------|-------|
| Warmup delay | 1000ms | `LED_WARMUP_DELAY_MS` | LED warmup time |
| Default duration | 500ms | `LED_DURATION` | Default LED on time |

### LED Patterns (via Particle LEDStatus)
- **Don't Touch**: Red solid, normal speed
- **Insert**: Green fade, normal speed
- **Remove**: Green blink, slow speed
- **Error**: Red solid, high priority

## Laser Controller

| Operation | Timing | Constant | Notes |
|-----------|--------|----------|-------|
| PWM on time | 20000us | `LASER_PWM_ON_US` | Pulse on duration |
| PWM cycle time | 2000us | `LASER_PWM_TOTAL_US` | Total cycle (note: appears inverted in legacy) |
| Warmup delay | 10ms | N/A | Wait after laser enable before reading |
| Maximum continuous | 30000ms | `LASER_MAX_CONTINUOUS_ON_MS` | Safety cutoff |

### Laser Channels
- Channel A: D5 (`PIN_LASER_A`)
- Channel B: D6 (`PIN_LASER_B`)
- Channel C: D7 (`PIN_LASER_C`)
- All channels are digital on/off (no PWM in current implementation)

## Spectrophotometer (AS7341)

| Operation | Timing | Constant | Notes |
|-----------|--------|----------|-------|
| Measurement timeout | 2000ms | `SPECTRO_TIMEOUT` | Maximum wait for reading |
| Channel stabilization | 2ms | N/A | After mux channel selection |
| I2C timeout | 100ms | `I2C_TIMEOUT_MS` | I2C operation timeout |
| Integration time | ~139ms | Default with ATIME=49, ASTEP=999 |

### Integration Time Formula
```
Integration Time (us) = (ATIME + 1) * (ASTEP + 1) * 2.78
Default: (49+1) * (999+1) * 2.78 = 139,000us = 139ms
```

### Reading Sequence
1. Select channel via PCA9536 mux (0x41)
2. Start F1-F4/Clear/NIR measurement
3. Wait for measurement complete
4. Read spectral data
5. Start F5-F8/Clear/NIR measurement
6. Wait for measurement complete
7. Read spectral data

### Coordinated Scanning (with motor)
For continuous scanning during stage movement:
- Calculate reading distance: `(integration_time_us / (2 * step_delay)) * 25um`
- Move stage while sensor integrates
- Perform two passes per segment (F1-F4, F5-F8)

## I2C Communication

| Operation | Timing | Constant | Notes |
|-----------|--------|----------|-------|
| Clock speed | 400kHz | `I2C_CLOCK_SPEED` | Fast mode |
| Init delay | 10000us | `I2C_INIT_DELAY_US` | After Wire.begin() |
| Operation timeout | 100ms | `I2C_TIMEOUT_MS` | Per-operation timeout |

### I2C Addresses
- AS7341 spectrophotometer: 0x39
- PCA9536 mux: 0x41

## Detector Debouncing

| Operation | Timing | Constant | Notes |
|-----------|--------|----------|-------|
| Cartridge detect debounce | 10ms | `DETECTOR_DEBOUNCE_DELAY` | Switch debounce |
| Limit switch debounce | 10ms | `DETECTOR_DEBOUNCE_DELAY` | Also used for limit |

## Cloud Operations

| Operation | Timing | Constant | Notes |
|-----------|--------|----------|-------|
| Validation timeout | 45000ms | `VALIDATION_TIMEOUT_MS` | Cartridge validation |
| Maximum timeout | 60000ms | `VALIDATION_MAX_TIMEOUT_MS` | Absolute maximum |
| Retry backoff base | 5000ms | `VALIDATION_RETRY_BACKOFF_BASE` | Exponential backoff |
| Recently tested cooldown | 30000ms | `RECENT_TEST_COOLDOWN_MS` | Prevent re-scan |
| Particle cloud delay | 5000ms | `PARTICLE_CLOUD_DELAY` | Between cloud ops |
| Publish retry delay | 22000ms | `PUBSUB_RETRY_DELAY` | Failed publish retry |

---

## Critical Timing Patterns

### Safe Motor Operation
```cpp
// Wake motor, wait for driver ready
HAL::motorWake();
delayMicroseconds(10000);  // 10ms wake time

// Step with minimum delay
digitalWrite(PIN_MOTOR_STEP, HIGH);
delayMicroseconds(MOTOR_MINIMUM_STEP_DELAY);  // 250us
digitalWrite(PIN_MOTOR_STEP, LOW);
delayMicroseconds(MOTOR_MINIMUM_STEP_DELAY);  // 250us
```

### Safe Heater Reading
```cpp
// Turn off heater before ADC read
HAL::setHeaterOff();
delayMicroseconds(HEATER_STABILIZATION_TIME_US);  // 5000us

// Read thermistor
uint16_t raw = analogRead(PIN_HEATER_THERMISTOR);
```

### Limit Switch Double-Check
```cpp
if (HAL::readLimitSwitchRaw()) {
    // Double-check with debounce
    delayMicroseconds(10000);  // 10ms
    if (HAL::readLimitSwitchRaw()) {
        // Confirmed at limit
        position = 0;
    }
}
```

---

*Document generated for Supabase Firmware Rewrite Project - BETA-020*
*Last updated: January 2026*
