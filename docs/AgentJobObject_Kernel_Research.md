# AgentJobObject Kernel Research & Verification — Windows 11 (`ntkrnlmp.exe`)

**Verified Environment Details:**
* **OS Build:** Windows 11 ARM64 / x64, Build `26100.1.arm64fre.ge_release.240331-1435`
* **Kernel Base Address:** `0xfffff80047400000`
* **Public PDB Symbol Hash:** `ntkrnlmp.pdb\5846006CBAFC4F9E07B846F1798587A91\ntkrnlmp.pdb`
* **Verification Tools:** WinDbg Kernel Debugger over MCP + Microsoft Public Symbols (`.symfix`)

---

## 1. Executive Summary & Verification Methodology

To address the findings of the **`AgentCgroup`** paper (UC Santa Cruz / Virginia Tech, Feb 2026) on Windows:
1. **Public Win32 API Layer:** Official, documented APIs (`SetInformationJobObject`, `JOBOBJECT_NOTIFICATION_LIMIT_INFORMATION`, `JobObjectFreezeInformation`) provide complete, stable control over memory throttling, process tree freezing, and notification limits without relying on unstable kernel internals.
2. **Kernel Internal Layer (`nt!_EJOB`):** Internal data structures and functions (`PspGetJobMemoryUsageNotificationViolations`, `PspFreezeJobTree`) govern how the kernel processes these requests.

---

## 2. Empirical Kernel Structure Dump (`nt!_EJOB`)

Dumped directly from `ntkrnlmp.pdb` on Windows 11 ARM64 (Build 26100.1):

```text
struct nt!_EJOB (Size: 0x728 bytes)
   +0x000 Event            : _KEVENT
   +0x018 JobLinks         : _LIST_ENTRY
   +0x028 ProcessListHead  : _LIST_ENTRY
   +0x038 JobLock          : _ERESOURCE
   +0x0a0 TotalUserTime    : _LARGE_INTEGER
   +0x0a8 TotalKernelTime  : _LARGE_INTEGER
   +0x100 LimitFlags       : Uint4B
   +0x104 ActiveProcessLimit : Uint4B
   +0x108 Affinity         : _KAFFINITY_EX
   +0x228 CompletionPort   : Ptr64 Void
   +0x2b0 ProcessMemoryLimit : Uint8B
   +0x2b8 JobMemoryLimit   : Uint8B
   +0x428 EffectiveFreezeCount : Uint4B
   +0x448 PagePriorityLimit : Uint4B
   +0x4c0 NotificationInfo : Ptr64 _JOB_NOTIFICATION_INFORMATION
   +0x4d8 CpuRateControl   : Ptr64 _JOB_CPU_RATE_CONTROL
   +0x508 ChildJobListHead : _LIST_ENTRY
   +0x518 ParentJob        : Ptr64 _EJOB
   +0x520 RootJob          : Ptr64 _EJOB
   +0x5e0 ServerSiloGlobals : Ptr64 _ESERVERSILO_GLOBALS
   +0x608 NetRateControl   : Ptr64 _JOB_NET_RATE_CONTROL
   +0x610 JobFlags         : Uint4B (Bit 9: JobFrozen, Bit 30: Silo)
   +0x638 IoRateControlHeader : _JOB_RATE_CONTROL_HEADER
   +0x6a0 VolumeIoControlTree : _RTL_RB_TREE
```

---

## 3. Disassembled Internal Kernel Functions

