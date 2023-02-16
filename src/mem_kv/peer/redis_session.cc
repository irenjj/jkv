// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/peer/redis_session.h"

#include <glib.h>
#include <jrpc/base/logging/logging.h>

#include <unordered_map>

#include "mem_kv/common/config.h"
#include "mem_kv/common/status.h"
#include "mem_kv/peer/peer_store.h"

namespace jkv {
namespace detail {

static const char* kOk = "+OK\r\n";
static const char* kErr = "-ERR %s\r\n";
static const char* kWrongType =
    "-WRONGTYPE Operation against a key holding the wrong kind of value\r\n";
static const char* kUnknownCommand = "-ERR unknown command `%s`\r\n";
static const char* kWrongNumberOfArguments =
    "-ERR wrong number of arguments for '%s' command\r\n";
static const char* kPong = "+PONG\r\n";
static const char* kNull = "$-1\r\n";

typedef std::function<void(RedisSessionPtr, struct redisReply* reply)>
    CommandCallback;

static std::unordered_map<std::string, CommandCallback> command_table = {
    {"ping", RedisSession::PingCommand}, {"PING", RedisSession::PingCommand},
    {"get", RedisSession::GetCommand},   {"GET", RedisSession::GetCommand},
    {"set", RedisSession::GetCommand},   {"SET", RedisSession::GetCommand},
    {"del", RedisSession::DelCommand},   {"DEL", RedisSession::DelCommand},
    {"keys", RedisSession::KeysCommand}, {"KEYS", RedisSession::KeysCommand},
};

static void BuildRedisStringArrayReply(const std::vector<std::string>& strs,
                                       std::string& reply) {
  char buf[64]{};
  snprintf(buf, sizeof(buf), "*%zu\r\n", strs.size());
  reply.append(buf);

  for (const std::string& str : strs) {
    snprintf(buf, sizeof(buf), "$%zu\r\n", str.size());
    reply.append(buf);

    if (!str.empty()) {
      reply.append(str);
      reply.append("\r\n");
    }
  }
}

}  // namespace detail

RedisSession::RedisSession(PeerStore* peer_store,
                           boost::asio::io_service& io_service)
    : peer_store_(peer_store),
      socket_(io_service),
      read_buffer_(kReceiveBufferSize),
      reader_(redisReaderCreate()),
      send_buffer_(),
      quit_(false) {}

RedisSession::~RedisSession() { redisReaderFree(reader_); }

void RedisSession::Start() {
  if (quit_) {
    return;
  }

  auto self = shared_from_this();
  auto buffer = boost::asio::buffer(read_buffer_.data(), read_buffer_.size());
  auto handler = [self](const boost::system::error_code& error, size_t bytes) {
    if (bytes == 0) {
      return;
    }
    if (error) {
      JLOG_ERROR << "read error " << error.message();
      return;
    }
    self->HandleRead(bytes);  // get bytes
  };
  socket_.async_read_some(buffer, std::move(handler));
}

void RedisSession::HandleRead(size_t bytes) {
  uint8_t* start = read_buffer_.data();
  uint8_t* end = read_buffer_.data() + bytes;
  int err = REDIS_OK;
  std::vector<struct redisReply*> replies;

  while (!quit_ && start < end) {
    auto p = (uint8_t*)memchr(start, '\n', bytes);
    if (!p) {
      this->Start();
      break;
    }

    size_t n = p + 1 - start;
    err = redisReaderFeed(reader_, (const char*)start, n);
    if (err != REDIS_OK) {
      JLOG_ERROR << "redis protocol err " << err << ", " << reader_->errstr;
      quit_ = true;
      break;
    }

    struct redisReply* reply = nullptr;
    err = redisReaderGetReply(reader_, (void**)&reply);
    if (err != REDIS_OK) {
      JLOG_ERROR << "redis protocol err " << err << ", " << reader_->errstr;
      quit_ = true;
      break;
    }
    if (reply) {
      replies.push_back(reply);
    }

    start += n;
    bytes -= n;
  }
  if (err == REDIS_OK) {
    for (auto reply : replies) {
      OnRedisReply(reply);
    }
    this->Start();
  }

  for (auto reply : replies) {
    freeReplyObject(reply);
  }
}

void RedisSession::OnRedisReply(struct redisReply* reply) {
  char buf[256]{};
  if (reply->type != REDIS_REPLY_ARRAY) {
    JLOG_ERROR << "wrong type " << reply->type;
    SendReply(detail::kWrongType, strlen(detail::kWrongType));
    return;
  }

  if (reply->elements < 1) {
    JLOG_ERROR << "wrong elements " << reply->elements;
    int n = snprintf(buf, sizeof(buf), detail::kWrongNumberOfArguments, "");
    SendReply(buf, n);
    return;
  }

  if (reply->element[0]->type != REDIS_REPLY_STRING) {
    SendReply(detail::kWrongType, strlen(detail::kWrongType));
    return;
  }

  std::string command(reply->element[0]->str, reply->element[0]->len);
  auto it = detail::command_table.find(command);
  if (it == detail::command_table.end()) {
    int n =
        snprintf(buf, sizeof(buf), detail::kUnknownCommand, command.c_str());
    SendReply(buf, n);
    return;
  }

  detail::CommandCallback& cb = it->second;
  cb(shared_from_this(), reply);
}

void RedisSession::SendReply(const char* data, uint32_t len) {
  uint32_t bytes = send_buffer_.ReadableBytes();
  send_buffer_.Put((uint8_t*)data, len);
  if (bytes == 0) {
    StartSend();
  }
}

void RedisSession::StartSend() {
  if (!send_buffer_.Readable()) {
    return;
  }
  auto self = shared_from_this();
  uint32_t remaining = send_buffer_.ReadableBytes();
  auto buffer = boost::asio::buffer(send_buffer_.Reader(), remaining);
  auto handler = [self](const boost::system::error_code& error,
                        std::size_t bytes) {
    if (bytes == 0) {
      return;
    }
    if (error) {
      JLOG_DEBUG << "send error " << error.message();
      return;
    }
    std::string str((const char*)self->send_buffer_.Reader(), bytes);
    self->send_buffer_.ReadBytes(bytes);
    self->StartSend();
  };
  boost::asio::async_write(socket_, buffer, std::move(handler));
}

void RedisSession::PingCommand(std::shared_ptr<RedisSession> self,
                               struct redisReply* reply) {
  self->SendReply(detail::kPong, strlen(detail::kPong));
}

void RedisSession::GetCommand(std::shared_ptr<RedisSession> self,
                              struct redisReply* reply) {
  assert(reply->type = REDIS_REPLY_ARRAY);
  assert(reply->elements > 0);

