#ifndef UTIL_TCP_H_
#define UTIL_TCP_H_

#ifdef _WIN32

#include <winsock2.h>
#define TCP_SOCKET_T       SOCKET
#define TCP_ERRNO          WSAGetLastError()

#include "util_str.h"
#include "util_file.h"

#else

#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#define TCP_SOCKET_T      int
#define TCP_ERRNO         errno

#endif

#define TCP_TIMEOUT 5

#endif /* UTIL_TCP_H_ */
