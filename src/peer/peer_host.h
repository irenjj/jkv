// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <net/rpc/rpc_client.h>

#include "common/config.h"

namespace jkv {

class Peer;
using PeerPtr = std::shared_ptr<Peer>;
using PeerTable = std::unordered_map<PeerId, PeerPtr>;

class PeerHost {
 public:
  explicit PeerHost(const PeerHostOption& host_opt);
  ~PeerHost();

  void AddLocalPeer(const PeerOption& peer_opt);
  std::shared_ptr<jrpc::RpcClient> GetClient(HostId host_id,
                                             jrpc::EventWorker* w,
                                             const jrpc::SockAddress& addr,
                                             uint64_t timeout_ms,
                                             const std::string& name = "",
                                             bool create_if_missing = true);

  HostId host_id() const { return host_opt_.host_id; }
  jrpc::EventWorker* mutable_worker() { return host_opt_.worker; }
  PeerPtr mutable_local_peers(uint64_t i) {
    if (local_peers_.find(i) == local_peers_.end()) {
      return nullptr;
    }
    return local_peers_[i];
  }
  std::string db_path() const { return host_opt_.db_path; }

 private:
  void CheckPeerOption(const PeerOption& peer_opt) const;

  PeerHostOption host_opt_;
  PeerTable local_peers_;
  std::unordered_map<HostId, std::shared_ptr<jrpc::RpcClient>> rpc_cli_mgr_;
};

using PeerHostPtr = std::shared_ptr<PeerHost>;

}  // namespace jkv
