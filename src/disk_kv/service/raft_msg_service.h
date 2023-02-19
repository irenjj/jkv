// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <jrpc/net/rpc/closure_guard.h>
#include <jrpc/net/rpc/rpc_controller.h>

#include "raft_msg.pb.h"

namespace jkv {

class PeerHost;

class RaftMsgServiceImpl : public RaftMsgService {
 public:
  explicit RaftMsgServiceImpl(PeerHost* host);
  ~RaftMsgServiceImpl() override = default;

  void HandleRaftMsg(::google::protobuf::RpcController* cntl,
                     const RaftMsgReq* req, RaftMsgResp* resp,
                     ::google::protobuf::Closure* done) override;

 private:
  void HandleRaftMsgCb(::google::protobuf::RpcController* cntl,
                       const RaftMsgReq* req, RaftMsgResp* resp,
                       ::google::protobuf::Closure* done);

  PeerHost* host_;
};

}  // namespace jkv
