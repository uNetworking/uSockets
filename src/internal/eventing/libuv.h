#ifndef LIBUV_H
#define LIBUV_H

#include "internal/loop.h"

#include <uv.h>
#define LIBUS_SOCKET_READABLE UV_READABLE
#define LIBUS_SOCKET_WRITABLE UV_WRITABLE

struct us_loop {
    struct us_loop_data data;

    uv_loop_t *uv_loop;

    uv_prepare_t uv_pre;
    uv_check_t uv_check;
};

struct us_poll {
    uv_poll_t uv_p;
    LIBUS_SOCKET_DESCRIPTOR fd;
    unsigned char poll_type;
};

#endif // LIBUV_H
