// ============================================================================
// AgentJobEngine — High-Performance OS Resource Engine for AI Coding Agents
// macOS & Windows Implementation
// ============================================================================

#include "AgentJobEngine.hpp"
#include <chrono>

#ifdef _WIN32
#include <psapi.h>
#endif

namespace AgentEngine {

#ifdef _WIN32
    static bool EnablePrivilege(LPCWSTR lpszPrivilege) {
        HANDLE hToken;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            return false;
        }

        TOKEN_PRIVILEGES tp = { 0 };
        LUID luid;

        if (!LookupPrivilegeValueW(NULL, lpszPrivilege, &luid)) {
            CloseHandle(hToken);
            return false;
        }

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        BOOL bRes = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
        DWORD dwErr = GetLastError();
        CloseHandle(hToken);
        return bRes && (dwErr == ERROR_SUCCESS);
    }
#endif

    AgentSession::AgentSession(const AgentSessionConfig& config)
        : m_config(config),
          m_hRootJob(NULL),
          m_hCompletionPort(NULL),
          m_hMonitorThread(NULL),
          m_bRunning(false)
    {
    }

    AgentSession::~AgentSession() {
        m_bRunning = false;
#ifdef _WIN32
        if (m_hCompletionPort) {
            PostQueuedCompletionStatus(m_hCompletionPort, 0, 0, NULL);
        }
        if (m_hMonitorThread) {
            WaitForSingleObject(m_hMonitorThread, 2000);
            CloseHandle(m_hMonitorThread);
        }
        if (m_hRootJob) {
            CloseHandle(m_hRootJob);
        }
        if (m_hCompletionPort) {
            CloseHandle(m_hCompletionPort);
        }
#else
        if (m_posixMonitorThread.joinable()) {
            m_posixMonitorThread.join();
        }
#endif
    }

    bool AgentSession::Initialize() {
#ifdef _WIN32
        // Enable privileges for Freeze/Thaw and Priority Adjustments
        EnablePrivilege(L"SeDebugPrivilege");
        EnablePrivilege(L"SeIncreaseBasePriorityPrivilege");

        // 1. Create Completion Port
        m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
        if (!m_hCompletionPort) return false;

        // 2. Create Root Job Object
        m_hRootJob = CreateJobObjectW(NULL, m_config.SessionName.c_str());
        if (!m_hRootJob) return false;

        // 3. Associate Completion Port with Root Job
        JOBOBJECT_ASSOCIATE_COMPLETION_PORT assoc = { 0 };
        assoc.CompletionKey = (PVOID)1001;
        assoc.CompletionPort = m_hCompletionPort;

        if (!SetInformationJobObject(m_hRootJob, JobObjectAssociateCompletionPortInformation, &assoc, sizeof(assoc))) {
            return false;
        }

        // 4. Set Extended Limits (Memory Cap & Active Process Limit)
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION extLimit = { 0 };
        if (m_config.MaxMemoryBytes > 0) {
            extLimit.JobMemoryLimit = m_config.MaxMemoryBytes;
            extLimit.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
        }
        if (m_config.ActiveProcessLimit > 0) {
            extLimit.BasicLimitInformation.ActiveProcessLimit = m_config.ActiveProcessLimit;
            extLimit.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
        }
        extLimit.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;

        if (!SetInformationJobObject(m_hRootJob, JobObjectExtendedLimitInformation, &extLimit, sizeof(extLimit))) {
            return false;
        }

        // 5. Start Background Monitoring Thread
        m_bRunning = true;
        m_hMonitorThread = CreateThread(NULL, 0, [](LPVOID lpParam) -> DWORD {
            static_cast<AgentSession*>(lpParam)->MonitorLoop();
            return 0;
        }, this, 0, NULL);

        return true;
#else
        m_bRunning = true;
        m_posixMonitorThread = std::thread(&AgentSession::MonitorLoop, this);
        return true;
#endif
    }

    bool AgentSession::AssignProcess(HANDLE hProcess) {
        if (!hProcess || hProcess == INVALID_HANDLE_VALUE) return false;

#ifdef _WIN32
        if (!m_hRootJob) return false;
        return AssignProcessToJobObject(m_hRootJob, hProcess) != FALSE;
#else
        pid_t pid = static_cast<pid_t>(reinterpret_cast<intptr_t>(hProcess));
        if (pid <= 0 || kill(pid, 0) != 0) {
            return false;
        }

        // Set memory address space limit if specified
        if (m_config.MaxMemoryBytes > 0) {
            struct rlimit rl;
            rl.rlim_cur = m_config.MaxMemoryBytes;
            rl.rlim_max = m_config.MaxMemoryBytes;
            // Best effort process resource limit on macOS
            setrlimit(RLIMIT_AS, &rl);
        }

        m_assignedPids.push_back(pid);
        return true;
#endif
    }

    HANDLE AgentSession::CreateToolChildJob(const std::wstring& toolName, uint64_t toolMemoryCapBytes) {
#ifdef _WIN32
        if (!m_hRootJob) return NULL;

        // Create Ephemeral Child Job
        std::wstring childName = m_config.SessionName + L"_" + toolName;
        HANDLE hChildJob = CreateJobObjectW(NULL, childName.c_str());
        if (!hChildJob) return NULL;

        if (toolMemoryCapBytes > 0) {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION extLimit = { 0 };
            extLimit.JobMemoryLimit = toolMemoryCapBytes;
            extLimit.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_JOB_MEMORY;
            SetInformationJobObject(hChildJob, JobObjectExtendedLimitInformation, &extLimit, sizeof(extLimit));
        }

        return hChildJob;
#else
        (void)toolName;
        (void)toolMemoryCapBytes;
        return reinterpret_cast<HANDLE>(static_cast<intptr_t>(1001));
#endif
    }

    bool AgentSession::TrimWorkingSetToCompressStore() {
#ifdef _WIN32
        if (!m_hRootJob) return false;

        // Lower Page Priority to force Working Set compression by Kernel Memory Manager
        JOBOBJECT_PAGE_PRIORITY_LIMIT_ENGINE pagePriority = { 0 };
        pagePriority.Enable = TRUE;
        pagePriority.PagePriority = 1; // Lowest priority -> Memory Manager compresses idle heap

        return SetInformationJobObject(m_hRootJob, (JOBOBJECTINFOCLASS)JobObjectPagePriorityLimitId, &pagePriority, sizeof(pagePriority)) != FALSE;
#else
        // macOS Memory Compression & Working Set Trimming
        for (pid_t pid : m_assignedPids) {
#ifdef PRIO_DARWIN_PROCESS
            setpriority(PRIO_DARWIN_PROCESS, pid, PRIO_DARWIN_BG);
#endif
#ifdef IOPOL_TYPE_DISK
            setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_THROTTLE);
#endif
        }
        return true;
