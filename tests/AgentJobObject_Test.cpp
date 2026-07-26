// ============================================================================
// AgentJobObject Integrated Validation PoC
// Demonstrates Non-Destructive Memory Throttling, Working Set Compression,
// and Intent-Driven LLM Feedback for AI Agent Sandboxing on Windows 10 / 11
// ============================================================================

#include "AgentJobEngine.hpp"
#include <stdio.h>
#include <stdlib.h>

#define NOTIFICATION_LIMIT_BYTES (50 * 1024 * 1024)  // 50 MB Cap
#define TARGET_ALLOCATION_BYTES  (100 * 1024 * 1024) // 100 MB Allocation Spike

// Worker Process entry (spikes memory)
void MemorySpikeWorker() {
    printf("[Child Worker] Starting tool execution memory spike (Target: 100 MB, Cap: 50 MB)...\n");
    
    // Attempt 100 MB allocation
    char* pBuffer = (char*)VirtualAlloc(NULL, TARGET_ALLOCATION_BYTES, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pBuffer) {
        printf("[Child Worker] VirtualAlloc exceeded 50 MB Job Memory Limit!\n");
        printf("[Child Worker] Graceful degradation: Retrying with smaller allocation (30 MB)...\n");
        
        // Fallback to smaller 30 MB allocation (Graceful Adaptation)
        pBuffer = (char*)VirtualAlloc(NULL, 30 * 1024 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (pBuffer) {
            for (size_t i = 0; i < 30 * 1024 * 1024; i += 4096) pBuffer[i] = 1;
            printf("[Child Worker] Fallback allocation of 30 MB succeeded!\n");
            Sleep(2000);
            VirtualFree(pBuffer, 0, MEM_RELEASE);
        }
        return;
    }

    // Touch pages repeatedly
    for (size_t i = 0; i < TARGET_ALLOCATION_BYTES; i += 4096) pBuffer[i] = 1;

    printf("[Child Worker] Memory committed. Holding for 3s...\n");
    Sleep(3000);
    VirtualFree(pBuffer, 0, MEM_RELEASE);
    printf("[Child Worker] Worker finished.\n");
}

int main(int argc, char* argv[]) {
    // If launched with "--worker", act as child process
    if (argc > 1 && strcmp(argv[1], "--worker") == 0) {
        MemorySpikeWorker();
        return 0;
    }

    printf("================================================================\n");
    printf(" AgentJobEngine Integrated Validation (C++ Engine API)\n");
    printf("================================================================\n\n");

    // 1. Configure Agent Engine Session
    AgentEngine::AgentSessionConfig config;
    config.SessionName = L"AgentSession_SWE_Bench_Task_101";
    config.MaxMemoryBytes = NOTIFICATION_LIMIT_BYTES; // 50 MB Soft/Hard Memory Cap
    config.ActiveProcessLimit = 10;                     // Max 10 active processes
    config.EnableAutoTrimOnIdle = true;                 // Memory compression during LLM reasoning

    // 2. Instantiate AgentSession Engine
    AgentEngine::AgentSession agentSession(config);

    // 3. Register Intent-Driven Natural-Language Feedback Callback for LLM
    agentSession.SetFeedbackCallback([](const std::string& feedbackMsg) {
        printf("\n  [>>> INTENT-DRIVEN FEEDBACK TO LLM MANAGER <<<]\n");
        printf("  %s\n\n", feedbackMsg.c_str());
    });

    if (!agentSession.Initialize()) {
        printf("[-] Failed to initialize AgentEngine Session!\n");
        return 1;
    }
    printf("[+] AgentEngine Session Initialized (Memory Cap: 50 MB, Process Limit: 10)\n");

    // 4. Spawn Child Tool Process
    char szSelfPath[MAX_PATH];
    GetModuleFileNameA(NULL, szSelfPath, MAX_PATH);

    char szCmdLine[MAX_PATH * 2];
    sprintf_s(szCmdLine, sizeof(szCmdLine), "\"%s\" --worker", szSelfPath);

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    if (!CreateProcessA(NULL, szCmdLine, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        printf("[-] Failed to launch child tool process (Error: %lu)\n", GetLastError());
        return 1;
    }

    // Assign tool process to Agent Engine Session
    if (!agentSession.AssignProcess(pi.hProcess)) {
        printf("[-] Failed to assign process to AgentEngine Session!\n");
        return 1;
    }

    ResumeThread(pi.hThread);
    printf("[+] Tool Subprocess spawned (PID: %lu) & bound to AgentEngine Session.\n\n", pi.dwProcessId);

    // 5. Test Memory Trimming (Simulating LLM Reasoning Phase)
    printf("[*] Simulating LLM Reasoning Phase (Idle)... Trimming Working Set to Memory Compression Store...\n");
    agentSession.TrimWorkingSetToCompressStore();

    // Wait for tool execution completion
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    printf("\n[+] AgentEngine Integrated Test completed successfully.\n");
    return 0;
}
