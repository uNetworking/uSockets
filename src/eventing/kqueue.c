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
#include "internal/common.h"
#include <stdlib.h>

#ifdef LIBUS_USE_KQUEUE

// loop
struct us_loop_t *us_create_loop(void *hint, void (*wakeup_cb)(struct us_loop_t *loop), void (*pre_cb)(struct us_loop_t *loop), void (*post_cb)(struct us_loop_t *loop), unsigned int ext_size) {
    struct us_loop_t *loop = (struct us_loop_t *) malloc(sizeof(struct us_loop_t) + ext_size);
    loop->num_polls = 0;
    loop->kqfd = kqueue();

    us_internal_loop_data_init(loop, wakeup_cb, pre_cb, post_cb);
    return loop;
}

void us_loop_free(struct us_loop_t *loop) {
    us_internal_loop_data_free(loop);
    close(loop->kqfd);
    free(loop);
}



void us_loop_run(struct us_loop_t *loop) {
    us_loop_integrate(loop);

    while (loop->num_polls) {
        us_internal_loop_pre(loop);

        loop->num_fd_ready = kevent(loop->kqfd, NULL, 0, loop->ready_events, 1024, NULL);

        for (loop->fd_iterator = 0; loop->fd_iterator < loop->num_fd_ready; loop->fd_iterator++) {
            struct us_poll_t *poll = (struct us_poll_t *) loop->ready_events[loop->fd_iterator].udata;
            us_internal_dispatch_ready_poll(poll, loop->ready_events[loop->fd_iterator].flags & (EV_EOF), loop->ready_events[loop->fd_iterator].flags);
        }
        us_internal_loop_post(loop);
    }
}

// poll
struct us_poll_t *us_create_poll(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    if (!fallthrough) {
        loop->num_polls++;
    }
    return malloc(sizeof(struct us_poll_t) + ext_size);
}

struct us_poll_t *us_poll_resize(struct us_poll_t *p, struct us_loop_t *loop, unsigned int ext_size) {
    return p;
    
    /*int events = us_poll_events(p);

    struct us_poll_t *new_p = realloc(p, sizeof(struct us_poll_t) + ext_size);
    if (p != new_p && events) {
        // forcefully update poll by stripping away already set events
        new_p->state.poll_type = us_internal_poll_type(new_p);
        us_poll_change(new_p, loop, events);
    }

    if (loop->ready_events[loop->fd_iterator].data.ptr != p) {
        for (int i = loop->fd_iterator; i < loop->num_fd_ready; i++) {
            if (loop->ready_events[i].data.ptr == p) {
                loop->ready_events[i].data.ptr = new_p;
                break;
            }
        }
    }

    return new_p;*/
}

void us_poll_free(struct us_poll_t *p, struct us_loop_t *loop) {
    loop->num_polls--;
    free(p);
}

void *us_poll_ext(struct us_poll_t *p) {
    return p + 1;
}

void us_poll_init(struct us_poll_t *p, LIBUS_SOCKET_DESCRIPTOR fd, int poll_type) {
    p->state.fd = fd;
    p->state.poll_type = poll_type;
}

int us_poll_events(struct us_poll_t *p) {
    return ((p->state.poll_type & POLL_TYPE_POLLING_IN) ? LIBUS_SOCKET_READABLE : 0) | ((p->state.poll_type & POLL_TYPE_POLLING_OUT) ? LIBUS_SOCKET_WRITABLE : 0);
}

void us_poll_start(struct us_poll_t *p, struct us_loop_t *loop, int events) {
    p->state.poll_type = us_internal_poll_type(p) | ((events & LIBUS_SOCKET_READABLE) ? POLL_TYPE_POLLING_IN : 0) | ((events & LIBUS_SOCKET_WRITABLE) ? POLL_TYPE_POLLING_OUT : 0);

    struct kevent event;
    EV_SET(&event, p->state.fd, events, EV_ADD, 0, 0, p);
    int ret = kevent(loop->kqfd, &event, 1, NULL, 0, NULL);

    printf("us_poll_start: %d\n", ret);
}

void us_poll_change(struct us_poll_t *p, struct us_loop_t *loop, int events) {
    if (us_poll_events(p) != events) {

        p->state.poll_type = us_internal_poll_type(p) | ((events & LIBUS_SOCKET_READABLE) ? POLL_TYPE_POLLING_IN : 0) | ((events & LIBUS_SOCKET_WRITABLE) ? POLL_TYPE_POLLING_OUT : 0);

        struct kevent event;
        EV_SET(&event, p->state.fd, events, EV_ADD, 0, 0, p);
        int ret = kevent(loop->kqfd, &event, 1, NULL, 0, NULL);

        printf("us_poll_change: %d\n", ret);
    }
}

