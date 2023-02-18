// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <memory>
#include <string>

#include "net/event/event_worker_colony.h"
#include "net/sock_address.h"

namespace jrpc {

class EventWorker;
class EventLoop;
class Buffer;

class EventHandler;
class ConnectedSocket;

enum NetErrorCode {
  kNetOk = 0,
  kNetConnStatError = 1,
  kNetConnectError = 2,
};

constexpr uint32_t kMetaSge = 1;
constexpr uint32_t kMaxSendSge = 64;
constexpr uint32_t kMaxSendDataSize = (1 << 30);
constexpr uint32_t kMaxSendMetaSize = (4 << 10);
constexpr uint32_t kMaxSendDepth = 65535;
extern std::atomic<int64_t> g_conn_id;

class TcpConnection : public std::enable_shared_from_this<TcpConnection>,
                      public jbase::noncopyable {
 public:
  static const int kDisconnected = 0;
  static const int kAccepting = 1;
  static const int kConnecting = 2;
  static const int kConnected = 3;

  static const int kTypeTcp = 0;

  TcpConnection(EventWorker* w, const std::string& name, int64_t conn_id,
                const SockAddress& peer_addr);

  ~TcpConnection();

  void Init();

  int Connect();
  void Accept(const std::shared_ptr<ConnectedSocket>& cs);

  void ConnectionEstablished();
  void HandleConnectEvent(int fd);
  void HandleReadEvent(int fd, const ConnectionPtr& conn);
  void HandleWriteEvent(int fd, const ConnectionPtr& conn);

  size_t ReadableLength();
  int SendData(const void* data, size_t data_len);
  ssize_t ReceiveData(void* data, size_t data_len);
  ssize_t DrainData(size_t len);
  ssize_t RemoveData(void* data, size_t data_len);

  bool IsClosed();
  void ForceClose();

  void SetConnectionSuccessCb(const ConnectionSuccessCb& cb) {
    connection_success_cb_ = cb;
  }
  void SetConnectionClosedCb(const ConnectionClosedCb& cb) {
    connection_closed_cb_ = cb;
  }
  void SetMessageReadCb(const MessageReadCb& cb) { message_read_cb_ = cb; }
  void SetMessageWriteCb(const MessageWriteCb& cb) { message_write_cb_ = cb; }

  void set_state(int state);
  int state() const;
  EventWorker* worker() const { return worker_; }
  uint32_t my_qp_num() const { return 0; }
  uint32_t peer_qp_num() const { return 0; }
  int64_t conn_id() const { return conn_id_; }
  const SockAddress& peer_addr() const { return peer_addr_; }
  const SockAddress& my_addr() const { return my_addr_; }
  void set_my_addr(const SockAddress& my_addr) { my_addr_ = my_addr; }
  const std::string& name() const { return name_; }
  void set_name(const std::string& name) { name_ = name; }
  // 框架会占用sge
  uint32_t max_send_sge() const { return max_send_sge_ - kMetaSge; }
  uint32_t max_send_size() const { return max_send_size_; }
  uint32_t max_send_data_size() const { return max_send_data_size_; }
  uint32_t max_send_meta_size() const { return max_send_meta_size_; }
  uint32_t max_send_depth() const { return max_send_depth_; }
  int type() const { return type_; }
  void set_ctx(void* ctx) { ctx_ = ctx; }
  void reset_ctx() { ctx_ = nullptr; }
  void* ctx() const { return ctx_; }

 private:
  void CheckBufferToSend();
  void ForceCloseInLoop();

  std::string name_;
  int64_t conn_id_;
  SockAddress peer_addr_;
  SockAddress my_addr_;
  int type_;
  void* ctx_;
  EventWorker* worker_ = nullptr;
  EventHandler* event_handler_ = nullptr;
  int connect_event_idx_;
  int read_event_idx_;
  int write_event_idx_;
  EventLoop* loop_ = nullptr;
  std::shared_ptr<ConnectedSocket> cs_;
  Buffer* in_buf_ = nullptr;
  Buffer* out_buf_ = nullptr;

  uint32_t max_send_sge_ = kMaxSendSge;
  uint32_t max_send_data_size_ = kMaxSendDataSize;
  uint32_t max_send_meta_size_ = kMaxSendMetaSize;
  uint32_t max_send_size_ = kMaxSendDataSize + kMaxSendMetaSize;
  uint32_t max_send_depth_ = kMaxSendDepth;

  ConnectionSuccessCb connection_success_cb_;
  ConnectionClosedCb connection_closed_cb_;
  MessageReadCb message_read_cb_;
  MessageWriteCb message_write_cb_;
};

}  // namespace jrpc
