/* Simple usage of two UDP sockets sending messages to eachother like ping/pong */

#define _GNU_SOURCE
#include <sys/socket.h>

#include <libusockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/internal/internal.h"

void on_wakeup(struct us_loop_t *loop) {

}

void on_pre(struct us_loop_t *loop) {

}

void on_post(struct us_loop_t *loop) {

}




       /*int recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
                    int flags, struct timespec *timeout);*/

                    /*
                               struct mmsghdr {
               struct msghdr msg_hdr; 
               unsigned int  msg_len;  
           };
           */

          #include <errno.h>

void on_server_read(struct us_udp_socket_t *s) {
    printf("UDP socket is readable!\n");

    int fd = us_poll_fd((struct us_poll_t *) s);

    printf("fd: %d\n", fd);


    struct mmsghdr msgvec[16];

    //struct sockaddr_storage name;

    struct bsd_addr_t name;



    struct iovec iov[16];
    

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


    int ret = recvmmsg(fd, msgvec, 16, 0, 0);

    // en udp-socket behöver en egen outgoing-buffer i form av ring-buffer

    // eller så har du en allokerad buffer (per loop):
    // us_create_udp_receive_buffer();
    // us_udp_socket_receive(socket, buffer);

    // us_udp_socket_get_packet(buffer, id);

    // us_create_udp_send_buffer();
    // us_udp_socket_send(socket, buffer);

    // us_udp_send_buffer_write(buffer, payload, len);

    // int ret = us_udp_socket_begin_read()


/*
    int
lsquic_engine_packet_in (lsquic_engine_t *,
    const unsigned char *udp_payload, size_t sz,
    const struct sockaddr *sa_local, - denna blir bsd_addr_t *
    const struct sockaddr *sa_peer, - denna med!
    void *peer_ctx, int ecn);*/

    if (ret > 0) {
        for (int i = 0; i < ret; i++) {
            printf("Pakcet length: %d\n", msgvec[i].msg_len);
            printf("Peer address length: %d\n", msgvec[i].msg_hdr.msg_namelen);
            //printf("Length: %ld\n", msgvec[i].msg_hdr.msg_iov[0].iov_len);


            // din IP är den du band till från början; NAT rebindar den till din publika IP
            // alltså kan du använda bsd_local_addr?

            struct bsd_addr_t local_addr;
            bsd_local_addr(fd, &local_addr);
            int local_port = bsd_addr_get_port(&local_addr);
            printf("Local port: %d\n", local_port);


            internal_finalize_bsd_addr(&name);

            int remote_port = bsd_addr_get_port(&name);

            int ip_length = bsd_addr_get_ip_length(&name);
            char *ip = bsd_addr_get_ip(&name);

            printf("peer ip length: %d\n", ip_length);

            unsigned int *ipnr = (unsigned int *) ip;

            printf("peer ip nr: %ld\n", *ipnr);

            printf("Peer port: %d\n", remote_port);

            // we need to get peer address, local address, int ecn, udp payload

            for (int k = 0; k < msgvec[i].msg_len; k++) {
                printf("%c", ((char *) msgvec[i].msg_hdr.msg_iov[0].iov_base)[k]);
            }
            printf("\n");
        }
    } else {
        printf("errno: %d\n", errno);
    }

    printf("Read: %d pakcets?\n", ret);



}

int main() {
	/* Create the event loop */
	struct us_loop_t *loop = us_create_loop(0, on_wakeup, on_pre, on_post, 0);

    /* Create two UDP sockets and bind them to their respective ports */
    struct us_udp_socket_t *server = us_create_udp_socket(loop, on_server_read);

    us_udp_socket_bind(server, "127.0.0.1", 5678);

    printf("Server socket: %p\n", server);

    us_loop_run(loop);
}
