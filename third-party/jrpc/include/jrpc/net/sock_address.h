// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <string>

namespace jrpc {

class SockAddress {
 public:
  SockAddress() {}
  // Constructs an endpoint with given ip and port.
  SockAddress(const std::string& ip, uint16_t port, bool ipv6 = false);

  explicit SockAddress(const std::string& path);
  explicit SockAddress(const struct sockaddr_in& addr) : addr_(addr) {}
  explicit SockAddress(const struct sockaddr_in6& addr) : addr6_(addr) {}
  explicit SockAddress(const struct sockaddr_un& addr) : un_addr_(addr) {}
  explicit SockAddress(const struct sockaddr_storage& addr) : gen_addr_(addr) {}

  sa_family_t family() const { return gen_addr_.ss_family; }

  std::string ToIpString() const;
  std::string ToIpPortString() const;
  uint16_t ToPort() const;

  std::string ToString() const;

  const struct sockaddr* GetSockAddr() const {
    return reinterpret_cast<const sockaddr*>(&gen_addr_);
  }
  int GetSockAddrLen() const;

  uint32_t IpNetEndian() const;
  uint16_t PortNetEndian() const;

 private:
  static void FromIpPort(const char* ip, uint16_t port,
                         struct sockaddr_in* addr);
  static void FromIpPort(const char* ip, uint16_t port,
                         struct sockaddr_in6* addr);
  static void FromUnPath(const char* path, struct sockaddr_un* addr);
  static void ToIpPortUtil(char* buf, size_t size, const struct sockaddr* addr);
  static void ToIpUtil(char* buf, size_t size, const struct sockaddr* addr);

  static const struct sockaddr_in* sockaddr_in_cast(
      const struct sockaddr* addr) {
    return reinterpret_cast<const struct sockaddr_in*>(addr);
  }

  static const struct sockaddr_in6* sockaddr_in6_cast(
      const struct sockaddr* addr) {
    return reinterpret_cast<const struct sockaddr_in6*>(addr);
  }

  union {
    struct sockaddr_in addr_;
    struct sockaddr_in6 addr6_;
    struct sockaddr_un un_addr_;
    struct sockaddr_storage gen_addr_;
  };
};

}  // namespace jrpc
