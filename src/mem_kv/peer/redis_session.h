// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <hiredis.h>

#include <boost/asio.hpp>
#include <memory>

#include "mem_kv/common/byte_buffer.h"

namespace jkv {

class PeerStore;

class RedisSession : public std::enable_shared_from_this<RedisSession> {
 public:
  explicit RedisSession(PeerStore* peer_store,
                        boost::asio::io_service& io_service);
  ~RedisSession();

  void Start();
  void HandleRead(size_t bytes);
  void OnRedisReply(struct redisReply* reply);
  void SendReply(const char* data, uint32_t len);

  void StartSend();

  static void PingCommand(std::shared_ptr<RedisSession> self,
                          struct redisReply* reply);
  static void GetCommand(std::shared_ptr<RedisSession> self,
                         struct redisReply* reply);
  static void SetCommand(std::shared_ptr<RedisSession> self,
                         struct redisReply* reply);
  static void DelCommand(std::shared_ptr<RedisSession> self,
                         struct redisReply* reply);
  static void KeysCommand(std::shared_ptr<RedisSession> self,
                          struct redisReply* reply);

  boost::asio::ip::tcp::socket& socket() { return socket_; }

 private:
  PeerStore* peer_store_;
  boost::asio::ip::tcp::socket socket_;
  std::vector<uint8_t> read_buffer_;
  redisReader* reader_;
  ByteBuffer send_buffer_;
  bool quit_;
};

using RedisSessionPtr = std::shared_ptr<RedisSession>;

}  // namespace jkv
