#include "AgentJobEngine_C_API.h"
#include "AgentJobEngine.hpp"

extern "C" {

AgentSessionHandle AgentEngine_CreateSession(const char* session_name, uint64_t max_memory_bytes, uint32_t cpu_rate_cap, bool auto_trim) {
    AgentEngine::AgentSessionConfig config;
    if (session_name) {
        std::string name(session_name);
        config.SessionName = std::wstring(name.begin(), name.end());
    } else {
        config.SessionName = L"DefaultSession";
    }
    config.MaxMemoryBytes = max_memory_bytes > 0 ? max_memory_bytes : (512 * 1024 * 1024ULL);
    config.CpuRateCap = cpu_rate_cap;
    config.ActiveProcessLimit = 50;
    config.EnableAutoTrimOnIdle = auto_trim;

    auto session = new AgentEngine::AgentSession(config);
    if (!session->Initialize()) {
        delete session;
        return nullptr;
    }
    return static_cast<AgentSessionHandle>(session);
}

void AgentEngine_DestroySession(AgentSessionHandle handle) {
    if (handle) {
        auto session = static_cast<AgentEngine::AgentSession*>(handle);
        delete session;
    }
}

bool AgentEngine_AssignProcess(AgentSessionHandle handle, int32_t pid) {
    if (!handle) return false;
    auto session = static_cast<AgentEngine::AgentSession*>(handle);
#ifdef _WIN32
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, (DWORD)pid);
    if (!hProc) return false;
    bool res = session->AssignProcess(hProc);
    CloseHandle(hProc);
    return res;
#else
    return session->AssignProcess((HANDLE)(intptr_t)pid);
#endif
}

bool AgentEngine_TrimWorkingSet(AgentSessionHandle handle) {
    if (!handle) return false;
    return static_cast<AgentEngine::AgentSession*>(handle)->TrimWorkingSetToCompressStore();
}

bool AgentEngine_FreezeJobTree(AgentSessionHandle handle) {
    if (!handle) return false;
    return static_cast<AgentEngine::AgentSession*>(handle)->FreezeJobTree();
}

bool AgentEngine_ThawJobTree(AgentSessionHandle handle) {
    if (!handle) return false;
    return static_cast<AgentEngine::AgentSession*>(handle)->ThawJobTree();
}

bool AgentEngine_SetIoRateLimit(AgentSessionHandle handle, const char* volume_name, uint64_t max_iops, uint64_t max_bandwidth) {
    if (!handle) return false;
    std::wstring vol = L"/";
    if (volume_name) {
        std::string v(volume_name);
        vol = std::wstring(v.begin(), v.end());
    }
    return static_cast<AgentEngine::AgentSession*>(handle)->SetIoRateLimit(vol, max_iops, max_bandwidth);
}

bool AgentEngine_SetNetworkRateLimit(AgentSessionHandle handle, uint64_t max_bandwidth) {
    if (!handle) return false;
    return static_cast<AgentEngine::AgentSession*>(handle)->SetNetworkRateLimit(max_bandwidth);
}

bool AgentEngine_CreateSiloSandbox(AgentSessionHandle handle) {
    if (!handle) return false;
    return static_cast<AgentEngine::AgentSession*>(handle)->CreateSiloSandbox();
}

}
