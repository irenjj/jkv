// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/peer/peer_store.h"

#include <boost/algorithm/string.hpp>

#include "commit_data.pb.h"
#include "mem_kv/common/status.h"
#include "mem_kv/peer/peer.h"
#include "mem_kv/peer/redis_session.h"

namespace jkv {

PeerStore::PeerStore(Peer* peer, const std::string& snap_data, uint16_t port)
    : peer_(peer), io_service_(), acceptor_(io_service_), next_request_id_(0) {
  if (!snap_data.empty()) {
    std::unordered_map<std::string, std::string> kvs;
    std::vector<std::string> kvps;
    // "k1,v1|k2,v2|..|kn,vn"
    boost::split(kvps, snap_data, boost::is_any_of("|"));
    for (const auto& kvp : kvps) {
      std::vector<std::string> kv;
      boost::split(kv, kvp, boost::is_any_of(","));
      kvs[std::move(kv[0])] = std::move(kv[1]);
    }

    std::swap(kvs_, kvs);
  }

  // initialize network
  auto addr = boost::asio::ip::address::from_string("0.0.0.0");
  auto ep = boost::asio::ip::tcp::endpoint(addr, port);
  // server start to listen
  acceptor_.open(ep.protocol());
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(1));
  acceptor_.bind(ep);
  acceptor_.listen();
}

PeerStore::~PeerStore() {
  if (worker_.joinable()) {
    worker_.join();
  }
}

void PeerStore::Start(std::promise<pthread_t>& promise) {
  StartAccept();

  worker_ = std::thread([this, &promise]() {
    promise.set_value(pthread_self());
    this->io_service_.run();
  });
}

void PeerStore::Stop() {
  io_service_.stop();
  if (worker_.joinable()) {
    worker_.join();
  }
}

bool PeerStore::Get(const std::string& key, std::string* val) {
  auto it = kvs_.find(key);
  if (it == kvs_.end()) {
    return false;
  }

  *val = it->second;
  return true;
}

void PeerStore::Set(const std::string& key, const std::string& val,
                    const StatusCallback& cb) {
  uint32_t commit_id = next_request_id_++;

  RaftCommit commit;
  commit.set_node_id(peer_->id());
  commit.set_commit_id(commit_id);
  auto redis_data = commit.mutable_redis_data();
  redis_data->set_type(kCommitSet);
  redis_data->mutable_strs()->Add(key.c_str());
  redis_data->mutable_strs()->Add(val.c_str());

  pending_reqs_[commit_id] = cb;

  std::string str;
  if (!commit.SerializeToString(&str)) {
    JLOG_FATAL << "failed to serialize to str";
  }
  peer_->Propose(std::move(str), [this, commit_id](const Status& status) {
    io_service_.post([this, status, commit_id]() {
      if (status.ok()) {
        return;
      }

      auto it = pending_reqs_.find(commit_id);
      if (it != pending_reqs_.end()) {
        it->second(status);
        pending_reqs_.erase(it);
      }
    });
  });
}

void PeerStore::Del(const std::vector<std::string>& keys,
                    const StatusCallback& cb) {
  uint32_t commit_id = next_request_id_++;

  RaftCommit commit;
  commit.set_node_id(peer_->id());
  commit.set_commit_id(commit_id);
  auto redis_data = commit.mutable_redis_data();
  redis_data->set_type(kCommitDel);
  for (const auto& key : keys) {
    redis_data->mutable_strs()->Add(std::move(key.c_str()));
  }

  pending_reqs_[commit_id] = cb;

  std::string str;
  if (!commit.SerializeToString(&str)) {
    JLOG_FATAL << "failed to serialize to str";
  }
  peer_->Propose(std::move(str), [this, commit_id](const Status& status) {
    io_service_.post([commit_id, status, this]() {
      auto it = pending_reqs_.find(commit_id);
      if (it != pending_reqs_.end()) {
        it->second(status);
        pending_reqs_.erase(it);
      }
    });
  });
}

void PeerStore::GetSnapshot(const GetSnapshotCallback& cb) {
  io_service_.post([this, cb] {
    std::shared_ptr<std::string> data = std::make_shared<std::string>();
    bool first = true;
    for (const auto& kv : kvs_) {
      if (first) {
        first = false;
      } else {
        *data += "|";
      }
      *data += kv.first + "," + kv.second;
    }
    cb(data);
  });
}

void PeerStore::RecoverFromSnapshot(const std::string& snap_data,
                                    const StatusCallback& cb) {
  io_service_.post([this, snap_data, cb] {
    if (!snap_data.empty()) {
      std::unordered_map<std::string, std::string> kvs;
      std::vector<std::string> kvps;
      // "k1,v1|k2,v2|..|kn,vn"
      boost::split(kvps, snap_data, boost::is_any_of("|"));
      for (const auto& kvp : kvps) {
        std::vector<std::string> kv;
        boost::split(kv, kvp, boost::is_any_of(","));
        kvs[std::move(kv[0])] = std::move(kv[1]);
      }

      std::swap(kvs_, kvs);
    }
    cb(Status::OK());
  });
}

