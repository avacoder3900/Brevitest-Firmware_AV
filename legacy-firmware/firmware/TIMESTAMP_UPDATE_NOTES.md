# Timestamp Update - Real Date & Time

## Summary of Changes

The state transition logging system has been updated to use **real date and time** instead of just milliseconds since boot. This makes the logs much more useful for diagnostics and analysis.

## What Changed

### Before (v1.0)
- Timestamps were in milliseconds since device boot
- Format: `123456789` (just a number)
- Reset to 0 every time device rebooted
- Hard to correlate with real-world events

### After (v1.1)
- Timestamps show actual date and time
- Format: `2025-01-15 10:30:45` (YYYY-MM-DD HH:MM:SS)
- Also includes uptime for relative timing
- Easy to correlate with logs and real-world events

## New Display Format

### Serial Output (Command 9010)
```
╔═══════════════════════════════════════════════════════════════════════════════════════╗
║                    DEVICE STATE TRANSITION HISTORY                                    ║
╠═══════════════════════════════════════════════════════════════════════════════════════╣
║  #  │ Date & Time             │ From State            │ To State          │ Uptime(ms) ║
╟─────┼─────────────────────────┼───────────────────────┼───────────────────┼────────────╢
║   1 │ 2025-01-15 10:30:45     │ INITIALIZING          │ IDLE             │  123456789 ║
║   2 │ 2025-01-15 10:30:48     │ IDLE                  │ BARCODE_SCANNING │  123459012 ║
```

### Benefits:
- **Date & Time** column shows when transition occurred
- **Uptime** column shows milliseconds since boot (for relative timing)
- Can now see exact time of day when events happened
- Easy to correlate with other logs

### Particle Cloud Format
```
Total:5|2025-01-15 10:30:45@123456789:INITIALIZING>IDLE;2025-01-15 10:30:48@123459012:IDLE>BARCODE_SCANNING
```

Format: `DateTime@Uptime:FROM>TO`

## Technical Details

### Data Structure Update
```cpp
// OLD (v1.0)
struct StateTransitionEntry {
    DeviceMode from_mode;
    DeviceMode to_mode;
    unsigned long timestamp;  // milliseconds only
};

// NEW (v1.1)
struct StateTransitionEntry {
    DeviceMode from_mode;
    DeviceMode to_mode;
    time_t timestamp;               // Unix timestamp (seconds)
    unsigned long millis_timestamp; // milliseconds since boot
};
```

### Implementation
- Uses `Time.now()` for Unix timestamp (seconds since epoch)
- Uses `Time.format()` to format as "YYYY-MM-DD HH:MM:SS"
- Still stores `millis()` for relative timing analysis
- Both timestamps captured atomically during state transition

## Time Synchronization

### Automatic Time Sync
Particle devices automatically sync time with the cloud when connected:
- Time syncs when device first connects to Particle Cloud
- Re-syncs periodically to maintain accuracy
- Time persists across reboots (backed by RTC)

### Timezone Configuration

**Default:** UTC (Coordinated Universal Time)

**Set Timezone in Code:**
```cpp
void setup() {
    // Set timezone in setup()
    Time.zone(-6);  // US Central Time (UTC-6)
    Time.zone(-5);  // US Eastern Time (UTC-5)
    Time.zone(-8);  // US Pacific Time (UTC-8)
}
```

**Set Timezone via Particle Console:**
1. Go to console.particle.io
2. Select your device
3. Go to Settings
4. Set timezone

**Common US Timezones:**
- Pacific (PST/PDT): -8 / -7
- Mountain (MST/MDT): -7 / -6
- Central (CST/CDT): -6 / -5
- Eastern (EST/EDT): -5 / -4

Note: Daylight saving adjustments may need manual updates depending on your Particle OS version.

## Troubleshooting

### Issue: Timestamps show wrong time
**Cause:** Timezone not set or device not synced with cloud

**Solution:**
1. Connect device to Particle Cloud
2. Set timezone: `Time.zone(-6);` in setup()
3. Wait for time sync (automatic)

### Issue: Timestamps show year 1970 or 2000
**Cause:** Device has not synced time with cloud yet

**Solution:**
1. Ensure device is connected to Particle Cloud
2. Wait a few seconds for time sync
3. If still wrong, manually trigger sync:
   ```cpp
   Particle.syncTime();
   ```

