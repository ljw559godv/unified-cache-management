// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*-
// vim: ts=8 sw=2 smarttab

#include <KVAgent.h>

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

nammespace {

const char* get_arg(int argc, char** argv, const std::string& name)
{
    for (int i = 0; i < argc; ++i) {
        if (argv[i] == name) {
            return argv[i + 1];
        }
    }
    return nullptr;
}

bool has_flag(int argc, char** argv, const std::string& name)
{
    for (int i = 0; i < argc; ++i) {
        if (argv[i] == name) {
            return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char** argv)
{
    const char *mon_host = get_arg(argc, argv, "--mon-host"); 
    const char *client_name = get_arg(argc, argv, "--client-name");
    const char *client_key = get_arg(argc, argv, "--client-key");
    const char *log_dir = get_arg(argc, argv, "--log-dir");
    const char *wait_arg = get_arg(argc, argv, "--wait");
    const char *pool_name = get_arg(argc, argv, "--pool-name");
    const char *ns_name = get_arg(argc, argv, "--ns-name");
    bool flag_create_only = has_flag(argc, argv, "--create-only");
    bool flag_delete_only = has_flag(argc, argv, "--delete-only");

    if (!mon_host || !client_name || !client_key) {
        std::cerr << "usage: crush-smoke --mon-host <host> "
                     "--client-name <client.name> --client-key <raw-cephx-key> "
                     "[--log-dir <directory>] [--wait <seconds>] "
                     "[--pool-name <kvpool-name>] [--ns-name <kvns-name>] "
                     "[--create-only] [--delete-only]"
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Default names for smoke test
    std::string pool_name_str(pool_name ? pool_name : "kvpool-test");
    std::string ns_name_str(ns_name ? ns_name : "kvns-test");

    unsigned wait_seconds = wait_arg ? std::strtoul(wait_arg, nullptr, 10) : 2;

    std::cerr << "[trace] main: pool_name=" << pool_name_str
              << " ns_name=" << ns_name_str << std::endl;

    // Initialize the agent
    kv_agent_handle *agent = kv_agent_create();
    if (!agent) {
        std::cerr << "kv_agent_create failed" << std::endl;
        return EXIT_FAILURE;
    }

    kv_osd_agent_config config{};
    config.monhost = mon_host;
    config.osd_keyring = client_key;
    config.osd_id = 0 // dummy, OSD id, not used for crush operations
    config.dpu_id = 0;
    config.osd_uuid = "bffe772f-e279-4919-99cb-82f9584172b4";
    config.osd_mon_heartbeat_interval = 10;
    config.boot_beacon_delay = 5;
    config.addrs = "0.0.0.0:0";

    int result = kv_agent_init(agent, &config, KV_OSD_AGENT_ABI_VERSION,
                               sizeof(config), log_dir);
    if (result < 0) {
        std::cerr << "kv_agent_init failed: " << result << std::endl;
        kv_agent_destroy(agent);
        return EXIT_FAILURE;
    }
    std::cerr << "[trace] kv_agent_init succeeded" << std::endl;

    // Give the agent time to connect and authenticate
    std::this_thread::sleep_for(std::chrono::seconds(wait_seconds));

    if (flag_delete_only) {
        // Delete operations only (for cleanup after failed create)
        std::cerr << "[trace] === delete-only mode ===" << std::endl;

        std::cerr << "[trace] calling kv_agent_delete_ceph_pool ns=" << ns_name_str << std::endl;
        result = kv_agent_delete_ceph_pool(agent, const_cast<char*>(ns_name_str.c_str()),
                                           static_cast<uint32_t>(ns_name_str.size()));
        std::cerr << "[trace] kv_agent_delete_ceph_pool result=" << result << std::endl;
        if (result != 0) {
            std::cerr << "kv_agent_delete_ceph_pool failed: " << result << std::endl;
        }

        int32_t osdIdList_empty[1] = {0};
        std::cerr << "[trace] calling kv_agent_delete_crush_root pool=" << pool_name_str << std::endl;
        result = kv_agent_delete_crush_root(agent, const_cast<char*>(pool_name_str.c_str()),
                                           static_cast<uint32_t>(pool_name_str.size()),
                                           osdIdList_empty, 0);
        std::cerr << "[trace] kv_agent_delete_crush_root result=" << result << std::endl;
        if (result != 0) {
            std::cerr << "kv_agent_delete_crush_root failed: " << result << std::endl;
        }

        kv_agent_destroy(agent);
        return (result == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (!flag_create_only) {
        // Full test: create then delete
        std::cerr << "[trace] === create phase ===" << std::endl;

        // Step 1: Create CRUSH root
        // Note: For a real test, you would pass actual OSD IDs
        int32_t osdIdList_empty[1] = {0};
        std::cerr << "[trace] calling kv_agent_create_crush_root pool=" << pool_name_str << std::endl;
        result = kv_agent_create_crush_root(agent, const_cast<char*>(pool_name_str.c_str()),
                                           static_cast<uint32_t>(pool_name_str.size()),
                                           osdIdList_empty, 1);
        std::cerr << "[trace] kv_agent_create_crush_root result=" << result << std::endl;
        if (result != 0) {
            std::cerr << "kv_agent_create_crush_root failed: " << result << std::endl;
            std::cerr << "[trace] cleanup: calling kv_agent_delete_crush_root for rollback" << std::endl;
            kv_agent_delete_crush_root(agent, const_cast<char*>(pool_name_str.c_str()),
                                       static_cast<uint32_t>(pool_name_str.size()),
                                       osdIdList_empty, 1);
            kv_agent_destroy(agent);
            return EXIT_FAILURE;
        }

        // Step 2: Create Ceph pool (depends on CRUSH rule created by kv_agent_create_crush_root)
        std::cerr << "[trace] calling kv_agent_create_ceph_pool pool=" << pool_name_str
                  << " ns=" << ns_name_str << std::endl;
        result = kv_agent_create_ceph_pool(agent, const_cast<char*>(pool_name_str.c_str()),
                                           static_cast<uint32_t>(pool_name_str.size()),
                                           const_cast<char*>(ns_name_str.c_str()),
                                           static_cast<uint32_t>(ns_name_str.size()));
        std::cerr << "[trace] kv_agent_create_ceph_pool result=" << result << std::endl;
        if (result != 0) {
            std::cerr << "kv_agent_create_ceph_pool failed: " << result << std::endl;
            std::cerr << "[trace] cleanup: calling kv_agent_delete_crush_root for rollback" << std::endl;
            kv_agent_delete_crush_root(agent, const_cast<char*>(pool_name_str.c_str()),
                                       static_cast<uint32_t>(pool_name_str.size()),
                                       osdIdList_empty, 0);
            kv_agent_destroy(agent);
            return EXIT_FAILURE;
        }

        std::cerr << "[trace] === delete phase ===" << std::endl;

        // Step 3: Delete Ceph pool
        std::cerr << "[trace] calling kv_agent_delete_ceph_pool ns=" << ns_name_str << std::endl;
        result = kv_agent_delete_ceph_pool(agent, const_cast<char*>(ns_name_str.c_str()),
                                           static_cast<uint32_t>(ns_name_str.size()));
        std::cerr << "[trace] kv_agent_delete_ceph_pool result=" << result << std::endl;
        if (result != 0) {
            std::cerr << "kv_agent_delete_ceph_pool failed: " << result << std::endl;
            // Continue to delete crush root anyway
        }

        // Step 4: Delete CRUSH root
        std::cerr << "[trace] calling kv_agent_delete_crush_root pool=" << pool_name_str << std::endl;
        result = kv_agent_delete_crush_root(agent, const_cast<char*>(pool_name_str.c_str()),
                                           static_cast<uint32_t>(pool_name_str.size()),
                                           osdIdList_empty, 1);
        std::cerr << "[trace] kv_agent_delete_crush_root result=" << result << std::endl;
        if (result != 0) {
            std::cerr << "kv_agent_delete_crush_root failed: " << result << std::endl;
        }
    } else {
        // Create only mode
        std::cerr << "[trace] === create-only mode ===" << std::endl;

        int32_t osdIdList_empty[1] = {0};
        std::cerr << "[trace] calling kv_agent_create_crush_root pool=" << pool_name_str << std::endl;
        result = kv_agent_create_crush_root(agent, const_cast<char*>(pool_name_str.c_str()),
                                           static_cast<uint32_t>(pool_name_str.size()),
                                           osdIdList_empty, 0);
        std::cerr << "[trace] kv_agent_create_crush_root result=" << result << std::endl;
        if (result != 0) {
            std::cerr << "kv_agent_create_crush_root failed: " << result << std::endl;
            kv_agent_destroy(agent);
            return EXIT_FAILURE;
        }

        std::cerr << "[trace] calling kv_agent_create_ceph_pool pool=" << pool_name_str
                  << " ns=" << ns_name_str << std::endl;
        result = kv_agent_create_ceph_pool(agent, const_cast<char*>(pool_name_str.c_str()),
                                           static_cast<uint32_t>(pool_name_str.size()),
                                           const_cast<char*>(ns_name_str.c_str()),
                                           static_cast<uint32_t>(ns_name_str.size()));
        std::cerr << "[trace] kv_agent_create_ceph_pool result=" << result << std::endl;
        if (result != 0) {
            std::cerr << "kv_agent_create_ceph_pool failed: " << result << std::endl;
            kv_agent_destroy(agent);
            return EXIT_FAILURE;
        }
    }

    kv_agent_destroy(agent);
    std::cerr << "[trace] done" << std::endl;
    return EXIT_SUCCESS;
}