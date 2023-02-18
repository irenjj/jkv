// Copyright (c) renjj - All Rights Reserved
#include "example/disk_kv/server/server_flags.h"

namespace jkv {

DEFINE_uint64(host_id, 0, "host id");
DEFINE_uint64(peer_id, 0, "peer id");
DEFINE_string(ip, "127.0.0.1", "ip address");
DEFINE_uint32(port, 0, "port number");
DEFINE_string(members, "", "host_id1,peer_id1,ip1,port1|host_id2,peer_id2,ip2,port2|...");

DEFINE_uint64(heartbeat_interval, 0, "heartbeat interval tick");
DEFINE_uint64(election_interval, 0, "election interval tick");

DEFINE_string(log_level, "info", "log level");
DEFINE_string(host_path, "./host", "host path");

}  // namespace jkv
