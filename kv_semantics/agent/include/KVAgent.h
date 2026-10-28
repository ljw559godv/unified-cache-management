#pragma once

#include <stddef.h>
#include <stdint.h>

#include "AgentLogger.h"

#define KV_OSD_AGENT_ABI_VERSION 1U

#define INVALID_AGENT_HANDLE (-20000)

#if defined(__GNUC__)
#define KV_AGENT_EXPORT __attribute__((visibility("default")))
#else
#define KV_AGENT_EXPORT
#endif

#ifdef __cpluspuls
extern "C" {
#endif

typedef struct kv_agent_handle kv_agent_handle;

typedef struct kv_osd_agent_config {
    uint32_t dpu_id;
    const char *mon_host;
    int32_t osd_id;
    // Raw CephX secret (for example, "AQ...==", not a keyring file path).
    const char *osd_keyring;
    const char *osd_uuid;
    uint32_t osd_mon_heartbeat_interval;
    uint32_t boot_beacon_delay;
    const char *addrs;
} kv_osd_agent_config;

KV_AGENT_EXPORT uint32_t kv_agent_get_abi_version(void);
KV_AGENT_EXPORT kv_agent_handle *kv_agent_create(void);
KV_AGENT_EXPORT int32_t kv_agent_init(
    kv_agent_handle *agent,
    const kv_osd_agent_config *config,
    uint32_t abi_version,
    size_t config_size,
    const char *log_dir);
KV_AGENT_EXPORT int32_t kv_agent_is_initialized(kv_agent_handle *agent);
// Starts the asynchronous heartbeat timer and returns immediately.
KV_AGENT_EXPORT int32_t kv_agent_start_heartbeat(kv_agent_handle *agent);
KV_AGENT_EXPORT void kv_agent_destroy(kv_agent_handle *agent);

KV_AGENT_EXPORT int32_t kv_agent_create_crush_root(kv_agent_handle *agent, char* kvPoolName, uint32_t nameLen, int32_t* osdIdList, uint32_t listSize);
KV_AGENT_EXPORT int32_t kv_agent_delete_crush_root(kv_agent_handle *agent, char* kvPoolName, uint32_t nameLen, int32_t* osdIdList, uint32_t listSize);
KV_AGENT_EXPORT int32_t kv_agent_create_ceph_pool(kv_agent_handle *agent, char* kvPoolName, uint32_t kvPoolLen, char* kvnsName, uint32_t kvnsLen);
KV_AGENT_EXPORT int32_t kv_agent_delete_ceph_pool(kv_agent_handle *agent, char* kvnsName, uint32_t nameLen);
#ifdef __cpluspuls
}
#endif
