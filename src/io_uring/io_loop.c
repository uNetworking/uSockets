/*
 * Authored by Alex Hultman, 2018-2021.
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

#ifdef LIBUS_USE_IO_URING


#include "libusockets.h"
#include "internal/internal.h"
#include "internal.h"
#include <stdlib.h>

char bufs[BUFFERS_COUNT][MAX_MESSAGE_LEN] = {0};
int group_id = 1337;


/* This functions should never run recursively */
void us_internal_timer_sweep(struct us_loop_t *loop) {
    struct us_loop_t *loop_data = loop;
    /* For all socket contexts in this loop */
    for (loop_data->iterator = loop_data->head; loop_data->iterator; loop_data->iterator = loop_data->iterator->next) {


        struct us_socket_context_t *context = loop_data->iterator;

        /* Update this context's timestamps (this could be moved to loop and done once) */
        context->global_tick++;
        unsigned char short_ticks = context->timestamp = context->global_tick % 240;
        unsigned char long_ticks = context->long_timestamp = (context->global_tick / 15) % 240;

        /* Begin at head */
        struct us_socket_t *s = context->head_sockets;
        while (s) {
            /* Seek until end or timeout found (tightest loop) */
            while (1) {
                /* We only read from 1 random cache line here */
                if (short_ticks == s->timeout || long_ticks == s->long_timeout) {
                    break;
                }

                /* Did we reach the end without a find? */
                if ((s = s->next) == 0) {
                    goto next_context;
                }
            }

            /* Here we have a timeout to emit (slow path) */
            context->iterator = s;

            if (short_ticks == s->timeout) {
                s->timeout = 255;
                context->on_socket_timeout(s);
            }

            if (context->iterator == s && long_ticks == s->long_timeout) {
                s->long_timeout = 255;
                context->on_socket_long_timeout(s);
            }   

            /* Check for unlink / link (if the event handler did not modify the chain, we step 1) */
            if (s == context->iterator) {
                s = s->next;
            } else {
                /* The iterator was changed by event handler */
                s = context->iterator;
            }
        }
        /* We always store a 0 to context->iterator here since we are no longer iterating this context */
        next_context:
        context->iterator = 0;
    }
}


