// Copyright (c) renjj - All Rights Reserved
#include <gtest/gtest.h>

#include "mem_kv/common/byte_buffer.h"

TEST(ByteBufferTest, SimpleTest) {
  jkv::ByteBuffer buf;

  buf.Put((const uint8_t*)"abc", 3);
  ASSERT_TRUE(buf.Readable());
  ASSERT_TRUE(buf.ReadableBytes() == 3);

  buf.ReadBytes(1);
  char new_buf[4096]{};
  memcpy(new_buf, buf.Reader(), buf.ReadableBytes());
  ASSERT_TRUE(buf.ReadableBytes() == 2);
  ASSERT_TRUE(new_buf == std::string("bc"));
  ASSERT_TRUE(buf.GetSlice() == "bc");

  buf.ReadBytes(2);
  ASSERT_TRUE(buf.ReadableBytes() == 0);

  ASSERT_TRUE(buf.GetSlice() == "");

  buf.Put((const uint8_t*)"123456", 6);
  ASSERT_TRUE(buf.GetSlice().ToString() == "123456");
}
