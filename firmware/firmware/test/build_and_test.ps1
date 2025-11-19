# PowerShell Build and Test Script for Windows
# Alternative to using 'make' on Windows

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Brevitest Firmware Test Builder" -ForegroundColor Cyan
Write-Host "  Windows PowerShell Version" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Check for GCC/G++
Write-Host "Checking for compiler..." -ForegroundColor Yellow
$gccPath = Get-Command gcc -ErrorAction SilentlyContinue
$gppPath = Get-Command g++ -ErrorAction SilentlyContinue

if (-not $gccPath -or -not $gppPath) {
    Write-Host "ERROR: GCC/G++ compiler not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "To install, you have several options:" -ForegroundColor Yellow
    Write-Host "1. Install MinGW: https://www.mingw-w64.org/" -ForegroundColor White
    Write-Host "2. Install via Chocolatey: choco install mingw" -ForegroundColor White
    Write-Host "3. Use WSL (Windows Subsystem for Linux) - RECOMMENDED" -ForegroundColor Green
    Write-Host ""
    Write-Host "After installation, restart PowerShell and run this script again." -ForegroundColor Yellow
    exit 1
}

Write-Host "Compiler found: $($gccPath.Source)" -ForegroundColor Green
Write-Host ""

# Create build directory
Write-Host "Creating build directory..." -ForegroundColor Yellow
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
    Write-Host "Build directory created" -ForegroundColor Green
} else {
    Write-Host "Build directory exists" -ForegroundColor Green
}

# Create reports directory
if (-not (Test-Path "reports")) {
    New-Item -ItemType Directory -Path "reports" | Out-Null
}

Write-Host ""
Write-Host "Compiling test framework..." -ForegroundColor Yellow

# Compile test_framework.cpp
$output = & g++ -Wall -Wextra -std=c++11 -g -I../src -c test_framework.cpp -o build/test_framework.o 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to compile test_framework.cpp" -ForegroundColor Red
    Write-Host $output
    exit 1
}
Write-Host "test_framework.cpp compiled" -ForegroundColor Green

# Compile test_serial_commands.cpp
$output = & g++ -Wall -Wextra -std=c++11 -g -I../src -c test_serial_commands.cpp -o build/test_serial_commands.o 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to compile test_serial_commands.cpp" -ForegroundColor Red
    Write-Host $output
    exit 1
}
Write-Host "test_serial_commands.cpp compiled" -ForegroundColor Green

# Compile test_state_transitions.cpp
$output = & g++ -Wall -Wextra -std=c++11 -g -I../src -c test_state_transitions.cpp -o build/test_state_transitions.o 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to compile test_state_transitions.cpp" -ForegroundColor Red
    Write-Host $output
    exit 1
}
Write-Host "test_state_transitions.cpp compiled" -ForegroundColor Green

# Compile test_main.cpp
$output = & g++ -Wall -Wextra -std=c++11 -g -I../src -c test_main.cpp -o build/test_main.o 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to compile test_main.cpp" -ForegroundColor Red
    Write-Host $output
    exit 1
}
Write-Host "test_main.cpp compiled" -ForegroundColor Green

Write-Host ""
Write-Host "Linking test runner..." -ForegroundColor Yellow

# Link everything together
$output = & g++ -Wall -Wextra -std=c++11 -g -o test_runner.exe build/test_framework.o build/test_serial_commands.o build/test_state_transitions.o build/test_main.o 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Host "Failed to link test runner" -ForegroundColor Red
    Write-Host $output
    exit 1
}
Write-Host "Test runner built successfully!" -ForegroundColor Green

Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Running Tests..." -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host ""

# Run the tests
& .\test_runner.exe

Write-Host ""
if ($LASTEXITCODE -eq 0) {
    Write-Host "==========================================" -ForegroundColor Green
    Write-Host "  ALL TESTS PASSED!" -ForegroundColor Green
    Write-Host "==========================================" -ForegroundColor Green
} else {
    Write-Host "==========================================" -ForegroundColor Red
    Write-Host "  SOME TESTS FAILED" -ForegroundColor Red
    Write-Host "==========================================" -ForegroundColor Red
}

Write-Host ""
Write-Host "Test runner is at: test_runner.exe" -ForegroundColor Yellow
Write-Host "You can run it again with: .\test_runner.exe" -ForegroundColor Yellow
Write-Host ""


