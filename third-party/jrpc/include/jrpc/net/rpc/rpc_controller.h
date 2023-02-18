// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <google/protobuf/message.h>
#include <google/protobuf/service.h>

#include <string>
#include <vector>

#include "net/types.h"

namespace jrpc {

const uint32_t kTimerInterval = 5;
const uint32_t kRpcMetaSge = 1;

class RpcServer;
class RpcClient;

enum RpcErrorCode {
  kRpcOk = 0,
  kRpcInternalError = 1,
  kRpcInvalidPara = 2,
  kRpcResourceLack = 3,
  kRpcConnError = 4,
  kRpcTimeout = 5,
  kRpcConnectError = 6,
};

class AttachmentEntry {
  friend class RpcController;

 public:
  AttachmentEntry(void* ptr, uint32_t size) : ptr_(ptr), size_(size) {}
  AttachmentEntry(const AttachmentEntry& entry, uint32_t offset,
                  uint32_t size) {
    *this = entry;
    ptr_ = reinterpret_cast<char*>(ptr_) + offset;
    size_ = size;
  }
  ~AttachmentEntry() {}

  void SetMrKey(uint32_t lk, uint32_t rk) {
    lkey_ = lk;
    rkey_ = rk;
    has_mr_key_ = true;
  }
  int GetMrKey(uint32_t* lk, uint32_t* rk) const {
    if (has_mr_key_) {
      *lk = lkey_;
      *rk = rkey_;
      return 0;
    }
    return -1;
  }

  void* ptr() const { return ptr_; }
  uint32_t size() const { return size_; }

 private:
  bool cntl_hold() const { return cntl_hold_; }
  void set_cntl_hold(bool hold) { cntl_hold_ = hold; }

  void* ptr_ = nullptr;
  uint32_t size_ = 0;
  bool cntl_hold_ = false;

  uint32_t lkey_ = 0;        // for cross thread rdma
  uint32_t rkey_ = 0;        // for cross thread rdma
  bool has_mr_key_ = false;  // for cross thread rdma
};

class RpcController : public google::protobuf::RpcController {
  friend class RpcServer;
  friend class RpcClient;

 public:
  explicit RpcController(const ConnectionPtr& conn);
  virtual ~RpcController();

  RpcController(const RpcController&) = delete;
  RpcController& operator=(const RpcController&) = delete;

  const ConnectionPtr& conn() const { return conn_; }

  void* Malloc(uint32_t size);
  void Free(void* ptr);

  void AddRequestAttachment(const AttachmentEntry& entry, bool cntl_hold) {
    request_attachment_vec_.emplace_back(entry);
    request_attachment_vec_.rbegin()->set_cntl_hold(cntl_hold);
    request_attachment_size_ += entry.size();
  }
  const std::vector<AttachmentEntry>& request_attachment_vec() const {
    return request_attachment_vec_;
  }
  uint64_t request_attachment_size() const { return request_attachment_size_; }

  void AddResponseAttachment(const AttachmentEntry& entry, bool cntl_hold) {
    response_attachment_vec_.emplace_back(entry);
    response_attachment_vec_.rbegin()->set_cntl_hold(cntl_hold);
    response_attachment_size_ += entry.size();
  }
  const std::vector<AttachmentEntry>& response_attachment_vec() const {
    return response_attachment_vec_;
  }
  uint64_t response_attachment_size() const {
    return response_attachment_size_;
  }

  const std::string& uuid();

  void Reset() override;

  bool Failed() const override { return failed_; }
  std::string ErrorText() const override { return error_text_; }
  int ErrorCode() const { return error_code_; }

  void StartCancel() override;

  void SetFailed(const std::string& reason) override;

  bool IsCanceled() const override;

  void NotifyOnCancel(google::protobuf::Closure* callback) override;

  uint64_t user_ctx() const { return user_ctx_; }
  void set_user_ctx(uint64_t user_ctx) { user_ctx_ = user_ctx; }

  void set_destroy_cb(const DestroyCb& cb) { destroy_cb_ = cb; }

  void set_user_hold(bool hold);

  uint64_t LatencyUs();
  uint64_t LatencyNs();

  uint32_t GetMaxAttachmentSize() const;
  uint32_t GetMaxAttachmentCnt() const;

 private:
  void set_request_msg(void* request_msg) { request_msg_ = request_msg; }
  void* request_msg() const { return request_msg_; }

  void set_response_msg(void* response_msg) { response_msg_ = response_msg; }
  void* response_msg() const { return response_msg_; }

  void set_seq_no(uint64_t seq_no);
  uint64_t seq_no() const { return seq_no_; }

  void set_rpc_method(const google::protobuf::MethodDescriptor* rpc_method) {
    rpc_method_ = rpc_method;
  }
  const google::protobuf::MethodDescriptor* rpc_method() const {
    return rpc_method_;
  }

  void set_request_body(const google::protobuf::Message* request, bool hold) {
    request_body_ = request;
    hold_request_body_ = hold;
  }
  const google::protobuf::Message* request_body() const {
    return request_body_;
  }

  void set_response_body(google::protobuf::Message* response, bool hold) {
    response_body_ = response;
    hold_response_body_ = hold;
  }
  google::protobuf::Message* response_body() const { return response_body_; }

  void set_done(google::protobuf::Closure* done) { done_ = done; }
  google::protobuf::Closure* done() const { return done_; }

  void set_rpc_server(RpcServer* rpc_server) { rpc_server_ = rpc_server; }

  void set_timer_cnt(uint32_t timer_cnt) { timer_cnt_ = timer_cnt; }
  bool DecTimerCnt();

  // 只有client会调用
  void CallDone(bool failed, int error_code, const std::string& error_text);

  void Begin();
  void End();
  void DestroyMyself();

  void set_jrpc_hold(bool hold);

  ConnectionPtr conn_;
  void* request_msg_ = nullptr;
  void* response_msg_ = nullptr;

  std::vector<AttachmentEntry> request_attachment_vec_;
  uint64_t request_attachment_size_ = 0;

  std::vector<AttachmentEntry> response_attachment_vec_;
  uint64_t response_attachment_size_ = 0;

  std::string uuid_;
  uint64_t seq_no_ = 0;

  const google::protobuf::MethodDescriptor* rpc_method_ = nullptr;
  const google::protobuf::Message* request_body_ = nullptr;
  google::protobuf::Message* response_body_ = nullptr;
  google::protobuf::Closure* done_ = nullptr;
  bool hold_request_body_ = false;
  bool hold_response_body_ = false;

  RpcServer* rpc_server_ = nullptr;

  uint32_t timer_cnt_ = 3000;

  bool failed_ = false;
  std::string error_text_;
  int error_code_;

  uint64_t user_ctx_ = 0;

  bool jrpc_hold_ = false;
  bool user_hold_ = false;

  DestroyCb destroy_cb_;

  uint64_t begin_tick_ = 0;
  uint64_t end_tick_ = 0;
};

}  // namespace jrpc
