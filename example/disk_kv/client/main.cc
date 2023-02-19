// Copyright (c) renjj - All Rights Reserved
#include <jrpc/base/logging/logging.h>
#include <jrpc/net/event/event_loop.h>
#include <jrpc/net/event/event_worker_colony.h>
#include <jrpc/net/event/worker_thread.h>
#include <jrpc/net/rpc/rpc_client.h>
#include <jrpc/net/rpc/rpc_controller.h>
#include <jrpc/net/types.h>

#include "example/disk_kv/client/client_flags.h"
#include "kv_op.pb.h"

class KvClient {
 public:
  KvClient(const std::string& ip, uint16_t port, jrpc::EventWorker* worker)
      : worker_(worker), client_(nullptr) {
    jrpc::SockAddress addr(ip, port);
    client_ = new jrpc::RpcClient(jrpc::kTcp, worker_, addr, "client");
    client_->set_timeout_ms(300);
    client_->set_rpc_depth(10000);
    if (client_->Connect()) {
      JLOG_FATAL << "failed to connect";
    }
  }
  ~KvClient() {
    if (client_) {
      delete client_;
    }
  }

  void Get(const std::string& key) {
    jkv::KvService_Stub stub(client_);
    auto* cntl = new jrpc::RpcController(client_->conn());
    jkv::KvReq req;
    req.set_cmd_type(jkv::kGet);
    req.mutable_get_req()->set_key(key);
    auto* resp = new jkv::KvResp();
    auto* done = google::protobuf::NewCallback(
        this, &KvClient::GetCb, cntl, resp);
    stub.KvOp(cntl, &req, resp, done);
  }

  void Put(const std::string& key, const std::string& val) {
    JLOG_INFO << "\n\n\nput1\n\n\n";
    jkv::KvService_Stub stub(client_);
    auto* cntl = new jrpc::RpcController(client_->conn());
    jkv::KvReq req;
    req.set_cmd_type(jkv::kPut);
    req.mutable_put_req()->set_key(key);
    req.mutable_put_req()->set_value(val);
    auto* resp = new jkv::KvResp();
    google::protobuf::Closure* done = google::protobuf::NewCallback(
        this, &KvClient::PutCb, cntl, resp);
    JLOG_INFO << "\n\n\nput2\n\n\n";
    stub.KvOp(cntl, &req, resp, done);
  }

  void Del(const std::string& key) {
    jkv::KvService_Stub stub(client_);
    auto* cntl = new jrpc::RpcController(client_->conn());
    jkv::KvReq req;
    req.set_cmd_type(jkv::kDel);
    req.mutable_del_req()->set_key(key);
    auto* resp = new jkv::KvResp();
    auto* done = google::protobuf::NewCallback(
        this, &KvClient::DelCb, cntl, resp);
    stub.KvOp(cntl, &req, resp, done);
  }

 private:
  void PutCb(jrpc::RpcController* cntl, jkv::KvResp* resp) {
    std::unique_ptr<jrpc::RpcController> cntl_guard(cntl);
    std::unique_ptr<jkv::KvResp> resp_guard(resp);

    if (cntl->Failed()) {
      JLOG_FATAL << "put kv rpc failed, error text: " << cntl->ErrorText();
    } else if (resp->rejected()) {
      JLOG_FATAL << "server rejected put request";
    } else {
      JLOG_INFO << "put kv request success";
    }
    ::exit(0);
  }

  void GetCb(jrpc::RpcController* cntl, jkv::KvResp* resp) {
    std::unique_ptr<jrpc::RpcController> cntl_guard(cntl);
    std::unique_ptr<jkv::KvResp> resp_guard(resp);

    if (cntl->Failed()) {
      JLOG_FATAL << "get kv rpc failed, error text: " << cntl->ErrorText();
    } else if (resp->rejected()) {
      JLOG_FATAL << "server rejected get request";
    } else {
      JLOG_INFO << "get kv request success";
    }
    ::exit(0);
  }

  void DelCb(jrpc::RpcController* cntl, jkv::KvResp* resp) {
    std::unique_ptr<jrpc::RpcController> cntl_guard(cntl);
    std::unique_ptr<jkv::KvResp> resp_guard(resp);

    if (cntl->Failed()) {
      JLOG_FATAL << "del kv rpc failed, error text: " << cntl->ErrorText();
    } else if (resp->rejected()) {
      JLOG_FATAL << "server rejected del request";
    } else {
      JLOG_INFO << "del kv request success";
    }
    ::exit(0);
  }

  jrpc::EventWorker* worker_;
  jrpc::RpcClient* client_;
};

void CheckFlags() {
  if (jkv::FLAGS_port < 1024 || jkv::FLAGS_port > 65535) {
    JLOG_FATAL << "should input correct port";
  }
  if (jkv::FLAGS_op_type.empty()) {
    JLOG_FATAL << "should input op type";
  }
  if (jkv::FLAGS_key.empty()) {
    JLOG_FATAL << "should input key";
  }
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  CheckFlags();

  // init log
  jrpc::AppOption app_opt;
  app_opt.log_option.level = jkv::FLAGS_log_level;
  jrpc::AppInit(app_opt);

  jrpc::WorkerOption w_opt;
  w_opt.enable_mempool = true;
  auto client_thread = std::make_shared<jrpc::WorkerThread>("client_thread", nullptr, w_opt);
  jrpc::EventWorker* worker = client_thread->Start();
  worker->loop()->RunInLoop([worker]() {
    auto client = std::make_shared<KvClient>(jkv::FLAGS_ip, jkv::FLAGS_port, worker);
    std::string type = jkv::FLAGS_op_type;
    if (type == "get") {
      client->Get(jkv::FLAGS_key);
    } else if (type == "put") {
      client->Put(jkv::FLAGS_key, jkv::FLAGS_val);
    } else if (type == "del") {
      client->Del(jkv::FLAGS_key);
    } else {
      JLOG_FATAL << "invalid type";
    }
  });

  jrpc::AppRunUntilAskedToQuit("kv_client");
  return 0;
}