/* The loop has 2 fallthrough polls */
void us_loop_run(struct us_loop_t *loop) {

    us_timer_set(loop->timer, NULL, LIBUS_TIMEOUT_GRANULARITY * 1000, LIBUS_TIMEOUT_GRANULARITY * 1000);

    // // register a timeout
    // struct __kernel_timespec ts;
    // ts.tv_sec = 4;
    // ts.tv_nsec = 0;

    // struct io_uring_sqe *sqe = io_uring_get_sqe(&loop->ring);
    // io_uring_prep_timeout(sqe, &ts, 1, IORING_TIMEOUT_REALTIME | IORING_TIMEOUT_ETIME_SUCCESS);
    // io_uring_sqe_set_data(sqe, (char *)loop + LOOP_TIMER);

    while (1) {
        io_uring_submit_and_wait(&loop->ring, 1);
        struct io_uring_cqe *cqe;
        unsigned head;
        unsigned count = 0;

        // go through all CQEs
        io_uring_for_each_cqe(&loop->ring, head, cqe) {
            ++count;
            //struct conn_info conn_i;
            //memcpy(&conn_i, &cqe->user_data, sizeof(conn_i));


            int pointer_tag = (int)((uintptr_t)io_uring_cqe_get_data(cqe) & (uintptr_t)0x7ull);
            void *object = (void *)((uintptr_t)io_uring_cqe_get_data(cqe) & (uintptr_t)0xFFFFFFFFFFFFFFF8ull);

            if (pointer_tag == 6) {
                printf("uuuuuuuuh: %d\n", cqe->res);
                exit(1);
            }

            //printf("pointer tag is %d\n", pointer_tag);

            int type = pointer_tag;//conn_i.type;
            if (cqe->res == -ENOBUFS) {
                fprintf(stdout, "bufs in automatic buffer selection empty, this should not happen...\n");
                fflush(stdout);
                exit(1);
            }/* else if (type == PROV_BUF) {
                if (cqe->res < 0) {
                    printf("cqe->res = %d\n", cqe->res);
                    exit(1);
                }
            } */else if (type == LISTEN_SOCKET_ACCEPT) {

                struct us_listen_socket_t *listen_s = object;

                // we need the listen_socket attached to the accept request to know the ext size and context
                struct us_socket_t *s = malloc(sizeof(struct us_socket_t) + listen_s->socket_ext_size);
                s->context = listen_s->context;
                s->dd = cqe->res;
                s->timeout = 255;
                s->long_timeout = 255;
                s->prev = s->next = 0;

                us_internal_socket_context_link_socket(listen_s->context, s);


                // register this send buffer as registered buffer (using the DD of the socket as index!)
                struct iovec iovecs = {s->sendBuf, 16 * 1024};
                //printf("register: %d\n", io_uring_register_buffers_update_tag(&loop->ring, s->dd, &iovecs, 0, 1));

                int sock_conn_fd = cqe->res;
                // only read when there is no error, >= 0
                if (sock_conn_fd >= 0) {
                    // this will not succeed since we don't have FD we have DD
                    //bsd_socket_nodelay(sock_conn_fd, 1);


                    struct io_uring_sqe *sqe = io_uring_get_sqe(&loop->ring);
                    io_uring_prep_recv_multishot(sqe, s->dd, NULL, 0/*message_size*/, 0);
                    io_uring_sqe_set_flags(sqe, IOSQE_BUFFER_SELECT | IOSQE_FIXED_FILE);
                    sqe->buf_group = group_id;

                    io_uring_sqe_set_data(sqe, (char *)s + SOCKET_READ);

                    //printf("starting to read on new socket: %p\n", s);
                }

                s->context->on_open(s, 1, 0, 0);

            } else if (type == SOCKET_READ) {
                int bytes_read = cqe->res;
                int bid = cqe->flags >> 16;
                if (cqe->res <= 0) {
                    // read failed, re-add the buffer
                    
                    //add_provide_buf(&loop->ring, bid, group_id);

                    io_uring_buf_ring_add(loop->buf_ring, bufs[bid], MAX_MESSAGE_LEN, bid, io_uring_buf_ring_mask(4096), 0);
                    loop->buf_ring->tail++;


                    // connection closed or error
                    //close(conn_i.fd);
                    struct io_uring_sqe *sqe = io_uring_get_sqe(&loop->ring);

                    struct us_socket_t *s = object;

                    //printf("closed socket: %p\n", s);

                    s->context->on_close(s, 0, 0);

                    io_uring_prep_close_direct(sqe, s->dd);
                    io_uring_sqe_set_flags(sqe, IOSQE_CQE_SKIP_SUCCESS);
                    io_uring_sqe_set_data64(sqe, 5);
                } else {


                    struct us_socket_t *s = object;

                    //printf("emitting read on new socket: %p\n", s);

                    s->context->on_data(s, bufs[bid], bytes_read);


                    io_uring_buf_ring_add(loop->buf_ring, bufs[bid], MAX_MESSAGE_LEN, bid, io_uring_buf_ring_mask(4096), 0);
                    loop->buf_ring->tail++;


                    //add_socket_write(&loop->ring, s->dd, bid, bytes_read, IOSQE_FIXED_FILE);
                }
            } else if (type == SOCKET_WRITE) {
                // write has been completed, first re-add the buffer
                //add_provide_buf(&loop->ring, conn_i.bid, group_id);
                // add a new read for the existing connection
                
            } else if (type == SOCKET_CONNECT) {
                //printf("we are connectred: %d\n", cqe->res);

                struct us_socket_t *s = object;


                struct io_uring_sqe *sqe = io_uring_get_sqe(&loop->ring);
                io_uring_prep_recv_multishot(sqe, s->dd, NULL, 0/*message_size*/, 0);
                io_uring_sqe_set_flags(sqe, IOSQE_BUFFER_SELECT | IOSQE_FIXED_FILE);
                sqe->buf_group = group_id;

                io_uring_sqe_set_data(sqe, (char *)s + SOCKET_READ);


                s->context->on_open(s, 1, 0, 0);
            } else if (type == LOOP_TIMER) {
                //if (cqe->res == 0) {
                    //printf("timer tick %d\n", cqe->res);
                    us_internal_timer_sweep(loop);
                //}

                struct us_timer_t *t = object;

                    struct io_uring_sqe *sqe = io_uring_get_sqe(&loop->ring);
                io_uring_prep_read(sqe, t->fd, &t->buf, 8, 0);
                io_uring_sqe_set_data(sqe, (char *)t + LOOP_TIMER);

                // struct __kernel_timespec ts;
                // ts.tv_sec = 4;
                // ts.tv_nsec = 0;

                // struct io_uring_sqe *sqe = io_uring_get_sqe(&loop->ring);
                // io_uring_prep_timeout(sqe, &ts, 1, IORING_TIMEOUT_REALTIME | IORING_TIMEOUT_ETIME_SUCCESS);
                // io_uring_sqe_set_data(sqe, (char *)loop + LOOP_TIMER);
            }
        }

        io_uring_cq_advance(&loop->ring, count);
    }
}

#include <sys/timerfd.h>

struct us_timer_t *us_create_timer(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {
    struct us_timer_t *timer = malloc(ext_size + sizeof(struct us_timer_t));

    timer->loop = loop;

    int timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd == -1) {
      return NULL;
    }

    timer->fd = timerfd;

    return timer;
}

