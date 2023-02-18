// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <google/protobuf/service.h>

#include <cstdint>
#include <list>
#include <string>
#include <unordered_map>

#include "net/event/timer.h"
#include "net/sock_address.h"
#include "net/types.h"

namespace jrpc {

class EventWorker;
class RpcController;
class TcpClient;

class RpcClient : public google::protobuf::RpcChannel {
 public:
  RpcClient(Transport tp, EventWorker* w, const SockAddress& peer_addr,
            const std::string& name);
  virtual ~RpcClient();

  RpcClient(const RpcClient&) = delete;
  RpcClient& operator=(const RpcClient&) = delete;

  int Connect();
  void DisConnect();

  uint32_t GetMaxSendDataSize() const;
  uint32_t GetMaxSendDataSge() const;

  void CallMethod(const google::protobuf::MethodDescriptor* method,
                  google::protobuf::RpcController* controller,
                  const google::protobuf::Message* request,
                  google::protobuf::Message* response,
                  google::protobuf::Closure* done) override;

  void set_rpc_depth(uint32_t rpc_depth) {
    rpc_depth_ = rpc_depth;
    if (rpc_depth_ == 0) {
      rpc_depth_ = UINT32_MAX;
    }
  }
  void set_timeout_ms(uint32_t timeout_ms) { timeout_ms_ = timeout_ms; }
  void set_conn_idle_time_s(uint64_t time_s) {
    conn_idle_time_s_ = time_s;
    if (conn_idle_time_s_ == 0) {
      conn_idle_time_s_ = UINT64_MAX;
    }
  }
  EventWorker* worker() const { return worker_; }
  const ConnectionPtr& conn() const;

 private:
  void ConnectionSuccessHandle(const ConnectionPtr& conn);
  void ConnectionClosedHandle(const ConnectionPtr& conn);
  void MessageReadHandle(const ConnectionPtr& conn);

  void SendRequestCb(int retcode, uint64_t seq_no);

  void ReceiveMessageTcp(const ConnectionPtr& conn);
  void HandleMessage(const ConnectionPtr& conn, void* msg,
                     uint16_t service_name_size, uint16_t method_name_size,
                     uint32_t rpc_header_size, uint32_t rpc_body_size,
                     void* attachment, uint32_t attachment_size);

  void DispatchRpc(RpcController* rpc_ctl);
  void DispatchPendingRpc();

  void RpcTimerCb();
  void ConnIdleTimerCb();

  Transport tp_;
  EventWorker* worker_ = nullptr;
  TcpClient* client_ = nullptr;
  uint64_t cur_seq_no_ = 0;
  std::unordered_map<uint64_t, RpcController*> inflight_rpcs_;
  std::list<RpcController*> pending_rpcs_;
  // maximum rpc req
  uint32_t rpc_depth_ = UINT32_MAX;
  uint32_t timeout_ms_ = 3000;
  uint64_t active_tick_ = 0;
  // disconnect when idle
  uint64_t conn_idle_time_s_ = UINT64_MAX;
  TimerId rpc_timer_;
  TimerId conn_idle_timer_;
};

}  // namespace jrpc
