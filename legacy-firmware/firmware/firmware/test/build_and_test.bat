@echo off
REM Batch file for building and running tests on Windows Command Prompt

echo ==========================================
echo   Brevitest Firmware Test Builder
echo   Windows Batch Version
echo ==========================================
echo.

echo Checking for compiler...
where gcc >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: GCC compiler not found!
    echo.
    echo To install, you have several options:
    echo 1. Install MinGW: https://www.mingw-w64.org/
    echo 2. Install via Chocolatey: choco install mingw
    echo 3. Use WSL (Windows Subsystem for Linux^) - RECOMMENDED
    echo.
    echo After installation, restart Command Prompt and run this script again.
    pause
    exit /b 1
)

echo Compiler found!
echo.

echo Creating directories...
if not exist "build" mkdir build
if not exist "reports" mkdir reports

echo.
echo Compiling test framework...
g++ -Wall -Wextra -std=c++11 -g -I../src -c test_framework.cpp -o build/test_framework.o
if %errorlevel% neq 0 (
    echo Failed to compile test_framework.cpp
    pause
    exit /b 1
)
echo test_framework.cpp compiled

g++ -Wall -Wextra -std=c++11 -g -I../src -c test_serial_commands.cpp -o build/test_serial_commands.o
if %errorlevel% neq 0 (
    echo Failed to compile test_serial_commands.cpp
    pause
    exit /b 1
)
echo test_serial_commands.cpp compiled

g++ -Wall -Wextra -std=c++11 -g -I../src -c test_state_transitions.cpp -o build/test_state_transitions.o
if %errorlevel% neq 0 (
    echo Failed to compile test_state_transitions.cpp
    pause
    exit /b 1
)
echo test_state_transitions.cpp compiled

g++ -Wall -Wextra -std=c++11 -g -I../src -c test_main.cpp -o build/test_main.o
if %errorlevel% neq 0 (
    echo Failed to compile test_main.cpp
    pause
    exit /b 1
)
echo test_main.cpp compiled

echo.
echo Linking test runner...
g++ -Wall -Wextra -std=c++11 -g -o test_runner.exe build/test_framework.o build/test_serial_commands.o build/test_state_transitions.o build/test_main.o
if %errorlevel% neq 0 (
    echo Failed to link test runner
    pause
    exit /b 1
)
echo Test runner built successfully!

echo.
echo ==========================================
echo   Running Tests...
echo ==========================================
echo.

test_runner.exe

echo.
if %errorlevel% equ 0 (
    echo ==========================================
    echo   ALL TESTS PASSED!
    echo ==========================================
) else (
    echo ==========================================
    echo   SOME TESTS FAILED
    echo ==========================================
)

echo.
echo Test runner is at: test_runner.exe
echo You can run it again with: test_runner.exe
echo.
pause


