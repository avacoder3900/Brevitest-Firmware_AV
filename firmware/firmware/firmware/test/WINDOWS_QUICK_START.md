# Windows Quick Start Guide 🪟
**Running Tests on Windows**

## The Problem

The test suite was designed for Linux/Mac, but you're on Windows. The `make` command doesn't work in PowerShell.

## The Solutions (Pick One)

### ⭐ OPTION 1: Use PowerShell Script (Easiest)

I created a PowerShell script that does what `make` does:

```powershell
# Navigate to test directory
cd "C:\Users\aleja\OneDrive - Linbeck Group, LLC\Documents\GitHub\brevitest-device\firmware\firmware\test"

# Run the build script
.\build_and_test.ps1
```

**First time only**: You may need to allow scripts:
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

### Prerequisites for PowerShell Script:
You need a C++ compiler. Choose one:

**A) MinGW (Recommended)**
1. Download: https://www.mingw-w64.org/
2. Install and add to PATH
3. Restart PowerShell

**B) Chocolatey (If you have it)**
```powershell
choco install mingw
```

**C) Visual Studio (If you have it)**
- Install "Desktop development with C++"
- Use "Developer PowerShell for VS"

---

### ⭐⭐ OPTION 2: Use WSL (Best Experience)

Windows Subsystem for Linux gives you a real Linux environment:

#### Step 1: Install WSL
```powershell
# In PowerShell (as Administrator)
wsl --install
```

Restart your computer.

#### Step 2: Open Ubuntu
- Click Start → Search "Ubuntu"
- Open the Ubuntu app

#### Step 3: Navigate and Run
```bash
# In Ubuntu terminal
cd /mnt/c/Users/aleja/OneDrive\ -\ Linbeck\ Group,\ LLC/Documents/GitHub/brevitest-device/firmware/firmware/test

# Install build tools (first time only)
sudo apt update
sudo apt install build-essential

# Run tests
make test
```

---

### OPTION 3: Use Git Bash

If you have Git for Windows installed:

```bash
# Open Git Bash
cd "/c/Users/aleja/OneDrive - Linbeck Group, LLC/Documents/GitHub/brevitest-device/firmware/firmware/test"

# Install make (if not installed)
# Download from: http://gnuwin32.sourceforge.net/packages/make.htm

# Run tests
make test
```

---

## Quick Test - Do You Have a Compiler?

Run this in PowerShell:

```powershell
gcc --version
```

### If you see version info:
✅ **You're ready!** Run: `.\build_and_test.ps1`

### If you see "command not found":
❌ **Install a compiler first** (see options above)

---

## Once Tests Are Running

You'll see output like this:

```
═══════════════════════════════════════════════════════════════
  Brevitest Firmware Unit Test Suite
  ISO 13485:2016 Compliant Testing Framework
╚═══════════════════════════════════════════════════════════════

Firmware Version: 43
Data Format Version: 39
Test Execution Date: Mon Nov 10 2025

▶ Running: Reset EEPROM [TC-CMD-002]
  ✅ PASS: Reset EEPROM

...

Total Tests:   122
✅ Passed:     122 (100.0%)
❌ Failed:     0 (0.0%)

✨ ALL TESTS PASSED ✨
```

---

## Troubleshooting

### "gcc is not recognized"
➜ Install MinGW or use WSL (see options above)

### "Execution policy" error
➜ Run: `Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser`

### "File not found" errors
➜ Make sure you're in the correct directory:
```powershell
cd "C:\Users\aleja\OneDrive - Linbeck Group, LLC\Documents\GitHub\brevitest-device\firmware\firmware\test"
pwd  # Should show the test directory
```

### Tests compile but fail
➜ This is normal! The tests are checking your firmware. Failed tests = bugs found.

---

## My Recommendation 🎯

**For Quick Testing Now:**
→ Use the PowerShell script (`.\build_and_test.ps1`)

**For Long-Term Development:**
→ Install WSL - it's the best Windows development experience for C/C++

---

## What Happens When Tests Run?

1. **Compile** - Turns .cpp files into .o object files
2. **Link** - Combines everything into `test_runner.exe`
3. **Execute** - Runs all 122 tests
4. **Report** - Shows pass/fail for each test

Total time: ~5-10 seconds ⚡

---

## Need Help?

- Can't install compiler? → Use WSL (Option 2)
- WSL not working? → Try Visual Studio (if you have it)
- Still stuck? → Email the team with the error message

---

**Ready to try?**

```powershell
# Step 1: Go to test directory
cd "C:\Users\aleja\OneDrive - Linbeck Group, LLC\Documents\GitHub\brevitest-device\firmware\firmware\test"

# Step 2: Run the build script
.\build_and_test.ps1
```

Good luck! 🚀

