# AgentOS Inter-Agent & Telemetry Protocol Specification (AOP v1.0)

## Overview
The **AgentOS Protocol (AOP v1.0)** defines the standardized Inter-Process Communication (IPC), telemetry event format, and intent-driven natural language prompt protocol between the OS Kernel Resource Controller (**AgentJobEngine**), the LLM Swarm Orchestrator, and individual multi-tenant AI coding agents (Claude Code, SWE-agent, OpenHands).

---

## 1. Universal JSON Schema (IPC & Telemetry)

All structured communication between the `AgentJobEngine` kernel background loop and the Swarm Orchestrator uses the following JSON schema:

```json
{
  "aop_version": "1.0",
  "message_id": "msg_9f8b2c1a",
  "timestamp_us": 1723218000000000,
  "session_id": "AgentSession_SWE_Bench_Task_101",
  "agent_id": "agent_worker_42",
  "type": "RESOURCE_ALERT",
  "severity": "WARNING",
  
  "payload": {
    "event_code": "MEMORY_CAP_EXCEEDED",
    "metrics": {
      "current_memory_mb": 48.5,
      "limit_memory_mb": 50.0,
      "cpu_usage_pct": 85.2,
      "iops_current": 420,
      "iops_limit": 500
    },
    "state": "REASONING_IDLE",
    "suggested_action": "REDUCE_THREAD_COUNT"
  },
  
  "natural_language_prompt": "[OS RESOURCE ALERT]: Memory cap reached (50 MB / 50 MB). Reduce tool allocation or execution threads."
}
```

---

## 2. Message Types

| Message Type (`type`) | Direction | Purpose |
| :--- | :--- | :--- |
| **`RESOURCE_ALERT`** | Engine ➔ Orchestrator / LLM | Non-destructive warning when memory/IOPS/CPU limits are approached. |
| **`STATE_CHANGE`** | Orchestrator ➔ Engine | Control directives: `FREEZE`, `THAW`, `TRIM_IDLE`, `CREATE_SANDBOX`. |
| **`INTER_AGENT_DIRECTIVE`** | Agent A ➔ Agent B | Subtask delegation between swarm workers with attached resource budgets. |
| **`TELEMETRY_HEARTBEAT`** | Engine ➔ Monitoring | Real-time RSS memory, CPU %, and IOPS metrics. |

---

## 3. Intent-Driven LLM Prompt Encodings

When resource thresholds are reached, the kernel translates engine state into structured prompt tags injected directly into the LLM context window:

### A. Memory Warning (Soft/Hard Cap Near Limit)
```markdown
[OS RESOURCE ALERT]
  Event: MEMORY_CAP_EXCEEDED
  Session: AgentSession_SWE_Bench_Task_101
  Current Usage: 48.5 MB / 50.0 MB
  Recommended Action: Execute garbage collection, reduce tool memory buffers or decrease process concurrency.
```

### B. Reasoning Phase Compression (Idle Working Set Trim)
```markdown
[OS KERNEL NOTICE]
  State: LLM_REASONING_PHASE
  Action: Working set trimmed & compressed (Memory: 185 MB -> 14.2 MB). Process suspended.
```

### C. I/O & Network Rate Limiting (Throttle Warning)
```markdown
[OS RESOURCE ALERT]
  Event: IOPS_LIMIT_REACHED
  Current Disk Bandwidth: 30 MB/s (Throttled via setiopolicy_np / JobObjects)
  Recommended Action: Batch file writes or defer large dependency installations.
```

---

## 4. Architectural Control Flow

```
[Agent Worker Process] ──(Memory Spike 49MB)──> [AgentJobEngine Monitor]
                                                       │
                                          (Formats AOP v1.0 JSON)
                                                       │
                                                       ▼
[LLM Orchestrator] <─── [OS RESOURCE ALERT Prompt] ────┘
       │
       ├──(Sends INTER_AGENT_DIRECTIVE to Agent B: "Throttle file scanning")
       └──(Invokes STATE_CHANGE: TRIM_IDLE)
```
