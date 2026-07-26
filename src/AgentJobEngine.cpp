// ============================================================================
// AgentJobEngine — High-Performance OS Resource Engine for AI Coding Agents
// Implementation
// ============================================================================

#include "AgentJobEngine.hpp"
#include <psapi.h>

namespace AgentEngine {

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
    }

    bool AgentSession::Initialize() {
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
    }

    bool AgentSession::AssignProcess(HANDLE hProcess) {
        if (!m_hRootJob || !hProcess) return false;
        return AssignProcessToJobObject(m_hRootJob, hProcess) != FALSE;
    }

    HANDLE AgentSession::CreateToolChildJob(const std::wstring& toolName, DWORD64 toolMemoryCapBytes) {
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
    }

    bool AgentSession::TrimWorkingSetToCompressStore() {
        if (!m_hRootJob) return false;

        // Lower Page Priority to force Working Set compression by Kernel Memory Manager
        JOBOBJECT_PAGE_PRIORITY_LIMIT_ENGINE pagePriority = { 0 };
        pagePriority.Enable = TRUE;
        pagePriority.PagePriority = 1; // Lowest priority -> Memory Manager compresses idle heap

        return SetInformationJobObject(m_hRootJob, (JOBOBJECTINFOCLASS)JobObjectPagePriorityLimitId, &pagePriority, sizeof(pagePriority)) != FALSE;
    }

    bool AgentSession::FreezeJobTree() {
        if (!m_hRootJob) return false;
        JOBOBJECT_FREEZE_INFORMATION_ENGINE freeze = { 0 };
        freeze.Freeze = TRUE;
        return SetInformationJobObject(m_hRootJob, (JOBOBJECTINFOCLASS)JobObjectFreezeInformation, &freeze, sizeof(freeze)) != FALSE;
    }

    bool AgentSession::ThawJobTree() {
        if (!m_hRootJob) return false;
        JOBOBJECT_FREEZE_INFORMATION_ENGINE freeze = { 0 };
        freeze.Freeze = FALSE;
        return SetInformationJobObject(m_hRootJob, (JOBOBJECTINFOCLASS)JobObjectFreezeInformation, &freeze, sizeof(freeze)) != FALSE;
    }

    void AgentSession::MonitorLoop() {
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
    }

} // namespace AgentEngine
