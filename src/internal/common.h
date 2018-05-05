#ifndef COMMON_H
#define COMMON_H

#ifdef LIBUS_USE_EPOLL
#include "internal/epoll.h"
#else
#include "internal/libuv.h"
#endif

#include "internal/bsd.h"

void us_dispatch_ready_poll(struct us_poll *p, int error, int events);
void us_timer_sweep(struct us_loop *loop);

// private methods for controlling polls
struct us_poll *us_loop_create_poll(struct us_loop *loop, int poll_size);
void us_poll_init(struct us_poll *p, LIBUS_SOCKET_DESCRIPTOR fd, int poll_type);
void us_poll_start(struct us_poll *p, struct us_loop *loop, int events);
void us_poll_change(struct us_poll *p, struct us_loop *loop, int events);
void us_poll_stop(struct us_poll *p, struct us_loop *loop);
int us_poll_type(struct us_poll *p);
LIBUS_SOCKET_DESCRIPTOR us_poll_fd(struct us_poll *p);

// per socket
int us_socket_fd(struct us_socket *s);

#endif // COMMON_H
