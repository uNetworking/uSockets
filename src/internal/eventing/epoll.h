#ifndef EPOLL_H
#define EPOLL_H

#include "internal/loop.h"

#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#define LIBUS_SOCKET_READABLE EPOLLIN
#define LIBUS_SOCKET_WRITABLE EPOLLOUT

struct us_loop {
    struct us_loop_data data;

    // epoll extensions
    int num_polls;
    int epfd;
    struct epoll_event ready_events[1024];
};

struct us_poll {
    struct {
        int fd : 28;
        unsigned int poll_type : 4;
    } state;
};

#endif // EPOLL_H
