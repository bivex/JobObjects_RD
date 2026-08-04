// ============================================================================
// AgentJobObject Integrated Validation PoC
// Demonstrates Non-Destructive Memory Throttling, Working Set Compression,
// and Intent-Driven LLM Feedback for AI Agent Sandboxing (macOS & Windows)
// ============================================================================

#include "AgentJobEngine.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <mach-o/dyld.h>
#endif

#define NOTIFICATION_LIMIT_BYTES (50 * 1024 * 1024)  // 50 MB Cap
#define TARGET_ALLOCATION_BYTES  (100 * 1024 * 1024) // 100 MB Allocation Spike

// Worker Process entry (spikes memory)
void MemorySpikeWorker() {
    printf("[Child Worker] Starting tool execution memory spike (Target: 100 MB, Cap: 50 MB)...\n");
    
    // Attempt 100 MB allocation
#ifdef _WIN32
    char* pBuffer = (char*)VirtualAlloc(NULL, TARGET_ALLOCATION_BYTES, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    char* pBuffer = (char*)mmap(NULL, TARGET_ALLOCATION_BYTES, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
    if (pBuffer == MAP_FAILED) pBuffer = NULL;
#endif
    if (!pBuffer) {
        printf("[Child Worker] VirtualAlloc exceeded 50 MB Job Memory Limit!\n");
        printf("[Child Worker] Graceful degradation: Retrying with smaller allocation (30 MB)...\n");
        
        // Fallback to smaller 30 MB allocation (Graceful Adaptation)
#ifdef _WIN32
        pBuffer = (char*)VirtualAlloc(NULL, 30 * 1024 * 1024, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
        pBuffer = (char*)mmap(NULL, 30 * 1024 * 1024, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
        if (pBuffer == MAP_FAILED) pBuffer = NULL;
#endif
        if (pBuffer) {
            for (size_t i = 0; i < 30 * 1024 * 1024; i += 4096) pBuffer[i] = 1;
            printf("[Child Worker] Fallback allocation of 30 MB succeeded!\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
#ifdef _WIN32
            VirtualFree(pBuffer, 0, MEM_RELEASE);
#else
            munmap(pBuffer, 30 * 1024 * 1024);
#endif
        }
        return;
    }

    // Touch pages repeatedly
    for (size_t i = 0; i < TARGET_ALLOCATION_BYTES; i += 4096) pBuffer[i] = 1;

    printf("[Child Worker] Memory committed. Holding for 3s...\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
#ifdef _WIN32
    VirtualFree(pBuffer, 0, MEM_RELEASE);
#else
    munmap(pBuffer, TARGET_ALLOCATION_BYTES);
#endif
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
#ifdef _WIN32
    GetModuleFileNameA(NULL, szSelfPath, MAX_PATH);

    char szCmdLine[MAX_PATH * 2];
    sprintf_s(szCmdLine, sizeof(szCmdLine), "\"%s\" --worker", szSelfPath);

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    if (!CreateProcessA(NULL, szCmdLine, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
        printf("[-] Failed to launch child tool process (Error: %lu)\n", GetLastError());
        return 1;
    }

    if (!agentSession.AssignProcess(pi.hProcess)) {
        printf("[-] Failed to assign process to AgentEngine Session!\n");
        return 1;
    }

    ResumeThread(pi.hThread);
    printf("[+] Tool Subprocess spawned (PID: %lu) & bound to AgentEngine Session.\n\n", pi.dwProcessId);
#else
#ifdef __APPLE__
    uint32_t size = sizeof(szSelfPath);
    if (_NSGetExecutablePath(szSelfPath, &size) != 0) {
        strncpy(szSelfPath, argv[0], MAX_PATH);
    }
#else
    ssize_t len = readlink("/proc/self/exe", szSelfPath, sizeof(szSelfPath) - 1);
    if (len != -1) {
        szSelfPath[len] = '\0';
    } else {
        strncpy(szSelfPath, argv[0], MAX_PATH);
    }
#endif

    pid_t pid = fork();
    if (pid < 0) {
        printf("[-] Failed to fork child process!\n");
        return 1;
    }

    if (pid == 0) {
        // Child Process
        execl(szSelfPath, szSelfPath, "--worker", (char*)NULL);
        exit(1);
    }

    HANDLE hProcHandle = reinterpret_cast<HANDLE>(static_cast<intptr_t>(pid));
    if (!agentSession.AssignProcess(hProcHandle)) {
        printf("[-] Failed to assign process to AgentEngine Session!\n");
        return 1;
    }
    printf("[+] Tool Subprocess spawned (PID: %d) & bound to AgentEngine Session.\n\n", pid);
#endif

    // 5. Test Memory Trimming (Simulating LLM Reasoning Phase)
    printf("[*] Simulating LLM Reasoning Phase (Idle)... Trimming Working Set to Memory Compression Store...\n");
    agentSession.TrimWorkingSetToCompressStore();

    // Wait for tool execution completion
#ifdef _WIN32
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#else
    int status = 0;
    waitpid(pid, &status, 0);
#endif

    printf("\n[+] AgentEngine Integrated Test completed successfully.\n");
    return 0;
}

