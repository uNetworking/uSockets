#ifndef LIBUV_H
#define LIBUV_H

#include <uv.h>
#define LIBUS_SOCKET_READABLE UV_READABLE
#define LIBUS_SOCKET_WRITABLE UV_WRITABLE

struct us_loop {
    uv_loop_t *uv_loop;

    // unrefed
    uv_timer_t uv_timer;
    uv_async_t uv_async;

    // common
    struct us_socket_context *head;
    void *recv_buf;
};

struct us_poll {
    uv_poll_t uv_p;
    LIBUS_SOCKET_DESCRIPTOR fd;
    unsigned char poll_type;
};

#endif // LIBUV_H
