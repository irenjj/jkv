// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <boost/asio.hpp>
#include <unordered_map>

#include "eraft.pb.h"
#include "mem_kv/common/config.h"

namespace jkv {

class Peer;

using EntryPtr = std::shared_ptr<jraft::Entry>;

class PeerStore {
 public:
  explicit PeerStore(Peer* peer, const std::string& snap_data, uint16_t port);
  ~PeerStore();

  void Start(std::promise<pthread_t>& promise);
  void Stop();

  bool Get(const std::string& key, std::string* val);
  void Set(const std::string& key, const std::string& val,
           const StatusCallback& cb);
  void Del(const std::vector<std::string>& keys, const StatusCallback& cb);

  void GetSnapshot(const GetSnapshotCallback& cb);
  void RecoverFromSnapshot(const std::string& snap_data,
                           const StatusCallback& cb);

  void Keys(const char* pattern, int len, std::vector<std::string>* keys);

  void ReadCommit(EntryPtr entry);

  static int StringMatchLen(const char* pattern, int pattern_len,
                            const char* str, int str_len, int no_case);

 private:
  void StartAccept();

  Peer* peer_;
  boost::asio::io_service io_service_;
  boost::asio::ip::tcp::acceptor acceptor_;
  uint32_t next_request_id_;
  std::thread worker_;
  std::unordered_map<std::string, std::string> kvs_;
  std::unordered_map<uint32_t, StatusCallback> pending_reqs_;
};

}  // namespace jkv
