/*
 * Authored by Alex Hultman, 2018-2019.
 * Intellectual property of third-party.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BSD_H
#define BSD_H

// top-most wrapper of bsd-like syscalls

// holds everything you need from the bsd/winsock interfaces, only included by internal libusockets.h
// here everything about the syscalls are inline-wrapped and included

#ifdef _WIN32
#define NOMINMAX
#include <WinSock2.h>
#include <Ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define SETSOCKOPT_PTR_TYPE const char *
#define LIBUS_SOCKET_ERROR INVALID_SOCKET
#else
#define _GNU_SOURCE
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#define SETSOCKOPT_PTR_TYPE int *
#define LIBUS_SOCKET_ERROR -1
#endif

#define DO_NOT_REUSE_PORT 2 // LIBUS_LISTEN_OPTION_LINUX_REUSE_PORT

static inline LIBUS_SOCKET_DESCRIPTOR apple_no_sigpipe(LIBUS_SOCKET_DESCRIPTOR fd) {
#ifdef __APPLE__
    if (fd != LIBUS_SOCKET_ERROR) {
        int no_sigpipe = 1;
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(int));
    }
#endif
    return fd;
}

static inline void bsd_socket_nodelay(LIBUS_SOCKET_DESCRIPTOR fd, int enabled) {
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *) &enabled, sizeof(enabled));
}

static inline void bsd_socket_flush(LIBUS_SOCKET_DESCRIPTOR fd) {
    // Linux TCP_CORK has the same underlying corking mechanism as with MSG_MORE
#ifdef TCP_CORK
    int enabled = 0;
    setsockopt(fd, IPPROTO_TCP, TCP_CORK, &enabled, sizeof(int));
#endif
}

static inline LIBUS_SOCKET_DESCRIPTOR bsd_create_socket(int domain, int type, int protocol) {
    // returns INVALID_SOCKET on error
    int flags = 0;
#if defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)
    flags = SOCK_CLOEXEC | SOCK_NONBLOCK;
#endif

    LIBUS_SOCKET_DESCRIPTOR created_fd = socket(domain, type | flags, protocol);

    return apple_no_sigpipe(created_fd);
}

static inline void bsd_close_socket(LIBUS_SOCKET_DESCRIPTOR fd) {
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

static inline void bsd_shutdown_socket(LIBUS_SOCKET_DESCRIPTOR fd) {
#ifdef _WIN32
    shutdown(fd, SD_SEND);
#else
    shutdown(fd, SHUT_WR);
#endif
}

struct bsd_addr_t {
    struct sockaddr_storage mem;
    socklen_t len;
    char *ip;
    int ip_length;
};

static inline void internal_finalize_bsd_addr(struct bsd_addr_t *addr) {
    // parse, so to speak, the address
    if (addr->mem.ss_family == AF_INET6) {
        addr->ip = (char *) &((struct sockaddr_in6 *) addr)->sin6_addr;
        addr->ip_length = sizeof(struct in6_addr);
    } else if (addr->mem.ss_family == AF_INET) {
        addr->ip = (char *) &((struct sockaddr_in *) addr)->sin_addr;
        addr->ip_length = sizeof(struct in_addr);
    } else {
        addr->ip_length = 0;
    }
}

static inline int bsd_socket_addr(LIBUS_SOCKET_DESCRIPTOR fd, struct bsd_addr_t *addr) {
    addr->len = sizeof(addr->mem);
    if (getpeername(fd, (struct sockaddr *) &addr->mem, &addr->len)) {
        return -1;
    }
    internal_finalize_bsd_addr(addr);
    return 0;
}

static inline char *bsd_addr_get_ip(struct bsd_addr_t *addr) {
    return addr->ip;
}

static inline int bsd_addr_get_ip_length(struct bsd_addr_t *addr) {
    return addr->ip_length;
}

// called by dispatch_ready_poll
static inline LIBUS_SOCKET_DESCRIPTOR bsd_accept_socket(LIBUS_SOCKET_DESCRIPTOR fd, struct bsd_addr_t *addr) {
    LIBUS_SOCKET_DESCRIPTOR accepted_fd;
    addr->len = sizeof(addr->mem);

#if defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)
    // Linux, FreeBSD
    accepted_fd = accept4(fd, (struct sockaddr *) addr, &addr->len, SOCK_CLOEXEC | SOCK_NONBLOCK);
#else
    // Windows, OS X
    accepted_fd = accept(fd, (struct sockaddr *) addr, &addr->len);
#endif

    internal_finalize_bsd_addr(addr);

    return apple_no_sigpipe(accepted_fd);
}

static inline int bsd_recv(LIBUS_SOCKET_DESCRIPTOR fd, void *buf, int length, int flags) {
    return recv(fd, buf, length, flags);
}

static inline int bsd_send(LIBUS_SOCKET_DESCRIPTOR fd, const char *buf, int length, int msg_more) {

    // MSG_MORE (Linux), MSG_PARTIAL (Windows), TCP_NOPUSH (BSD)

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

#ifdef MSG_MORE

    // for Linux we do not want signals
    return send(fd, buf, length, (msg_more * MSG_MORE) | MSG_NOSIGNAL);

#else

    // use TCP_NOPUSH

    return send(fd, buf, length, MSG_NOSIGNAL);

#endif
}

static inline int bsd_would_block() {
#ifdef _WIN32
    return WSAGetLastError() == WSAEWOULDBLOCK;
#else
    return errno == EWOULDBLOCK;// || errno == EAGAIN;
#endif
}

// return LIBUS_SOCKET_ERROR or the fd that represents listen socket
// listen both on ipv6 and ipv4
static inline LIBUS_SOCKET_DESCRIPTOR bsd_create_listen_socket(const char *host, int port, int options) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_string[16];
    sprintf(port_string, "%d", port);

    if (getaddrinfo(host, port_string, &hints, &result)) {
        return LIBUS_SOCKET_ERROR;
    }

    LIBUS_SOCKET_DESCRIPTOR listenFd = LIBUS_SOCKET_ERROR;
    struct addrinfo *listenAddr;
    for (struct addrinfo *a = result; a && listenFd == LIBUS_SOCKET_ERROR; a = a->ai_next) {
        if (a->ai_family == AF_INET6) {
            listenFd = bsd_create_socket(a->ai_family, a->ai_socktype, a->ai_protocol);
            listenAddr = a;
        }
    }

    for (struct addrinfo *a = result; a && listenFd == LIBUS_SOCKET_ERROR; a = a->ai_next) {
        if (a->ai_family == AF_INET) {
            listenFd = bsd_create_socket(a->ai_family, a->ai_socktype, a->ai_protocol);
            listenAddr = a;
        }
    }

    if (listenFd == LIBUS_SOCKET_ERROR) {
        freeaddrinfo(result);
        return LIBUS_SOCKET_ERROR;
    }

    /* Always enable SO_REUSEPORT and SO_REUSEADDR _unless_ options specify otherwise */
