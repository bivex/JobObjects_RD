# JobObjects & Silos Internals (Windows AI Agent Resource Controller)

Technical specification, kernel research, and empirical C++ validation PoC for Windows **Job Objects (`_EJOB`)** and **Silos (`_ESERVERSILO_GLOBALS`)** — the native Windows equivalent of Linux **cgroups v2** and **AgentCgroup**.

## Project Files
- [AgentJobObject_Kernel_Research.md](file:///Volumes/External/Code/JobObjects/AgentJobObject_Kernel_Research.md) — Verified kernel research (Build 26100), disassembled functions (`PspFreezeJobTree`, `PspGetJobMemoryUsageNotificationViolations`, `PspImplicitAssignProcessToJob`), and memory eviction tuning.
- [JobObjects_Internals_Win11.md](file:///Volumes/External/Code/JobObjects/JobObjects_Internals_Win11.md) — Architectural overview, structure offsets, comparison with Linux `cgroups v2`.
- [AgentJobObject_Test.cpp](file:///Volumes/External/Code/JobObjects/AgentJobObject_Test.cpp) — Empirical C++ validation PoC (Non-destructive Notification Limits + Job Freezing + IoCompletionPort).
- [CMakeLists.txt](file:///Volumes/External/Code/JobObjects/CMakeLists.txt) — Modern CMake build configuration.
- [build_test.cmd](file:///Volumes/External/Code/JobObjects/build_test.cmd) — Standalone MSVC build script.

## Building with CMake
```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