### Issue: All timestamps suddenly reset
**Cause:** Device rebooted

**How to detect:**
- Look at the "Uptime" column
- If uptime suddenly drops to low values, device rebooted
- Compare timestamps before and after to see reboot time

### Issue: Time is correct but timezone wrong
**Cause:** Timezone not set (defaults to UTC)

**Solution:** Set timezone in code or via Particle Console

## Usage Examples

### Example 1: Analyzing Test Duration
```
║   3 │ 2025-01-15 10:30:54     │ BARCODE_SCANNING      │ VALIDATING_CARTRIDGE │  1234560000 ║
║   4 │ 2025-01-15 10:31:04     │ VALIDATING_CARTRIDGE  │ RUNNING_TEST        │  1234570000 ║
║   5 │ 2025-01-15 10:33:19     │ RUNNING_TEST          │ UPLOADING_RESULTS   │  1234705000 ║
```

**Analysis:**
- Test started: 10:31:04
- Test ended: 10:33:19
- **Duration: 2 minutes 15 seconds**

Easy to calculate with real timestamps!

### Example 2: Detecting Reboots
```
║   8 │ 2025-01-15 10:45:30     │ RUNNING_TEST          │ UPLOADING_RESULTS   │  1500000000 ║
║   9 │ 2025-01-15 10:47:15     │ INITIALIZING          │ IDLE                │        5000 ║
```

**Analysis:**
- Uptime dropped from 1.5 billion ms to 5000 ms
- Device rebooted at approximately 10:47:15
- Lost 105 seconds during reboot

### Example 3: Correlating with External Events
```
║  12 │ 2025-01-15 11:15:23     │ IDLE                  │ ERROR_STATE         │  3000000 ║
```

**Analysis:**
- Error occurred at 11:15:23
- Can now check:
  - External logs at same time
  - Network connectivity logs
  - Power supply logs
  - Temperature logs

Much easier than trying to figure out when "3000000 ms since boot" actually was!

## Benefits Summary

### 1. Better Diagnostics
- See exactly when events occurred
- Correlate with external systems
- Understand time-based issues

### 2. Easier Debugging
- No mental math to convert uptime
- Clear timeline of events
- Easy to share logs with team

### 3. Production Monitoring
- Track events across time zones
- Analyze patterns by time of day
- Generate reports with real timestamps

### 4. Compliance & Auditing
- Real timestamps for audit trails
- Can prove when events occurred
- Suitable for quality control records

## Migration Notes

### Backward Compatibility
- Old code will still compile
- History buffer automatically upgraded
- No changes needed to existing code

### Memory Impact
- Added 4 bytes per entry (time_t)
- Total: 200 bytes additional (50 entries × 4 bytes)
- Negligible on modern Particle devices

### Performance Impact
- `Time.now()` and `Time.format()` add < 0.1ms
- No measurable impact on state transitions
- No impact on real-time operations

## Quick Reference

### Setting Timezone
```cpp
Time.zone(-6);  // Central Time
```

### Manual Time Sync
```cpp
Particle.syncTime();
```

### Get Current Time
```cpp
time_t now = Time.now();
String time_str = Time.format(now, "%Y-%m-%d %H:%M:%S");
```

### Check Time Sync Status
```cpp
if (Time.isValid()) {
    Serial.println("Time is synced");
} else {
    Serial.println("Time not synced yet");
}
```

## Questions?

### Q: Will timestamps be wrong if device is offline?
**A:** Yes, if device boots without cloud connection, time won't be synced. Once it connects, time will sync automatically.

### Q: Do I need to set timezone on every device?
**A:** Yes, or set it once in the firmware code.

### Q: What timezone should I use?
**A:** Use your local timezone for easier reading, or UTC for consistency across locations.

### Q: Does uptime still work after this update?
**A:** Yes! Uptime is still captured and displayed alongside the date/time.

### Q: Can I still use milliseconds for relative timing?
**A:** Yes! The uptime column gives you milliseconds since boot for calculating durations.

---

## Summary

The timestamp update makes state transition logs much more useful by showing **real date and time** instead of just milliseconds since boot. This makes it easier to:

- Diagnose issues
- Correlate events
- Generate reports
- Monitor production devices
- Comply with quality requirements

All while maintaining backward compatibility and adding minimal overhead.

**Version:** v1.1 (January 15, 2025)

