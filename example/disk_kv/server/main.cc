// Copyright (c) renjj - All Rights Reserved
#include <jrpc/base/logging/logging.h>
#include <jrpc/net/rpc/rpc_server.h>
#include <jrpc/net/types.h>

#include "example/disk_kv/server/server_flags.h"
#include "disk_kv/common/config.h"
#include "disk_kv/peer/peer.h"
#include "disk_kv/peer/peer_host.h"
#include "disk_kv/service/raft_msg_service.h"

void CheckFlags() {
  if (jkv::FLAGS_host_id == 0) {
    JLOG_FATAL << "should input host id";
  }
  if (jkv::FLAGS_peer_id == 0) {
    JLOG_FATAL << "should input peer id";
  }
  if (jkv::FLAGS_ip.empty()) {
    JLOG_FATAL << "should input ip";
  }
  if (jkv::FLAGS_port < 1024 || jkv::FLAGS_port > 65535) {
    JLOG_FATAL << "should input true port";
  }
  if (jkv::FLAGS_members.empty()) {
    JLOG_FATAL << "should input members";
  }
  if (jkv::FLAGS_log_level.empty()) {
    JLOG_FATAL << "should input log level";
  }
  if (jkv::FLAGS_host_path.empty()) {
    JLOG_FATAL << "should input host path";
  }
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  CheckFlags();

  // init log
  jrpc::AppOption app_opt;
  app_opt.log_option.level = jkv::FLAGS_log_level;
  app_opt.log_option.path = "";
  jrpc::AppInit(app_opt);

  // init opt
  jrpc::WorkerOption w_opt;
  w_opt.enable_mempool = true;
  jkv::PeerHostOption host_opt;
  host_opt.host_id = jkv::FLAGS_host_id;
  host_opt.db_path = jkv::FLAGS_host_path + "/db";

  // construct host
  jrpc::SockAddress addr(jkv::FLAGS_ip, jkv::FLAGS_port);
  auto event_wc = new jrpc::EventWorkerColony(1, w_opt, nullptr);
  event_wc->Start();
  auto* rpc_server = new jrpc::RpcServer(jrpc::kTcp, addr, "kv_server",
                                                    event_wc->option(), event_wc);
  host_opt.worker = event_wc->GetWorker();
  host_opt.storage = std::make_shared<jraft::MemoryStorage>();
  auto host = new jkv::PeerHost(host_opt);

  // construct peer
  jkv::PeerOption peer_opt;
  peer_opt.host_id = jkv::FLAGS_host_id;
  peer_opt.peer_id = jkv::FLAGS_peer_id;
  peer_opt.ip = jkv::FLAGS_ip;
  peer_opt.port = jkv::FLAGS_port;
  peer_opt.worker = host_opt.worker;
  peer_opt.members = jkv::FLAGS_members;
  peer_opt.storage = host_opt.storage;
  host->AddLocalPeer(peer_opt);
  host->mutable_local_peers(jkv::FLAGS_peer_id)->StartTimer();

  // register service
  auto raft_mgs_service = new jkv::RaftMsgServiceImpl(host);
  rpc_server->AddService(raft_mgs_service);
  rpc_server->Start();

  jrpc::AppRunUntilAskedToQuit("kv_server");
  return 0;
}
