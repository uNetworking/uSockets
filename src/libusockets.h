#ifndef LIBUSOCKETS_H
#define LIBUSOCKETS_H

/* 512kb shared receive buffer */
#define LIBUS_RECV_BUFFER_LENGTH 524288
/* a timeout granularity of 4 seconds means give or take 4 seconds from set timeout */
#define LIBUS_TIMEOUT_GRANULARITY 4

#ifdef __cplusplus
extern "C" {
#endif

/* I guess these are listening options */
enum {
    OPTION_REUSE_PORT
};

/* represents an event loop */
struct us_loop;

/* a socket context is a set of shared callbacks (shared behavior) of a set of sockets.
 * it also holds shared context extension memory (user data) which can be accessed by all sockets */
struct us_socket_context;

/* represents a TCP-only socket */
struct us_socket;

/* represents a TCP-only listening socket */
struct us_listen_socket;

struct us_socket_context {
    struct us_loop *loop;
    //unsigned short timeout;
    struct us_socket *head;
    struct us_socket_context *next;

    // on_open -> structure created, on_creation
    // on_close -> structure destroyed? on_destruction
    // on_data
    // on_timeout

    void (*on_accepted)(struct us_socket *);
    void (*on_data)(struct us_socket *, void *data, int length);
    void (*on_writable)(struct us_socket *);
    void (*on_end)(struct us_socket *);
    //void (*on_timeout)(struct us_socket_context *);
    void (*on_socket_timeout)(struct us_socket *);
};

// void us_listen_socket_stop(..

/* creates a new event loop, a shared 1-per-thread-and-only-1-per-thread resource */
struct us_loop *us_create_loop(void (*wakeup_cb)(struct us_loop *loop), int userdata_size);

/* retrieves loop extension memory */
void *us_loop_userdata(struct us_loop *loop);

/* blocks the calling thread until no more async operations are scheduled, drives the event loop.
 * never call this function again until it returns, never call it from within another run call */
void us_loop_run(struct us_loop *loop);

// per context
struct us_socket_context *us_create_socket_context(struct us_loop *loop, int context_size);
struct us_listen_socket *us_socket_context_listen(struct us_socket_context *context, const char *host, int port, int options, int socket_ext_size);
void us_context_connect(const char *host, int port, int options, int ext_size, void (*cb)(struct us_socket *), void *user_data);
void us_socket_context_link(struct us_socket_context *context, struct us_socket *s);

// per socket
int us_socket_write(struct us_socket *s, const char *data, int length, int msg_more);
void us_socket_timeout(struct us_socket *s, unsigned int seconds);
inline void *us_socket_ext(struct us_socket *s);
int us_socket_type(struct us_socket *s);
void *us_socket_get_context(struct us_socket *s);

#ifdef __cplusplus
}
#endif

// default to epoll
#if !defined(LIBUS_USE_EPOLL) && !defined(LIBUS_USE_LIBUV)
// todo: if not on linux, swap to libuv by default
#define LIBUS_USE_EPOLL
#endif

#ifdef __WIN32
#include <windows.h>
#define LIBUS_SOCKET_DESCRIPTOR SOCKET
#else
#define LIBUS_SOCKET_DESCRIPTOR int
#endif

#endif // LIBUSOCKETS_H
