#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

void us_internal_loop_data_init(struct us_loop *loop, void (*wakeup_cb)(struct us_loop *loop), void (*pre_cb)(struct us_loop *loop), void (*post_cb)(struct us_loop *loop)) {
    loop->data.sweep_timer = us_create_timer(loop, 1, 0);
    loop->data.recv_buf = malloc(LIBUS_RECV_BUFFER_LENGTH);
    loop->data.ssl_data = 0;
    loop->data.head = 0;

    loop->data.pre_cb = pre_cb;
    loop->data.post_cb = post_cb;

    // create the async here too!
    loop->data.wakeup_async = us_internal_create_async(loop, 1, 0);

    // we need a shim callback that takes the internal_async and then calls the user level callback with the loop!
    us_internal_async_set(loop->data.wakeup_async, (void (*)(struct us_internal_async *)) wakeup_cb);
}

void us_wakeup_loop(struct us_loop *loop) {
    us_internal_async_wakeup(loop->data.wakeup_async);
}

void us_internal_timer_sweep(struct us_loop *loop) {
    printf("sweeping timers now\n");
    for (struct us_socket_context *context = loop->data.head; context; context = context->next) {
        for (struct us_socket *s = context->head; s; s = s->next) {
            if (--(s->timeout) == 0) {
                context->on_socket_timeout(s);
            }
        }
    }
}

void sweep_timer_cb(struct us_internal_callback *cb) {
    us_internal_timer_sweep(cb->loop);
}

void us_internal_dispatch_ready_poll(struct us_poll *p, int error, int events) {
    switch (us_internal_poll_type(p)) {
    case POLL_TYPE_CALLBACK: {
            us_internal_accept_poll_event(p);
            struct us_internal_callback *cb = (struct us_internal_callback *) p;
            cb->cb(cb->cb_expects_the_loop ? (struct us_internal_callback *) cb->loop : (struct us_internal_callback *) &cb->p);
        }
        break;
    case POLL_TYPE_LISTEN_SOCKET: {
            struct us_listen_socket *listen_socket = (struct us_listen_socket *) p;

            LIBUS_SOCKET_DESCRIPTOR client_fd = bsd_accept_socket(us_poll_fd(p));
            if (client_fd == LIBUS_SOCKET_ERROR) {
                // start timer here

            } else {

                // stop timer if any

                do {
                    struct us_poll *p = us_create_poll(us_socket_get_context(&listen_socket->s)->loop, 0, sizeof(struct us_socket) - sizeof(struct us_poll) + listen_socket->socket_ext_size);
                    us_poll_init(p, client_fd, POLL_TYPE_SOCKET);
                    us_poll_start(p, listen_socket->s.context->loop, LIBUS_SOCKET_READABLE);

                    struct us_socket *s = (struct us_socket *) p;

                    s->context = listen_socket->s.context;

                    // make sure to always set nodelay!
                    bsd_socket_nodelay(client_fd, 1);

                    // make sure to link this socket into its context!
                    us_socket_context_link(listen_socket->s.context, s);

                    listen_socket->s.context->on_open(s);
                } while ((client_fd = bsd_accept_socket(us_poll_fd(p))) != LIBUS_SOCKET_ERROR);
            }
        }
        break;
    case POLL_TYPE_SOCKET_SHUT_DOWN:
    case POLL_TYPE_SOCKET: {
            struct us_socket *s = (struct us_socket *) p;

            if (events & LIBUS_SOCKET_WRITABLE) {
                s->context->loop->data.last_write_failed = 0;
                s->context->on_writable(s);
                if (!s->context->loop->data.last_write_failed) {
                    us_poll_change(p, us_socket_get_context(s)->loop, LIBUS_SOCKET_READABLE);

                    // it is safe (for us) to call shutdown twice, if last failed due to polling for writable
                    if (us_socket_is_shutting_down(s)) {
                        us_socket_shutdown(s);
                    }
                }
            }

            if (events & LIBUS_SOCKET_READABLE) {
                int length = bsd_recv(us_poll_fd(p), s->context->loop->data.recv_buf, LIBUS_RECV_BUFFER_LENGTH, 0);
                if (length > 0) {
                    s->context->on_data((struct us_socket *) p, s->context->loop->data.recv_buf, length);
                } else if (!length || (length == LIBUS_SOCKET_ERROR && !bsd_would_block())) {

                    // först måste vi hantera onEnd? nej?

                    //unlink
                    us_socket_context_unlink(s->context, s);

                    s->context->on_close(s);

                    us_poll_stop(p, s->context->loop);
                    bsd_close_socket(us_poll_fd(p));
                    us_poll_free(p);
                }
            }
        }
        break;
    }
}

void *us_loop_ext(struct us_loop *loop) {
    return loop + 1;
}
