// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <boost/asio.hpp>
#include <thread>
#include <unordered_map>

#include "eraft.pb.h"

namespace jkv {

class Peer;
class Server;
using ServerPtr = std::shared_ptr<Server>;
class Client;
using ClientPtr = std::shared_ptr<Client>;
using MessagePtr = std::shared_ptr<jraft::Message>;

class Transport {
 public:
  Transport(Peer* peer, uint64_t id);
  ~Transport();

  void Start(const std::string& host);
  void AddPeer(uint64_t id, const std::string& peer);
  void RemovePeer(uint64_t id);
  void Send(const std::vector<jraft::Message>& msgs);
  void Stop();

 private:
  uint64_t id_;
  Peer* peer_;

  std::thread io_thread_;
  boost::asio::io_service io_service_;

  std::mutex mutex_;
  std::unordered_map<uint64_t, ClientPtr> peers_;

  ServerPtr server_;
};

}  // namespace jkv
