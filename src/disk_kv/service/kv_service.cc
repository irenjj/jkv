// Copyright (c) renjj - All Rights Reserved
#include "disk_kv/service/kv_service.h"

#include <jrpc/net/event/event_loop.h>
#include <jrpc/net/rpc/closure_guard.h>

#include "disk_kv/peer/peer.h"
#include "disk_kv/peer/peer_host.h"

namespace jkv {

KvServiceImpl::KvServiceImpl(PeerHost* host) : host_(host) {}

void KvServiceImpl::KvOp(::google::protobuf::RpcController* cntl,
                         const ::jkv::KvReq* req, ::jkv::KvResp* resp,
                         ::google::protobuf::Closure* done) {
  JLOG_INFO << "\n\n\ncome here\n\n\n";

  host_->mutable_worker()->loop()->RunInLoop(
      std::bind(&KvServiceImpl::KvOpCb, this, cntl, req, resp, done));
}

void KvServiceImpl::KvOpCb(::google::protobuf::RpcController* cntl,
                           const ::jkv::KvReq* req, ::jkv::KvResp* resp,
                           ::google::protobuf::Closure* done) {
  PeerPtr peer = host_->mutable_local_peers(1);
  if (!peer) {
    JLOG_FATAL << "peer should exist";
  }

  if (!peer->IsLeader()) {
    jrpc::ClosureGuard done_guard(done);
    resp->set_rejected(true);
    return;
  }

  peer->ProposeKvOp(req, resp, done);
}

}  // namespace jkv
