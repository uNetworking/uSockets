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

/*
void add_socket_write(struct io_uring *ring, int fd, __u16 bid, size_t message_size, unsigned flags) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_send(sqe, fd, &bufs[bid], message_size, 0);
    io_uring_sqe_set_flags(sqe, flags);
    io_uring_sqe_set_data(sqe, );
}*/

void add_provide_buf(struct io_uring *ring, __u16 bid, unsigned gid) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    io_uring_prep_provide_buffers(sqe, bufs[bid], MAX_MESSAGE_LEN, 1, gid, bid);
    io_uring_sqe_set_flags(sqe, IOSQE_CQE_SKIP_SUCCESS);

    io_uring_sqe_set_data64(sqe, 6);
}


/* The loop has 2 fallthrough polls */
void us_loop_run(struct us_loop_t *loop) {

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

                // we need the listen_socket attached to the accept request to know the ext size
                struct us_socket_t *s = malloc(sizeof(struct us_socket_t) + 128);
                s->context = loop->context_head;
                s->dd = cqe->res;

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

                loop->context_head->on_open(s, 1, 0, 0);

            } else if (type == SOCKET_READ) {
                int bytes_read = cqe->res;
                int bid = cqe->flags >> 16;
                if (cqe->res <= 0) {
                    // read failed, re-add the buffer
                    add_provide_buf(&loop->ring, bid, group_id);
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

                    loop->context_head->on_data(s, bufs[bid], bytes_read);

                    struct io_uring_sqe *sqe = io_uring_get_sqe(&loop->ring);
                    io_uring_prep_provide_buffers(sqe, bufs[bid], MAX_MESSAGE_LEN, 1, group_id, bid);
                    io_uring_sqe_set_flags(sqe, IOSQE_CQE_SKIP_SUCCESS);

                    io_uring_sqe_set_data64(sqe, 6); // nothing we handle


                    //add_socket_write(&loop->ring, s->dd, bid, bytes_read, IOSQE_FIXED_FILE);
                }
            } else if (type == SOCKET_WRITE) {
                // write has been completed, first re-add the buffer
                //add_provide_buf(&loop->ring, conn_i.bid, group_id);
                // add a new read for the existing connection
                
            }
        }

        io_uring_cq_advance(&loop->ring, count);
    }
}

struct us_timer_t *us_create_timer(struct us_loop_t *loop, int fallthrough, unsigned int ext_size) {

}

void us_timer_set(struct us_timer_t *t, void (*cb)(struct us_timer_t *t), int ms, int repeat_ms) {

}

void us_loop_free(struct us_loop_t *loop) {

}

struct us_loop_t *us_create_loop(void *hint, void (*wakeup_cb)(struct us_loop_t *loop), void (*pre_cb)(struct us_loop_t *loop), void (*post_cb)(struct us_loop_t *loop), unsigned int ext_size) {

    struct us_loop_t *loop = malloc(ext_size + sizeof(struct us_loop_t));

// initialize io_uring
    struct io_uring_params params;
    //struct io_uring ring;
    memset(&params, 0, sizeof(params));

    params.flags = IORING_SETUP_COOP_TASKRUN | IORING_SETUP_SINGLE_ISSUER;//IORING_SETUP_SQPOLL;
    //params.sq_thread_idle = 10000;

    if (io_uring_queue_init_params(2048, &loop->ring, &params) < 0) {
        perror("io_uring_init_failed...\n");
        exit(1);
    }

    if (io_uring_register_files_sparse(&loop->ring, 1024)) {
        exit(1);
    }

    // check if IORING_FEAT_FAST_POLL is supported
    if (!(params.features & IORING_FEAT_FAST_POLL)) {
        printf("IORING_FEAT_FAST_POLL not available in the kernel, quiting...\n");
        exit(0);
    }

    // check if buffer selection is supported
    struct io_uring_probe *probe;
    probe = io_uring_get_probe_ring(&loop->ring);
    if (!probe || !io_uring_opcode_supported(probe, IORING_OP_PROVIDE_BUFFERS)) {
        printf("Buffer select not supported, skipping...\n");
        exit(0);
    }
    io_uring_free_probe(probe);

    // register buffers for buffer selection
    struct io_uring_sqe *sqe;
    struct io_uring_cqe *cqe;

    sqe = io_uring_get_sqe(&loop->ring);
    io_uring_prep_provide_buffers(sqe, bufs, MAX_MESSAGE_LEN, BUFFERS_COUNT, group_id, 0);

    // also register these buffers as fixed
    

    io_uring_submit(&loop->ring);
    io_uring_wait_cqe(&loop->ring, &cqe);
    if (cqe->res < 0) {
        printf("cqe->res = %d\n", cqe->res);
        exit(1);
    }
    io_uring_cqe_seen(&loop->ring, cqe);

    return loop;
}

void us_internal_loop_data_free(struct us_loop_t *loop) {

}

void us_wakeup_loop(struct us_loop_t *loop) {

}

void us_internal_loop_link(struct us_loop_t *loop, struct us_socket_context_t *context) {

}

/* Unlink is called before free */
void us_internal_loop_unlink(struct us_loop_t *loop, struct us_socket_context_t *context) {

}

/* This functions should never run recursively */
void us_internal_timer_sweep(struct us_loop_t *loop) {

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

}

#endif