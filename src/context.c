#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

// put this in loop.c?
struct us_socket_context *us_create_socket_context(struct us_loop *loop, int context_ext_size) {
    struct us_socket_context *context = malloc(sizeof(struct us_socket_context) + context_ext_size);

    // us_socket_context_init(loop)
    context->loop = loop;
    context->head = 0;
    context->iterator = 0;
    context->next = 0;

    us_internal_loop_link(loop, context);

    return context;
}

void us_socket_context_free(struct us_socket_context *context) {
    free(context);
}

struct us_listen_socket *us_socket_context_listen(struct us_socket_context *context, const char *host, int port, int options, int socket_ext_size) {

    LIBUS_SOCKET_DESCRIPTOR listen_socket_fd = bsd_create_listen_socket(host, port, options);

    if (listen_socket_fd == LIBUS_SOCKET_ERROR) {
        return 0;
    }

    struct us_poll *p = us_create_poll(context->loop, 0, sizeof(struct us_listen_socket));
    us_poll_init(p, listen_socket_fd, POLL_TYPE_SEMI_SOCKET);
    us_poll_start(p, context->loop, LIBUS_SOCKET_READABLE);

    struct us_listen_socket *ls = (struct us_listen_socket *) p;

    //us_internal_init_socket(&ls->s, context);

    // this is common, should be like us_internal_socket_init(context);
    ls->s.context = context;
    ls->s.timeout = 0;
    ls->s.next = 0;
    us_internal_socket_context_link(context, &ls->s);

    ls->socket_ext_size = socket_ext_size;

    return ls;
}

void us_listen_socket_close(struct us_listen_socket *ls) {
    us_internal_socket_context_unlink(ls->s.context, &ls->s);

    us_poll_stop((struct us_poll *) &ls->s, ls->s.context->loop);
    bsd_close_socket(us_poll_fd((struct us_poll *) &ls->s));

    /* let's just free it here */
    us_poll_free((struct us_poll *) &ls->s, ls->s.context->loop);
}

struct us_socket *us_socket_context_connect(struct us_socket_context *context, const char *host, int port, int options, int socket_ext_size) {
    LIBUS_SOCKET_DESCRIPTOR connect_socket_fd = bsd_create_connect_socket(host, port, options);

    if (connect_socket_fd == LIBUS_SOCKET_ERROR) {
        return 0;
    }

    // connect sockets are also listen sockets but with writable ready
    struct us_poll *p = us_create_poll(context->loop, 0, sizeof(struct us_socket) + socket_ext_size);
    us_poll_init(p, connect_socket_fd, POLL_TYPE_SEMI_SOCKET);
    us_poll_start(p, context->loop, LIBUS_SOCKET_WRITABLE);

    struct us_socket *connect_socket = (struct us_socket *) p;

    connect_socket->context = context;

    // make sure to link this socket into its context!
    us_internal_socket_context_link(context, connect_socket);

    return connect_socket;
}

struct us_socket_context *us_create_child_socket_context(struct us_socket_context *context, int context_ext_size) {
    // in the case of TCP, nothing is shared
    return us_create_socket_context(context->loop, context_ext_size);
}

// bug: this will set timeout to 0
struct us_socket *us_socket_context_adopt_socket(struct us_socket_context *context, struct us_socket *s, int ext_size) {
    // if you do this while closed, it's your own problem
    if (us_internal_socket_is_closed(s)) {
        return s;
    }

    // this will update the iterator if in on_timeout
    us_internal_socket_context_unlink(s->context, s);

    struct us_socket *new_s = (struct us_socket *) us_poll_resize(&s->p, s->context->loop, sizeof(struct us_socket) + ext_size);

    us_internal_socket_context_link(context, new_s);

    return new_s;
}

// check and update context->iterator against s!
void us_internal_socket_context_unlink(struct us_socket_context *context, struct us_socket *s) {
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
void us_internal_socket_context_link(struct us_socket_context *context, struct us_socket *s) {
    s->context = context; // this was added
    s->timeout = 0; // this was needed? bug: do not reset timeout when adopting socket!
    s->next = context->head;
    s->prev = 0;
    if (context->head) {
        context->head->prev = s;
    }
    context->head = s;
}

void us_socket_context_on_open(struct us_socket_context *context, struct us_socket *(*on_open)(struct us_socket *s, int is_client)) {
    context->on_open = on_open;
}

void us_socket_context_on_close(struct us_socket_context *context, struct us_socket *(*on_close)(struct us_socket *s)) {
    context->on_close = on_close;
}

void us_socket_context_on_data(struct us_socket_context *context, struct us_socket *(*on_data)(struct us_socket *s, char *data, int length)) {
    context->on_data = on_data;
}

void us_socket_context_on_writable(struct us_socket_context *context, struct us_socket *(*on_writable)(struct us_socket *s)) {
    context->on_writable = on_writable;
}

void us_socket_context_on_timeout(struct us_socket_context *context, struct us_socket *(*on_timeout)(struct us_socket *)) {
    context->on_socket_timeout = on_timeout;
}

void us_socket_context_on_end(struct us_socket_context *context, struct us_socket *(*on_end)(struct us_socket *)) {
    context->on_end = on_end;
}

void *us_socket_context_ext(struct us_socket_context *context) {
    return context + 1;
}

struct us_loop *us_socket_context_loop(struct us_socket_context *context) {
    return context->loop;
}
