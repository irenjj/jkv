// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <memory>
#include <set>
#include <string>

#include "net/event/event_worker_colony.h"
#include "net/sock_address.h"
#include "net/types.h"

namespace jrpc {

class ConnectedSocket;
class EventLoop;
class Processor;

class TcpServer {
 public:
  TcpServer(const SockAddress& addr, const std::string& name,
            const WorkerOption& w_option,
            EventWorkerColony* w_colony = nullptr);
  ~TcpServer();

  int Start();
  int Bind();
  int Stop();

  void CreateConnection(EventWorker* w,
                        const std::shared_ptr<ConnectedSocket>& cli_socket,
                        const SockAddress& addr);
  void RemoveConnection(const ConnectionPtr& conn);
  void RemoveConnectionBySrcIp(const std::string& ip);
  void AddBlacklist(const std::string& ip, uint32_t refuse_seconds);
  void RemoveBlacklist(const std::string& ip);

  bool VerifySrcIp(const std::string& ip);

  void SetConnectionSuccessCb(const ConnectionSuccessCb& cb) {
    connection_success_cb_ = cb;
  }
  void SetConnectionClosedCb(const ConnectionClosedCb& cb) {
    connection_closed_cb_ = cb;
  }
  void SetMessageReadCb(const MessageReadCb& cb) { message_read_cb_ = cb; }
  void SetMessageWriteCb(const MessageWriteCb& cb) { message_write_cb_ = cb; }
  void SetCreateUserHandleCb(const CreateUserHandleCb& cb);

  void set_worker_num(uint32_t worker_num);
  uint32_t worker_num() const { return worker_num_; }
  const WorkerOption& worker_option() const { return worker_option_; }
  EventWorkerColony* worker_colony() const { return worker_colony_; }
  const std::string& name() const { return name_; }

 private:
  friend class Processor;

  SockAddress server_addr_;
  std::string name_;
  WorkerOption worker_option_;
  bool started_ = false;
  uint32_t worker_num_ = 1;
  std::map<int64_t, ConnectionPtr> accepted_conns_;
  EventWorker* worker_ = nullptr;
  EventWorkerColony* worker_colony_ = nullptr;

  ConnectionSuccessCb connection_success_cb_;
  ConnectionClosedCb connection_closed_cb_;
  MessageReadCb message_read_cb_;
  MessageWriteCb message_write_cb_;
  CreateUserHandleCb create_user_handle_cb_ = nullptr;
  // client ip.
  std::set<std::string> blacklist_;
  Processor* processor_ = nullptr;
};

}  // namespace jrpc