  if (reply->elements != 2) {
    char buf[64]{};
    JLOG_ERROR << "wrong elements " << reply->elements;
    int n = snprintf(buf, sizeof(buf), detail::kWrongNumberOfArguments, "get");
    self->SendReply(buf, n);
    return;
  }

  if (reply->element[1]->type != REDIS_REPLY_STRING) {
    JLOG_ERROR << "wrong type " << reply->element[1]->type;
    self->SendReply(detail::kWrongType, strlen(detail::kWrongType));
    return;
  }

  std::string value;
  std::string key(reply->element[1]->str, reply->element[1]->len);
  bool get = self->peer_store_->Get(key, &value);
  if (!get) {
    self->SendReply(detail::kNull, strlen(detail::kNull));
  } else {
    char* str = g_strdup_printf("$%lu\r\n%s\r\n", value.size(), value.c_str());
    self->SendReply(str, strlen(str));
    g_free(str);
  }
}

void RedisSession::SetCommand(std::shared_ptr<RedisSession> self,
                              struct redisReply* reply) {
  assert(reply->type = REDIS_REPLY_ARRAY);
  assert(reply->elements > 0);

  if (reply->elements != 3) {
    char buf[64]{};
    JLOG_ERROR << "wrong elements " << reply->elements;
    int n = snprintf(buf, sizeof(buf), detail::kWrongNumberOfArguments, "set");
    self->SendReply(buf, n);
    return;
  }

  if (reply->element[1]->type != REDIS_REPLY_STRING ||
      reply->element[2]->type != REDIS_REPLY_STRING) {
    JLOG_ERROR << "wrong type " << reply->element[1]->type;
    self->SendReply(detail::kWrongType, strlen(detail::kWrongType));
    return;
  }
  std::string key(reply->element[1]->str, reply->element[1]->len);
  std::string value(reply->element[2]->str, reply->element[2]->len);
  self->peer_store_->Set(std::move(key), std::move(value),
                         [self](const Status& status) {
                           if (status.ok()) {
                             self->SendReply(detail::kOk, strlen(detail::kOk));
                           } else {
                             char buff[256];
                             int n = snprintf(buff, sizeof(buff), detail::kErr,
                                              status.ToString().c_str());
                             self->SendReply(buff, n);
                           }
                         });
}

void RedisSession::DelCommand(std::shared_ptr<RedisSession> self,
                              struct redisReply* reply) {
  assert(reply->type = REDIS_REPLY_ARRAY);
  assert(reply->elements > 0);

  if (reply->elements <= 1) {
    char buf[64]{};
    int n = snprintf(buf, sizeof(buf), detail::kWrongNumberOfArguments, "del");
    self->SendReply(buf, n);
    return;
  }

  std::vector<std::string> keys;
  for (size_t i = 1; i < reply->elements; ++i) {
    redisReply* element = reply->element[i];
    if (element->type != REDIS_REPLY_STRING) {
      self->SendReply(detail::kWrongType, strlen(detail::kWrongType));
      return;
    }

    keys.emplace_back(element->str, element->len);
  }

  self->peer_store_->Del(std::move(keys), [self](const Status& status) {
    if (status.ok()) {
      self->SendReply(detail::kOk, strlen(detail::kOk));
    } else {
      char buff[256];
      int n =
          snprintf(buff, sizeof(buff), detail::kErr, status.ToString().c_str());
      self->SendReply(buff, n);
    }
  });
}

void RedisSession::KeysCommand(std::shared_ptr<RedisSession> self,
                               struct redisReply* reply) {
  assert(reply->type = REDIS_REPLY_ARRAY);
  assert(reply->elements > 0);

  if (reply->elements != 2) {
    char buf[64]{};
    int n = snprintf(buf, sizeof(buf), detail::kWrongNumberOfArguments, "keys");
    self->SendReply(buf, n);
    return;
  }

  redisReply* element = reply->element[1];

  if (element->type != REDIS_REPLY_STRING) {
    self->SendReply(detail::kWrongType, strlen(detail::kWrongType));
    return;
  }

  std::vector<std::string> keys;
  self->peer_store_->Keys(element->str, element->len, &keys);
  std::string str;
  detail::BuildRedisStringArrayReply(keys, str);
  self->SendReply(str.data(), str.size());
}

}  // namespace jkv
