// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <cstdint>

#include "mem_kv/wal/wal_option.h"

#define MAX_WAL_RECORD_LEN (0x00FFFFFF)

namespace jkv {

#pragma pack(1)
class WalRecord {
 public:
  WalRecord();
  WalRecord(WalType type, uint32_t crc);
  ~WalRecord() = default;

  WalType type() const { return type_; }
  void set_type(WalType type) { type_ = type; }
  uint32_t crc() const { return crc_; }
  void set_crc(uint32_t crc) { crc_ = crc; }
  uint32_t len() const;
  void set_len(uint32_t len);

 private:
  WalType type_;      // data type
  uint32_t crc_;      // crc32 for data
  uint8_t len_[3]{};  // data length, max len: 0x00FFFFFF
  char data_[0];
};
#pragma pack()

}  // namespace jkv
