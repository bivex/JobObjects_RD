// ============================================================================
// AgentJobEngine — High-Performance OS Resource Engine for AI Coding Agents
// Native Windows Kernel Architecture (_EJOB / Silos / Working Set Compression)
// ============================================================================

#ifndef AGENT_JOB_ENGINE_HPP
#define AGENT_JOB_ENGINE_HPP

#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

// Custom struct definitions for undocumented / extended Job APIs
#ifndef JobObjectFreezeInformation
#define JobObjectFreezeInformation 18
#endif

#ifndef JobObjectPagePriorityLimitId
#define JobObjectPagePriorityLimitId 14
#endif

#ifndef JOB_OBJECT_MSG_NOTIFICATION_LIMIT
#define JOB_OBJECT_MSG_NOTIFICATION_LIMIT 12
#endif

typedef struct _JOBOBJECT_FREEZE_INFORMATION_ENGINE {
    ULONG ComponentFlags;
    BOOLEAN Freeze;
} JOBOBJECT_FREEZE_INFORMATION_ENGINE;

typedef struct _JOBOBJECT_PAGE_PRIORITY_LIMIT_ENGINE {
    DWORD Enable;
    DWORD PagePriority;
} JOBOBJECT_PAGE_PRIORITY_LIMIT_ENGINE;

namespace AgentEngine {

    // Callback type for receiving Intent-Driven Natural Language Feedback
    using ResourceFeedbackCallback = std::function<void(const std::string& feedbackMsg)>;

    // Config options for an Agent Session
    struct AgentSessionConfig {
        std::wstring SessionName;
        DWORD64 MaxMemoryBytes;       // Hard / Soft Memory Cap
        DWORD CpuRateCap;             // CPU Hard Cap percentage (1-100)
        DWORD ActiveProcessLimit;     // Max child processes
        bool EnableAutoTrimOnIdle;    // Compress working set during LLM reasoning
    };

    class AgentSession {
    public:
        AgentSession(const AgentSessionConfig& config);
        ~AgentSession();

        bool Initialize();
        bool AssignProcess(HANDLE hProcess);
        
        // Tool Call Lifecycle (Nested Job management)
        HANDLE CreateToolChildJob(const std::wstring& toolName, DWORD64 toolMemoryCapBytes);
        
        // Memory Optimization: Compress Working Set to Memory Compression Store
        bool TrimWorkingSetToCompressStore();

        // Control Execution State (Freeze / Thaw)
        bool FreezeJobTree();
        bool ThawJobTree();

        // Register Feedback Handler
        void SetFeedbackCallback(ResourceFeedbackCallback callback) { m_feedbackCallback = callback; }

    private:
        void MonitorLoop();

        AgentSessionConfig m_config;
        HANDLE m_hRootJob;
        HANDLE m_hCompletionPort;
        HANDLE m_hMonitorThread;
        bool m_bRunning;
        ResourceFeedbackCallback m_feedbackCallback;
    };

} // namespace AgentEngine

#endif // AGENT_JOB_ENGINE_HPP
