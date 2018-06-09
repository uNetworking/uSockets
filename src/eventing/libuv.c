#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

#ifdef LIBUS_USE_LIBUV

// poll dispatch
static void poll_cb(uv_poll_t *p, int status, int events) {
    us_internal_dispatch_ready_poll(p, status < 0, events);
}

static void prepare_cb(uv_prepare_t *p) {
    struct us_loop *loop = p->data;
    loop->data.pre_cb(loop);
}

/* Note: libuv timers execute AFTER the post callback */
static void check_cb(uv_check_t *p) {
    struct us_loop *loop = p->data;
    loop->data.post_cb(loop);
}

static void close_cb(uv_handle_t *h) {

}

static void timer_cb(uv_timer_t *t) {
    struct us_internal_callback *cb = t->data;
    cb->cb(cb);
}

static void async_cb(uv_async_t *a) {
    struct us_internal_callback *cb = a->data;
    // internal asyncs give their loop, not themselves
    cb->cb(cb->loop);
}

// poll
void us_poll_init(struct us_poll *p, LIBUS_SOCKET_DESCRIPTOR fd, int poll_type) {
    p->poll_type = poll_type;
    p->fd = fd;
}

void us_poll_free(struct us_poll *p) {
    uv_close(&p->uv_p, close_cb);
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

unsigned int us_internal_accept_poll_event(struct us_poll *p) {
    return 0;
}

int us_internal_poll_type(struct us_poll *p) {
    return p->poll_type;
}

LIBUS_SOCKET_DESCRIPTOR us_poll_fd(struct us_poll *p) {
    uv_os_fd_t fd;
    uv_fileno((uv_handle_t *) &p->uv_p, &fd);
    return fd;
}

// loop
struct us_loop *us_create_loop(void (*wakeup_cb)(struct us_loop *loop), void (*pre_cb)(struct us_loop *loop), void (*post_cb)(struct us_loop *loop), int userdata_size) {
    struct us_loop *loop = (struct us_loop *) malloc(sizeof(struct us_loop) + userdata_size);

    // default or not?
    loop->uv_loop = uv_loop_new();

    uv_prepare_init(loop->uv_loop, &loop->uv_pre);
    uv_prepare_start(&loop->uv_pre, prepare_cb);
    loop->uv_pre.data = loop;

    uv_check_init(loop->uv_loop, &loop->uv_check);
    uv_check_start(&loop->uv_check, check_cb);
    loop->uv_check.data = loop;

    us_internal_loop_data_init(loop, wakeup_cb, pre_cb, post_cb);
    return loop;
}

void us_loop_run(struct us_loop *loop) {
    uv_run(loop->uv_loop, UV_RUN_DEFAULT);
}

struct us_poll *us_create_poll(struct us_loop *loop, int fallthrough, int ext_size) {
    return malloc(ext_size);
}

// timer
struct us_timer *us_create_timer(struct us_loop *loop, int fallthrough, int ext_size) {
    struct us_internal_callback *cb = malloc(sizeof(struct us_internal_callback) + sizeof(uv_timer_t) + ext_size);

    cb->loop = loop;
    cb->cb_expects_the_loop = 0;

    uv_timer_t *uv_timer = (uv_timer_t *) (cb + 1);
    uv_timer_init(loop->uv_loop, uv_timer);
    uv_timer->data = cb;

    return (struct us_timer *) cb;
}

void us_timer_set(struct us_timer *t, void (*cb)(struct us_timer *t), int ms, int repeat_ms) {
    struct us_internal_callback *internal_cb = (struct us_internal_callback *) t;

    internal_cb->cb = cb;

    uv_timer_t *uv_timer = (uv_timer_t *) (internal_cb + 1);
    if (!ms) {
        uv_timer_stop(uv_timer);
    } else {
        uv_timer_start(uv_timer, timer_cb, ms, repeat_ms);
    }
}

struct us_loop *us_timer_loop(struct us_timer *t) {
    struct us_internal_callback *internal_cb = (struct us_internal_callback *) t;

    return internal_cb->loop;
}

// async (internal only)
struct us_internal_async *us_internal_create_async(struct us_loop *loop, int fallthrough, int ext_size) {
    struct us_internal_callback *cb = malloc(sizeof(struct us_internal_callback) + sizeof(uv_async_t) + ext_size);

    cb->loop = loop;
    return (struct us_internal_async *) cb;
}

void us_internal_async_set(struct us_internal_async *a, void (*cb)(struct us_internal_async *)) {
    struct us_internal_callback *internal_cb = (struct us_internal_callback *) a;

    internal_cb->cb = cb;

    uv_async_t *uv_async = (uv_async_t *) (internal_cb + 1);
    uv_async_init(internal_cb->loop->uv_loop, uv_async, async_cb);
    uv_async->data = internal_cb;
}

void us_internal_async_wakeup(struct us_internal_async *a) {
    struct us_internal_callback *internal_cb = (struct us_internal_callback *) a;

    uv_async_t *uv_async = (uv_async_t *) (internal_cb + 1);
    uv_async_send(uv_async);
}

#endif
