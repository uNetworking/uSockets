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

#ifndef COMMON_H
#define COMMON_H

#include "internal/networking/bsd.h"

#ifdef LIBUS_USE_EPOLL
#include "internal/eventing/epoll.h"
#else
#include "internal/eventing/libuv.h"
#endif

enum {
    /* Two first bits */
    POLL_TYPE_SOCKET = 0,
    POLL_TYPE_SOCKET_SHUT_DOWN = 1,
    POLL_TYPE_SEMI_SOCKET = 2,
    POLL_TYPE_CALLBACK = 3,

    /* Two last bits */
    POLL_TYPE_POLLING_OUT = 4,
    POLL_TYPE_POLLING_IN = 8
};

int us_internal_poll_type(struct us_poll *p);
void us_internal_poll_set_type(struct us_poll *p, int poll_type);
void us_internal_init_socket(struct us_socket *s);

void us_internal_dispatch_ready_poll(struct us_poll *p, int error, int events);
void us_internal_timer_sweep(struct us_loop *loop);
void us_internal_free_closed_sockets(struct us_loop *loop);
unsigned int us_internal_accept_poll_event(struct us_poll *p);

void us_internal_init_loop_ssl_data(struct us_loop *loop);
void us_internal_free_loop_ssl_data(struct us_loop *loop);

void us_internal_socket_context_link(struct us_socket_context *context, struct us_socket *s);
void us_internal_socket_context_unlink(struct us_socket_context *context, struct us_socket *s);

void us_internal_loop_link(struct us_loop *loop, struct us_socket_context *context);

struct us_socket {
    alignas(LIBUS_EXT_ALIGNMENT) struct us_poll p;
    struct us_socket_context *context;
    struct us_socket *prev, *next;
    unsigned short timeout;
};

struct us_listen_socket {
    alignas(LIBUS_EXT_ALIGNMENT) struct us_socket s;
    unsigned int socket_ext_size;
};

struct us_socket_context {
    alignas(LIBUS_EXT_ALIGNMENT) struct us_loop *loop;
    //unsigned short timeout;
    struct us_socket *head;
    struct us_socket *iterator;
    struct us_socket_context *prev, *next;

    struct us_socket *(*on_open)(struct us_socket *, int is_client);
    struct us_socket *(*on_data)(struct us_socket *, char *data, int length);
    struct us_socket *(*on_writable)(struct us_socket *);
    struct us_socket *(*on_close)(struct us_socket *);
    //void (*on_timeout)(struct us_socket_context *);
    struct us_socket *(*on_socket_timeout)(struct us_socket *);
    struct us_socket *(*on_end)(struct us_socket *);
    int (*ignore_data)(struct us_socket *);
};

struct us_internal_callback {
    struct us_poll p;
    struct us_loop *loop;
    int cb_expects_the_loop;
    void (*cb)(struct us_internal_callback *cb);
};

void sweep_timer_cb(struct us_internal_callback *cb);

#endif // COMMON_H
