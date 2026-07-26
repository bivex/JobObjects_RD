# AgentJobEngine — High-Density AI Agent OS Resource Controller (Windows Kernel)

**AgentJobEngine** is a high-performance C++ engine for multi-tenant AI coding agents (Claude Code, SWE-agent, OpenHands) running on Windows. Based on OS resource management principles from the *AgentCgroup* research (UC Santa Cruz / Virginia Tech, Feb 2026), it leverages native Windows Kernel **Job Objects (`_EJOB`)**, **Memory Compression**, **Process Tree Freezing**, and **Silos**.

---

## Key Features & Kernel Capabilities
- **10× – 15× Swarm Concurrency:** Increases concurrent AI agent density from 32 to 500+ agents on a 128 GB RAM server.
- **Idle Memory Compression (`TrimWorkingSetToCompressStore`):** Uses `nt!PspSetPagePriorityLimitJobTree` (`JobObjectPagePriorityLimitId` = 14) to compress idle Node.js/Python framework heaps from **185 MB down to < 15 MB** during LLM reasoning phases (40–45% of task time).
- **Process Tree Freeze & Thaw (`FreezeJobTree` / `ThawJobTree`):** Synchronizes process suspension across complex agent worker trees via `JobObjectFreezeInformation` (class 18, `ComponentFlags = 1`, `Freeze = 1/0`) backed by kernel function `nt!PspFreezeJobTree`.
- **Non-Destructive Memory Limits (Zero OOM Kills):** Intercepts memory spikes via `CompletionPort` (`JOB_OBJECT_MSG_JOB_MEMORY_LIMIT`). Processes do NOT crash or lose LLM context; they degrade gracefully.
- **Intent-Driven Natural Language Feedback:** Automatically generates structured feedback messages (`[OS RESOURCE ALERT]`) for the LLM to adjust tool execution parameters dynamically.

---

## Directory Layout
```text
JobObjects/
├── CMakeLists.txt                 # CMake build configuration
├── CMakePresets.json              # Visual Studio 2022/2026 Native Presets
├── README.md                      # Index Documentation
├── run_build_and_tests.cmd        # 1-Click Automated Build and Test Execution Script
├── include/
│   └── AgentJobEngine.hpp         # C++ Core Engine Header
├── src/
│   └── AgentJobEngine.cpp         # C++ Core Engine Implementation
├── tests/
│   ├── AgentJobObject_Test.cpp    # Integrated Validation Test (50MB Cap + LLM Feedback)
│   └── AgentJobEngine_EdgeCases_Test.cpp # Defensive Edge Cases & Freeze/Thaw Unit Tests
└── docs/
    ├── AgentJobObject_Kernel_Research.md  # Verified WinDbg Kernel Offsets & Disassembly (Build 26100.1)
    ├── JobObjects_Internals_Win11.md     # Windows _EJOB vs Linux cgroups v2 Architecture
    └── Swarm_Scalability_Benchmark.md    # Swarm Scalability Math & Benchmarks
```

---

## Building & Testing

### Option A: 1-Click Script (Recommended)
Run the automated build and full test suite runner:
```cmd
.\run_build_and_tests.cmd
```

### Option B: Command Line (CMake + MSVC)
```cmd
cd Y:\Code\JobObjects
cmake -B out/build -A x64
cmake --build out/build --config Debug
.\out\build\bin\AgentJobObject_Test.exe
.\out\build\bin\AgentJobEngine_EdgeCases_Test.exe
```

### Option C: Visual Studio 2022 / 2026
1. Open the `Y:\Code\JobObjects` folder in Visual Studio.
2. Select **`AgentJobObject_Test.exe`** or **`AgentJobEngine_EdgeCases_Test.exe`** as the startup item.
3. Press **F5** or **Ctrl + F5** to run.
