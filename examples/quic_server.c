/* experimental QUIC server */

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

#include <netinet/in.h>

#include "lsquic.h"

lsquic_engine_t *engine;


void on_server_read(struct us_udp_socket_t *s) {





    // returns how many packets
    int packets = us_udp_socket_receive(s, buf);
    printf("Packets: %d\n", packets);




    for (int i = 0; i < packets; i++) {


        // pass packets to lsquic




        // payload, length, peer addr (behöver inte veta längd bara void), local addr (vet redan), cong
        char *payload = us_udp_packet_buffer_payload(buf, i);
        int length = us_udp_packet_buffer_payload_length(buf, i);
        int ecn = us_udp_packet_buffer_ecn(buf, i);

        // viktig
        void *peer_addr = us_udp_packet_buffer_peer(buf, i);

        // use same addr as peer but change port
        /*struct sockaddr_storage local_addr;
        memcpy(&local_addr, peer_addr, sizeof(struct sockaddr_storage));
        struct sockaddr_in *addr = (struct sockaddr_in *) &local_addr;
        addr->sin_port = htons(5678);*/

        printf("passing packet to quic: %p, legth: %d\n", engine, length);
        int ret = lsquic_engine_packet_in(engine, payload, length, /*(struct sockaddr *) &local_addr*/ peer_addr, peer_addr, (void *) 12, 0);

        printf("ret: %d\n", ret);


    printf("processing conns\n");
    lsquic_engine_process_conns(engine);
    printf("done processing conns\n");





        //struct sockaddr_in *addr = peer_addr;
        //printf("Family: %d av %d\n", addr->sin_family, AF_INET);


        //printf("ip = %u, port = %hu\n", addr->sin_addr.s_addr, htons(addr->sin_port));

        //addr->sin_addr.s_addr = 16777343;

        //for (int k = 0; k < length; k++) {
            //printf("%c", payload[k]);
        //}
        //printf("\n");


        //us_udp_buffer_set_packet_payload(send_buf, i, payload, length, peer_addr);

        //printf("Sent: %d\n", sent);

    }



    //int sent = us_udp_socket_send(s, send_buf, packets);
    //printf("Sent: %d\n", sent);
}

/* Return number of packets sent or -1 on error */
static int
send_packets_out (void *ctx, const struct lsquic_out_spec *specs,
                                                unsigned n_specs)
{

    printf("sending packets!\n");
    return n_specs;

    struct msghdr msg;
    int sockfd;
    unsigned n;

    memset(&msg, 0, sizeof(msg));
    sockfd = (int) (uintptr_t) ctx;

    for (n = 0; n < n_specs; ++n)
    {
        msg.msg_name       = (void *) specs[n].dest_sa;
        msg.msg_namelen    = sizeof(struct sockaddr_in);
        msg.msg_iov        = specs[n].iov;
        msg.msg_iovlen     = specs[n].iovlen;
        if (sendmsg(sockfd, &msg, 0) < 0)
            break;
    }

    return (int) n;
}

lsquic_conn_ctx_t *on_new_conn(void *stream_if_ctx, lsquic_conn_t *c) {
    printf("new connn!\n");


}

void on_conn_closed(lsquic_conn_t *c) {

}

lsquic_stream_ctx_t *on_new_stream(void *stream_if_ctx, lsquic_stream_t *s) {
    printf("new stream!\n");
}

void on_read     (lsquic_stream_t *s, lsquic_stream_ctx_t *h) {

}

void on_write    (lsquic_stream_t *s, lsquic_stream_ctx_t *h) {

}

void on_close    (lsquic_stream_t *s, lsquic_stream_ctx_t *h) {
    
}

// this one is required for servers
struct ssl_ctx_st *get_ssl_ctx(void *peer_ctx, const struct sockaddr *local) {
    printf("getting ssl ctx now\n");

    return 13;
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

    /* Init lsquic engine */
    if (0 != lsquic_global_init(LSQUIC_GLOBAL_CLIENT|LSQUIC_GLOBAL_SERVER)) {
        exit(EXIT_FAILURE);
    }

    struct lsquic_stream_if stream_callbacks = {
        .on_close = on_close,
        .on_conn_closed = on_conn_closed,
        .on_write = on_write,
        .on_read = on_read,
        .on_new_stream = on_new_stream,
        .on_new_conn = on_new_conn
    };

    //memset(&stream_callbacks, 13, sizeof(struct lsquic_stream_if));



    struct lsquic_engine_api engine_api = {
        .ea_packets_out     = send_packets_out,
        .ea_packets_out_ctx = (void *) server,  /* For example */
        .ea_stream_if       = &stream_callbacks,
        .ea_stream_if_ctx   = server,

        .ea_get_ssl_ctx = get_ssl_ctx,
    };

    /* Create an engine in server mode with HTTP behavior: */
    engine = lsquic_engine_new(LSENG_SERVER/*|LSENG_HTTP*/, &engine_api);

    printf("Engine: %p\n", engine);

    /* Send packets from one UDP socket to the next, starting the loop */
    us_loop_run(loop);
}
