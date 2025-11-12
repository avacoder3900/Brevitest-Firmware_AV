# State Management and Transition Logging Guide

## Overview

The Brevitest firmware now includes comprehensive state transition logging and monitoring capabilities. This system allows you to track state changes, diagnose issues, and remotely monitor device state via both Serial commands and Particle Cloud functions.

## Features

### 1. **State Transition History Tracking**
- Automatically logs all state transitions with timestamps
- Maintains a circular buffer of the last 50 transitions
- Includes "from" and "to" states with millisecond-precision timestamps

### 2. **Serial Command Interface (9000 series)**
- Query current state and transition history via Serial monitor
- Clear and formatted output tables
- Easy-to-use diagnostic commands

### 3. **Particle Cloud Integration**
- Query device state remotely via Particle Cloud functions
- Retrieve transition history from anywhere
- Force state transitions for recovery/testing (use with caution)

---

## Serial Commands (9000 Series)

### State Information Commands

#### `9000` - Display State Management Help
Shows all available state management commands.

```
9000
```

#### `9001` - Show Current Device State
Displays the current state of all state machine components.

```
9001
```

**Output:**
```
╔═══════════════════════════════════════════════════════════════╗
║                 CURRENT DEVICE STATE                          ║
╠═══════════════════════════════════════════════════════════════╣
║ Device Mode:      IDLE                                        ║
║ Previous Mode:    INITIALIZING                                ║
║ Test State:       NOT_STARTED                                 ║
║ Cartridge State:  NOT_INSERTED                                ║
╚═══════════════════════════════════════════════════════════════╝
```

#### `9002` - Show Detailed State Information
Displays comprehensive state information including hardware state, cloud operations, and errors.

```
9002
```

**Output includes:**
- Primary state (current and previous modes)
- Sub-states (test state, cartridge state)
- Hardware state (detector, heater status)
- Cloud operations (pending status, elapsed time)
- Error tracking (last error message)
- Transition history statistics

### Transition History Commands

#### `9010` - Show All State Transitions
Displays the complete transition history (up to 50 entries).

```
9010
```

**Output:**
```
╔═══════════════════════════════════════════════════════════════════════════════════════╗
║                    DEVICE STATE TRANSITION HISTORY                                    ║
╠═══════════════════════════════════════════════════════════════════════════════════════╣
║ Total Transitions: 15                                                                 ║
║ Showing: 15                                                                           ║
╠═══════════════════════════════════════════════════════════════════════════════════════╣
║  #  │ Date & Time             │ From State            │ To State          │ Uptime(ms) ║
╟─────┼─────────────────────────┼───────────────────────┼───────────────────┼────────────╢
║   1 │ 2025-01-15 10:30:45     │ INITIALIZING          │ IDLE             │  123456789 ║
║   2 │ 2025-01-15 10:30:48     │ IDLE                  │ BARCODE_SCANNING │  123459012 ║
║   3 │ 2025-01-15 10:30:54     │ BARCODE_SCANNING      │ VALIDATING_CARTRIDGE │  123465234 ║
...
```

#### `9011` - Show Last 10 Transitions
Displays the 10 most recent transitions.

```
9011
```

#### `9012` - Show Last 20 Transitions
Displays the 20 most recent transitions.

```
9012
```

#### `9013` - Clear Transition History
Clears the transition history buffer.

```
9013
```

**Output:**
```
✓ State transition history cleared
```

### Query Commands

#### `9020` - Get Transition Count
Returns the total number of transitions logged.

```
9020
```

**Output:**
```
Total state transitions logged: 15
```

#### `9021` - Get Compact History String
Displays the transition history in compact format (suitable for Particle Cloud).

```
9021
```

**Output:**
```
╔═══════════════════════════════════════════════════════════════╗
║          COMPACT TRANSITION HISTORY (Cloud Format)            ║
╠═══════════════════════════════════════════════════════════════╣
Total:15|2025-01-15 10:30:45@123456789:INITIALIZING>IDLE;2025-01-15 10:30:48@123459012:IDLE>BARCODE_SCANNING;...
╚═══════════════════════════════════════════════════════════════╝
```

**Format:** `DateTime@Uptime(ms):FROM>TO`

---

## Particle Cloud Functions

