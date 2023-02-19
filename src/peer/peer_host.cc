// Copyright (c) renjj - All Rights Reserved
#include "peer/peer_host.h"

#include <jrpc/base/logging/logging.h>

#include "peer/peer.h"

namespace jkv {

PeerHost::PeerHost(const PeerHostOption& host_opt) : host_opt_(host_opt) {}

PeerHost::~PeerHost() {}

void PeerHost::AddLocalPeer(const PeerOption& peer_opt) {
  if (local_peers_.find(peer_opt.peer_id) != local_peers_.end()) {
    JLOG_FATAL << "RaftNode already existed";
  }

  CheckPeerOption(peer_opt);

  JLOG_INFO << "host: " << host_opt_.host_id
            << " start to add peer: " << peer_opt.peer_id;

  local_peers_[peer_opt.peer_id] = std::make_shared<Peer>(peer_opt, this);
}

std::shared_ptr<jrpc::RpcClient> PeerHost::GetClient(
    HostId host_id, jrpc::EventWorker* w, const jrpc::SockAddress& addr,
    uint64_t timeout_ms, const std::string& name, bool create_if_missing) {
  std::shared_ptr<jrpc::RpcClient> client = nullptr;
  auto it = rpc_cli_mgr_.find(host_id);
  if (it == rpc_cli_mgr_.end()) {
    if (!create_if_missing) {
      return nullptr;
    }
    client = std::make_shared<jrpc::RpcClient>(jrpc::kTcp, w, addr, name);
    client->set_timeout_ms(timeout_ms);
    client->set_rpc_depth(10000);
    int ret = client->Connect();
    if (ret) {
      JLOG_FATAL << "connect to " << addr.ToString() << " failed";
    }
    rpc_cli_mgr_[host_id] = client;
    JLOG_INFO << "succeed to create new client to " << host_id;
  } else {
    client = it->second;
  }
  return client;
}

void PeerHost::CheckPeerOption(const PeerOption& peer_opt) const {
  if (peer_opt.peer_id <= 0) {
    JLOG_FATAL << "peer id should be valid";
  }
  if (peer_opt.host_id != host_opt_.host_id) {
    JLOG_FATAL << "peer's host_id: " << peer_opt.host_id
               << " should be the same as local host_id: " << host_opt_.host_id;
  }
}

}  // namespace jkv
