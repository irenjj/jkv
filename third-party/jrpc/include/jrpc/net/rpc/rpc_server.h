// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <google/protobuf/service.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/obj_pool.h"
#include "net/sock_address.h"
#include "net/types.h"

namespace jrpc {

class EventWorker;
class EventWorkerColony;
class RpcController;
class TcpServer;

class RpcServer {
  friend class RpcController;

 public:
  RpcServer(Transport tp, const SockAddress& addr, const std::string& name,
            const WorkerOption& w_option,
            EventWorkerColony* w_colony = nullptr);
  ~RpcServer();

  RpcServer(const RpcServer&) = delete;
  RpcServer& operator=(const RpcServer&) = delete;

  // call before Start()
  int AddService(google::protobuf::Service* service);
  // call before Start()
  void SetWorkerNum(uint32_t worker_num);
  // call before Start()
  void SetCreateUserHandleCb(const CreateUserHandleCb& cb);

  void AddBlacklist(const std::string& ip, uint32_t refuse_seconds);
  void RemoveBlacklist(const std::string& ip);

  int Start();
  // can't work well
  int Stop();

  Transport tp() const { return tp_; }
  EventWorkerColony* worker_colony() const;

 private:
  void ConnectionSuccessHandle(const ConnectionPtr& conn);
  void ConnectionClosedHandle(const ConnectionPtr& conn);
  void MessageReadHandle(const ConnectionPtr& conn);
  void ReceiveMessageTcp(const ConnectionPtr& conn);
  void HandleMessage(const ConnectionPtr& conn, void* msg,
                     uint16_t service_name_size, uint16_t method_name_size,
                     uint32_t rpc_header_size, uint32_t rpc_body_size,
                     void* attachment, uint32_t attachment_size);
  void SendResponse(RpcController* rpc_ctl);
  void SendResponseCb(int retcode, RpcController* rpc_ctl);
  void DestroyRpcController(RpcController* rpc_ctl);
  void ReportObjPoolInfo(uint32_t worker_id);

  // key: service_name
  std::unordered_map<std::string, google::protobuf::Service*> rpc_services_;
  Transport tp_;
  TcpServer* server_ = nullptr;

  std::vector<jbase::ObjPool<RpcController>*> rpc_ctl_pools_;
};

}  // namespace jrpc
