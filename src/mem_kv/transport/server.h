// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <memory>

#include "eraft.pb.h"
#include "transport_meta.pb.h"

namespace jkv {

class Server;
class Transport;
using MessagePtr = std::shared_ptr<jraft::Message>;
class Peer;

class ServerSession : public std::enable_shared_from_this<ServerSession> {
 public:
  ServerSession(boost::asio::io_service& io_service, Server* server);
  ~ServerSession() = default;

  void StartReadMeta();
  void StartReadMessage();
  void DecodeMessage(uint32_t len);
  void OnReceiveStreamMessage(MessagePtr msg);

  boost::asio::ip::tcp::socket& socket() { return socket_; }

 private:
  boost::asio::ip::tcp::socket socket_;
  Server* server_;
  TransportMeta meta_;
  std::vector<uint8_t> buffer_;
};

using ServerSessionPtr = std::shared_ptr<ServerSession>;

class Server {
 public:
  Server(boost::asio::io_service& io_service, const std::string& host,
         Peer* peer);
  ~Server() = default;

  void Start();
  void Stop();
  void OnMessage(MessagePtr msg);

 private:
  boost::asio::io_service& io_service_;
  boost::asio::ip::tcp::acceptor acceptor_;
  Peer* peer_;
};

}  // namespace jkv
