// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <string>

#include "net/event/tcp_connection.h"
#include "net/sock_address.h"
#include "net/types.h"

namespace jrpc {

class EventWorker;

class TcpClient {
 public:
  TcpClient(EventWorker* w, const SockAddress& peer_addr,
            const std::string& name);
  ~TcpClient();

  int Connect();
  int TryConnect();
  void DisConnect();

  void SetConnectionSuccessCb(const ConnectionSuccessCb& cb) {
    connection_success_cb_ = cb;
  }
  void SetConnectionClosedCb(const ConnectionClosedCb& cb) {
    connection_closed_cb_ = cb;
  }
  void SetMessageReadCb(const MessageReadCb& cb) { message_read_cb_ = cb; }
  void SetMessageWriteCb(const MessageWriteCb& cb) { message_write_cb_ = cb; }

  const SockAddress& peer_addr() const { return peer_addr_; }
  const std::string& name() const { return name_; }
  const ConnectionPtr& conn() const { return conn_; }

 private:
  EventWorker* worker_;
  SockAddress peer_addr_;
  std::string name_;
  ConnectionPtr conn_;

  ConnectionSuccessCb connection_success_cb_;
  ConnectionClosedCb connection_closed_cb_;
  MessageReadCb message_read_cb_;
  MessageWriteCb message_write_cb_;
};

}  // namespace jrpc
