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

#if defined(LIBUS_USE_EPOLL) || defined(LIBUS_USE_KQUEUE)

#ifdef LIBUS_USE_EPOLL
#define GET_READY_POLL(loop, index) (struct us_poll_t *) loop->ready_events[index].data.ptr
#define SET_READY_POLL(loop, index, poll) loop->ready_events[index].data.ptr = poll
#else
#define GET_READY_POLL(loop, index) (struct us_poll_t *) loop->ready_events[index].udata
#define SET_READY_POLL(loop, index, poll) loop->ready_events[index].udata = poll
#endif

/* Loop */
void us_loop_free(struct us_loop_t *loop) {
    us_internal_loop_data_free(loop);
#ifdef LIBUS_USE_EPOLL
    close(loop->epfd);
#else
    close(loop->kqfd);
#endif
    free(loop);
}

/* Poll */
struct us_poll_t *us_create_poll(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    if (!fallthrough) {
        loop->num_polls++;
    }
    return malloc(sizeof(struct us_poll_t) + ext_size);
}

/* Todo: this one should be us_internal_poll_free */
void us_poll_free(struct us_poll_t *p, struct us_loop_t *loop) {
    loop->num_polls--;
    free(p);
}

void *us_poll_ext(struct us_poll_t *p) {
    return p + 1;
}

/* Todo: why have us_poll_create AND us_poll_init!? libuv legacy! */
void us_poll_init(struct us_poll_t *p, LIBUS_SOCKET_DESCRIPTOR fd, int poll_type) {
    p->state.fd = fd;
    p->state.poll_type = poll_type;
}

int us_poll_events(struct us_poll_t *p) {
    return ((p->state.poll_type & POLL_TYPE_POLLING_IN) ? LIBUS_SOCKET_READABLE : 0) | ((p->state.poll_type & POLL_TYPE_POLLING_OUT) ? LIBUS_SOCKET_WRITABLE : 0);
}

LIBUS_SOCKET_DESCRIPTOR us_poll_fd(struct us_poll_t *p) {
    return p->state.fd;
}

/* Returns any of listen socket, socket, shut down socket or callback */
int us_internal_poll_type(struct us_poll_t *p) {
    return p->state.poll_type & 3;
}

/* Bug: doesn't really SET, rather read and change, so needs to be inited first! */
void us_internal_poll_set_type(struct us_poll_t *p, int poll_type) {
    p->state.poll_type = poll_type | (p->state.poll_type & 12);
}

/* Timer */
void *us_timer_ext(struct us_timer_t *timer) {
    return ((struct us_internal_callback_t *) timer) + 1;
}

struct us_loop_t *us_timer_loop(struct us_timer_t *t) {
    struct us_internal_callback_t *internal_cb = (struct us_internal_callback_t *) t;

    return internal_cb->loop;
}

/* Loop */
struct us_loop_t *us_create_loop(void *hint, void (*wakeup_cb)(struct us_loop_t *loop), void (*pre_cb)(struct us_loop_t *loop), void (*post_cb)(struct us_loop_t *loop), unsigned int ext_size) {
    struct us_loop_t *loop = (struct us_loop_t *) malloc(sizeof(struct us_loop_t) + ext_size);
    loop->num_polls = 0;

#ifdef LIBUS_USE_EPOLL
    loop->epfd = epoll_create1(EPOLL_CLOEXEC);
#else
    loop->kqfd = kqueue();
#endif

    us_internal_loop_data_init(loop, wakeup_cb, pre_cb, post_cb);
    return loop;
}

void us_loop_run(struct us_loop_t *loop) {
    us_loop_integrate(loop);

    while (loop->num_polls) {
        us_internal_loop_pre(loop);

#ifdef LIBUS_USE_EPOLL
        loop->num_fd_ready = epoll_wait(loop->epfd, loop->ready_events, 1024, -1);
#else
        loop->num_fd_ready = kevent(loop->kqfd, NULL, 0, loop->ready_events, 1024, NULL);
#endif

        for (loop->fd_iterator = 0; loop->fd_iterator < loop->num_fd_ready; loop->fd_iterator++) {
            struct us_poll_t *poll = GET_READY_POLL(loop, loop->fd_iterator);
            /* Any ready poll marked with nullptr will be ignored */
            if (poll) {
#ifdef LIBUS_USE_EPOLL
                us_internal_dispatch_ready_poll(poll, loop->ready_events[loop->fd_iterator].events & (EPOLLERR | EPOLLHUP), loop->ready_events[loop->fd_iterator].events);
#else
                us_internal_dispatch_ready_poll(poll, loop->ready_events[loop->fd_iterator].flags & (EV_EOF), -loop->ready_events[loop->fd_iterator].flags);
#endif
            }
        }
        us_internal_loop_post(loop);
    }
}

