// Copyright (c) renjj - All Rights Reserved
#include <gtest/gtest.h>

#include "mem_kv/wal/wal.h"
#include "mem_kv/wal/wal_record.h"

TEST(WalTest, TestLen) {
  jkv::WalRecord record;
  for (int i = 0; i < MAX_WAL_RECORD_LEN; i++) {
    record.set_len(i);
    ASSERT_EQ(record.len(), i) << "i: " << i;
  }

  ASSERT_EQ(MAX_WAL_RECORD_LEN, 0xff << 16 | 0xff << 8 | 0xff);

  uint32_t len = MAX_WAL_RECORD_LEN + 1;
  record.set_len(len);
  ASSERT_EQ(record.len(), MAX_WAL_RECORD_LEN);
}

TEST(WalTest, TestScanWalName) {
  struct TestArgs {
    std::string str;
    uint64_t wseq;
    uint64_t windex;
    bool wok;
  } tests[] {
      {"0000000000000000-0000000000000000.wal", 0, 0, true},
      {"0000000000000100-0000000000000101.wal", 0x100, 0x101, true},
      {"0000000000000000.wa", 0, 0, false},
      {"0000000000000000-0000000000000000.snap", 0, 0, false}
  };

  int i = 0;
  for (const auto& test : tests) {
    uint64_t seq;
    uint64_t index;
    bool ok = jkv::Wal::ParseWalName(test.str, &seq, &index);

    ASSERT_EQ(ok, test.wok) << "i: " << i;
    ASSERT_EQ(seq, test.wseq) << "i: " << i;
    ASSERT_EQ(index, test.windex) << "i: " << i;

    i++;
  }
}

TEST(WalTest, TestSearchIndex) {
  struct TestArgs {
    std::vector<std::string> names;
    uint64_t index;
    int windex;
    bool wok;
  } tests[] {
      {{"0000000000000000-0000000000000000.wal", "0000000000000001-0000000000001000.wal", "0000000000000002-0000000000002000.wal"}, 0x1000, 1, true},
      {{"0000000000000001-0000000000004000.wal", "0000000000000002-0000000000003000.wal", "0000000000000003-0000000000005000.wal"}, 0x4000, 1, true},
      {{"0000000000000001-0000000000002000.wal", "0000000000000002-0000000000003000.wal", "0000000000000003-0000000000005000.wal"}, 0x1000, -1, false}
  };

  int i = 0;
  for (const auto& test : tests) {
    uint64_t index;
    bool ok = jkv::Wal::SearchIndex(test.names, test.index, &index);
    ASSERT_EQ(ok, test.wok) << "i: " << i;
    ASSERT_EQ(index, test.windex) << "i: " << i;

    i++;
  }
}
