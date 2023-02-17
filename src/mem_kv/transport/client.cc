// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/transport/client.h"

#include <jrpc/base/logging/logging.h>

namespace jkv {

ClientSession::ClientSession(boost::asio::io_service& io_service,
                             Client* client)
    : socket_(io_service),
      endpoint_(client->endpoint_),
      peer_id_(client->peer_id_),
      client_(nullptr),
      connected_(false),
      buffer_() {}

void ClientSession::Connect() {
  socket_.async_connect(endpoint_,
                        [this](const boost::system::error_code& err) {
                          if (err) {
                            JLOG_ERROR << "connect " << this->peer_id_
                                       << " error " << err.message();
                            return;
                          }
                          this->connected_ = true;
                          JLOG_WARN << "connected to " << this->peer_id_;

                          if (this->buffer_.Readable()) {
                            this->StartWrite();
                          }
                        });
}

void ClientSession::Send(TransportType type, const uint8_t* data,
                         uint32_t len) {
  uint32_t remaining = buffer_.ReadableBytes();
  TransportMeta meta;
  meta.set_type(type);
  meta.set_len(htonl(len));
  // assert(sizeof(TransportMeta) == 5);
  buffer_.Put((const uint8_t*)&meta, sizeof(TransportMeta));
  buffer_.Put(data, len);
  assert(remaining + sizeof(TransportMeta) + len == buffer_.ReadableBytes());

  if (connected_ && remaining == 0) {
    StartWrite();
  }
}

void ClientSession::StartWrite() {
  if (!buffer_.Readable()) {
    JLOG_ERROR << "buffer is not readable";
    return;
  }

  uint32_t remaining = buffer_.ReadableBytes();
  auto buffer = boost::asio::buffer(buffer_.Reader(), remaining);
  auto handler = [this](const boost::system::error_code& error,
                        std::size_t bytes) {
    if (error || bytes == 0) {
      JLOG_ERROR << "send " << this->peer_id_ << " error " << error.message();
      this->CloseSession();
      return;
    }

    JLOG_WARN << "start write";
    this->buffer_.ReadBytes(bytes);
    this->StartWrite();
  };
  boost::asio::async_write(socket_, buffer, handler);
}

void ClientSession::CloseSession() { client_->session_ = nullptr; }

Client::Client(uint64_t peer_id, boost::asio::io_service& io_service,
               const std::string& peer_str)
    : peer_id_(peer_id),
      io_service_(io_service),
      timer_(io_service),
      session_(nullptr) {
  std::vector<std::string> strs;
  boost::split(strs, peer_str, boost::is_any_of(":"));
  if (strs.size() != 2) {
    JLOG_FATAL << "invalid host " << peer_str;
  }
  auto addr = boost::asio::ip::address::from_string(strs[0]);
  int port = std::atoi(strs[1].c_str());
  endpoint_ = boost::asio::ip::tcp::endpoint(addr, port);
}

void Client::Start() { StartTimer(); }

void Client::Send(const jraft::Message& msg) {
  std::string str;
  if (!msg.SerializeToString(&str)) {
    JLOG_FATAL << "failed to serialize to str";
  }
  DoSendData(kStream, (const uint8_t*)str.data(), (uint32_t)str.size());
}

void Client::SendSnapshot(SnapshotPtr snap) {
  JLOG_FATAL << "not implemented now";
}

void Client::Update(const std::string& peer) {
  JLOG_FATAL << "not implemented now";
}

uint64_t Client::ActiveSince() {
  JLOG_FATAL << "not implemented now";
  return 0;
}

void Client::Stop() {}

void Client::DoSendData(TransportType type, const uint8_t* data, uint32_t len) {
  if (!session_) {
    // send at the first time
    session_ = std::make_shared<ClientSession>(io_service_, this);
    session_->Send(type, data, len);
    session_->Connect();
  } else {
    session_->Send(type, data, len);
  }
}

void Client::StartTimer() {
  timer_.expires_from_now(boost::posix_time::seconds(3));
  timer_.async_wait([this](const boost::system::error_code& err) {
    if (err) {
      JLOG_ERROR << "timer waiter error " << err.message();
      return;
    }
    this->StartTimer();
  });
}

}  // namespace jkv
