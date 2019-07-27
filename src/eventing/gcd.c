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

#include "libusockets.h"
#include "internal/internal.h"
#include <stdlib.h>

#ifdef LIBUS_USE_GCD

/* Loops */
struct us_loop_t *us_create_loop(void *hint, void (*wakeup_cb)(struct us_loop_t *loop), void (*pre_cb)(struct us_loop_t *loop), void (*post_cb)(struct us_loop_t *loop), unsigned int ext_size) {
    struct us_loop_t *loop = (struct us_loop_t *) malloc(sizeof(struct us_loop_t) + ext_size);

    // init the queue

    us_internal_loop_data_init(loop, wakeup_cb, pre_cb, post_cb);
    return loop;
}

void us_loop_free(struct us_loop_t *loop) {

}

void us_loop_run(struct us_loop_t *loop) {
    us_loop_integrate(loop);
    dispatch_main();
}

void gcd_read_handler(void *p) {
    us_internal_dispatch_ready_poll((struct us_poll_t *) p, 0, LIBUS_SOCKET_READABLE);
}

void gcd_write_handler(void *p) {
    us_internal_dispatch_ready_poll((struct us_poll_t *) p, 0, LIBUS_SOCKET_WRITABLE);
}

/* Polls */
void us_poll_init(struct us_poll_t *p, LIBUS_SOCKET_DESCRIPTOR fd, int poll_type) {
    p->poll_type = poll_type;
    p->fd = fd;

    p->gcd_read = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, p->fd, 0, dispatch_get_main_queue());
    dispatch_set_context(p->gcd_read, p);
    dispatch_source_set_event_handler_f(p->gcd_read, gcd_read_handler);

    p->gcd_write = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, p->fd, 0, dispatch_get_main_queue());
    dispatch_set_context(p->gcd_write, p);
    dispatch_source_set_event_handler_f(p->gcd_write, gcd_write_handler);
}

void us_poll_free(struct us_poll_t *p, struct us_loop_t *loop) {

}

void us_poll_start(struct us_poll_t *p, struct us_loop_t *loop, int events) {
    p->events = events;

    if (events & LIBUS_SOCKET_READABLE) {
        dispatch_activate(p->gcd_read);
    }

    if (events & LIBUS_SOCKET_WRITABLE) {
        dispatch_activate(p->gcd_write);
    }
}

void us_poll_change(struct us_poll_t *p, struct us_loop_t *loop, int events) {

    if (events & LIBUS_SOCKET_READABLE) {
        dispatch_resume(p->gcd_read);
    } else {
        dispatch_suspend(p->gcd_read);
    }

    if (events & LIBUS_SOCKET_WRITABLE) {
        dispatch_resume(p->gcd_write);
    } else {
        dispatch_suspend(p->gcd_write);
    }

    p->events = events;
}

void us_poll_stop(struct us_poll_t *p, struct us_loop_t *loop) {
    p->events = 0;
    dispatch_suspend(p->gcd_read);
    dispatch_suspend(p->gcd_write);
}

int us_poll_events(struct us_poll_t *p) {
    return p->events;
}

unsigned int us_internal_accept_poll_event(struct us_poll_t *p) {
    printf("us_internal_accept_poll_event\n");
    return 0;
}

int us_internal_poll_type(struct us_poll_t *p) {
    return p->poll_type & 3;
}

void us_internal_poll_set_type(struct us_poll_t *p, int poll_type) {
    p->poll_type = poll_type;
}

LIBUS_SOCKET_DESCRIPTOR us_poll_fd(struct us_poll_t *p) {
    return p->fd;
}

struct us_poll_t *us_create_poll(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    struct us_poll_t *poll = (struct us_poll_t *) malloc(sizeof(struct us_poll_t) + ext_size);

    return poll;
}

struct us_poll_t *us_poll_resize(struct us_poll_t *p, struct us_loop_t *loop, unsigned int ext_size) {
    return 0;
}

/* Timers */
void gcd_timer_handler(void *t) {
    struct us_internal_callback_t *internal_cb = (struct us_internal_callback_t *) t;

    internal_cb->cb(t);
}

struct us_timer_t *us_create_timer(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    struct us_internal_callback_t *cb = malloc(sizeof(struct us_internal_callback_t) + sizeof(dispatch_source_t) + ext_size);

    cb->loop = loop;
    cb->cb_expects_the_loop = 0;

    dispatch_source_t *gcd_timer = (dispatch_source_t *) (cb + 1);

    *gcd_timer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler_f(*gcd_timer, gcd_timer_handler);
    dispatch_set_context(*gcd_timer, cb);

    if (fallthrough) {
        //uv_unref((uv_handle_t *) uv_timer);
    }

    return (struct us_timer_t *) cb;
}

void *us_timer_ext(struct us_timer_t *timer) {
    //struct us_internal_callback_t *cb = (struct us_internal_callback_t *) timer;

    //return (cb + 1);

    return 0;
}

void us_timer_close(struct us_timer_t *t) {

}

void us_timer_set(struct us_timer_t *t, void (*cb)(struct us_timer_t *t), int ms, int repeat_ms) {
    struct us_internal_callback_t *internal_cb = (struct us_internal_callback_t *) t;

    internal_cb->cb = (void(*)(struct us_internal_callback_t *)) cb;

    dispatch_source_t *gcd_timer = (dispatch_source_t *) (internal_cb + 1);
    uint64_t nanos = (uint64_t)ms * 1000000;
    dispatch_source_set_timer(*gcd_timer, 0, nanos, 0);
    dispatch_activate(*gcd_timer);
}

struct us_loop_t *us_timer_loop(struct us_timer_t *t) {
    return 0;
}

/* Asyncs */
struct us_internal_async *us_internal_create_async(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    printf("create async\n");
    return 0;
}

void us_internal_async_close(struct us_internal_async *a) {

}

void us_internal_async_set(struct us_internal_async *a, void (*cb)(struct us_internal_async *)) {

}

void us_internal_async_wakeup(struct us_internal_async *a) {

}

#endif
