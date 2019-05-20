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

void us_internal_loop_data_init(struct us_loop *loop, void (*wakeup_cb)(struct us_loop *loop), void (*pre_cb)(struct us_loop *loop), void (*post_cb)(struct us_loop *loop)) {
    loop->data.sweep_timer = us_create_timer(loop, 1, 0);
    loop->data.recv_buf = malloc(LIBUS_RECV_BUFFER_LENGTH + LIBUS_RECV_BUFFER_PADDING * 2);
    loop->data.ssl_data = 0;
    loop->data.head = 0;
    loop->data.iterator = 0;
    loop->data.closed_head = 0;

    loop->data.pre_cb = pre_cb;
    loop->data.post_cb = post_cb;
    loop->data.iteration_nr = 0;

    // create the async here too!
    loop->data.wakeup_async = us_internal_create_async(loop, 1, 0);

    // we need a shim callback that takes the internal_async and then calls the user level callback with the loop!
    us_internal_async_set(loop->data.wakeup_async, (void (*)(struct us_internal_async *)) wakeup_cb);
}

void us_internal_loop_data_free(struct us_loop *loop) {
    // we need to hook into the SSL layer here to free stuff
#ifndef LIBUS_NO_SSL
    us_internal_free_loop_ssl_data(loop);
#endif

    free(loop->data.recv_buf);

    us_timer_close(loop->data.sweep_timer);
    us_internal_async_close(loop->data.wakeup_async);
}

void us_wakeup_loop(struct us_loop *loop) {
    us_internal_async_wakeup(loop->data.wakeup_async);
}

void us_internal_loop_link(struct us_loop *loop, struct us_socket_context *context) {
    context->next = loop->data.head;
    context->prev = 0;
    if (loop->data.head) {
        loop->data.head->prev = context;
    }
    loop->data.head = context;
}

void us_internal_timer_sweep(struct us_loop *loop) {

    // we need loop->socket_iterator
    // and loop->context_iterator
    // then unlink/link functions for both
    // also make sure to check for recursion here! timer_sweep should never run recursively!

    struct us_loop_data *loop_data = &loop->data;

    //printf("sweeping timers now\n");
    for (loop_data->iterator = loop_data->head; loop_data->iterator; loop_data->iterator = loop_data->iterator->next) {

        struct us_socket_context *context = loop_data->iterator;

        //printf("Sweeping context: %ld\n", context);

        for (context->iterator = context->head; context->iterator; ) {

            struct us_socket *s = context->iterator;

            // this shouldn't count down if already at 0!
            if (s->timeout && --(s->timeout) == 0) {

                // if we adopt in the middle here we are fucked and thus it crashes
                context->on_socket_timeout(s);

                // did they somehow update the iterator? (they unlinked, etc?)
                if (s == context->iterator) {
                    context->iterator = s->next;
                }
            } else {
                context->iterator = s->next;
            }


        }
    }
}

// this one also works with the linked list
void us_internal_free_closed_sockets(struct us_loop *loop) {
    // free all closed sockets (maybe we want to reverse this order?)
    if (loop->data.closed_head) {
        for (struct us_socket *s = loop->data.closed_head; s; ) {
            struct us_socket *next = s->next;
            //printf("Freeing a closed poll now\n");
            us_poll_free((struct us_poll *) s, loop);
            s = next;
        }
        loop->data.closed_head = 0;
    }
}

void sweep_timer_cb(struct us_internal_callback *cb) {
    us_internal_timer_sweep(cb->loop);
}

long long us_loop_iteration_number(struct us_loop *loop) {
    return loop->data.iteration_nr;
}

// called by epoll or libuv event loops (they decide what makes up these)
void us_internal_loop_pre(struct us_loop *loop) {
    loop->data.iteration_nr++;
    loop->data.pre_cb(loop);
}

void us_internal_loop_post(struct us_loop *loop) {
    us_internal_free_closed_sockets(loop);
    loop->data.post_cb(loop);
}

