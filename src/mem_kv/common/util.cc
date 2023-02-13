// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/common/util.h"

#include <boost/crc.hpp>

namespace jkv {

uint32_t ComputeCrc32(const char* data, size_t len) {
  boost::crc_32_type crc32;
  crc32.process_bytes(data, len);
  return crc32();
}

// IsMustFlush() returns true if the HardState and count of raft entries
// indicate that a synchronous write to persistent storage is required.
bool IsMustFlush(const jraft::HardState& hs, const jraft::HardState& prev_hs,
                 size_t ents_num) {
  // Persistent state on all servers:
  // (Updated on stable storage before responding to RPCs)
  // + currentTerm
  // + votedFor
  // + log[]
  return ents_num != 0 || hs.vote() != prev_hs.vote() ||
         hs.term() != prev_hs.term();
}

}  // namespace jkv