/* Poll */

struct us_poll_t *us_poll_resize(struct us_poll_t *p, struct us_loop_t *loop, unsigned int ext_size) {
    int events = us_poll_events(p);

    struct us_poll_t *new_p = realloc(p, sizeof(struct us_poll_t) + ext_size);
    if (p != new_p && events) {
#ifdef LIBUS_USE_EPOLL
        // forcefully update poll by stripping away already set events
        new_p->state.poll_type = us_internal_poll_type(new_p);
        us_poll_change(new_p, loop, events);
#else
        // NOT FOR KQUEUE! forcefully update poll by stripping away already set events
        // new_p->state.poll_type = us_internal_poll_type(new_p);
        us_poll_change(p, loop, /*events*/ 0);
        us_poll_change(new_p, loop, events);
#endif

    }

    if (GET_READY_POLL(loop, loop->fd_iterator) != p) {
        for (int i = loop->fd_iterator; i < loop->num_fd_ready; i++) {
            if (GET_READY_POLL(loop, i) == p) {
                SET_READY_POLL(loop, i, new_p);
                break;
            }
        }
    }

    return new_p;
}

void us_poll_start(struct us_poll_t *p, struct us_loop_t *loop, int events) {
    p->state.poll_type = us_internal_poll_type(p) | ((events & LIBUS_SOCKET_READABLE) ? POLL_TYPE_POLLING_IN : 0) | ((events & LIBUS_SOCKET_WRITABLE) ? POLL_TYPE_POLLING_OUT : 0);

#ifdef LIBUS_USE_EPOLL
    struct epoll_event event;
    event.events = events;
    event.data.ptr = p;
    epoll_ctl(loop->epfd, EPOLL_CTL_ADD, p->state.fd, &event);
#else
    struct kevent event;
    EV_SET(&event, p->state.fd, -events, EV_ADD, 0, 0, p);
    int ret = kevent(loop->kqfd, &event, 1, NULL, 0, NULL);
#endif
}

void us_poll_change(struct us_poll_t *p, struct us_loop_t *loop, int events) {

    int old_events = us_poll_events(p);
    if (old_events != events) {

        p->state.poll_type = us_internal_poll_type(p) | ((events & LIBUS_SOCKET_READABLE) ? POLL_TYPE_POLLING_IN : 0) | ((events & LIBUS_SOCKET_WRITABLE) ? POLL_TYPE_POLLING_OUT : 0);

#ifdef LIBUS_USE_EPOLL
        struct epoll_event event;
        event.events = events;
        event.data.ptr = p;
        epoll_ctl(loop->epfd, EPOLL_CTL_MOD, p->state.fd, &event);
#else
        struct kevent event[2];
        int event_length = 0;

        /* Update read if changed */
        if ((old_events & LIBUS_SOCKET_READABLE) != (events & LIBUS_SOCKET_READABLE)) {
            EV_SET(&event[event_length++], p->state.fd, -LIBUS_SOCKET_READABLE, (events & LIBUS_SOCKET_READABLE) ? EV_ADD : EV_DELETE, 0, 0, p);
        }

        /* Update write if changed */
        if ((old_events & LIBUS_SOCKET_WRITABLE) != (events & LIBUS_SOCKET_WRITABLE)) {
            EV_SET(&event[event_length++], p->state.fd, -LIBUS_SOCKET_WRITABLE, (events & LIBUS_SOCKET_WRITABLE) ? EV_ADD : EV_DELETE, 0, 0, p);
        }

        kevent(loop->kqfd, event, event_length, NULL, 0, NULL);
#endif

        /* We are not allowed to emit old events we no longer subscribe to,
         * so for simplicity we simply disable this poll's entry in the ready polls
         * vector if we are not the currently dispatched poll. In other words we postpone
         * any events until the next iteration to make sure they are entirely correct */
        if (GET_READY_POLL(loop, loop->fd_iterator) != p) {
            for (int i = loop->fd_iterator; i < loop->num_fd_ready; i++) {
                if (GET_READY_POLL(loop, i) == p) {
                    /* Mark us disabled for this iteration, just like we do for us_poll_stop */
                    SET_READY_POLL(loop, i, 0);
                    break;
                }
            }
        }
    }
}