void us_internal_dispatch_ready_poll(struct us_poll *p, int error, int events) {
    //printf("us_internal_dispatch_ready_poll, poll: %ld, error: %d\n", p, error);

    switch (us_internal_poll_type(p)) {
    case POLL_TYPE_CALLBACK: {
            us_internal_accept_poll_event(p);
            struct us_internal_callback *cb = (struct us_internal_callback *) p;
            cb->cb(cb->cb_expects_the_loop ? (struct us_internal_callback *) cb->loop : (struct us_internal_callback *) &cb->p);
        }
        break;
    case POLL_TYPE_SEMI_SOCKET: {
            // is this a listen socket or connect socket?
            if (us_poll_events(p) == LIBUS_SOCKET_WRITABLE) {
                struct us_socket *s = (struct us_socket *) p;

                us_poll_change(p, s->context->loop, LIBUS_SOCKET_READABLE);

                // make sure to always set nodelay!
                bsd_socket_nodelay(us_poll_fd(p), 1);

                // change type to socket here
                us_internal_poll_set_type(p, POLL_TYPE_SOCKET);

                s->context->on_open(s, 1, 0, 0);
            } else {
                struct us_listen_socket *listen_socket = (struct us_listen_socket *) p;
                struct bsd_addr_t addr;

                LIBUS_SOCKET_DESCRIPTOR client_fd = bsd_accept_socket(us_poll_fd(p), &addr);
                if (client_fd == LIBUS_SOCKET_ERROR) {
                    // start timer here

                } else {

                    // stop timer if any

                    do {
                        struct us_poll *p = us_create_poll(us_socket_get_context(&listen_socket->s)->loop, 0, sizeof(struct us_socket) - sizeof(struct us_poll) + listen_socket->socket_ext_size);
                        us_internal_init_socket((struct us_socket *)p, 0);

                        us_poll_init(p, client_fd, POLL_TYPE_SOCKET);
                        us_poll_start(p, listen_socket->s.context->loop, LIBUS_SOCKET_READABLE);

                        struct us_socket *s = (struct us_socket *) p;

                        // this is shared!
                        s->context = listen_socket->s.context;

                        // make sure to always set nodelay!
                        bsd_socket_nodelay(client_fd, 1);

                        // make sure to link this socket into its context!
                        us_internal_socket_context_link(listen_socket->s.context, s);

                        listen_socket->s.context->on_open(s, 0, bsd_addr_get_ip(&addr), bsd_addr_get_ip_length(&addr));
                    } while ((client_fd = bsd_accept_socket(us_poll_fd(p), &addr)) != LIBUS_SOCKET_ERROR);
                }
            }
        }
        break;
    case POLL_TYPE_SOCKET_SHUT_DOWN:
    case POLL_TYPE_SOCKET: {
            // any use of p after this point should be invalid
            struct us_socket *s = (struct us_socket *) p;

            // epollerr epollhup
            if (error) {
                s = us_socket_close(s);
                return;
            }

            if (events & LIBUS_SOCKET_WRITABLE) {
                // bug: what if we changed context! then last_write_failed will not point to the same!
                s->context->loop->data.last_write_failed = 0;

                s = s->context->on_writable(s);

                if (us_socket_is_closed(s)) {
                    return;
                }

                // if we shut down then do this for sure!
                if (!s->context->loop->data.last_write_failed || us_socket_is_shut_down(s)) {
                    us_poll_change(&s->p, us_socket_get_context(s)->loop, us_poll_events(&s->p) & LIBUS_SOCKET_READABLE);
                }
            }

            if (events & LIBUS_SOCKET_READABLE) {

                // we give the context a chance to ignore data for balancing purposes
                // useful for balancing SSL opening handshakes as they are very CPU intensive
                if (s->context->ignore_data(s)) {
                    break;
                }

                int length = bsd_recv(us_poll_fd(&s->p), s->context->loop->data.recv_buf + LIBUS_RECV_BUFFER_PADDING, LIBUS_RECV_BUFFER_LENGTH, 0);
                if (length > 0) {
                    s = s->context->on_data(s, s->context->loop->data.recv_buf + LIBUS_RECV_BUFFER_PADDING, length);
                } else if (!length) {
                    // is_shut_down is better name now that we do not wait for writing finished
                    if (us_socket_is_shut_down(s)) {
                        s = us_socket_close(s);
                    } else {
                        us_poll_change(&s->p, us_socket_get_context(s)->loop, us_poll_events(&s->p) & LIBUS_SOCKET_WRITABLE);
                        // for HTTP and other similar high-level protocols a close is needed
                        s = s->context->on_end(s);
                    }
                } else if (length == LIBUS_SOCKET_ERROR && !bsd_would_block()) {
                    s = us_socket_close(s);
                }

                // here we need is_closed and free or queue up the poll for removal in next loop iteration
                // context == null should be perfect signal for is_closed
            }
        }
        break;
    }
}

// sets up the sweep timer
void us_loop_integrate(struct us_loop *loop) {
    us_timer_set(loop->data.sweep_timer, (void (*)(struct us_timer *)) sweep_timer_cb, LIBUS_TIMEOUT_GRANULARITY * 1000, LIBUS_TIMEOUT_GRANULARITY * 1000);
}

void *us_loop_ext(struct us_loop *loop) {
    return loop + 1;
}
