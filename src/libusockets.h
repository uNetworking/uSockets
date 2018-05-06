#ifndef LIBUSOCKETS_H
#define LIBUSOCKETS_H

#ifdef __cplusplus
extern "C" {
#endif

enum {
    OPTION_REUSE_PORT
};

enum {
    POLL_TYPE_SOCKET,
    POLL_TYPE_LISTEN_SOCKET,
    POLL_TYPE_ASYNC
};

#define LIBUS_SOCKET_DESCRIPTOR int
#define LIBUS_RECV_BUFFER_LENGTH (300 * 2014)
#define LIBUS_TIMEOUT_GRANULARITY 3

#if defined(LIBUS_USE_EPOLL) || !defined(LIBUS_USE_LIBUV)
#define LIBUS_USE_EPOLL

struct us_loop;

struct us_poll {
    struct {
        int fd : 28;
        unsigned int poll_type : 4;
    } state;
};

struct us_timer {

};
#elif defined(LIBUS_USE_LIBUV)
#include <uv.h>

struct us_loop;

struct us_poll {
    uv_poll_t uv_p;
    LIBUS_SOCKET_DESCRIPTOR fd;
    unsigned char poll_type;
};

struct us_timer {

};
#elif defined(LIBUS_USE_ASIO)
#include "asio.h" // good luck, has to be C++
#endif

struct us_socket;

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

struct us_socket {
    struct us_poll p;
    struct us_socket_context *context;
    struct us_socket *prev, *next;
    unsigned short timeout;
};

// user gets this to cancel listen with
struct us_listen_socket {
    struct us_socket s;
    int socket_size;
    void *user_data;
};

// void us_listen_socket_stop(..

struct us_loop *us_create_loop(void (*wakeup_cb)(struct us_loop *loop), int userdata_size);
void *us_loop_userdata(struct us_loop *loop);
void us_loop_run(struct us_loop *loop);

// per context
struct us_socket_context *us_create_socket_context(struct us_loop *loop, int context_size);
struct us_listen_socket *us_socket_context_listen(struct us_socket_context *context, const char *host, int port, int options, int socket_size);
void us_context_connect(const char *host, int port, int options, int ext_size, void (*cb)(struct us_socket *), void *user_data);

void us_socket_context_link(struct us_socket_context *context, struct us_socket *s);

// per socket
int us_socket_write(struct us_socket *s, const char *data, int length);
void us_socket_timeout(struct us_socket *s, unsigned int seconds);

void *us_socket_ext(struct us_socket *s);
int us_socket_type(struct us_socket *s);

#ifdef __cplusplus
}
#endif

#endif // LIBUSOCKETS_H