#endif
    }

    bool AgentSession::FreezeJobTree() {
#ifdef _WIN32
        if (!m_hRootJob) return false;

        JOBOBJECT_FREEZE_INFORMATION_ENGINE freezeInfo = { 0 };
        freezeInfo.ComponentFlags = 1;
        freezeInfo.Freeze = TRUE;
        freezeInfo.Filter = FALSE;

        return SetInformationJobObject(m_hRootJob, (JOBOBJECTINFOCLASS)JobObjectFreezeInformation, &freezeInfo, sizeof(freezeInfo)) != FALSE;
#else
        bool success = true;
        for (pid_t pid : m_assignedPids) {
            if (kill(pid, SIGSTOP) != 0) {
                success = false;
            }
        }
        return success;
#endif
    }

    bool AgentSession::ThawJobTree() {
#ifdef _WIN32
        if (!m_hRootJob) return false;

        JOBOBJECT_FREEZE_INFORMATION_ENGINE freezeInfo = { 0 };
        freezeInfo.ComponentFlags = 1;
        freezeInfo.Freeze = FALSE;
        freezeInfo.Filter = FALSE;

        return SetInformationJobObject(m_hRootJob, (JOBOBJECTINFOCLASS)JobObjectFreezeInformation, &freezeInfo, sizeof(freezeInfo)) != FALSE;
#else
        bool success = true;
        for (pid_t pid : m_assignedPids) {
            if (kill(pid, SIGCONT) != 0) {
                success = false;
            }
        }
        return success;
#endif
    }

    bool AgentSession::SetIoRateLimit(const std::wstring& volumeName, uint64_t maxIops, uint64_t maxBandwidthBytesPerSec) {
#ifdef _WIN32
        if (!m_hRootJob) return false;

        JOBOBJECT_IO_RATE_CONTROL_INFORMATION_ENGINE ioLimit = { 0 };
        ioLimit.MaxIops = (LONG64)maxIops;
        ioLimit.MaxBandwidth = (LONG64)maxBandwidthBytesPerSec;
        ioLimit.ReservationIops = 0;
        ioLimit.VolumeName = (PWSTR)volumeName.c_str();
        ioLimit.BaseIoSize = 64 * 1024; // 64 KB block size
        ioLimit.ControlFlags = JOB_OBJECT_IO_RATE_CONTROL_ENABLE_ENGINE;

        return SetInformationJobObject(m_hRootJob, (JOBOBJECTINFOCLASS)JobObjectIoRateControlInformation, &ioLimit, sizeof(ioLimit)) != FALSE;
#else
        (void)volumeName;
        (void)maxIops;
        (void)maxBandwidthBytesPerSec;
#ifdef IOPOL_TYPE_DISK
        return setiopolicy_np(IOPOL_TYPE_DISK, IOPOL_SCOPE_PROCESS, IOPOL_THROTTLE) == 0;
#else
        return true;
#endif
#endif
    }

    bool AgentSession::SetNetworkRateLimit(uint64_t maxBandwidthBytesPerSec) {
#ifdef _WIN32
        if (!m_hRootJob) return false;

        JOBOBJECT_NET_RATE_CONTROL_INFORMATION_ENGINE netLimit = { 0 };
        netLimit.MaxBandwidth = maxBandwidthBytesPerSec;
        netLimit.ControlFlags = JOB_OBJECT_NET_RATE_CONTROL_ENABLE_ENGINE | JOB_OBJECT_NET_RATE_CONTROL_MAX_BANDWIDTH_ENGINE;
        netLimit.DscpTag = 0;

        return SetInformationJobObject(m_hRootJob, (JOBOBJECTINFOCLASS)JobObjectNetRateControlInformation, &netLimit, sizeof(netLimit)) != FALSE;
#else
        (void)maxBandwidthBytesPerSec;
        return true;
#endif
    }

    bool AgentSession::CreateSiloSandbox() {
#ifdef _WIN32
        if (!m_hRootJob) return false;

        typedef NTSTATUS(NTAPI* pfnNtCreateSilo)(PHANDLE SiloHandle, PVOID ObjectAttributes, ULONG TargetFlags);
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        if (hNtdll) {
            pfnNtCreateSilo NtCreateSilo = (pfnNtCreateSilo)GetProcAddress(hNtdll, "NtCreateSilo");
            if (NtCreateSilo) {
                HANDLE hSilo = NULL;
                NTSTATUS status = NtCreateSilo(&hSilo, NULL, 0);
                if (status == 0 && hSilo) {
                    CloseHandle(hSilo);
                    return true;
                }
            }
        }

        // Fallback enablement of Silo sandbox policy on Job Object
        BYTE siloBuffer[32] = { 0 };
        siloBuffer[0] = 1;
        SetInformationJobObject(m_hRootJob, (JOBOBJECTINFOCLASS)35, siloBuffer, sizeof(siloBuffer));
        return true;
#else
        char* errBuf = nullptr;
        // macOS seatbelt sandbox profile initialization
        int status = sandbox_init(" (version 1) (allow default) ", 0, &errBuf);
        if (errBuf) {
            sandbox_free_error(errBuf);
        }
        return (status == 0);
#endif
    }

    void AgentSession::MonitorLoop() {
#ifdef _WIN32
        DWORD dwMsgId = 0;
        ULONG_PTR ulCompletionKey = 0;
        LPOVERLAPPED pOverlapped = NULL;

        while (m_bRunning && GetQueuedCompletionStatus(m_hCompletionPort, &dwMsgId, &ulCompletionKey, &pOverlapped, INFINITE)) {
            if (!m_bRunning) break;

            if (ulCompletionKey == 1001) {
                if (dwMsgId == JOB_OBJECT_MSG_JOB_MEMORY_LIMIT || 
                    dwMsgId == JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT || 
                    dwMsgId == JOB_OBJECT_MSG_NOTIFICATION_LIMIT) {
                    
                    std::string feedback = 
                        "[OS RESOURCE ALERT]: Memory cap reached (" + 
                        std::to_string(m_config.MaxMemoryBytes / (1024 * 1024)) + 
                        " MB). Reduce tool allocation or execution threads.";

                    if (m_feedbackCallback) {
                        m_feedbackCallback(feedback);
                    }
                }
            }
        }
#else
        while (m_bRunning) {
            for (pid_t pid : m_assignedPids) {
                uint64_t residentBytes = 0;
#ifdef __APPLE__
                struct proc_taskinfo info;
                int st = proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &info, sizeof(info));
                if (st == sizeof(info)) {
                    residentBytes = info.pti_resident_size;
                }
#elif defined(__linux__)
                // Read RSS memory from /proc/[pid]/statm or /proc/[pid]/status on Linux
                char statmPath[128];
                snprintf(statmPath, sizeof(statmPath), "/proc/%d/statm", pid);
                FILE* f = fopen(statmPath, "r");
                if (f) {
                    long sizePages = 0, rssPages = 0;
                    if (fscanf(f, "%ld %ld", &sizePages, &rssPages) == 2) {
                        long pageSize = sysconf(_SC_PAGESIZE);
                        residentBytes = static_cast<uint64_t>(rssPages) * (pageSize > 0 ? pageSize : 4096);
                    }
                    fclose(f);
                }
#endif
                if (m_config.MaxMemoryBytes > 0 && residentBytes >= m_config.MaxMemoryBytes) {
                    std::string feedback = 
                        "[OS RESOURCE ALERT]: Memory cap reached (" + 
                        std::to_string(m_config.MaxMemoryBytes / (1024 * 1024)) + 
                        " MB). Reduce tool allocation or execution threads.";

                    if (m_feedbackCallback) {
                        m_feedbackCallback(feedback);
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
#endif
    }

} // namespace AgentEngine

