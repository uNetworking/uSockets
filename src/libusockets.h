#ifndef LIBUSOCKETS_H
#define LIBUSOCKETS_H

/* 512kb shared receive buffer */
#define LIBUS_RECV_BUFFER_LENGTH 524288
/* A timeout granularity of 4 seconds means give or take 4 seconds from set timeout */
#define LIBUS_TIMEOUT_GRANULARITY 4

/* Define what a socket descriptor is based on platform */
#ifdef __WIN32
#include <windows.h>
#define LIBUS_SOCKET_DESCRIPTOR SOCKET
#else
#define LIBUS_SOCKET_DESCRIPTOR int
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* I guess these are listening options */
enum {
    OPTION_REUSE_PORT
};

/* Library types publicly available */
struct us_socket;
struct us_timer;
struct us_socket_context;
struct us_loop;
struct us_ssl_socket_context;
struct us_ssl_socket;

/* These are public interfaces sorted by subject */
#include "interfaces/timer.h"
#include "interfaces/context.h"
#include "interfaces/loop.h"
#include "interfaces/poll.h"
#include "interfaces/ssl.h"
#include "interfaces/socket.h"

#ifdef __cplusplus
}
#endif

/* Decide what eventing system to use */
#if !defined(LIBUS_USE_EPOLL) && !defined(LIBUS_USE_LIBUV)
// todo: if not on linux, swap to libuv by default
#define LIBUS_USE_EPOLL
#endif

#endif // LIBUSOCKETS_H
