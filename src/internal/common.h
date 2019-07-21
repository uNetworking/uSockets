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
#endif
#ifdef LIBUS_USE_LIBUV
#include "internal/eventing/libuv.h"
#endif
#ifdef LIBUS_USE_GCD
#include "internal/eventing/gcd.h"
#endif

#include "internal/ssl.h"

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

int us_internal_poll_type(struct us_poll_t *p);
void us_internal_poll_set_type(struct us_poll_t *p, int poll_type);
void us_internal_init_socket(struct us_socket_t *s);

void us_internal_dispatch_ready_poll(struct us_poll_t *p, int error, int events);
void us_internal_timer_sweep(struct us_loop_t *loop);
void us_internal_free_closed_sockets(struct us_loop_t *loop);
unsigned int us_internal_accept_poll_event(struct us_poll_t *p);

void us_internal_init_loop_ssl_data(struct us_loop_t *loop);
void us_internal_free_loop_ssl_data(struct us_loop_t *loop);

void us_internal_socket_context_link(struct us_socket_context_t *context, struct us_socket_t *s);
void us_internal_socket_context_unlink(struct us_socket_context_t *context, struct us_socket_t *s);

void us_internal_loop_link(struct us_loop_t *loop, struct us_socket_context_t *context);

struct us_socket_t {
    alignas(LIBUS_EXT_ALIGNMENT) struct us_poll_t p;
    struct us_socket_context_t *context;
    struct us_socket_t *prev, *next;
    unsigned short timeout;
};

struct us_listen_socket_t {
    alignas(LIBUS_EXT_ALIGNMENT) struct us_socket_t s;
    unsigned int socket_ext_size;
};

struct us_socket_context_t {
    alignas(LIBUS_EXT_ALIGNMENT) struct us_loop_t *loop;
    //unsigned short timeout;
    struct us_socket_t *head;
    struct us_socket_t *iterator;
    struct us_socket_context_t *prev, *next;

    struct us_socket_t *(*on_open)(struct us_socket_t *, int is_client, char *ip, int ip_length);
    struct us_socket_t *(*on_data)(struct us_socket_t *, char *data, int length);
    struct us_socket_t *(*on_writable)(struct us_socket_t *);
    struct us_socket_t *(*on_close)(struct us_socket_t *);
    //void (*on_timeout)(struct us_socket_context *);
    struct us_socket_t *(*on_socket_timeout)(struct us_socket_t *);
    struct us_socket_t *(*on_end)(struct us_socket_t *);
    int (*ignore_data)(struct us_socket_t *);
};

struct us_internal_callback {
    struct us_poll_t p;
    struct us_loop_t *loop;
    int cb_expects_the_loop;
    void (*cb)(struct us_internal_callback *cb);
};

void sweep_timer_cb(struct us_internal_callback *cb);

#endif // COMMON_H