void us_poll_stop(struct us_poll_t *p, struct us_loop_t *loop) {
#ifdef LIBUS_USE_EPOLL
    struct epoll_event event;
    epoll_ctl(loop->epfd, EPOLL_CTL_DEL, p->state.fd, &event);
#else
    int old_events = us_poll_events(p);
    if (old_events) {
        struct kevent event;
        EV_SET(&event, p->state.fd, -old_events, EV_DELETE, 0, 0, p);
        kevent(loop->kqfd, &event, 1, NULL, 0, NULL);
    }
#endif

    /* If we are not currently the one dispatched poll, we need
    to see if we are in the ready_polls vector and disable us if so */
    if (GET_READY_POLL(loop, loop->fd_iterator) != p) {
        for (int i = loop->fd_iterator; i < loop->num_fd_ready; i++) {
            if (GET_READY_POLL(loop, i) == p) {
                /* Mark us as disabled */
                SET_READY_POLL(loop, i, 0);
                break;
            }
        }
    }
}

unsigned int us_internal_accept_poll_event(struct us_poll_t *p) {
#ifdef LIBUS_USE_EPOLL
    int fd = us_poll_fd(p);
    uint64_t buf;
    int read_length = read(fd, &buf, 8);
    return buf;
#else
    /* Kqueue has no underlying FD for timers */
    return 0;
#endif
}

/* Timer */
#ifdef LIBUS_USE_EPOLL
struct us_timer_t *us_create_timer(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    struct us_poll_t *p = us_create_poll(loop, fallthrough, sizeof(struct us_internal_callback_t) + ext_size);
    us_poll_init(p, timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC), POLL_TYPE_CALLBACK);

    struct us_internal_callback_t *cb = (struct us_internal_callback_t *) p;
    cb->loop = loop;
    cb->cb_expects_the_loop = 0;

    return (struct us_timer_t *) cb;
}
#else
struct us_timer_t *us_create_timer(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    struct us_internal_callback_t *cb = malloc(sizeof(struct us_internal_callback_t) + ext_size);

    cb->loop = loop;
    cb->cb_expects_the_loop = 0;

    /* Bug: us_internal_poll_set_type does not SET the type, it only CHANGES it */
    cb->p.state.poll_type = 0;
    us_internal_poll_set_type((struct us_poll_t *) cb, POLL_TYPE_CALLBACK);

    if (!fallthrough) {
        loop->num_polls++;
    }

    return (struct us_timer_t *) cb;
}
#endif

#ifdef LIBUS_USE_EPOLL
void us_timer_close(struct us_timer_t *timer) {
    struct us_internal_callback_t *cb = (struct us_internal_callback_t *) timer;

    us_poll_stop(&cb->p, cb->loop);
    close(us_poll_fd(&cb->p));

    /* (regular) sockets are the only polls which are not freed immediately */
    us_poll_free((struct us_poll_t *) timer, cb->loop);
}

void us_timer_set(struct us_timer_t *t, void (*cb)(struct us_timer_t *t), int ms, int repeat_ms) {
    struct us_internal_callback_t *internal_cb = (struct us_internal_callback_t *) t;

    internal_cb->cb = (void (*)(struct us_internal_callback_t *)) cb;

    struct itimerspec timer_spec = {
        {repeat_ms / 1000, ((long)repeat_ms * 1000000) % 1000000000},
        {ms / 1000, ((long)ms * 1000000) % 1000000000}
    };

    timerfd_settime(us_poll_fd((struct us_poll_t *) t), 0, &timer_spec, NULL);
    us_poll_start((struct us_poll_t *) t, internal_cb->loop, LIBUS_SOCKET_READABLE);
}
#else
void us_timer_close(struct us_timer_t *timer) {
    struct us_internal_callback_t *internal_cb = (struct us_internal_callback_t *) timer;

    struct kevent event;
    EV_SET(&event, (uintptr_t) internal_cb, EVFILT_TIMER, EV_DELETE, 0, 0, internal_cb);
    kevent(internal_cb->loop->kqfd, &event, 1, NULL, 0, NULL);

    /* (regular) sockets are the only polls which are not freed immediately */
    us_poll_free((struct us_poll_t *) timer, internal_cb->loop);
}

