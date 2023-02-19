// Copyright (c) renjj - All Rights Reserved
#include "storage/disk_storage.h"

#include <jrpc/base/logging/logging.h>

#include <boost/filesystem.hpp>

namespace jkv {

DiskStorage::DiskStorage(const std::string& db_path)
    : db_path_(db_path), db_(nullptr) {
  // create dir
  if (!boost::filesystem::exists(db_path_)) {
    boost::filesystem::create_directories(db_path_);
  }

  leveldb::Options opts;
  opts.create_if_missing = true;
  // open db
  leveldb::Status status = leveldb::DB::Open(opts, db_path_, &db_);
  if (!status.ok()) {
    JLOG_FATAL << "can't open db, err_msg: " << status.ToString();
  }
}

DiskStorage::~DiskStorage() {
  if (db_) {
    delete db_;
  }
}

std::string DiskStorage::Get(const leveldb::Slice& key) {
  std::string val;
  leveldb::Status status = db_->Get(leveldb::ReadOptions(), key, &val);
  if (!status.ok()) {
    JLOG_FATAL << "can't get kv from db, err_msg: " << status.ToString();
  }
  return val;
}

void DiskStorage::Put(const leveldb::Slice& key, const leveldb::Slice& val) {
  leveldb::Status status = db_->Put(leveldb::WriteOptions(), key, val);
  if (!status.ok()) {
    JLOG_FATAL << "can't put kv to db, err_msg: " << status.ToString();
  }
}

void DiskStorage::Del(const leveldb::Slice& key) {
  leveldb::Status status = db_->Delete(leveldb::WriteOptions(), key);
  if (!status.ok()) {
    JLOG_FATAL << "can't del kv from db, err_msg: " << status.ToString();
  }
}

}  // namespace jkv