void us_timer_set(struct us_timer_t *t, void (*cb)(struct us_timer_t *t), int ms, int repeat_ms) {


    struct itimerspec timer_spec = {
        {repeat_ms / 1000, (long) (repeat_ms % 1000) * (long) 1000000},
        {ms / 1000, (long) (ms % 1000) * (long) 1000000}
    };

    timerfd_settime(t->fd, 0, &timer_spec, NULL);

    // prep read into uint64_t of timer

    struct io_uring_sqe *sqe = io_uring_get_sqe(&t->loop->ring);
    io_uring_prep_read(sqe, t->fd, &t->buf, 8, 0);
    io_uring_sqe_set_data(sqe, (char *)t + LOOP_TIMER);

}

void *us_timer_ext(struct us_timer_t *timer) {
    return timer + 1;
}

void us_timer_close(struct us_timer_t *timer) {

}

struct us_loop_t *us_timer_loop(struct us_timer_t *t) {
    return t->loop;
}

void us_loop_free(struct us_loop_t *loop) {

}

struct us_loop_t *us_create_loop(void *hint, void (*wakeup_cb)(struct us_loop_t *loop), void (*pre_cb)(struct us_loop_t *loop), void (*post_cb)(struct us_loop_t *loop), unsigned int ext_size) {

    struct us_loop_t *loop = malloc(ext_size + sizeof(struct us_loop_t));

    loop->timer = us_create_timer(loop, 1, 0);

    loop->head = 0;
    loop->iterator = 0;
    //loop->closed_head = 0;
    //loop->low_prio_head = 0;
    //loop->low_prio_budget = 0;

    //loop->pre_cb = pre_cb;
    //loop->post_cb = post_cb;
    //loop->iteration_nr = 0;

    struct io_uring_params params;
    memset(&params, 0, sizeof(params));
    params.flags = IORING_SETUP_COOP_TASKRUN | IORING_SETUP_SINGLE_ISSUER;
    if (io_uring_queue_init_params(2048, &loop->ring, &params) < 0) {
        perror("io_uring_init_failed...\n");
        exit(1);
    }

    if (io_uring_register_files_sparse(&loop->ring, 1024)) {
        exit(1);
    }

    io_uring_register_ring_fd(&loop->ring);

    // create buffer ring here
    struct io_uring_buf_reg reg = {0};
    posix_memalign(&reg.ring_addr, 1024 * 4, sizeof(struct io_uring_buf) * 4096);
    reg.ring_entries = 4096;
    reg.bgid = 1337;
    loop->buf_ring = reg.ring_addr;

    // registrera buffer ring bvuffer
    if (io_uring_register_buf_ring(&loop->ring, &reg, 0)) {
        printf("Failed to register ring\n");
        exit(1);
    }
    io_uring_buf_ring_init(loop->buf_ring);

    for (int i = 0; i < 4096; i++) {
        io_uring_buf_ring_add(loop->buf_ring, bufs[i], MAX_MESSAGE_LEN, i, io_uring_buf_ring_mask(4096), i);
    }
    io_uring_buf_ring_advance(loop->buf_ring, 4096);

    return loop;
}

void us_internal_loop_data_free(struct us_loop_t *loop) {

}

void us_wakeup_loop(struct us_loop_t *loop) {

}

void us_internal_loop_link(struct us_loop_t *loop, struct us_socket_context_t *context) {
    /* Insert this context as the head of loop */
    context->next = loop->head;
    context->prev = 0;
    if (loop->head) {
        loop->head->prev = context;
    }
    loop->head = context;
}

/* Unlink is called before free */
void us_internal_loop_unlink(struct us_loop_t *loop, struct us_socket_context_t *context) {

}

/* We do not want to block the loop with tons and tons of CPU-intensive work for SSL handshakes.
 * Spread it out during many loop iterations, prioritizing already open connections, they are far
 * easier on CPU */
static const int MAX_LOW_PRIO_SOCKETS_PER_LOOP_ITERATION = 5;

void us_internal_handle_low_priority_sockets(struct us_loop_t *loop) {

}

/* Note: Properly takes the linked list and timeout sweep into account */
void us_internal_free_closed_sockets(struct us_loop_t *loop) {

}

long long us_loop_iteration_number(struct us_loop_t *loop) {
    return 0;
}

/* These may have somewhat different meaning depending on the underlying event library */
void us_internal_loop_pre(struct us_loop_t *loop) {

}

void us_internal_loop_post(struct us_loop_t *loop) {

}

/* Integration only requires the timer to be set up */
void us_loop_integrate(struct us_loop_t *loop) {

}

void *us_loop_ext(struct us_loop_t *loop) {
    return loop + 1;
}

#endif