All Particle Cloud functions can be called from the [Particle Console](https://console.particle.io) or via the Particle CLI.

### Function: `get_state`

**Purpose:** Get the current device state

**Parameters:** None (pass empty string)

**Returns:** Integer representing DeviceMode enum (0-10)

**Device Mode Enum Values:**
- 0 = IDLE
- 1 = INITIALIZING
- 2 = HEATING
- 3 = BARCODE_SCANNING
- 4 = VALIDATING_CARTRIDGE
- 5 = VALIDATING_MAGNETOMETER
- 6 = RUNNING_TEST
- 7 = UPLOADING_RESULTS
- 8 = RESETTING_CARTRIDGE
- 9 = STRESS_TESTING
- 10 = ERROR_STATE

**Example (Particle CLI):**
```bash
particle call <device-name> get_state ""
```

**Example (Console):**
Navigate to your device → Functions → get_state → Call with empty argument

---

### Function: `get_trans_count`

**Purpose:** Get the number of state transitions logged

**Parameters:** None (pass empty string)

**Returns:** Integer count of transitions

**Example:**
```bash
particle call <device-name> get_trans_count ""
```

---

### Function: `get_history`

**Purpose:** Get state transition history

**Parameters:** 
- Number of transitions to retrieve (1-20, default: 10)
- Pass empty string for default

**Returns:** 
- Return value: Length of history string
- History published as event: `state_history`

**Format:**
```
Total:15|timestamp:FROM>TO;timestamp:FROM>TO;...
```

**Example:**
```bash
# Get last 10 transitions (default)
particle call <device-name> get_history ""

# Get last 5 transitions
particle call <device-name> get_history "5"

# Subscribe to the event to see the data
particle subscribe state_history
```

**Parsing the output:**
```
Total:15|2025-01-15 10:30:45@123456789:INITIALIZING>IDLE;2025-01-15 10:30:48@123459012:IDLE>BARCODE_SCANNING
         ^                   ^         ^             ^    ^
         |                   |         |             |    |
      Count             DateTime    Uptime(ms)     From  To
```

**Format Details:**
- `Total:N` - Total number of transitions
- Each transition: `YYYY-MM-DD HH:MM:SS@uptime_ms:FROM_STATE>TO_STATE`
- Transitions separated by semicolon (`;`)

---

### Function: `clear_history`

**Purpose:** Clear the state transition history buffer

**Parameters:** None (pass empty string)

**Returns:** 1 on success

**Example:**
```bash
particle call <device-name> clear_history ""
```

⚠️ **Warning:** This permanently clears the transition history. Use only when needed.

---

### Function: `force_state`

**Purpose:** Force a state transition remotely

**Parameters:** Target state as integer (0-10, see DeviceMode enum above)

**Returns:** 
- 1 on success
- -1 on invalid transition

⚠️ **DANGER - USE WITH EXTREME CAUTION**

This function allows you to force state transitions remotely. Invalid transitions may cause device malfunction or undefined behavior. Only use for:
- Recovery from error states
- Testing and diagnostics
- Emergency device reset

**Example:**
```bash
# Force device to IDLE state (0)
particle call <device-name> force_state "0"

# Attempt to force to RUNNING_TEST (6) - will fail if invalid transition
particle call <device-name> force_state "6"
```

---

## Integration with console.particle.io

### Viewing Device State in Real-Time

1. **Navigate to your device** in the [Particle Console](https://console.particle.io)
2. **Click on "Functions"** in the left sidebar
3. **Call `get_state`** to see current state
4. **Call `get_history`** to retrieve transition history
5. **Subscribe to `state_history` events** to monitor transitions in real-time

### Setting Up Dashboards

You can create custom dashboards to monitor state transitions:

1. **Create a new dashboard** in Particle Console
2. **Add widgets** for:
   - Current state (call `get_state` periodically)
   - Transition count (call `get_trans_count`)
   - Latest transitions (subscribe to `state_history` events)

### Webhooks and Integrations

You can create webhooks to:
- Send state transitions to external monitoring systems
- Trigger alerts on specific state changes
- Log transitions to a database for analysis

**Example webhook payload:**
```json
{
  "event": "state_history",
  "data": "Total:15|123456789:INITIALIZING>IDLE;...",
  "published_at": "2025-01-15T10:30:00.000Z",
  "coreid": "abc123..."
}
```

---

## Diagnostic Workflows

### Troubleshooting State Issues

1. **Check current state:**
   ```
   9001
   ```

2. **Review transition history:**
   ```
   9010
   ```

3. **Look for error patterns:**
   - Rapid transitions between states
   - Stuck in ERROR_STATE
   - Unexpected transition sequences

4. **Remote recovery:**
   ```bash
   # Force device back to IDLE if stuck
   particle call <device-name> force_state "0"
   ```

### Monitoring During Testing

1. **Clear history before test:**
   ```
   9013
   ```

2. **Run your test sequence**

3. **Review transitions:**
   ```
   9010
   ```

4. **Verify expected flow:**
   - IDLE → BARCODE_SCANNING → VALIDATING_CARTRIDGE → RUNNING_TEST → UPLOADING_RESULTS → IDLE

### Remote Diagnostics

```bash
# Get current state
particle call <device-name> get_state ""

# Get transition count
particle call <device-name> get_trans_count ""

# Get recent history and subscribe to see it
particle subscribe state_history &
particle call <device-name> get_history "10"
```

---

## State Transition Reference

### Valid Transitions

**From IDLE:**
- → BARCODE_SCANNING (cartridge inserted)
- → HEATING (temperature drops)
- → STRESS_TESTING (stress test cartridge)

**From BARCODE_SCANNING:**
- → VALIDATING_CARTRIDGE (test cartridge scanned)
- → VALIDATING_MAGNETOMETER (magnetometer scanned)
- → STRESS_TESTING (stress test detected)
- → IDLE (cartridge removed)
- → ERROR_STATE (invalid barcode)

**From VALIDATING_CARTRIDGE:**
- → RUNNING_TEST (validation success)
- → RESETTING_CARTRIDGE (cartridge already used)
- → IDLE (cartridge removed)
- → ERROR_STATE (validation timeout/error)

**From RUNNING_TEST:**
- → UPLOADING_RESULTS (test complete)
- → IDLE (cartridge removed/cancelled)

**From UPLOADING_RESULTS:**
- → IDLE (upload complete)

**From ERROR_STATE:**
- → IDLE (error cleared)

---

## Technical Details

### Timestamps and Time Zones

**Timestamp Format:**
- **Date/Time:** Uses Particle Time API (`Time.now()`) for Unix timestamps
- **Format:** `YYYY-MM-DD HH:MM:SS` (e.g., "2025-01-15 10:30:45")
- **Uptime:** Also stores milliseconds since boot for relative timing

**Setting Timezone:**
The device uses UTC by default. You can set your timezone via Particle Cloud:

```bash
# Set timezone to US Central Time (CST/CDT)
particle call <device-name> set_timezone "-6,0"

# Set timezone to US Eastern Time (EST/EDT)
particle call <device-name> set_timezone "-5,0"

# Set timezone to US Pacific Time (PST/PDT)
particle call <device-name> set_timezone "-8,0"
```

Or in your device code:
```cpp
// In setup()
Time.zone(-6);  // Central Time (UTC-6)
```

**Time Synchronization:**
- Particle devices automatically sync time with cloud when connected
- Time persists across reboots (backed up by RTC)
- If device boots without cloud connection, time may be incorrect until first sync

### Data Structure

**StateTransitionEntry:**
```cpp
struct StateTransitionEntry {
    DeviceMode from_mode;           // State transitioned from
    DeviceMode to_mode;             // State transitioned to
    time_t timestamp;               // Unix timestamp (seconds since epoch)
    unsigned long millis_timestamp; // Milliseconds since boot
};
```

### Circular Buffer

- **Size:** 50 entries
- **Behavior:** When full, oldest entries are overwritten
- **Access:** Most recent is index 0, oldest is index (count-1)

### Memory Usage

- **Per entry:** ~16 bytes (2 enums + time_t + unsigned long)
- **Total buffer:** ~800 bytes (50 entries)
- **Additional overhead:** ~20 bytes (indices and counters)
- **Total:** ~820 bytes

### Performance Impact

- **Logging overhead:** < 1ms per transition
- **Query time:** < 5ms for full history
- **No impact on real-time operations**

---

## Best Practices

### 1. **Regular Monitoring**
- Check transition history after each test run
- Monitor for unexpected state changes
- Track transition patterns over time

### 2. **Error Diagnosis**
- Always check transition history when troubleshooting
- Look for repeated error transitions
- Verify expected state flow

### 3. **Testing**
- Clear history before important tests
- Document expected transition sequences
- Compare actual vs. expected transitions

### 4. **Remote Management**
- Use Particle Console for remote monitoring
- Set up alerts for ERROR_STATE transitions
- Create dashboards for production devices

### 5. **Safety**
- Use `force_state` only as last resort
- Always validate transitions are allowed
- Document any forced transitions

---

## Examples

### Example 1: Normal Test Flow

```
# Serial output from 9010
║   1 │ 2025-01-15 10:30:45     │ IDLE                 │ BARCODE_SCANNING    │ 1234567890 ║
║   2 │ 2025-01-15 10:30:48     │ BARCODE_SCANNING     │ VALIDATING_CARTRIDGE│ 1234570123 ║
║   3 │ 2025-01-15 10:30:54     │ VALIDATING_CARTRIDGE │ RUNNING_TEST        │ 1234575456 ║
║   4 │ 2025-01-15 10:32:39     │ RUNNING_TEST         │ UPLOADING_RESULTS   │ 1234680789 ║
║   5 │ 2025-01-15 10:32:44     │ UPLOADING_RESULTS    │ IDLE                │ 1234685012 ║
```

**Analysis:**
- Device was in IDLE at 10:30:45
- Cartridge inserted at 10:30:48
- Test ran from 10:30:54 to 10:32:39 (105 seconds)
- Results uploaded by 10:32:44

### Example 2: Error Recovery

```bash
# Check state - device is stuck
particle call device get_state ""
# Returns: 10 (ERROR_STATE)

# Force back to IDLE
particle call device force_state "0"
# Returns: 1 (success)

# Verify state changed
particle call device get_state ""
# Returns: 0 (IDLE)
```

### Example 3: Automated Monitoring

```bash
#!/bin/bash
# Monitor state transitions every 30 seconds

while true; do
  state=$(particle call my-device get_state "")
  count=$(particle call my-device get_trans_count "")
  echo "$(date): State=$state, Transitions=$count"
  sleep 30
done
```

---

## Troubleshooting

### Issue: History appears empty
**Solution:** Device may have just started. History clears on power cycle.

### Issue: Cannot force state transition
**Solution:** Transition may not be valid. Check state machine rules in `DeviceState.cpp`.

### Issue: Particle function times out
**Solution:** Device may not be connected to cloud. Check network status.

### Issue: Timestamps show wrong time
**Solution:** 
- Device may not be connected to cloud for time sync
- Check timezone setting: `Time.zone()` in device code or use Particle Console
- Time is in UTC by default - set timezone for local time

### Issue: Timestamps all show year 2000 or 1970
**Solution:** Device has not synced time with cloud yet. Connect to Particle Cloud to sync time.

### Issue: Uptime counter reset to low value
**Solution:** Device rebooted. Check for power issues or firmware crashes. Compare uptime values to detect reboots.

---

## Version History

- **v1.1** (2025-01-15): Added real date/time timestamps
  - Changed from milliseconds-only to Unix timestamps with date/time
  - Added uptime (milliseconds) alongside date/time
  - Format: `YYYY-MM-DD HH:MM:SS` with uptime column
  - Better diagnostics with actual timestamps

- **v1.0** (2025-01-15): Initial implementation
  - State transition logging with circular buffer
  - Serial commands (9000 series)
  - Particle Cloud functions
  - Comprehensive documentation

---

## Support

For issues or questions:
1. Check transition history with `9010`
2. Review this guide
3. Check device logs in Particle Console
4. Contact development team with:
   - Current state (`9001`)
   - Transition history (`9010`)
   - Error messages from logs

---

## Related Documentation

- [STATE_FLOWCHARTS.md](STATE_FLOWCHARTS.md) - Visual state machine diagrams
- [DeviceState.h](src/DeviceState.h) - State machine header
- [DeviceState.cpp](src/DeviceState.cpp) - State machine implementation
- [brevitest-firmware.ino](src/brevitest-firmware.ino) - Main firmware

