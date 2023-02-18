// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <memory>

#include "net/sock_address.h"

namespace jrpc {

class TcpServer;
class EventWorker;
class EventLoop;

class ConnectedSocket {
 public:
  ConnectedSocket() : fd_(-1) {}
  ConnectedSocket(const SockAddress& sa, int fd, int state)
      : fd_(fd), sa_(sa), state_(state) {}
  ~ConnectedSocket() { Close(); }

  void set_state(int state) { state_ = state; }
  int state() const { return state_; }
  int fd() const { return fd_; }
  void Shutdown();
  void Close();

 private:
  int fd_;
  SockAddress sa_;
  int state_;
};

class ServerSocket {
 public:
  explicit ServerSocket(int fd) : fd_(fd) {}

  int Accept(std::shared_ptr<ConnectedSocket>* sock, SockAddress* out);
  int fd() const { return fd_; }
  void StopAccept();

 private:
  int fd_;
};

class Processor {
 public:
  Processor(TcpServer* server, EventWorker* w);
  ~Processor() = default;

  int Bind(const SockAddress& server_addr);
  void Start();
  void Accept();
  void Stop();

 private:
  static void AcceptEventCbWrapper(int fd, short event, void* arg);

  TcpServer* server_;
  EventWorker* worker_;
  EventLoop* loop_;
  int accept_event_idx_;
  std::shared_ptr<ServerSocket> server_socket_;
};

}  // namespace jrpc
