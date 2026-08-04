# AgentJobEngine — High-Density AI Agent OS Resource Controller (macOS & Windows)

**AgentJobEngine** is a high-performance C++ engine for multi-tenant AI coding agents (Claude Code, SWE-agent, OpenHands) running on macOS and Windows. Based on OS resource management principles from the *AgentCgroup* research (UC Santa Cruz / Virginia Tech, Feb 2026), it leverages native **macOS Darwin Kernel & Windows Kernel (`_EJOB`)** primitives, **Memory Compression**, **Process Tree Freezing**, **Disk/Net Rate Limits**, and **Container Sandboxing**.

---

## Key Features & Kernel Capabilities
- **10× – 15× Swarm Concurrency:** Increases concurrent AI agent density from 32 to 500+ agents on a 128 GB RAM server.
- **Disk I/O Rate Control (`SetIoRateLimit`):** Limits volume IOPS & throughput via macOS `setiopolicy_np` / Windows `JobObjectIoRateControlInformation` (class 19) preventing NVMe exhaustion during `npm install` or `git clone`.
- **Network Bandwidth Control (`SetNetworkRateLimit`):** Restricts per-agent network throughput (e.g. 100 Mbps via `JobObjectNetRateControlInformation`, class 32).
- **Container Sandbox (`CreateSiloSandbox`):** Provides process sandbox isolation via macOS Seatbelt Sandbox (`sandbox_init`) and Windows Server Silo container virtualization.
- **Idle Memory Compression (`TrimWorkingSetToCompressStore`):** Uses Darwin QoS background policy / Windows `nt!PspSetPagePriorityLimitJobTree` (`JobObjectPagePriorityLimitId` = 14) to compress idle Node.js/Python framework heaps from **185 MB down to < 15 MB** during LLM reasoning phases (40–45% of task time).
- **Process Tree Freeze & Thaw (`FreezeJobTree` / `ThawJobTree`):** Synchronizes process suspension across complex agent worker trees via Mach / POSIX `SIGSTOP`/`SIGCONT` signals and Windows `JobObjectFreezeInformation` (class 18, `ComponentFlags = 1`, `Freeze = 1/0`).
- **Non-Destructive Memory Limits (Zero OOM Kills):** Intercepts memory spikes via background monitor loops and Windows `CompletionPort` (`JOB_OBJECT_MSG_JOB_MEMORY_LIMIT`). Processes do NOT crash or lose LLM context; they degrade gracefully.
- **Intent-Driven Natural Language Feedback:** Automatically generates structured feedback messages (`[OS RESOURCE ALERT]`) for the LLM to adjust tool execution parameters dynamically.

---

## Directory Layout
```text
JobObjects/
├── CMakeLists.txt                 # CMake build configuration
├── CMakePresets.json              # Visual Studio 2022/2026 Native Presets
├── README.md                      # Index Documentation
├── run_build_and_tests.sh         # 1-Click macOS Automated Build and Test Script
├── run_build_and_tests.cmd        # 1-Click Windows Automated Build and Test Script
├── include/
│   └── AgentJobEngine.hpp         # C++ Core Engine Header (Freeze, I/O, Net, Silos API)
├── src/
│   └── AgentJobEngine.cpp         # C++ Core Engine Implementation (macOS & Windows)
├── tests/
│   ├── AgentJobObject_Test.cpp    # Integrated Validation Test (50MB Cap + LLM Feedback)
│   ├── AgentJobEngine_EdgeCases_Test.cpp # 7 Defensive Unit Tests (Breakaway, Freeze/Thaw, IO, Net, Silos)
│   └── AgentSwarm_Benchmark.cpp   # Empirical Swarm Density & Concurrency Benchmark
└── docs/
    ├── AgentJobObject_Kernel_Research.md  # Verified WinDbg Kernel Offsets & Disassembly (Build 26100.1)
    ├── JobObjects_Internals_Win11.md     # Windows _EJOB vs Linux cgroups v2 Architecture
    └── Swarm_Scalability_Benchmark.md    # Swarm Scalability Math & Benchmarks
```

---

## Building & Testing

### macOS (1-Click Script - Recommended)
```bash
./run_build_and_tests.sh
```

### macOS (Manual CMake Build)
```bash
cmake -B out/build -DCMAKE_BUILD_TYPE=Debug
cmake --build out/build
./out/build/bin/AgentJobObject_Test
./out/build/bin/AgentJobEngine_EdgeCases_Test
```

### Windows (1-Click Script)
```cmd
.\run_build_and_tests.cmd
```

### Windows (Command Line - CMake + MSVC)
```cmd
cmake -B out/build -A x64
cmake --build out/build --config Debug
.\out\build\bin\AgentJobObject_Test.exe
.\out\build\bin\AgentJobEngine_EdgeCases_Test.exe
```

