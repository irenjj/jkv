// Copyright (c) renjj - All Rights Reserved
#pragma once

#include "kv_op.pb.h"

namespace jkv {

class PeerHost;

class KvServiceImpl : public KvService {
 public:
  explicit KvServiceImpl(PeerHost* host);
  ~KvServiceImpl() override = default;

  void KvOp(::google::protobuf::RpcController* cntl, const ::jkv::KvReq* req,
            ::jkv::KvResp* resp, ::google::protobuf::Closure* done) override;

 private:
  void KvOpCb(::google::protobuf::RpcController* cntl, const ::jkv::KvReq* req,
              ::jkv::KvResp* resp, ::google::protobuf::Closure* done);

  PeerHost* host_;
};

}  // namespace jkv
