// ============================================================================
// AgentJobEngine — High-Performance OS Resource Engine for AI Coding Agents
// macOS Darwin Kernel & Native Windows Kernel Architecture
// ============================================================================

#ifndef AGENT_JOB_ENGINE_HPP
#define AGENT_JOB_ENGINE_HPP

#include <stdio.h>
#include <stdlib.h>
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <sys/sysctl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <signal.h>
#include <pthread.h>
#include <mach/mach.h>
#include <mach/task.h>
#include <sandbox.h>
#include <libproc.h>
#include <sys/mman.h>

#ifndef HANDLE
typedef void* HANDLE;
#endif
#ifndef DWORD
typedef uint32_t DWORD;
#endif
#ifndef DWORD64
typedef uint64_t DWORD64;
#endif
#ifndef LONG64
typedef int64_t LONG64;
#endif
#ifndef PWSTR
typedef wchar_t* PWSTR;
#endif
#ifndef LPCWSTR
typedef const wchar_t* LPCWSTR;
#endif
#ifndef MAX_PATH
#define MAX_PATH 1024
#endif
#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)
#endif
#endif

// Custom struct definitions for extended Job APIs
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
    uint32_t ComponentFlags; // +0x00: 1 = JOB_OBJECT_FREEZE_OPTION_FREEZE
    uint8_t  Freeze;         // +0x04: TRUE = Freeze, FALSE = Thaw
    uint8_t  Filter;         // +0x05: 0
    uint8_t  Reserved[10];   // +0x06: 16 bytes total size
} JOBOBJECT_FREEZE_INFORMATION_ENGINE;

typedef struct _JOBOBJECT_PAGE_PRIORITY_LIMIT_ENGINE {
    uint32_t Enable;
    uint32_t PagePriority;
} JOBOBJECT_PAGE_PRIORITY_LIMIT_ENGINE;

typedef enum _JOB_OBJECT_IO_RATE_CONTROL_FLAGS_ENGINE {
    JOB_OBJECT_IO_RATE_CONTROL_ENABLE_ENGINE = 0x1,
    JOB_OBJECT_IO_RATE_CONTROL_STANDALONE_VOLUME_ENGINE = 0x2,
    JOB_OBJECT_IO_RATE_CONTROL_FORCE_UNIT_ACCESS_ENGINE = 0x4
} JOB_OBJECT_IO_RATE_CONTROL_FLAGS_ENGINE;

typedef struct _JOBOBJECT_IO_RATE_CONTROL_INFORMATION_ENGINE {
    int64_t MaxIops;
    int64_t MaxBandwidth;
    int64_t ReservationIops;
    PWSTR   VolumeName;
    uint32_t BaseIoSize;
    uint32_t ControlFlags;
} JOBOBJECT_IO_RATE_CONTROL_INFORMATION_ENGINE;

typedef enum _JOB_OBJECT_NET_RATE_CONTROL_FLAGS_ENGINE {
    JOB_OBJECT_NET_RATE_CONTROL_ENABLE_ENGINE = 0x1,
    JOB_OBJECT_NET_RATE_CONTROL_MAX_BANDWIDTH_ENGINE = 0x2,
    JOB_OBJECT_NET_RATE_CONTROL_DSCP_TAG_ENGINE = 0x4
} JOB_OBJECT_NET_RATE_CONTROL_FLAGS_ENGINE;

typedef struct _JOBOBJECT_NET_RATE_CONTROL_INFORMATION_ENGINE {
    uint64_t MaxBandwidth;
    uint32_t ControlFlags;
    uint8_t  DscpTag;
} JOBOBJECT_NET_RATE_CONTROL_INFORMATION_ENGINE;

namespace AgentEngine {

    // Callback type for receiving Intent-Driven Natural Language Feedback
    using ResourceFeedbackCallback = std::function<void(const std::string& feedbackMsg)>;

    // Config options for an Agent Session
    struct AgentSessionConfig {
        std::wstring SessionName;
        uint64_t MaxMemoryBytes;       // Hard / Soft Memory Cap
        uint32_t CpuRateCap;             // CPU Hard Cap percentage (1-100)
        uint32_t ActiveProcessLimit;     // Max child processes
        bool EnableAutoTrimOnIdle;    // Compress working set during LLM reasoning
    };

    class AgentSession {
    public:
        AgentSession(const AgentSessionConfig& config);
        ~AgentSession();

        bool Initialize();
        bool AssignProcess(HANDLE hProcess);
        
        // Tool Call Lifecycle (Nested Job management)
        HANDLE CreateToolChildJob(const std::wstring& toolName, uint64_t toolMemoryCapBytes);
        
        // Memory Optimization: Compress Working Set to Memory Compression Store
        bool TrimWorkingSetToCompressStore();

        // Control Execution State (Freeze / Thaw)
        bool FreezeJobTree();
        bool ThawJobTree();

        // Resource Control: Disk I/O & Network Rate Limiting
        bool SetIoRateLimit(const std::wstring& volumeName, uint64_t maxIops, uint64_t maxBandwidthBytesPerSec);
        bool SetNetworkRateLimit(uint64_t maxBandwidthBytesPerSec);

        // Container Sandbox: Server Silos / macOS Sandbox
        bool CreateSiloSandbox();

        // Register Feedback Handler
        void SetFeedbackCallback(ResourceFeedbackCallback callback) { m_feedbackCallback = callback; }

    private:
        void MonitorLoop();

        AgentSessionConfig m_config;
        HANDLE m_hRootJob;
        HANDLE m_hCompletionPort;
        HANDLE m_hMonitorThread;
        std::atomic<bool> m_bRunning;
        ResourceFeedbackCallback m_feedbackCallback;
#ifndef _WIN32
        std::vector<pid_t> m_assignedPids;
        std::thread m_posixMonitorThread;
#endif
    };

} // namespace AgentEngine

#endif // AGENT_JOB_ENGINE_HPP
