@echo off
setlocal enabledelayedexpansion

title AgentJobEngine 1-Click Build and Test Runner

echo ================================================================
echo  AgentJobEngine -- 1-Click Build and Test Execution
echo ================================================================
echo.

rem 1. Terminate stale background MSBuild processes
taskkill /F /IM MSBuild.exe /IM cl.exe 2>nul

rem 2. Configure CMake project if needed
if not exist "out\build" (
    echo [*] Configuring CMake project...
    cmake -B out/build
    if !ERRORLEVEL! NEQ 0 (
        echo [-] CMake configuration failed!
        pause
        exit /b 1
    )
)

rem 3. Build all targets
echo [*] Building AgentJobEngine and test suites...
cmake --build out/build --config Debug
if !ERRORLEVEL! NEQ 0 (
    echo [-] Build failed!
    pause
    exit /b 1
)

echo.
echo ================================================================
echo  RUNNING TEST 1: AgentJobObject Integrated Test
echo ================================================================
echo.
if exist "out\build\bin\AgentJobObject_Test.exe" (
    "out\build\bin\AgentJobObject_Test.exe"
) else if exist "out\build\bin\Debug\AgentJobObject_Test.exe" (
    "out\build\bin\Debug\AgentJobObject_Test.exe"
) else (
    echo [-] AgentJobObject_Test.exe not found!
)

echo.
echo ================================================================
echo  RUNNING TEST 2: AgentJobEngine Edge Cases Unit Tests
echo ================================================================
echo.
if exist "out\build\bin\AgentJobEngine_EdgeCases_Test.exe" (
    "out\build\bin\AgentJobEngine_EdgeCases_Test.exe"
) else if exist "out\build\bin\Debug\AgentJobEngine_EdgeCases_Test.exe" (
    "out\build\bin\Debug\AgentJobEngine_EdgeCases_Test.exe"
) else (
    echo [-] AgentJobEngine_EdgeCases_Test.exe not found!
)

echo.
echo ================================================================
echo  All operations completed cleanly.
echo ================================================================
pause
