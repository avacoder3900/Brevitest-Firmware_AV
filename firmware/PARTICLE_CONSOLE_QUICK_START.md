# Particle Console Quick Start Guide
## Using State Management Functions

All state management functions are **self-documenting**! Just call any function with `?` to see usage instructions.

---

## 🎯 Quick Reference Card

### Function: `get_state`
**What it does:** Shows current device state

**How to use:**
1. Go to Functions tab
2. Click `get_state`
3. Enter `?` → Click "Call" → See help in Events tab
4. Enter `` (empty) → Click "Call" → See current state

**Return Values:**
- `0-10` = Current state number
- `-1` = Help displayed (check Events)

**Check Events tab for:**
- `device_state` event with state name
- `state_help` event with usage instructions

---

### Function: `get_trans_count`
**What it does:** Shows how many state transitions have been logged

**How to use:**
1. Go to Functions tab
2. Click `get_trans_count`
3. Enter `?` → See help, OR
4. Enter `` (empty) → Get count

**Return Values:**
- `0-50` = Number of transitions logged
- `-1` = Help displayed

**Check Events tab for:**
- `trans_count` event with friendly message

---

### Function: `get_history`
**What it does:** Retrieves state transition history with timestamps

**How to use:**
1. Go to Functions tab
2. Click `get_history`
3. Enter `?` → See help, OR
4. Enter `` (empty) → Get last 10 transitions, OR
5. Enter `5` → Get last 5 transitions, OR
6. Enter `20` → Get last 20 transitions (max)

**Return Values:**
- `>0` = Length of history data (success)
- `-1` = Help displayed
- `-2` = Invalid input

**⚠️ IMPORTANT:** History data is in Events tab!
- Look for `state_history` event
- Contains: `Total:N|DateTime@Uptime:FROM>TO;...`

---

### Function: `clear_history`
**What it does:** Clears all transition history (permanent!)

**How to use:**
1. Go to Functions tab
2. Click `clear_history`
3. Enter `?` → See help, OR
4. Enter `yes` → Confirm and clear

**Return Values:**
- `1` = Success (history cleared)
- `-1` = Help displayed
- `-2` = Need confirmation (enter "yes")

**⚠️ WARNING:** This is permanent! Use with caution.

---

### Function: `force_state`
**What it does:** Forces device to specific state (EMERGENCY USE ONLY!)

**How to use:**
1. Go to Functions tab
2. Click `force_state`
3. Enter `?` → See help with state numbers, OR
4. Enter `0` → Force to IDLE (most common recovery)

**State Numbers:**
- `0` = IDLE (use this for recovery)
- `1` = INITIALIZING
- `2` = HEATING
- `3` = BARCODE_SCANNING
- `4` = VALIDATING_CARTRIDGE
- `5` = VALIDATING_MAGNETOMETER
- `6` = RUNNING_TEST
- `7` = UPLOADING_RESULTS
- `8` = RESETTING_CARTRIDGE
- `9` = STRESS_TESTING
- `10` = ERROR_STATE

**Return Values:**
- `1` = Success
- `-1` = Help displayed
- `-2` = No state provided
- `-3` = Invalid state number
- `-4` = Transition not allowed

**⚠️ DANGER:** Only use for recovery or testing!

---

## 📋 Step-by-Step Usage Examples

### Example 1: Check Current State
```
1. Open console.particle.io
2. Select your device
3. Click "Functions" tab
4. Click "get_state"
5. Leave argument EMPTY
6. Click "Call"
7. See return value (e.g., 0 = IDLE)
8. Switch to "Events" tab
9. See "device_state" event: "State: IDLE (value: 0)"
```

### Example 2: Get Help for Any Function
```
1. Click any function (e.g., "get_history")
2. Enter: ?
3. Click "Call"
4. Return value: -1 (indicates help shown)
5. Switch to "Events" tab
6. See "[function]_help" event with instructions
```

### Example 3: View Transition History
```
1. Click "get_history"
2. Enter: 10
3. Click "Call"
4. Return value: >0 (data length)
5. Switch to "Events" tab
6. Find "state_history" event
7. See: Total:10|2025-01-15 10:30:45@123456789:IDLE>BARCODE_SCANNING;...
```

### Example 4: Clear History (with confirmation)
```
1. Click "clear_history"
2. Enter: (empty) → Returns -2 (needs confirmation)
3. Check Events: "clear_confirm" message
4. Click "clear_history" again
5. Enter: yes
6. Click "Call"
7. Return value: 1 (success)
8. Check Events: "clear_success" with count deleted
```

### Example 5: Emergency Recovery (Force to IDLE)
```
Device is stuck in ERROR_STATE...

1. Click "force_state"
2. Enter: 0
3. Click "Call"
4. Check Events tab:
   - "force_warning": Shows transition
   - "force_success": Confirms new state
5. Verify with get_state
```

---

## 🎨 Console Layout Tips

### Recommended Tab Layout
Keep these tabs open for monitoring:

**Tab 1: Functions**
- Call functions here
- See return values instantly

**Tab 2: Events**
- See detailed responses
- Watch real-time updates
- Find history data here

