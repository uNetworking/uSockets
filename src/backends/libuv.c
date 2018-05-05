#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

#ifdef LIBUS_USE_LIBUV

// poll dispatch
void poll_cb(uv_poll_t *p, int status, int events) {
    us_dispatch_ready_poll(p, status < 0, events);
}

void timer_cb(uv_timer_t *t) {

    struct us_loop *loop = t->data;

    us_timer_sweep(loop);
}

// poll
void us_poll_init(struct us_poll *p, LIBUS_SOCKET_DESCRIPTOR fd, int poll_type) {
    p->poll_type = poll_type;
    p->fd = fd;
}

void us_poll_start(struct us_poll *p, struct us_loop *loop, int events) {
    uv_poll_init(loop->uv_loop, &p->uv_p, p->fd);
    uv_poll_start(&p->uv_p, events, poll_cb);
}

void us_poll_change(struct us_poll *p, struct us_loop *loop, int events) {
    uv_poll_start(&p->uv_p, events, poll_cb);
}

void us_poll_stop(struct us_poll *p, struct us_loop *loop) {
    uv_poll_stop(&p->uv_p);
}

int us_poll_type(struct us_poll *p) {
    return p->poll_type;
}

LIBUS_SOCKET_DESCRIPTOR us_poll_fd(struct us_poll *p) {
    uv_os_fd_t fd;
    uv_fileno((uv_handle_t *) &p->uv_p, &fd);
    return fd;
}

// loop
struct us_loop *us_create_loop(void (*wakeup_cb)(struct us_loop *loop)) {
    struct us_loop *loop = (struct us_loop *) malloc(sizeof(struct us_loop));

    // common init!
    loop->recv_buf = malloc(LIBUS_RECV_BUFFER_LENGTH);
    loop->head = 0;

    // default or not?
    loop->uv_loop = uv_loop_new();

    // default timer and async
    uv_timer_init(loop->uv_loop, &loop->uv_timer);
    uv_timer_start(&loop->uv_timer, timer_cb, LIBUS_TIMEOUT_GRANULARITY * 1000, LIBUS_TIMEOUT_GRANULARITY * 1000);
    loop->uv_timer.data = loop;

    return loop;
}

void us_loop_run(struct us_loop *loop) {
    uv_run(loop->uv_loop, UV_RUN_DEFAULT);
}

struct us_poll *us_loop_create_poll(struct us_loop *loop, int poll_size) {
    return malloc(poll_size);
}

#endif
