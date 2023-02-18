// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <gflags/gflags.h>

namespace jkv {

// host configuration
DECLARE_uint64(host_id);
DECLARE_uint64(peer_id);
DECLARE_string(ip);
DECLARE_uint32(port);
DECLARE_string(members);

// raft configuration
DECLARE_uint64(heartbeat_interval);
DECLARE_uint64(election_interval);

DECLARE_string(log_level);
DECLARE_string(host_path);

}  // namespace jkv
