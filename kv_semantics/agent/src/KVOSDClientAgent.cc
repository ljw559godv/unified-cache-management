/*
* Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
* Description: KV OSD Client Agent 实现
*/

#include "KVClientAgentImpl.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "include/ceph_assert.h"
#include "common/async/context_pool.h"
#include "common/ceph_argparse.h"
#include "common/ceph_context.h"
#include "common/common_init.h"
#include "common/config.h"
#include "common/errno.h"
#include "include/ceph_fs.h"
#include "include/encoding.h"
#include "messages.h/MOSDMap.h"
#include "mon/MonClient.h"
#include "msg/Dispatcher.h"
#include "msg/Messenger.h"
#include "osd/OSDMap.h"