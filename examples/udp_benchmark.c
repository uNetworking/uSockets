/* Simple usage of two UDP sockets sending messages to eachother like ping/pong */
#include <libusockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef LIBUS_USE_IO_URING

/* This one we allow here since we use it to create the peer addr.
 * We should remove this and replace it with bsd_addr_t and a builder function */
#include <netinet/in.h>

struct us_udp_packet_buffer_t *send_buf;
float messages = 0;

void on_wakeup(struct us_loop_t *loop) {

}

void on_pre(struct us_loop_t *loop) {

}

void on_post(struct us_loop_t *loop) {
    /* It can be a good idea to use this callback
     * for sending off what we have in our outgoing buffer */
}

void timer_cb(struct us_timer_t *timer) {
    printf("Messages per second: %f\n", messages);
    messages = 0;
}

/* Called whenever you can write more datagrams after a failure to write */
void on_server_drain(struct us_udp_socket_t *s) {

}

/* Called whenever there are received datagrams for the app to consume */
void on_server_data(struct us_udp_socket_t *s, struct us_udp_packet_buffer_t *buf, int packets) {
    /* Iterate all received packets */
    for (int i = 0; i < packets; i++) {
        char *payload = us_udp_packet_buffer_payload(buf, i);
        int length = us_udp_packet_buffer_payload_length(buf, i);
        int ecn = us_udp_packet_buffer_ecn(buf, i);
        void *peer_addr = us_udp_packet_buffer_peer(buf, i);

        //printf("ECN is: %d\n", ecn);


        char ip[16];
        int ip_length = us_udp_packet_buffer_local_ip(buf, i, ip);
        if (!ip_length) {
            printf("We got no ip on received packet!\n");
            exit(0);
        }

        //printf("Our received destination IP length is: %d\n", ip_length);

        int port = us_udp_socket_bound_port(s);
        //printf("We received packet on port: %d\n", port);

        /* Echo it back */
        us_udp_buffer_set_packet_payload(send_buf, i, 0, payload, length, peer_addr);
        
        /* Let's count a one whole message as one whole roundtrip for easier comparison with TCP echo benchmark */
        messages += 1;
    }

    int sent = us_udp_socket_send(s, send_buf, packets);
}

int main(int argc, char **argv) {

    int is_client = 0;
    int is_ipv6 = 0;
    if (argc > 1 && !strcmp(argv[argc - 1], "ipv6")) {
        is_ipv6 = 1;
        printf("Using IPv6 UDP\n");
    }
    if (argc >= 2 && !strcmp(argv[1], "client")) {
        is_client = 1;
        printf("Running as client\n");
    } else {
        printf("Running as server\n");
    }

    /* Allocate per thread, UDP packet buffers */
    struct us_udp_packet_buffer_t *receive_buf = us_create_udp_packet_buffer();
    /* We also want a send buffer we can assemble while iterating the read buffer */
    send_buf = us_create_udp_packet_buffer();

	/* Create the event loop */
	struct us_loop_t *loop = us_create_loop(0, on_wakeup, on_pre, on_post, 0);

    /* Create two UDP sockets and bind them to their respective ports */
    struct us_udp_socket_t *server;
    struct us_udp_socket_t *client;
    
    if (is_client) {
        if (is_ipv6) {
            client = us_create_udp_socket(loop, receive_buf, on_server_data, on_server_drain, "::1", 0, 0);
        } else {
            client = us_create_udp_socket(loop, receive_buf, on_server_data, on_server_drain, "127.0.0.1", 0, 0);
        }
    } else {
        if (is_ipv6) {
            server = us_create_udp_socket(loop, receive_buf, on_server_data, on_server_drain, "::1", 5678, 0);
        } else {
            server = us_create_udp_socket(loop, receive_buf, on_server_data, on_server_drain, "127.0.0.1", 5678, 0);
        }
    }
    if (!client && !server) {
        printf("Failed to create UDP sockets!\n");
        return 1;
    }

    /* Send first packets from client to server */

    /* This is ugly and needs to be wrapped in bsd_addr_t */
    struct sockaddr_storage storage = {};
    if (is_ipv6) {
        struct sockaddr_in6 *addr = (struct sockaddr_in6 *) &storage;
        addr->sin6_addr.s6_addr[15] = 1;
        addr->sin6_port = htons(5678);
        addr->sin6_family = AF_INET6;
    } else {
        struct sockaddr_in *addr = (struct sockaddr_in *) &storage;
        addr->sin_addr.s_addr = 16777343;
        addr->sin_port = htons(5678);
        addr->sin_family = AF_INET;
    }

    /* Send initial message batch */
    if (is_client) {
        for (int i = 0; i < 40; i++) {
            us_udp_buffer_set_packet_payload(send_buf, i, 0, "Hello UDP!", 10, &storage);
        }
        int sent = us_udp_socket_send(client, send_buf, 40);
        printf("Sent initial packets: %d\n", sent);
    }

    /* Start a counting timer */
    struct us_timer_t *timer = us_create_timer(loop, 0, 0);
    us_timer_set(timer, timer_cb, 1000, 1000);

    /* Send packets from one UDP socket to the next, starting the loop */
    us_loop_run(loop);
}

#else

int main() {
    printf("Not yet available with io_uring backend\n");
}

#endif