#ifndef EPOLL_H
#define EPOLL_H

#include <sys/epoll.h>
#define LIBUS_SOCKET_READABLE EPOLLIN
#define LIBUS_SOCKET_WRITABLE EPOLLOUT

struct us_loop {
    int num_polls;
    int epfd;
    int delay;
    struct epoll_event ready_events[1024];

    // common
    struct us_socket_context *head;
    void *recv_buf;
};

struct us_poll {
    struct {
        int fd : 28;
        unsigned int poll_type : 4;
    } state;
};

#endif // EPOLL_H
