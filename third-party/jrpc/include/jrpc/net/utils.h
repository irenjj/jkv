// Copyright (c) renjj - All Rights Reserved
#pragma once

#include <pthread.h>

#include <string>

#include "net/sock_address.h"

namespace jrpc {

// for socket
int GetSocketError(int fd);
int CreateSocket(const SockAddress& sa);
int SetTcpNoDelay(int fd, bool on);
int SetReuseAddr(int fd, bool on);
int SetKeepAlive(int fd, bool on);
int SetNonBlocking(int fd);
int CreateEventFd();

}  // namespace jrpc
