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

// håller en buffer efter detta (16k * 512)
struct us_internal_udp_packet_buffer {
    struct mmsghdr msgvec[512];
    struct iovec iov[512];
    struct sockaddr_storage addr[512];
};

void us_udp_buffer_set_packet_payload(struct us_udp_packet_buffer_t *send_buf, int index, void *payload, int length, void *peer_addr) {
        // struct us_udp_packet_t

        // copy whatever the payload is, to send buffer


        


        struct mmsghdr *ss = (struct mmsghdr *) send_buf;


        // send buffern ska packa utgående packet tätt, inte i per-16kb, den ska alltså inte ha en fix längd


        // en sändbuffer ska alltså ha offset i byte och en 


        // copy the peer address
        memcpy(ss[index].msg_hdr.msg_name, peer_addr, ss[index].msg_hdr.msg_namelen);

        // move iov_base packed

        // copy the payload
        ss[index].msg_hdr.msg_iov->iov_len = length;



        // copy to base
        memcpy(ss[index].msg_hdr.msg_iov->iov_base, payload, length);


        //ss->msg_hdr.msg_iov->iov_base

        //memcpy(ss->msg_hdr.msg_iov->iov_base)


        //ss->msg_len = 12; - denna håller hur mycket av header som har sänts, kan inte sättas av oss
        


}

#include <netinet/in.h>

void on_client_read(struct us_udp_socket_t *s) {
    printf("on client read\n");

    int packets = us_udp_socket_receive(s, buf);

    printf("Packets: %d\n", packets);

    for (int i = 0; i < packets; i++) {

    }
}

int messages = 0;

void timer_cb(struct us_timer_t *timer) {
    printf("Messages per second (either side!): %d\n", messages);
    messages = 0;
}

void on_server_read(struct us_udp_socket_t *s) {





    // returns how many packets
    int packets = us_udp_socket_receive(s, buf);

    //printf("Packets: %d\n", packets);

    for (int i = 0; i < packets; i++) {

        // payload, length, peer addr (behöver inte veta längd bara void), local addr (vet redan), cong
        char *payload = us_udp_packet_buffer_payload(buf, i);
        int length = us_udp_packet_buffer_payload_length(buf, i);
        int ecn = us_udp_packet_buffer_ecn(buf, i);

        // viktig
        void *peer_addr = us_udp_packet_buffer_peer(buf, i);
        //struct sockaddr_in *addr = peer_addr;
        //printf("Family: %d av %d\n", addr->sin_family, AF_INET);


        //printf("ip = %u, port = %hu\n", addr->sin_addr.s_addr, htons(addr->sin_port));

        //addr->sin_addr.s_addr = 16777343;

        //for (int k = 0; k < length; k++) {
            //printf("%c", payload[k]);
        //}
        //printf("\n");


        us_udp_buffer_set_packet_payload(send_buf, i, payload, length, peer_addr);

        //printf("Sent: %d\n", sent);


        messages++;
    }

    int sent = us_udp_socket_send(s, send_buf, packets);
    //printf("Sent: %d\n", sent);
}

int main() {
    /* Allocate per thread, UDP packet buffers */
    buf = us_create_udp_packet_buffer();
    send_buf = us_create_udp_packet_buffer();

	/* Create the event loop */
	struct us_loop_t *loop = us_create_loop(0, on_wakeup, on_pre, on_post, 0);

    /* Create two UDP sockets and bind them to their respective ports */
    struct us_udp_socket_t *server = us_create_udp_socket(loop, on_server_read, 5678);
    printf("Server socket: %p\n", server);

    struct us_udp_socket_t *client = us_create_udp_socket(loop, on_server_read, 5679);

    /* Send first packet from client to server */
    struct sockaddr_storage storage;
    struct sockaddr_in *addr = (struct sockaddr_in *) &storage;

    addr->sin_addr.s_addr = 16777343;
    addr->sin_port = htons(5678);
    addr->sin_family = AF_INET;

    for (int i = 0; i < 100; i++) {
        us_udp_buffer_set_packet_payload(send_buf, i, "Hello UDP!", 10, &storage);
    }


    int sent = us_udp_socket_send(client, send_buf, 100); // buffer should know how many it holds!
    printf("Sent: %d\n", sent);

    /* Start a counting timer */
    struct us_timer_t *timer = us_create_timer(loop, 0, 0);
    us_timer_set(timer, timer_cb, 1000, 1000);

    /* Send packets from one UDP socket to the next, starting the loop */
    us_loop_run(loop);
}