LIBUS_SOCKET_DESCRIPTOR us_poll_fd(struct us_poll_t *p) {
    return p->state.fd;
}

/* Returns any of listen socket, socket, shut down socket or callback */
int us_internal_poll_type(struct us_poll_t *p) {
    return p->state.poll_type & 3;
}

void us_internal_poll_set_type(struct us_poll_t *p, int poll_type) {
    p->state.poll_type = poll_type | (p->state.poll_type & 12);
}

void us_poll_stop(struct us_poll_t *p, struct us_loop_t *loop) {
    struct kevent event;
    EV_SET(&event, p->state.fd, 0, EV_DELETE, 0, 0, p);
    int ret = kevent(loop->kqfd, &event, 1, NULL, 0, NULL);

    printf("us_poll_stop: %d\n", ret);
}

unsigned int us_internal_accept_poll_event(struct us_poll_t *p) {
    int fd = us_poll_fd(p);
    uint64_t buf;
    int read_length = read(fd, &buf, 8);
    return buf;
}

// timer
struct us_timer_t *us_create_timer(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    /*struct us_poll_t *p = us_create_poll(loop, fallthrough, sizeof(struct us_internal_callback) + ext_size);
    us_poll_init(p, timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC), POLL_TYPE_CALLBACK);

    struct us_internal_callback *cb = (struct us_internal_callback *) p;
    cb->loop = loop;
    cb->cb_expects_the_loop = 0;

    return (struct us_timer_t *) cb;*/

    return 0;
}

void *us_timer_ext(struct us_timer_t *timer) {
    return ((struct us_internal_callback *) timer) + 1;
}

void us_timer_close(struct us_timer_t *timer) {
    struct us_internal_callback *cb = (struct us_internal_callback *) timer;

    us_poll_stop(&cb->p, cb->loop);
    close(us_poll_fd(&cb->p));

    /* (regular) sockets are the only polls which are not freed immediately */
    us_poll_free((struct us_poll_t *) timer, cb->loop);
}

void us_timer_set(struct us_timer_t *t, void (*cb)(struct us_timer_t *t), int ms, int repeat_ms) {
    /*struct us_internal_callback *internal_cb = (struct us_internal_callback *) t;

    internal_cb->cb = (void (*)(struct us_internal_callback *)) cb;

    struct itimerspec timer_spec = {
        {repeat_ms / 1000, ((long)repeat_ms * 1000000) % 1000000000},
        {ms / 1000, ((long)ms * 1000000) % 1000000000}
    };

    timerfd_settime(us_poll_fd((struct us_poll_t *) t), 0, &timer_spec, NULL);
    us_poll_start((struct us_poll_t *) t, internal_cb->loop, LIBUS_SOCKET_READABLE);*/


}

struct us_loop_t *us_timer_loop(struct us_timer_t *t) {
    struct us_internal_callback *internal_cb = (struct us_internal_callback *) t;

    return internal_cb->loop;
}

// async (internal only)
struct us_internal_async *us_internal_create_async(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    /*struct us_poll_t *p = us_create_poll(loop, fallthrough, sizeof(struct us_internal_callback) + ext_size);
    us_poll_init(p, eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC), POLL_TYPE_CALLBACK);

    struct us_internal_callback *cb = (struct us_internal_callback *) p;
    cb->loop = loop;
    cb->cb_expects_the_loop = 1;

    return (struct us_internal_async *) cb;*/

    return 0;
}

// identical code as for timer, make it shared for "callback types"
void us_internal_async_close(struct us_internal_async *a) {
    /*struct us_internal_callback *cb = (struct us_internal_callback *) a;

    us_poll_stop(&cb->p, cb->loop);
    close(us_poll_fd(&cb->p));

    /* (regular) sockets are the only polls which are not freed immediately */
    //us_poll_free((struct us_poll_t *) a, cb->loop);
}

void us_internal_async_set(struct us_internal_async *a, void (*cb)(struct us_internal_async *)) {
    /*struct us_internal_callback *internal_cb = (struct us_internal_callback *) a;

    internal_cb->cb = (void (*)(struct us_internal_callback *)) cb;

    us_poll_start((struct us_poll_t *) a, internal_cb->loop, LIBUS_SOCKET_READABLE);*/
}

void us_internal_async_wakeup(struct us_internal_async *a) {
    /*uint64_t one = 1;
    int written = write(us_poll_fd((struct us_poll_t *) a), &one, 8);*/
}

#endif
