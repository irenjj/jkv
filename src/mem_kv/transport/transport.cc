// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/transport/transport.h"

#include <jrpc/base/logging/logging.h>

#include "mem_kv/transport/client.h"
#include "mem_kv/transport/server.h"

namespace jkv {

Transport::Transport(Peer* peer, uint64_t id) : peer_(peer), id_(id) {}

Transport::~Transport() {
  if (io_thread_.joinable()) {
    io_thread_.join();
    JLOG_INFO << "transport stopped";
  }
}

void Transport::Start(const std::string& host) {
  server_ = std::make_shared<Server>(io_service_, host, peer_);
  server_->Start();
  io_thread_ = std::thread([this]() { this->io_service_.run(); });
}

void Transport::AddPeer(uint64_t id, const std::string& peer) {
  std::lock_guard<std::mutex> lock_guard(mutex_);

  auto it = peers_.find(id);
  if (it != peers_.end()) {
    JLOG_INFO << "peer: " << id << " already exists";
    return;
  }
  JLOG_INFO << "node: " << id_ << ", add peer: " << id << ", addr: " << peer;
  ClientPtr p = std::make_shared<Client>(id, io_service_, peer);
  p->Start();
  peers_[id] = p;
}

void Transport::RemovePeer(uint64_t id) { JLOG_FATAL << "not implemented now"; }

void Transport::Send(const std::vector<jraft::Message>& msgs) {
  auto cb = [this](const std::vector<jraft::Message>& msgs) {
    for (auto msg : msgs) {
      if (msg.to() == 0) {
        // ignore intentionally dropped message
        continue;
      }
      auto it = peers_.find(msg.to());
      if (it != peers_.end()) {
        it->second->Send(msg);
        continue;
      }
      JLOG_INFO << "ignored message " << msg.type() << " (sent to unknown peer "
                << msg.to() << ")";
    }
  };
  io_service_.post(std::bind(cb, msgs));
}

void Transport::Stop() { io_service_.stop(); }

}  // namespace jkv
