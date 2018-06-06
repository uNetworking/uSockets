#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

void us_timer_sweep(struct us_loop *loop) {
    for (struct us_socket_context *context = loop->head; context; context = context->next) {
        for (struct us_socket *s = context->head; s; s = s->next) {
            if (--(s->timeout) == 0) {
                context->on_socket_timeout(s);
            }
        }
    }
}

// internal only
void us_dispatch_ready_poll(struct us_poll *p, int error, int events) {
    switch (us_poll_type(p)) {
    case POLL_TYPE_LISTEN_SOCKET: {
            struct us_listen_socket *listen_socket = (struct us_listen_socket *) p;

            LIBUS_SOCKET_DESCRIPTOR client_fd = bsd_accept_socket(us_poll_fd(p));
            if (client_fd == LIBUS_SOCKET_ERROR) {
                // start timer here
            } else {

                // stop timer if any

                do {
                    struct us_socket *s = malloc(listen_socket->socket_size);
                    us_poll_init(s, client_fd, POLL_TYPE_SOCKET);
                    us_poll_start(s, listen_socket->s.context->loop, LIBUS_SOCKET_READABLE);

                    s->context = listen_socket->s.context;

                    // make sure to always set nodelay!
                    bsd_socket_nodelay(client_fd, 1);

                    listen_socket->s.context->on_open(s);
                } while ((client_fd = bsd_accept_socket(us_poll_fd(p))) != LIBUS_SOCKET_ERROR);
            }
        }
        break;
    case POLL_TYPE_SOCKET: {
            struct us_socket *s = (struct us_socket *) p;

            if (events & LIBUS_SOCKET_WRITABLE) {
                s->context->on_writable(s);
            }

            if (events & LIBUS_SOCKET_READABLE) {
                int length = bsd_recv(us_poll_fd(p), s->context->loop->recv_buf, LIBUS_RECV_BUFFER_LENGTH, 0);
                if (length > 0) {
                    s->context->on_data(s, s->context->loop->recv_buf, length);
                } else if (!length || (length == LIBUS_SOCKET_ERROR && !bsd_would_block())) {

                    // först måste vi hantera onEnd? nej?

                    s->context->on_close(s);

                    // vi äger socketen och tar bort den här
                    us_poll_stop(s, s->context->loop);
                    bsd_close_socket(us_poll_fd(s));
                    free(s);
                }
            }
        }
        break;
    }
}

// loop.c?
void *us_loop_userdata(struct us_loop *loop) {
    return loop + 1;
}

void *us_socket_get_context(struct us_socket *s) {
    return s->context;
}

void us_socket_context_on_open(struct us_socket_context *context, void (*on_open)(struct us_socket *s)) {
    context->on_open = on_open;
}

void us_socket_context_on_close(struct us_socket_context *context, void (*on_close)(struct us_socket *s)) {
    context->on_close = on_close;
}

void us_socket_context_on_data(struct us_socket_context *context, void (*on_data)(struct us_socket *s, char *data, int length)) {
    context->on_data = on_data;
}

void us_socket_context_on_writable(struct us_socket_context *context, void (*on_writable)(struct us_socket *s)) {
    context->on_writable = on_writable;
}

void us_socket_context_on_timeout(struct us_socket_context *context, void (*on_timeout)(struct us_socket *)) {
    context->on_socket_timeout = on_timeout;
}

void *us_socket_context_ext(struct us_socket_context *context) {
    return context + 1;
}
