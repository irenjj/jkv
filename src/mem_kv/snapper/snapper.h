// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <string>

#include "eraft.pb.h"
#include "mem_kv/common/status.h"

namespace jkv {

class Snapper {
 public:
  explicit Snapper(const std::string& dir) : dir_(dir) {}
  ~Snapper() = default;

  Status LoadSnap(jraft::Snapshot* snap);
  Status SaveSnap(const jraft::Snapshot& snap);

  static std::string SnapName(uint64_t term, uint64_t index);

 private:
  void GetSnapNames(std::vector<std::string>* names);
  Status LoadSnap(jraft::Snapshot* snap, const std::string& filename);

  std::string dir_;
};

}  // namespace jkv