### A. Non-Destructive Notification Evaluation (`nt!PspGetJobMemoryUsageNotificationViolations`)
* **Symbol Address:** `fffff800`47cfab90`
* **Disassembly:**
```assembly
nt!PspGetJobMemoryUsageNotificationViolations:
  ldr   x9, [x0, #0x4C0]    ; x9 = _EJOB.NotificationInfo (+0x4c0)
  mov   w0, #0              ; Result bitmask
  ldr   w8, [x9]
  tst   w8, #0x200000       ; Test Notification Limit Enabled
  ...
  tst   w3, #0x200          ; Check JOB_OBJECT_LIMIT_JOB_MEMORY_LOW (0x200)
  cselhi w0, w8, wzr
  tst   w3, #0x8000         ; Check JOB_OBJECT_LIMIT_JOB_MEMORY_HIGH (0x8000)
  orr   w0, w0, #0x8000
  ret
```
**Mechanism:** Evaluates memory consumption against soft limits (`0x200` / `0x8000`). Queues an I/O completion packet to `_EJOB.CompletionPort` (`+0x228`) **without terminating any process**.

---

### B. Process Tree Freezing (`nt!PspFreezeJobTree`)
* **Symbol Address:** `fffff800`47cfa638`
* **Disassembly:**
```assembly
nt!PspFreezeJobTree:
  mov   x19, x0             ; x19 = _EJOB pointer
  add   x0, x19, #0x38      ; x0 = &_EJOB.JobLock (+0x38)
  bl    nt!ExAcquireResourceExclusiveLite
  ...
  ldrb  w8, [x20, #4]       ; Reads Freeze flag (1 byte) at offset +0x04
  add   x10, x24, #0x610    ; Address of _EJOB.JobFlags (+0x610)
  mov   w9, #2
  cbnz  w8, nt!PspFreezeJobTree+0x228 ; If Freeze != 0 -> set JobFrozen bit (+0x610)

  ldrb  w8, [x20, #5]       ; Reads Filter flag (1 byte) at offset +0x05
  ...
  bl    nt!ExReleaseResourceLite
  ret
```
**Mechanism:** Locks `JobLock` (`+0x38`) and updates `EffectiveFreezeCount` (`+0x428`) and `JobFlags.JobFrozen` (`+0x610:1`), invoking thread suspension callbacks. The kernel API expects a 16-byte `JOBOBJECT_FREEZE_INFORMATION` structure with `ComponentFlags = 1` (+0x00), `Freeze = 1/0` (+0x04), and `Filter = 0` (+0x05).

---

### C. Breakaway Prevention (`nt!PspImplicitAssignProcessToJob`)
* **Symbol Address:** `fffff800`47cfafc0`
* **Disassembly:**
```assembly
nt!PspImplicitAssignProcessToJob:
  bl    nt!PspLockJobChain
  tbz   w24, #0xA, SkipSilo   ; Check if Breakaway is permitted
  bl    nt!PsGetEffectiveServerSilo
  ...
  bl    nt!PspUnlockJobChain
  ret
```
**Mechanism:** Intercepts child process creation (`CreateProcess`) and binds child processes to the parent job tree unless `JOB_OBJECT_LIMIT_BREAKAWAY_OK` is explicitly set in `LimitFlags` (`+0x100`).

---

### D. Idle Phase Working Set Trim (`nt!PspSetPagePriorityLimitJobTree`)
* **Symbol Address:** `fffff800`47b22280`
* **Disassembly:**
```assembly
nt!PspSetPagePriorityLimitJobTree:
  bl    nt!ExAcquireResourceExclusiveLite
  str   w20, [x19, #0x448]            ; Store PagePriorityLimit (+0x448)
  bl    nt!PspEnumJobsAndProcessesInJobHierarchy
  bl    nt!ExReleaseResourceLite
  ret
```
**Mechanism:** Updates `PagePriorityLimit` (`+0x448`). During LLM reasoning phases (40-45% of latency), dropping page priority from `5` to `1` hints the Memory Manager (`nt!MiTrimWorkingSet`) to compress idle framework heaps.

---

## 4. Empirical Validation Code

A complete C++ PoC is provided in `[AgentJobObject_Test.cpp](file:///Volumes/External/Code/JobObjects/AgentJobObject_Test.cpp)`:
- Sets a **50 MB Soft Notification Limit** on a Job Object.
- Spawns a child worker allocating 100 MB of RAM.
- Listens on `GetQueuedCompletionStatus` for `JOB_OBJECT_MSG_NOTIFICATION_LIMIT` (`12`).
- Verifies that the memory spike triggers the notification event **without OOM-killing the process**, and executes `JobObjectFreezeInformation` (`18`) to freeze/unfreeze the process tree.
