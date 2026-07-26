// ============================================================================
// AgentJobEngine — High-Performance OS Resource Engine for AI Coding Agents
// Native Windows Kernel Architecture (_EJOB / Silos / Working Set Compression)
// ============================================================================

#ifndef AGENT_JOB_ENGINE_HPP
#define AGENT_JOB_ENGINE_HPP

#include <windows.h>
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

#ifndef JobObjectIoRateControlInformation
#define JobObjectIoRateControlInformation 19
#endif

#ifndef JobObjectNetRateControlInformation
#define JobObjectNetRateControlInformation 32
#endif

#ifndef JOB_OBJECT_MSG_NOTIFICATION_LIMIT
#define JOB_OBJECT_MSG_NOTIFICATION_LIMIT 12
#endif

typedef struct _JOBOBJECT_FREEZE_INFORMATION_ENGINE {
    ULONG ComponentFlags; // +0x00: 1 = JOB_OBJECT_FREEZE_OPTION_FREEZE
    BOOLEAN Freeze;       // +0x04: TRUE = Freeze, FALSE = Thaw
    BOOLEAN Filter;       // +0x05: 0
    UCHAR Reserved[10];   // +0x06: 16 bytes total size
} JOBOBJECT_FREEZE_INFORMATION_ENGINE;

typedef struct _JOBOBJECT_PAGE_PRIORITY_LIMIT_ENGINE {
    DWORD Enable;
    DWORD PagePriority;
} JOBOBJECT_PAGE_PRIORITY_LIMIT_ENGINE;

typedef enum _JOB_OBJECT_IO_RATE_CONTROL_FLAGS_ENGINE {
    JOB_OBJECT_IO_RATE_CONTROL_ENABLE_ENGINE = 0x1,
    JOB_OBJECT_IO_RATE_CONTROL_STANDALONE_VOLUME_ENGINE = 0x2,
    JOB_OBJECT_IO_RATE_CONTROL_FORCE_UNIT_ACCESS_ENGINE = 0x4
} JOB_OBJECT_IO_RATE_CONTROL_FLAGS_ENGINE;

typedef struct _JOBOBJECT_IO_RATE_CONTROL_INFORMATION_ENGINE {
    LONG64 MaxIops;
    LONG64 MaxBandwidth;
    LONG64 ReservationIops;
    PWSTR  VolumeName;
    DWORD  BaseIoSize;
    DWORD  ControlFlags;
} JOBOBJECT_IO_RATE_CONTROL_INFORMATION_ENGINE;

typedef enum _JOB_OBJECT_NET_RATE_CONTROL_FLAGS_ENGINE {
    JOB_OBJECT_NET_RATE_CONTROL_ENABLE_ENGINE = 0x1,
    JOB_OBJECT_NET_RATE_CONTROL_MAX_BANDWIDTH_ENGINE = 0x2,
    JOB_OBJECT_NET_RATE_CONTROL_DSCP_TAG_ENGINE = 0x4
} JOB_OBJECT_NET_RATE_CONTROL_FLAGS_ENGINE;

typedef struct _JOBOBJECT_NET_RATE_CONTROL_INFORMATION_ENGINE {
    DWORD64 MaxBandwidth;
    DWORD   ControlFlags;
    BYTE    DscpTag;
} JOBOBJECT_NET_RATE_CONTROL_INFORMATION_ENGINE;

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

        // Resource Control: Disk I/O & Network Rate Limiting
        bool SetIoRateLimit(const std::wstring& volumeName, DWORD64 maxIops, DWORD64 maxBandwidthBytesPerSec);
        bool SetNetworkRateLimit(DWORD64 maxBandwidthBytesPerSec);

        // Container Sandbox: Server Silos (HKLM Registry & Object Namespace Isolation)
        bool CreateSiloSandbox();

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
