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

#define LIBUS_UDP_MAX_SIZE (64 * 1024)
#define LIBUS_UDP_MAX_NUM 1024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define _GNU_SOURCE
#include <sys/socket.h>

/* Internal structure of packet buffer */
struct us_internal_udp_packet_buffer {
    struct mmsghdr msgvec[LIBUS_UDP_MAX_NUM];
    struct iovec iov[LIBUS_UDP_MAX_NUM];
    struct sockaddr_storage addr[LIBUS_UDP_MAX_NUM];
};

WIN32_EXPORT int us_udp_packet_buffer_ecn(struct us_udp_packet_buffer_t *buf, int index) {
    return 0;
}

WIN32_EXPORT char *us_udp_packet_buffer_peer(struct us_udp_packet_buffer_t *buf, int index) {
    return ((struct mmsghdr *) buf)[index].msg_hdr.msg_name;
}

WIN32_EXPORT char *us_udp_packet_buffer_payload(struct us_udp_packet_buffer_t *buf, int index) {
    return ((struct mmsghdr *) buf)[index].msg_hdr.msg_iov[0].iov_base;
}

WIN32_EXPORT int us_udp_packet_buffer_payload_length(struct us_udp_packet_buffer_t *buf, int index) {
    return ((struct mmsghdr *) buf)[index].msg_len;
}

WIN32_EXPORT int us_udp_socket_send(struct us_udp_socket_t *s, struct us_udp_packet_buffer_t *buf, int num) {

    int fd = us_poll_fd((struct us_poll_t *) s);


    int ret = sendmmsg(fd, (struct mmsghdr *) buf, num, 0);

    return ret;
}

WIN32_EXPORT int us_udp_socket_receive(struct us_udp_socket_t *s, struct us_udp_packet_buffer_t *buf) {


    int fd = us_poll_fd((struct us_poll_t *) s);

    int ret = recvmmsg(fd, (struct mmsghdr *) buf, LIBUS_UDP_MAX_NUM, 0, 0);

    return ret;
}

#include <netinet/in.h>

void us_udp_buffer_set_packet_payload(struct us_udp_packet_buffer_t *send_buf, int index, int offset, void *payload, int length, void *peer_addr) {


        //printf("length: %d, offset: %d\n", length, offset);

        struct mmsghdr *ss = (struct mmsghdr *) send_buf;

        // copy the peer address
        memcpy(ss[index].msg_hdr.msg_name, peer_addr, /*ss[index].msg_hdr.msg_namelen*/ sizeof(struct sockaddr_in));

        // copy the payload
        
        ss[index].msg_hdr.msg_iov->iov_len = length + offset;


        memcpy(((char *) ss[index].msg_hdr.msg_iov->iov_base) + offset, payload, length);
}

/* The maximum UDP payload size is 64kb, but in IPV6 you can have jumbopackets larger than so.
 * We do not support those jumbo packets currently, but will safely ignore them.
 * Any sane sender would assume we don't support them if we consistently drop them.
 * Therefore a udp_packet_buffer_t will be 64 MB in size (64kb * 1024). */
WIN32_EXPORT struct us_udp_packet_buffer_t *us_create_udp_packet_buffer() {

    /* Allocate 64kb times 1024 */
    struct us_internal_udp_packet_buffer *b = malloc(sizeof(struct us_internal_udp_packet_buffer) + LIBUS_UDP_MAX_SIZE * LIBUS_UDP_MAX_NUM);

    for (int n = 0; n < LIBUS_UDP_MAX_NUM; ++n) {

        b->iov[n].iov_base = &((char *) (b + 1))[n * LIBUS_UDP_MAX_SIZE];
        b->iov[n].iov_len = LIBUS_UDP_MAX_SIZE;

        b->msgvec[n].msg_hdr = (struct msghdr) {
            .msg_name       = &b->addr,
            .msg_namelen    = sizeof (struct sockaddr_storage),

            .msg_iov        = &b->iov[n],
            .msg_iovlen     = 1,

            .msg_control    = 0,
            .msg_controllen = 0,
        };
    }

    return (struct us_udp_packet_buffer_t *) b;
}

WIN32_EXPORT struct us_udp_socket_t *us_create_udp_socket(struct us_loop_t *loop, void (*read_cb)(struct us_udp_socket_t *), unsigned short port) {
    
    
    LIBUS_SOCKET_DESCRIPTOR fd = bsd_create_udp_socket("127.0.0.1", port);

    if (fd == LIBUS_SOCKET_ERROR) {
        return 0;
    }

    //printf("UDP: %d\n", fd);

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