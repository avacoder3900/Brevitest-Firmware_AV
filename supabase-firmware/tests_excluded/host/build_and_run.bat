@echo off
REM Build and run host-based unit tests for Brevitest firmware
REM Requires: MinGW g++ or MSVC cl.exe

echo ========================================
echo Building Brevitest Host Unit Tests
echo ========================================
echo.

REM Try MinGW g++ first
where g++ >nul 2>&1
if %ERRORLEVEL% == 0 (
    echo Using: MinGW g++
    echo.

    g++ -std=c++11 -I../../src ^
        test_host.cpp ^
        ../../src/StateMachine.cpp ^
        ../../src/DataTypes.cpp ^
        -o test_host.exe

    if %ERRORLEVEL% == 0 (
        echo.
        echo Build successful! Running tests...
        echo.
        test_host.exe
        exit /b %ERRORLEVEL%
    ) else (
        echo.
        echo Build FAILED with g++
        exit /b 1
    )
)

REM Try MSVC cl.exe
where cl >nul 2>&1
if %ERRORLEVEL% == 0 (
    echo Using: MSVC cl.exe
    echo.

    cl /EHsc /I../../src ^
        test_host.cpp ^
        ../../src/StateMachine.cpp ^
        ../../src/DataTypes.cpp ^
        /Fe:test_host.exe

    if %ERRORLEVEL% == 0 (
        echo.
        echo Build successful! Running tests...
        echo.
        test_host.exe
        exit /b %ERRORLEVEL%
    ) else (
        echo.
        echo Build FAILED with cl.exe
        exit /b 1
    )
)

echo ERROR: No C++ compiler found!
echo.
echo Please install one of:
echo   - MinGW-w64: https://www.mingw-w64.org/downloads/
echo   - Visual Studio Build Tools: https://visualstudio.microsoft.com/downloads/
echo.
echo Or use WSL:
echo   wsl g++ -std=c++11 -I../../src test_host.cpp ../../src/StateMachine.cpp ../../src/DataTypes.cpp -o test_host
echo   wsl ./test_host
echo.
exit /b 1
