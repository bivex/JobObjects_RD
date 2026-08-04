#ifndef AGENT_JOB_ENGINE_C_API_H
#define AGENT_JOB_ENGINE_C_API_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* AgentSessionHandle;

AgentSessionHandle AgentEngine_CreateSession(const char* session_name, uint64_t max_memory_bytes, uint32_t cpu_rate_cap, bool auto_trim);
void AgentEngine_DestroySession(AgentSessionHandle handle);
bool AgentEngine_AssignProcess(AgentSessionHandle handle, int32_t pid);
bool AgentEngine_TrimWorkingSet(AgentSessionHandle handle);
bool AgentEngine_FreezeJobTree(AgentSessionHandle handle);
bool AgentEngine_ThawJobTree(AgentSessionHandle handle);
bool AgentEngine_SetIoRateLimit(AgentSessionHandle handle, const char* volume_name, uint64_t max_iops, uint64_t max_bandwidth);
bool AgentEngine_SetNetworkRateLimit(AgentSessionHandle handle, uint64_t max_bandwidth);
bool AgentEngine_CreateSiloSandbox(AgentSessionHandle handle);

#ifdef __cplusplus
}
#endif

#endif // AGENT_JOB_ENGINE_C_API_H
