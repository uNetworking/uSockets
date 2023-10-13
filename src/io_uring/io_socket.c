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
#include <string.h>
#include <stdint.h>
#include <stdio.h>



/* Shared with SSL */

int us_socket_local_port(int ssl, struct us_socket_t *s) {
    return 0;
}

int us_socket_remote_port(int ssl, struct us_socket_t *s) {
    return 0;
}

void us_socket_shutdown_read(int ssl, struct us_socket_t *s) {

}

void us_socket_remote_address(int ssl, struct us_socket_t *s, char *buf, int *length) {

}

struct us_socket_context_t *us_socket_context(int ssl, struct us_socket_t *s) {
    return s->context;
}

void us_socket_timeout(int ssl, struct us_socket_t *s, unsigned int seconds) {
    if (seconds) {
        s->timeout = ((unsigned int)s->context->timestamp + ((seconds + 3) >> 2)) % 240;
    } else {
        s->timeout = 255;
    }
}

void us_socket_long_timeout(int ssl, struct us_socket_t *s, unsigned int minutes) {
    if (minutes) {
        s->long_timeout = ((unsigned int)s->context->long_timestamp + minutes) % 240;
    } else {
        s->long_timeout = 255;
    }
}

void us_socket_flush(int ssl, struct us_socket_t *s) {

}

int us_socket_is_closed(int ssl, struct us_socket_t *s) {
    return 0;
}

int us_socket_is_established(int ssl, struct us_socket_t *s) {
    return 1;
}

/* Exactly the same as us_socket_close but does not emit on_close event */
struct us_socket_t *us_socket_close_connecting(int ssl, struct us_socket_t *s) {
    return s;
}

int us_socket_write2(int ssl, struct us_socket_t *s, const char *header, int header_length, const char *payload, int payload_length) {
    exit(1);
}

char *us_socket_send_buffer(int ssl, struct us_socket_t *s) {
    return s->sendBuf;
}

/* Same as above but emits on_close */
struct us_socket_t *us_socket_close(int ssl, struct us_socket_t *s, int code, void *reason) {
    return s;
}

/* Not shared with SSL */

void *us_socket_get_native_handle(int ssl, struct us_socket_t *s) {
    return 0;
}

int us_socket_write(int ssl, struct us_socket_t *s, const char *data, int length, int msg_more) {

    //printf("writing on socket now\n");

    if (data != s->sendBuf) {
        //printf("WHAT THE HECK!\n");
        memcpy(s->sendBuf, data, length);
    }
    
    
    struct io_uring_sqe *sqe = io_uring_get_sqe(&s->context->loop->ring);

    io_uring_prep_send(sqe, s->dd, s->sendBuf, length, 0);

    //io_uring_prep_write_fixed(sqe, s->dd, s->sendBuf, length, 0, s->dd);

    io_uring_sqe_set_flags(sqe, IOSQE_FIXED_FILE);
    io_uring_sqe_set_data(sqe, (char *)s + SOCKET_WRITE);

    return length;

}

void *us_socket_ext(int ssl, struct us_socket_t *s) {
    return s + 1;
}

int us_socket_is_shut_down(int ssl, struct us_socket_t *s) {
    return 0;
}

void us_socket_shutdown(int ssl, struct us_socket_t *s) {

}

#endif
