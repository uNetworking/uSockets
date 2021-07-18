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
#include <stdlib.h>

#define _GNU_SOURCE
#include <sys/socket.h>

// samma buffer för receive och send, samma get, set?

// en buffer med 16 paket - getta vilket du vill efter ID
// sätt vilkt du vill med id?

WIN32_EXPORT int us_udp_packet_buffer_ecn(struct us_udp_packet_buffer_t *buf, int index) {
    return 0;
}

WIN32_EXPORT char *us_udp_packet_buffer_peer(struct us_udp_packet_buffer_t *buf, int index) {
    return ((struct mmsghdr *) buf)[index].msg_hdr.msg_name;
}

WIN32_EXPORT char *us_udp_packet_buffer_payload(struct us_udp_packet_buffer_t *buf, int index) {

    return ((struct mmsghdr *) buf)[index].msg_hdr.msg_iov[0].iov_base;

/*
    return (char *) msgvec[i].msg_hdr.msg_iov[0].iov_base;

                printf("Pakcet length: %d\n", msgvec[i].msg_len);

            for (int k = 0; k < msgvec[i].msg_len; k++) {
                printf("%c", ((char *) msgvec[i].msg_hdr.msg_iov[0].iov_base)[k]);
            }
*/
}

WIN32_EXPORT int us_udp_packet_buffer_payload_length(struct us_udp_packet_buffer_t *buf, int index) {
    return ((struct mmsghdr *) buf)[index].msg_len;
}

WIN32_EXPORT int us_udp_socket_receive(struct us_udp_socket_t *s, struct us_udp_packet_buffer_t *buf) {

        printf("UDP socket is readable!\n");

    int fd = us_poll_fd((struct us_poll_t *) s);

    printf("fd: %d\n", fd);

    int ret = recvmmsg(fd, (struct mmsghdr *) buf, 16, 0, 0);

    return ret;
}


WIN32_EXPORT struct us_udp_packet_buffer_t *us_create_udp_packet_buffer() {






    static struct mmsghdr msgvec[16];

    //struct sockaddr_storage name;

    struct bsd_addr_t name;



    static struct iovec iov[16];
    

    /* Initialize mmsghdrs */
    for (int n = 0; n < 16; ++n)
    {

        iov[n].iov_base = malloc(1024 * 16);
        iov[n].iov_len = 1024 * 16;


        msgvec[n].msg_hdr = (struct msghdr) {
            .msg_name       = &name,
            .msg_namelen    = sizeof (name),

            .msg_iov        = &iov[n],
            .msg_iovlen     = 1,

            .msg_control    = 0,
            .msg_controllen = 0,
        };
    }




    return (struct us_udp_packet_buffer_t *) msgvec;


}
    // us_udp_socket_receive(socket, buffer);

    // us_udp_socket_get_packet(buffer, id);


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