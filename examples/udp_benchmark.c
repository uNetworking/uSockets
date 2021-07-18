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

struct us_udp_packet_buffer_t *buf;
struct us_udp_packet_buffer_t *send_buf;
int outgoing_packets = 0;

void on_post(struct us_loop_t *loop) {
    // send whatever in buffer here

    // us_udp_socket_send(s, send_buf, 3);
}


struct us_internal_udp_packet_buffer {
    struct mmsghdr msgvec[512];
    struct iovec iov[512];
    struct sockaddr_storage addr[512];
};


void on_server_read(struct us_udp_socket_t *s) {


    // returns how many packets
    int packets = us_udp_socket_receive(s, buf);

    printf("Packets: %d\n", packets);

    for (int i = 0; i < packets; i++) {

        // payload, length, peer addr (behöver inte veta längd bara void), local addr (vet redan), cong
        char *payload = us_udp_packet_buffer_payload(buf, i);
        int length = us_udp_packet_buffer_payload_length(buf, i);
        int ecn = us_udp_packet_buffer_ecn(buf, i);

        // viktig
        void *peer_addr = us_udp_packet_buffer_peer(buf, i);
        struct sockaddr *addr = peer_addr;
        printf("Family: %d av %d\n", addr->sa_family, AF_INET);


        for (int k = 0; k < length; k++) {
            printf("%c", payload[k]);
        }
        printf("\n");

        // struct us_udp_packet_t

        // copy whatever the payload is, to send buffer

        struct mmsghdr *ss = send_buf;


        // send buffern ska packa utgående packet tätt, inte i per-16kb, den ska alltså inte ha en fix längd


        // en sändbuffer ska alltså ha offset i byte och en 


        // copy the peer address
        memcpy(ss->msg_hdr.msg_name, peer_addr, ss->msg_hdr.msg_namelen);

        //ss->msg_len = 12; - denna håller hur mycket av header som har sänts, kan inte sättas av oss
        

        int sent = us_udp_socket_send(s, send_buf);
        printf("Sent: %d\n", sent);


    }

    // us_create_udp_send_buffer();
    // us_udp_socket_send(socket, buffer);
    // us_udp_send_buffer_write(buffer, payload, len);
}

int main() {
    /* Allocate per thread, UDP packet buffers */
    buf = us_create_udp_packet_buffer();
    send_buf = us_create_udp_packet_buffer();

	/* Create the event loop */
	struct us_loop_t *loop = us_create_loop(0, on_wakeup, on_pre, on_post, 0);

    /* Create two UDP sockets and bind them to their respective ports */
    struct us_udp_socket_t *server = us_create_udp_socket(loop, on_server_read);
    //us_udp_socket_bind(server, "127.0.0.1", 5678);



    printf("Server socket: %p\n", server);

    /* Send packets from one UDP socket to the next, starting the loop */
    us_loop_run(loop);
}
