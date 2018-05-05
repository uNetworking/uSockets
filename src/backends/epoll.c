#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

#ifdef LIBUS_USE_EPOLL

struct us_loop *us_create_loop(void (*wakeup_cb)(struct us_loop *loop)) {
    struct us_loop *loop = (struct us_loop *) malloc(sizeof(struct us_loop));
    loop->num_polls = 0;
    loop->epfd = epoll_create1(EPOLL_CLOEXEC);

    // common init
    loop->recv_buf = malloc(LIBUS_RECV_BUFFER_LENGTH);
    loop->head = 0;

    return loop;
}

void us_loop_run(struct us_loop *loop) {
    while (loop->num_polls) {

        int delay = LIBUS_TIMEOUT_GRANULARITY * 1000;

        int num_fd_ready = epoll_wait(loop->epfd, loop->ready_events, 1024, delay);

        //printf("%d ready fds\n", num_fd_ready);

        for (int i = 0; i < num_fd_ready; i++) {
            struct us_poll *poll = (struct us_poll *) loop->ready_events[i].data.ptr;
            us_dispatch_ready_poll(poll, loop->ready_events[i].events & EPOLLERR, loop->ready_events[i].events);
        }

        if (num_fd_ready == 0) {
            us_timer_sweep(loop);
        }

    }
}

struct us_poll *us_loop_create_poll(struct us_loop *loop, int poll_size) {
    loop->num_polls++;
    return malloc(poll_size);
}

void us_poll_init(struct us_poll *p, LIBUS_SOCKET_DESCRIPTOR fd, int poll_type) {
    p->state.fd = fd;
    p->state.poll_type = poll_type;
}

void us_poll_start(struct us_poll *p, struct us_loop *loop, int events) {
    struct epoll_event event;
    event.events = events;
    event.data.ptr = p;
    epoll_ctl(loop->epfd, EPOLL_CTL_ADD, p->state.fd, &event);
}

void us_poll_change(struct us_poll *p, struct us_loop *loop, int events) {
    struct epoll_event event;
    event.events = events;
    event.data.ptr = p;
    epoll_ctl(loop->epfd, EPOLL_CTL_MOD, p->state.fd, &event);
}

LIBUS_SOCKET_DESCRIPTOR us_poll_fd(struct us_poll *p) {
    return p->state.fd;
}

int us_poll_type(struct us_poll *p) {
    return p->state.poll_type;
}

void us_poll_stop(struct us_poll *p, struct us_loop *loop) {
    struct epoll_event event;
    epoll_ctl(loop->epfd, EPOLL_CTL_DEL, p->state.fd, &event);
}

#endif