#if defined(__linux) && defined(SO_REUSEPORT)
    if (!(options & DO_NOT_REUSE_PORT)) {
        int optval = 1;
        setsockopt(listenFd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    }
#endif

    int enabled = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (SETSOCKOPT_PTR_TYPE) &enabled, sizeof(enabled));

#ifdef IPV6_V6ONLY
    int disabled = 0;
    setsockopt(listenFd, IPPROTO_IPV6, IPV6_V6ONLY, (SETSOCKOPT_PTR_TYPE) &disabled, sizeof(disabled));
#endif

    if (bind(listenFd, listenAddr->ai_addr, listenAddr->ai_addrlen) || listen(listenFd, 512)) {
        bsd_close_socket(listenFd);
        freeaddrinfo(result);
        return LIBUS_SOCKET_ERROR;
    }

    freeaddrinfo(result);
    return listenFd;
}

static inline LIBUS_SOCKET_DESCRIPTOR bsd_create_connect_socket(const char *host, int port, int options) {
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_string[16];
    sprintf(port_string, "%d", port);

    if (getaddrinfo(host, port_string, &hints, &result) != 0) {
        return LIBUS_SOCKET_ERROR;
    }

    LIBUS_SOCKET_DESCRIPTOR fd = bsd_create_socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (fd == LIBUS_SOCKET_ERROR) {
        freeaddrinfo(result);
        return LIBUS_SOCKET_ERROR;
    }

    connect(fd, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);

    return fd;
}

#endif // BSD_H
