// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/wal/wal_record.h"

#include <algorithm>

namespace jkv {

WalRecord::WalRecord() : type_(kWalInvalidType), crc_(0) {}

WalRecord::WalRecord(WalType type, uint32_t crc) : type_(type), crc_(crc) {}

void WalRecord::set_len(uint32_t len) {
  len = std::min(len, (uint32_t)MAX_WAL_RECORD_LEN);
  len_[2] = (len >> 16) & 0x000000FF;
  len_[1] = (len >> 8) & 0x000000FF;
  len_[0] = (len >> 0) & 0x000000FF;
}

uint32_t WalRecord::len() const {
  return uint32_t(len_[2]) << 16 | uint32_t(len_[1]) << 8 |
         uint32_t(len_[0]) << 0;
}

}  // namespace jkv
