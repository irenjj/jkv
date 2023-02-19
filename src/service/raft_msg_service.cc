// Copyright (c) renjj - All Rights Reserved
#include "service/raft_msg_service.h"

#include <jrpc/base/logging/logging.h>
#include <jrpc/net/event/event_loop.h>

#include "peer/peer.h"
#include "peer/peer_host.h"

namespace jkv {

RaftMsgServiceImpl::RaftMsgServiceImpl(PeerHost* host) : host_(host) {}

void RaftMsgServiceImpl::HandleRaftMsg(::google::protobuf::RpcController* cntl,
                                       const RaftMsgReq* req, RaftMsgResp* resp,
                                       ::google::protobuf::Closure* done) {
  if (req->to_peer().host_id() != host_->host_id()) {
    JLOG_FATAL << "send to wrong host";
  }

  host_->mutable_worker()->loop()->RunInLoop(std::bind(
      &RaftMsgServiceImpl::HandleRaftMsgCb, this, cntl, req, resp, done));
}

void RaftMsgServiceImpl::HandleRaftMsgCb(
    ::google::protobuf::RpcController* cntl, const RaftMsgReq* req,
    RaftMsgResp* resp, ::google::protobuf::Closure* done) {
  jrpc::ClosureGuard done_guard(done);
  PeerPtr peer = host_->mutable_local_peers(req->to_peer().peer_id());
  if (!peer) {
    JLOG_FATAL << "peer should exist";
  }
  peer->Step(req->msg());
}

}  // namespace jkv
