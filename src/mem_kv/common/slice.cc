// Copyright (c) renjj - All Rights Reserved
#include "mem_kv/common/slice.h"

namespace jkv {

// Create an empty slice.
Slice::Slice() : data_(""), size_(0) {}

// Create a slice that refers to d[0,n-1].
Slice::Slice(const char* d, size_t n) : data_(d), size_(n) {}

// Create a slice that refers to the contents of "s"
Slice::Slice(const std::string& s) : data_(s.data()), size_(s.size()) {}

// Create a slice that refers to s[0,strlen(s)-1]
Slice::Slice(const char* s) : data_(s), size_(strlen(s)) {}

// Three-way comparison.  Returns value:
//   <  0 iff "*this" <  "b",
//   == 0 iff "*this" == "b",
//   >  0 iff "*this" >  "b"
inline int Slice::compare(const Slice& b) const {
  const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
  int r = memcmp(data_, b.data_, min_len);
  if (r == 0) {
    if (size_ < b.size_)
      r = -1;
    else if (size_ > b.size_)
      r = +1;
  }
  return r;
}

}  // namespace jkv
