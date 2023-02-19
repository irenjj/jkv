// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <leveldb/db.h>

namespace jkv {

class DiskStorage {
 public:
  explicit DiskStorage(const std::string& db_path);
  ~DiskStorage();

  std::string Get(const leveldb::Slice& key);
  void Put(const leveldb::Slice& key, const leveldb::Slice& val);
  void Del(const leveldb::Slice& key);

 private:
  std::string db_path_;
  leveldb::DB* db_;
};

}  // namespace jkv