void us_timer_set(struct us_timer_t *t, void (*cb)(struct us_timer_t *t), int ms, int repeat_ms) {
    struct us_internal_callback_t *internal_cb = (struct us_internal_callback_t *) t;

    internal_cb->cb = (void (*)(struct us_internal_callback_t *)) cb;

    /* Bug: repeat_ms must be the same as ms, or 0 */
    struct kevent event;
    EV_SET(&event, (uintptr_t) internal_cb, EVFILT_TIMER | (repeat_ms ? 0 : EV_ONESHOT), EV_ADD, 0, ms, internal_cb);
    kevent(internal_cb->loop->kqfd, &event, 1, NULL, 0, NULL);
}
#endif

/* Async (internal helper for loop's wakeup feature) */
#ifdef LIBUS_USE_EPOLL
struct us_internal_async *us_internal_create_async(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    struct us_poll_t *p = us_create_poll(loop, fallthrough, sizeof(struct us_internal_callback_t) + ext_size);
    us_poll_init(p, eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC), POLL_TYPE_CALLBACK);

    struct us_internal_callback_t *cb = (struct us_internal_callback_t *) p;
    cb->loop = loop;
    cb->cb_expects_the_loop = 1;

    return (struct us_internal_async *) cb;
}

// identical code as for timer, make it shared for "callback types"
void us_internal_async_close(struct us_internal_async *a) {
    struct us_internal_callback_t *cb = (struct us_internal_callback_t *) a;

    us_poll_stop(&cb->p, cb->loop);
    close(us_poll_fd(&cb->p));

    /* (regular) sockets are the only polls which are not freed immediately */
    us_poll_free((struct us_poll_t *) a, cb->loop);
}

void us_internal_async_set(struct us_internal_async *a, void (*cb)(struct us_internal_async *)) {
    struct us_internal_callback_t *internal_cb = (struct us_internal_callback_t *) a;

    internal_cb->cb = (void (*)(struct us_internal_callback_t *)) cb;

    us_poll_start((struct us_poll_t *) a, internal_cb->loop, LIBUS_SOCKET_READABLE);
}

void us_internal_async_wakeup(struct us_internal_async *a) {
    uint64_t one = 1;
    int written = write(us_poll_fd((struct us_poll_t *) a), &one, 8);
}
#else
struct us_internal_async *us_internal_create_async(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    struct us_internal_callback_t *cb = malloc(sizeof(struct us_internal_callback_t) + ext_size);

    cb->loop = loop;
    cb->cb_expects_the_loop = 1;

    /* Bug: us_internal_poll_set_type does not SET the type, it only CHANGES it */
    cb->p.state.poll_type = 0;
    us_internal_poll_set_type((struct us_poll_t *) cb, POLL_TYPE_CALLBACK);

    if (!fallthrough) {
        loop->num_polls++;
    }

    return (struct us_internal_async *) cb;
}

// identical code as for timer, make it shared for "callback types"
void us_internal_async_close(struct us_internal_async *a) {
    struct us_internal_callback_t *internal_cb = (struct us_internal_callback_t *) a;

    /* (regular) sockets are the only polls which are not freed immediately */
    us_poll_free((struct us_poll_t *) a, internal_cb->loop);
}

void us_internal_async_set(struct us_internal_async *a, void (*cb)(struct us_internal_async *)) {
    struct us_internal_callback_t *internal_cb = (struct us_internal_callback_t *) a;

    internal_cb->cb = (void (*)(struct us_internal_callback_t *)) cb;
}

void us_internal_async_wakeup(struct us_internal_async *a) {
    struct us_internal_callback_t *internal_cb = (struct us_internal_callback_t *) a;

    /* In kqueue you really only need to add a triggered oneshot event */
    struct kevent event;
    EV_SET(&event, (uintptr_t) internal_cb, EVFILT_USER, EV_ADD | EV_ONESHOT, NOTE_TRIGGER, 0, internal_cb);
    kevent(internal_cb->loop->kqfd, &event, 1, NULL, 0, NULL);
}
#endif

#endif