void PeerStore::Keys(const char* pattern, int len,
                     std::vector<std::string>* keys) {
  for (const auto& it : kvs_) {
    if (StringMatchLen(pattern, len, it.first.c_str(), it.first.size(), 0)) {
      keys->push_back(it.first);
    }
  }
}

void PeerStore::ReadCommit(EntryPtr entry) {
  auto cb = [this, entry] {
    RaftCommit commit;
    if (!commit.ParseFromString(entry->data())) {
      JLOG_FATAL << "failed to parse from str";
    }
    RedisCommitData* data = commit.mutable_redis_data();

    if (data->type() == kCommitSet) {
      if (data->strs().size() != 2) {
        JLOG_FATAL << "invalid str size";
      }
      this->kvs_.insert(
          {std::move(data->strs(0).c_str()), std::move(data->strs(1).c_str())});
    } else if (data->type() == kCommitDel) {
      for (const auto& key : data->strs()) {
        this->kvs_.erase(key);
      }
    } else {
      JLOG_FATAL << "invalid data type";
    }

    if (commit.node_id() == peer_->id()) {
      auto it = pending_reqs_.find(commit.commit_id());
      if (it != pending_reqs_.end()) {
        it->second(Status::OK());
        pending_reqs_.erase(it);
      }
    }
  };

  io_service_.post(std::move(cb));
}

int PeerStore::StringMatchLen(const char* pattern, int pattern_len,
                              const char* str, int str_len, int no_case) {
  while (pattern_len && str_len) {
    switch (pattern[0]) {
      case '*':
        while (pattern[1] == '*') {
          pattern++;
          pattern_len--;
        }
        if (pattern_len == 1) return 1; /* match */
        while (str_len) {
          if (StringMatchLen(pattern + 1, pattern_len - 1, str, str_len,
                             no_case))
            return 1; /* match */
          str++;
          str_len--;
        }
        return 0; /* no match */
        break;
      case '?':
        if (str_len == 0) return 0; /* no match */
        str++;
        str_len--;
        break;
      case '[': {
        int not_match, match;

        pattern++;
        pattern_len--;
        not_match = pattern[0] == '^';
        if (not_match) {
          pattern++;
          pattern_len--;
        }
        match = 0;
        while (1) {
          if (pattern[0] == '\\' && pattern_len >= 2) {
            pattern++;
            pattern_len--;
            if (pattern[0] == str[0]) match = 1;
          } else if (pattern[0] == ']') {
            break;
          } else if (pattern_len == 0) {
            pattern--;
            pattern_len++;
            break;
          } else if (pattern[1] == '-' && pattern_len >= 3) {
            int start = pattern[0];
            int end = pattern[2];
            int c = str[0];
            if (start > end) {
              int t = start;
              start = end;
              end = t;
            }
            if (no_case) {
              start = tolower(start);
              end = tolower(end);
              c = tolower(c);
            }
            pattern += 2;
            pattern_len -= 2;
            if (c >= start && c <= end) match = 1;
          } else {
            if (!no_case) {
              if (pattern[0] == str[0]) match = 1;
            } else {
              if (tolower((int)pattern[0]) == tolower((int)str[0])) match = 1;
            }
          }
          pattern++;
          pattern_len--;
        }
        if (not_match) match = !match;
        if (!match) return 0; /* no match */
        str++;
        str_len--;
        break;
      }
      case '\\':
        if (pattern_len >= 2) {
          pattern++;
          pattern_len--;
        }
        /* fall through */
      default:
        if (!no_case) {
          if (pattern[0] != str[0]) return 0; /* no match */
        } else {
          if (tolower((int)pattern[0]) != tolower((int)str[0]))
            return 0; /* no match */
        }
        str++;
        str_len--;
        break;
    }
    pattern++;
    pattern_len--;
    if (str_len == 0) {
      while (*pattern == '*') {
        pattern++;
        pattern_len--;
      }
      break;
    }
  }
  if (pattern_len == 0 && str_len == 0) return 1;
  return 0;
}

void PeerStore::StartAccept() {
  RedisSessionPtr session(new RedisSession(this, io_service_));

  acceptor_.async_accept(
      session->socket(),
      [this, session](const boost::system::error_code& error) {
        if (error) {
          JLOG_ERROR << "accept error " << error.message();
          return;
        }
        this->StartAccept();
        session->Start();
      });
}

}  // namespace jkv
