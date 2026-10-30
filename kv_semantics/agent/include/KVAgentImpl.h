// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include "AgentLogger.h"

#define KV_AGENT_MON_AUTH_TIMEOUT_SECONDS 20U

namespace agent {

using KVAgentAsyncCallback = std::function<void(int32_t result, void *user_data)>;

struct KVAgentConfig {
    uint32_t dpu_id = 0;
    int32_t osd_id = -1;
    std::string mon_host;
    std::string osd_key;
    std::string osd_uuid;
    uint32_t osd_mon_heartbeat_interval = 0;
    uint32_t boot_beacon_delay = 0;
    std::string addrs;
};

class KVAgentImpl {
public:
    virtual ~KVAgentImpl() = default;
    virtual int init(const KVAgentConfig& config, AgentLogger *logger) = 0;
    virtual bool is_initialized() const = 0;
    virtual int start_heartbeat() = 0;
    virtual int32_t kv_agent_create_crush_root(char* kvPoolName, uint32_t nameLen, int32_t* osdIdList, uint32_t listSize, KVAgentAsyncCallback cb, void* user_data) = 0;
    virtual int32_t kv_agent_delete_crush_root(char* kvPoolName, uint32_t nameLen, int32_t* osdIdList, uint32_t listSize, KVAgentAsyncCallback cb, void* user_data) = 0;
    virtual int32_t kv_agent_create_ceph_pool(char* kvPoolName, uint32_t kvPoolLen, char* kvnsName, uint32_t kvnsLen, KVAgentAsyncCallback cb, void* user_data) = 0;
    virtual int32_t kv_agent_delete_ceph_pool(char* kvnsName, uint32_t nameLen, KVAgentAsyncCallback cb, void* user_data) = 0;
};

std::unique_ptr<KVAgentImpl> create_kv_osd_agent();

} // namespace agent
