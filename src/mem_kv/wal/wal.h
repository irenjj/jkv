// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <cstdint>
#include <string>

#include "eraft.pb.h"
#include "mem_kv/wal/wal_option.h"
#include "wal_snapshot.pb.h"

namespace jkv {

class Status;
class WalFile;
using WalFilePtr = std::shared_ptr<WalFile>;
using EntryPtr = std::shared_ptr<jraft::Entry>;

class Wal {
 public:
  Wal();
  explicit Wal(const std::string& dir);
  Wal(const std::string& dir, const WalSnapshot& snap);
  ~Wal() = default;

  static void Create(const std::string& dir);

  Status ReadAll(jraft::HardState* hs, std::vector<EntryPtr>* ents);

  Status Save(const jraft::HardState& hs, const std::vector<EntryPtr>& ents);
  Status SaveSnapshot(const WalSnapshot& snap);
  Status SaveEntry(const jraft::Entry& entry);
  Status SaveHardState(const jraft::HardState& hs);
  Status Cut();

  static Status ReleaseTo(uint64_t index);

  static void GetWalNames(const std::string& dir,
                          std::vector<std::string>* names);
  static bool ParseWalName(const std::string& name, uint64_t* seq,
                           uint64_t* index);

  static bool IsValidSeq(const std::vector<std::string>& names);
  static bool SearchIndex(const std::vector<std::string>& names, uint64_t index,
                          uint64_t* name_index);

 private:
  static std::string WalFileName(uint64_t seq, uint64_t index);
  void HandleRecordWalRecord(WalType type, const char* data, size_t data_len,
                             bool* match_snap, jraft::HardState* hs,
                             std::vector<EntryPtr>* ents);

  std::string dir_;
  WalSnapshot start_;    // snapshot to start reading
  uint64_t last_index_;  // index of the last entry saved to the wal
  jraft::HardState hard_state_;
  std::vector<WalFilePtr> files_;
};

using WalPtr = std::shared_ptr<Wal>;

}  // namespace jkv
