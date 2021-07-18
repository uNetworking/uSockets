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

#include "libusockets.h"
#include "internal/internal.h"

#include <stdio.h>

WIN32_EXPORT struct us_udp_socket_t *us_create_udp_socket(struct us_loop_t *loop, void (*read_cb)(struct us_udp_socket_t *)) {
    
    
    LIBUS_SOCKET_DESCRIPTOR fd = bsd_create_udp_socket("127.0.0.1", 5678);

    printf("UDP: %d\n", fd);

    int ext_size = 0;
    int fallthrough = 0;

    struct us_poll_t *p = us_create_poll(loop, fallthrough, sizeof(struct us_internal_callback_t) + ext_size);
    us_poll_init(p, fd, POLL_TYPE_CALLBACK);

    struct us_internal_callback_t *cb = (struct us_internal_callback_t *) p;
    cb->loop = loop;
    cb->cb_expects_the_loop = 0;
    cb->leave_poll_ready = 1;

    cb->cb = (void (*)(struct us_internal_callback_t *)) read_cb;

    us_poll_start((struct us_poll_t *) cb, cb->loop, LIBUS_SOCKET_READABLE);
    
    return (struct us_udp_socket_t *) cb;
}

// not in use?
WIN32_EXPORT int us_udp_socket_bind(struct us_udp_socket_t *s, const char *hostname, unsigned int port) {

}