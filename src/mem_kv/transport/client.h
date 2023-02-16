// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>

#include "eraft.pb.h"
#include "mem_kv/common/byte_buffer.h"
#include "transport_meta.pb.h"

namespace jkv {

class Client;
using MessagePtr = std::shared_ptr<jraft::Message>;
using SnapshotPtr = std::shared_ptr<jraft::Snapshot>;

class ClientSession {
 public:
  ClientSession(boost::asio::io_service& io_service, Client* client);
  ~ClientSession() = default;

  void Connect();
  void Send(TransportType type, const uint8_t* data, uint32_t len);
  void StartWrite();
  void CloseSession();

 private:
  boost::asio::ip::tcp::socket socket_;
  boost::asio::ip::tcp::endpoint endpoint_;
  uint64_t peer_id_;
  Client* client_;
  bool connected_;
  ByteBuffer buffer_;
};

class Client {
 public:
  Client(uint64_t peer_id, boost::asio::io_service& io_service,
         const std::string& peer_str);
  ~Client() = default;

  void Start();
  void Send(const jraft::Message& msg);
  void SendSnapshot(SnapshotPtr snap);
  void Update(const std::string& peer);
  uint64_t ActiveSince();
  void Stop();

 private:
  friend class ClientSession;

  void DoSendData(TransportType type, const uint8_t* data, uint32_t len);
  void StartTimer();

  uint64_t peer_id_;
  boost::asio::io_service& io_service_;
  boost::asio::deadline_timer timer_;
  std::shared_ptr<ClientSession> session_;
  boost::asio::ip::tcp::endpoint endpoint_;
};

using ClientPtr = std::shared_ptr<Client>;

}  // namespace jkv
