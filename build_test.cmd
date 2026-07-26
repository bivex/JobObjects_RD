@echo off
rem Build AgentJobObject_Test.cpp using MSVC cl.exe
cl.exe /EHsc /W3 AgentJobObject_Test.cpp /link /OUT:AgentJobObject_Test.exe
if %ERRORLEVEL% EQU 0 (
    echo Build Succeeded: AgentJobObject_Test.exe created.
) else (
    echo Build Failed!
)
