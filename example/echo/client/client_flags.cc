// Copyright (c) renjj - All Rights Reserved
#include "client_flags.h"

namespace jkv {

DEFINE_string(ip, "127.0.0.1", "ip address");
DEFINE_uint32(port, 0, "port number");
DEFINE_string(op_type, "", "get/put/del");
DEFINE_string(key, "", "key");
DEFINE_string(val, "", "val");

DEFINE_string(log_level, "debug", "log level");

}  // namespace jkv
