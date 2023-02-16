// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/transport/server.h"

#include <jrpc/base/logging/logging.h>

#include "mem_kv/common/status.h"
#include "mem_kv/peer/peer.h"

namespace jkv {

ServerSession::ServerSession(boost::asio::io_service& io_service,
                             Server* server)
    : socket_(io_service), server_(server) {}

void ServerSession::StartReadMeta() {
  assert(sizeof(meta_) == 5);
  auto self = shared_from_this();
  auto buffer = boost::asio::buffer(&meta_, sizeof(meta_));
  auto handler = [self](const boost::system::error_code& error,
                        std::size_t bytes) {
    if (bytes == 0) {
      return;
    }
    if (error) {
      JLOG_ERROR << "read error " << error.message();
      return;
    }

    if (bytes != sizeof(meta_)) {
      JLOG_ERROR << "invalid data len " << bytes;
      return;
    }
    self->StartReadMessage();
  };

  boost::asio::async_read(
      socket_, buffer, boost::asio::transfer_exactly(sizeof(meta_)), handler);
}

void ServerSession::StartReadMessage() {
  uint32_t len = ntohl(meta_.len());
  if (buffer_.capacity() < len) {
    buffer_.resize(len);
  }

  auto self = shared_from_this();
  auto buffer = boost::asio::buffer(buffer_.data(), len);
  auto handler = [self, len](const boost::system::error_code& error,
                             std::size_t bytes) {
    assert(len == ntohl(self->meta_.len()));
    if (error || bytes == 0) {
      JLOG_ERROR << "read error " << error.message();
      return;
    }

    if (bytes != len) {
      JLOG_ERROR << "invalid data len " << bytes << ", " << len;
      return;
    }
    self->DecodeMessage(len);
  };
}

void ServerSession::DecodeMessage(uint32_t len) {
  if (meta_.type() == kStream) {
    MessagePtr msg = std::make_shared<jraft::Message>();
    if (!msg->ParseFromArray((const void*)buffer_.data(), len)) {
      JLOG_FATAL << "failed to parse from array";
    }
    OnReceiveStreamMessage(msg);
  } else {
    JLOG_FATAL << "not implemented yet";
  }
}

void ServerSession::OnReceiveStreamMessage(MessagePtr msg) {
  server_->OnMessage(msg);
}

Server::Server(boost::asio::io_service& io_service, const std::string& host,
               Peer* peer)
    : io_service_(io_service), acceptor_(io_service), peer_(peer) {}

void Server::Start() {
  ServerSessionPtr session(new ServerSession(io_service_, this));
  acceptor_.async_accept(
      session->socket(),
      [this, session](const boost::system::error_code& error) {
        if (error) {
          JLOG_ERROR << "accept error " << error.message();
          return;
        }

        this->Start();
        session->StartReadMeta();  // async_read;
      });
}

void Server::Stop() {}

void Server::OnMessage(MessagePtr msg) {
  peer_->Process(msg, [](const Status& status) {
    if (!status.ok()) {
      JLOG_ERROR << "process error " << status.ToString();
    }
  });
}

}  // namespace jkv