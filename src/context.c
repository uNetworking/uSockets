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

int default_ignore_data_handler(struct us_socket_t *s) {
    return 0;
}

// put this in loop.c?
struct us_socket_context_t *us_create_socket_context(int ssl, struct us_loop_t *loop, int context_ext_size, struct us_socket_context_options_t options) {
    struct us_socket_context_t *context = malloc(sizeof(struct us_socket_context_t) + context_ext_size);

    // us_socket_context_init(loop)
    context->loop = loop;
    context->head = 0;
    context->iterator = 0;
    context->next = 0;

    // by default never ignore any data
    context->ignore_data = default_ignore_data_handler;

    us_internal_loop_link(loop, context);

    return context;
}

void us_socket_context_free(int ssl, struct us_socket_context_t *context) {
    free(context);
}

struct us_listen_socket_t *us_socket_context_listen(int ssl, struct us_socket_context_t *context, const char *host, int port, int options, int socket_ext_size) {

    LIBUS_SOCKET_DESCRIPTOR listen_socket_fd = bsd_create_listen_socket(host, port, options);

    if (listen_socket_fd == LIBUS_SOCKET_ERROR) {
        return 0;
    }

    struct us_poll_t *p = us_create_poll(context->loop, 0, sizeof(struct us_listen_socket_t));
    us_poll_init(p, listen_socket_fd, POLL_TYPE_SEMI_SOCKET);
    us_poll_start(p, context->loop, LIBUS_SOCKET_READABLE);

    struct us_listen_socket_t *ls = (struct us_listen_socket_t *) p;

    //us_internal_init_socket(&ls->s, context);

    // this is common, should be like us_internal_socket_init(context);
    ls->s.context = context;
    ls->s.timeout = 0;
    ls->s.next = 0;
    us_internal_socket_context_link(context, &ls->s);

    ls->socket_ext_size = socket_ext_size;

    return ls;
}

void us_listen_socket_close(int ssl, struct us_listen_socket_t *ls) {
    us_internal_socket_context_unlink(ls->s.context, &ls->s);

    us_poll_stop((struct us_poll_t *) &ls->s, ls->s.context->loop);
    bsd_close_socket(us_poll_fd((struct us_poll_t *) &ls->s));

    /* let's just free it here */
    us_poll_free((struct us_poll_t *) &ls->s, ls->s.context->loop);
}

struct us_socket_t *us_socket_context_connect(int ssl, struct us_socket_context_t *context, const char *host, int port, int options, int socket_ext_size) {
    LIBUS_SOCKET_DESCRIPTOR connect_socket_fd = bsd_create_connect_socket(host, port, options);

    if (connect_socket_fd == LIBUS_SOCKET_ERROR) {
        return 0;
    }

    // connect sockets are also listen sockets but with writable ready
    struct us_poll_t *p = us_create_poll(context->loop, 0, sizeof(struct us_socket_t) + socket_ext_size);
    us_poll_init(p, connect_socket_fd, POLL_TYPE_SEMI_SOCKET);
    us_poll_start(p, context->loop, LIBUS_SOCKET_WRITABLE);

    struct us_socket_t *connect_socket = (struct us_socket_t *) p;

    connect_socket->context = context;

    // make sure to link this socket into its context!
    us_internal_socket_context_link(context, connect_socket);

    return connect_socket;
}

struct us_socket_context_t *us_create_child_socket_context(int ssl, struct us_socket_context_t *context, int context_ext_size) {
    // in the case of TCP, nothing is shared

    struct us_socket_context_options_t options = {}; // what happens with options?

    return us_create_socket_context(ssl, context->loop, context_ext_size, options);
}

// bug: this will set timeout to 0
struct us_socket_t *us_socket_context_adopt_socket(int ssl, struct us_socket_context_t *context, struct us_socket_t *s, int ext_size) {
    // if you do this while closed, it's your own problem
    if (us_socket_is_closed(ssl, s)) {
        return s;
    }

    // this will update the iterator if in on_timeout
    us_internal_socket_context_unlink(s->context, s);

    struct us_socket_t *new_s = (struct us_socket_t *) us_poll_resize(&s->p, s->context->loop, sizeof(struct us_socket_t) + ext_size);

    us_internal_socket_context_link(context, new_s);

    return new_s;
}

// check and update context->iterator against s!
void us_internal_socket_context_unlink(struct us_socket_context_t *context, struct us_socket_t *s) {
    // update iterator
    if (s == context->iterator) {
        context->iterator = s->next;
    }

    if (s->prev == s->next) {
        context->head = 0;
    } else {
        if (s->prev) {
            s->prev->next = s->next;
        } else {
            context->head = s->next;
        }
        if (s->next) {
            s->next->prev = s->prev;
        }
    }
}

// also check iterator, etc
// we always add in the top, which means we never modify any next
void us_internal_socket_context_link(struct us_socket_context_t *context, struct us_socket_t *s) {
    s->context = context; // this was added
    s->timeout = 0; // this was needed? bug: do not reset timeout when adopting socket!
    s->next = context->head;
    s->prev = 0;
    if (context->head) {
        context->head->prev = s;
    }
    context->head = s;
}

void us_socket_context_on_open(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_open)(struct us_socket_t *s, int is_client, char *ip, int ip_length)) {
    context->on_open = on_open;
}

void us_socket_context_on_close(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_close)(struct us_socket_t *s)) {
    context->on_close = on_close;
}

void us_socket_context_on_data(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_data)(struct us_socket_t *s, char *data, int length)) {
    context->on_data = on_data;
}

void us_socket_context_on_writable(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_writable)(struct us_socket_t *s)) {
    context->on_writable = on_writable;
}

void us_socket_context_on_timeout(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_timeout)(struct us_socket_t *)) {
    context->on_socket_timeout = on_timeout;
}

void us_socket_context_on_end(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_end)(struct us_socket_t *)) {
    context->on_end = on_end;
}

void *us_socket_context_ext(int ssl, struct us_socket_context_t *context) {
    return context + 1;
}

struct us_loop_t *us_socket_context_loop(int ssl, struct us_socket_context_t *context) {
    return context->loop;
}
