#ifndef BSD_H
#define BSD_H

// top-most wrapper of bsd-like syscalls

// holds everything you need from the bsd/winsock interfaces, only included by internal libusockets.h
// here everything about the syscalls are inline-wrapped and included

// why does accept4 require this?
#define __USE_GNU

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

#define LIBUS_SOCKET_DESCRIPTOR int // wrong!
#define LIBUS_SOCKET_ERROR -1

#define ONLY_IPV4 1 // stupid option that should never be used! support both by default!
#define REUSE_PORT 2

#ifndef INVALID_SOCKET // not a windoze box
#define INVALID_SOCKET -1
#define TCP_CORK -1
#endif

#ifndef MSG_MORE // macOS, see https://nwat.xyz/blog/2014/01/16/porting-msg_more-and-msg_nosigpipe-to-osx/
# define MSG_MORE 0
#define MSG_NOSIGNAL 0
#endif

static inline void bsd_socket_nodelay(LIBUS_SOCKET_DESCRIPTOR fd, int enabled) {
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *) &enabled, sizeof(enabled));
}

static inline void bsd_socket_flush(LIBUS_SOCKET_DESCRIPTOR fd) {
    int enabled = 0;
    setsockopt(fd, IPPROTO_TCP, TCP_CORK, &enabled, sizeof(int));
}

static inline LIBUS_SOCKET_DESCRIPTOR bsd_create_socket(int domain, int type, int protocol) {
    // returns INVALID_SOCKET on error
    int flags = 0;
#if defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)
    flags = SOCK_CLOEXEC | SOCK_NONBLOCK;
#endif

    LIBUS_SOCKET_DESCRIPTOR created_fd = socket(domain, type | flags, protocol);

#ifdef __APPLE__
    if (created_fd != INVALID_SOCKET) {
        int no_sigpipe = 1;
        setsockopt(created_fd, SOL_SOCKET, SO_NOSIGPIPE, &no_sigpipe, sizeof(int));
    }
#endif

    return created_fd;
}

static inline void bsd_close_socket(LIBUS_SOCKET_DESCRIPTOR fd) {
    close(fd);
}

static inline void bsd_shutdown_socket(LIBUS_SOCKET_DESCRIPTOR fd) {
    shutdown(fd, SHUT_WR);
}

// called by dispatch_ready_poll
static inline LIBUS_SOCKET_DESCRIPTOR bsd_accept_socket(LIBUS_SOCKET_DESCRIPTOR fd) {
    LIBUS_SOCKET_DESCRIPTOR acceptedFd;
#if defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)
    // Linux, FreeBSD
    acceptedFd = accept4(fd, 0, 0, SOCK_CLOEXEC | SOCK_NONBLOCK);
#else
    // Windows, OS X
    acceptedFd = accept(fd, 0, 0);
#endif

#ifdef __APPLE__
    if (acceptedFd != INVALID_SOCKET) {
        int noSigpipe = 1;
        setsockopt(acceptedFd, SOL_SOCKET, SO_NOSIGPIPE, &noSigpipe, sizeof(int));
    }
#endif
    return acceptedFd;
}

static inline int bsd_recv(LIBUS_SOCKET_DESCRIPTOR fd, void *buf, int length, int flags) {
    return recv(fd, buf, length, flags);
}

static inline int bsd_send(LIBUS_SOCKET_DESCRIPTOR fd, const char *buf, int length, int flags) {
    // for Linux we do not want signals
    return send(fd, buf, length, flags | MSG_NOSIGNAL);
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

    if (getaddrinfo(host, /*std::to_string(port).c_str()*/ port_string, &hints, &result)) {
        return 0;
    }

    LIBUS_SOCKET_DESCRIPTOR listenFd = LIBUS_SOCKET_ERROR;
    struct addrinfo *listenAddr;
    if ((options & ONLY_IPV4) == 0) {
        for (struct addrinfo *a = result; a && listenFd == LIBUS_SOCKET_ERROR; a = a->ai_next) {
            if (a->ai_family == AF_INET6) {
                listenFd = bsd_create_socket(a->ai_family, a->ai_socktype, a->ai_protocol);
                listenAddr = a;
            }
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
        return 0;
    }

#if defined(__linux) && defined(SO_REUSEPORT)
    if (options & REUSE_PORT) {
        int optval = 1;
        setsockopt(listenFd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    }
#endif

    int enabled = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

    if (bind(listenFd, listenAddr->ai_addr, listenAddr->ai_addrlen) || listen(listenFd, 512)) {
        bsd_close_socket(listenFd);
        freeaddrinfo(result);
        return 0;
    }

    return listenFd;
}

#endif // BSD_H
