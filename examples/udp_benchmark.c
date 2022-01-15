/* Simple usage of two UDP sockets sending messages to eachother like ping/pong */

#define _GNU_SOURCE
#include <sys/socket.h>

#include <libusockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netinet/in.h>

#include "../src/internal/internal.h"

void on_wakeup(struct us_loop_t *loop) {

}

void on_pre(struct us_loop_t *loop) {

}

struct us_udp_packet_buffer_t *buf;
struct us_udp_packet_buffer_t *send_buf;

void on_post(struct us_loop_t *loop) {
    // send whatever in buffer here

    // us_udp_socket_send(s, send_buf, 3);
}

int messages = 0;

void timer_cb(struct us_timer_t *timer) {
    printf("Messages per second (either side!): %d\n", messages);
    messages = 0;
}

void on_server_read(struct us_udp_socket_t *s) {

    int packets = us_udp_socket_receive(s, buf);
    //printf("Packets: %d\n", packets);

    for (int i = 0; i < packets; i++) {
        // payload, length, peer addr (behöver inte veta längd bara void), local addr (vet redan), cong
        char *payload = us_udp_packet_buffer_payload(buf, i);
        int length = us_udp_packet_buffer_payload_length(buf, i);
        int ecn = us_udp_packet_buffer_ecn(buf, i);
        void *peer_addr = us_udp_packet_buffer_peer(buf, i);

        us_udp_buffer_set_packet_payload(send_buf, i, 0, payload, length, peer_addr);
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
        us_udp_buffer_set_packet_payload(send_buf, i, 0, "Hello UDP!", 10, &storage);
    }


    int sent = us_udp_socket_send(client, send_buf, 100); // buffer should know how many it holds!
    printf("Sent: %d\n", sent);

    /* Start a counting timer */
    struct us_timer_t *timer = us_create_timer(loop, 0, 0);
    us_timer_set(timer, timer_cb, 1000, 1000);

    /* Send packets from one UDP socket to the next, starting the loop */
    us_loop_run(loop);
}
