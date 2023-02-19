// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <jraft/memory_storage.h>
#include <jraft/read_only.h>
#include <jraft/storage.h>
#include <jrpc/net/event/event_worker_colony.h>

#include <cstdint>

namespace jkv {

using HostId = uint64_t;
using PeerId = uint64_t;

constexpr HostId kNoHost = 0;
constexpr PeerId kNoPeer = 0;

struct PeerHostOption {
  HostId host_id = kNoHost;
  // which thread to be use on this store.
  jrpc::EventWorker* worker = nullptr;
  std::string db_path;
  jraft::MemoryStoragePtr storage;
};

struct PeerOption {
  // on which host
  HostId host_id = kNoHost;
  // on which group of host
  PeerId peer_id = kNoPeer;
  std::string ip = "127.0.0.1";
  uint16_t port = 0;
  // all peers share the same worker on the host
  jrpc::EventWorker* worker = nullptr;
  std::string members;

  // passed to Config, for more information, see Config
  int election_tick = 0;
  int heartbeat_tick = 0;
  jraft::MemoryStoragePtr storage = nullptr;
  uint64_t max_size_per_msg = UINT64_MAX;
  uint64_t max_committed = UINT64_MAX;
  uint64_t max_uncommitted_entries_size = 0;
  size_t max_inflight_msgs = SIZE_MAX;
  uint64_t max_inflight_bytes = 0;
  bool check_quorum = false;
  bool pre_vote = false;
  jraft::ReadOnlyOption read_only_option = jraft::kReadOnlySafe;
  bool disable_proposal_forwarding = false;
};

}  // namespace jkv