**Tab 3: Variables** (future enhancement)
- Quick state visibility
- No function calls needed

---

## 🔔 Understanding Return Values

### Positive Numbers (Success)
- `0-50` = Data value (state, count, etc.)
- `>50` = Data length (history string)
- `1` = Operation successful

### Negative Numbers (Help/Error)
- `-1` = Help displayed (check Events)
- `-2` = Need confirmation or invalid input
- `-3` = Invalid parameter value
- `-4` = Operation not allowed

---

## 📊 Reading Events Tab Output

### Event Types You'll See:

**Information Events:**
- `device_state` - Current state with name
- `trans_count` - Transition count with message
- `history_info` - Summary of history retrieval
- `clear_success` - Confirmation of clearing

**Help Events:**
- `state_help` - get_state usage
- `trans_count_help` - get_trans_count usage
- `history_help` - get_history usage
- `clear_help` - clear_history usage
- `force_help` - force_state usage

**Warning Events:**
- `force_warning` - State being forced
- `force_invalid` - Invalid transition attempt

**Error Events:**
- `history_error` - Invalid history count
- `clear_confirm` - Need confirmation
- `force_error` - Force state error

**Data Events:**
- `state_history` - **This is where history data lives!**

---

## 💡 Pro Tips

### Tip 1: Always Check Events Tab
Many functions publish detailed info to Events that's more useful than the return value!

### Tip 2: Use `?` When Unsure
Every function responds to `?` with helpful usage info.

### Tip 3: Subscribe to Events (CLI)
```bash
particle subscribe state_history
particle subscribe device_state
```

### Tip 4: Create a Dashboard
Add widgets for:
- Function buttons (quick access)
- Event stream (live monitoring)
- Custom charts (transition trends)

### Tip 5: Mobile Access
Particle Console works on mobile - monitor your device anywhere!

---

## 🚨 Common Questions

### Q: Where's my history data?
**A:** Check the **Events tab** for `state_history` event. The function return value is just the data length.

### Q: Function returns -1, what happened?
**A:** That means help was displayed. Check Events tab for the help message.

### Q: How do I know what state number to use?
**A:** Call `get_state` with `?` to see all state numbers and meanings.

### Q: Can I force any state transition?
**A:** No, only valid transitions are allowed. The state machine prevents invalid moves.

### Q: History seems empty?
**A:** Device may have just rebooted. History clears on power cycle.

### Q: What timezone are timestamps in?
**A:** UTC by default. Set timezone in device code with `Time.zone(-6)` for Central, etc.

---

## 📞 Quick Help Reference

### Getting Help from Console:
```
Function          | Enter | Result
------------------|-------|------------------
get_state         |   ?   | See state numbers
get_trans_count   |   ?   | See usage info
get_history       |   ?   | See format & limits
clear_history     |   ?   | See confirmation steps
force_state       |   ?   | See state numbers & warnings
```

### Common Operations:
```
Task                      | Function       | Argument
--------------------------|----------------|----------
Check current state       | get_state      | (empty)
Count transitions         | get_trans_count| (empty)
Get last 10 transitions   | get_history    | (empty)
Get last 5 transitions    | get_history    | 5
Clear history             | clear_history  | yes
Force to IDLE (recovery)  | force_state    | 0
```

---

## 🎓 Tutorial: First-Time User

**Step 1: See What State Device Is In**
1. Functions → `get_state` → Enter: (empty) → Call
2. Note return value (e.g., 0)
3. Events → See `device_state` event → "State: IDLE (value: 0)"

**Step 2: Check How Many Transitions Logged**
1. Functions → `get_trans_count` → Enter: (empty) → Call
2. Note return value (e.g., 15)
3. Events → See `trans_count` event → "Total transitions: 15 (max 50 stored)"

**Step 3: View Transition History**
1. Functions → `get_history` → Enter: 5 → Call
2. Events → Find `state_history` event
3. Read format: `Total:N|DateTime@Uptime:FROM>TO;...`

**Step 4: Get Help on Any Function**
1. Functions → `force_state` → Enter: ? → Call
2. Events → See `force_help` with complete instructions

**Congratulations!** You now know how to use all state management functions! 🎉

---

## 📚 Additional Resources

- **Full Documentation:** `STATE_MANAGEMENT_GUIDE.md`
- **Implementation Details:** `IMPLEMENTATION_SUMMARY.md`
- **Timestamp Guide:** `TIMESTAMP_UPDATE_NOTES.md`
- **State Diagrams:** `STATE_FLOWCHARTS.md`

---

## 🆘 Emergency Recovery

**Device Stuck? Try This:**

1. Check state: `get_state` → (empty)
2. If in ERROR_STATE (10):
   - `force_state` → 0
   - Check Events for confirmation
3. Verify: `get_state` → (empty) → Should be 0 (IDLE)
4. Check history: `get_history` → 10 → See what went wrong

**Still Stuck?**
- Try device reset (hardware button)
- Check transition history for clues
- Contact support with history data

---

**Version:** v1.1 - Self-Documenting Functions
**Last Updated:** January 15, 2025

**Remember:** Every function responds to `?` - when in doubt, ask for help! 🎯

