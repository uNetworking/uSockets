/* experimental QUIC server */

#define _GNU_SOURCE
#include <sys/socket.h>

#include <libusockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/internal/internal.h"

#define printf

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

    printf("processing conns\n");
    lsquic_engine_process_conns(engine);
    printf("done processing conns\n");

    //int sent = us_udp_socket_send(s, send_buf, packets);
    //printf("Sent: %d\n", sent);
}

int udp_fd;

struct us_udp_socket_t *server;


/* Return number of packets sent or -1 on error */
static int
send_packets_out (void *ctx, const struct lsquic_out_spec *specs,
                                                unsigned n_specs)
{

goto better;

    for (int n = 0; n < n_specs; ++n)
    {

        int offset = 0;

        printf("antal scatters: %d\n", specs[n].iovlen);

        for (int i = 0; i < specs[n].iovlen; i++) {
            us_udp_buffer_set_packet_payload(send_buf, n, offset, specs[n].iov[i].iov_base, specs[n].iov[i].iov_len, (void *) specs[n].dest_sa);

            offset += specs[n].iov[i].iov_len;
            printf("offset = %d\n", offset);
        }



        printf("final offset = %d\n", offset);

        int sent = us_udp_socket_send(server, send_buf, 1);

        // appenda (gather) alla iov till en enda linjär buffer för detta paket

        // lägg till offset!

        

        /*msg.msg_name       = (void *) specs[n].dest_sa;
        msg.msg_namelen    = sizeof(struct sockaddr_in);
        msg.msg_iov        = specs[n].iov;
        msg.msg_iovlen     = specs[n].iovlen;
        if (sendmsg(sockfd, &msg, 0) < 0)
            break;*/
    }

    // ska ske i post!
    //int sent = us_udp_socket_send(server, send_buf, n_specs);

    return n_specs;


better:

    {
        struct mmsghdr msgs[512] = {};

        //struct msghdr msg;
        int sockfd = udp_fd;
        unsigned n;

        

        for (n = 0; n < n_specs; ++n)
        {
            memset(&msgs[n], 0, sizeof(struct mmsghdr));
            msgs[n].msg_hdr.msg_name       = (void *) specs[n].dest_sa;
            msgs[n].msg_hdr.msg_namelen    = sizeof(struct sockaddr_in);
            msgs[n].msg_hdr.msg_iov        = specs[n].iov;
            msgs[n].msg_hdr.msg_iovlen     = specs[n].iovlen;
            /*if (sendmsg(sockfd, &msg, 0) < 0)
                break;*/
        }

        n = sendmmsg(sockfd, msgs, n_specs, 0);

        printf("Sent: %d\n", n);

        if (n != n_specs) {
            printf("CANNOT SEND PACKETS!\n");
            exit(0);
        }

        return (int) n;
    }


    printf("outgoing packets: %d\n", n_specs);




    {
        struct msghdr msg;
        int sockfd = udp_fd;
        unsigned n;

        memset(&msg, 0, sizeof(msg));

        for (n = 0; n < n_specs; ++n)
        {
            msg.msg_name       = (void *) specs[n].dest_sa;
            msg.msg_namelen    = sizeof(struct sockaddr_in);
            msg.msg_iov        = specs[n].iov;
            msg.msg_iovlen     = specs[n].iovlen;
            if (sendmsg(sockfd, &msg, 0) < 0)
                break;
        }



        printf("Sent: %d\n", n);

        if (n != n_specs) {
            printf("CANNOT SEND PACKETS!\n");
            exit(0);
        }

        return (int) n;
    }
}

lsquic_conn_ctx_t *on_new_conn(void *stream_if_ctx, lsquic_conn_t *c) {
    printf("new connn!\n");


}

void on_conn_closed(lsquic_conn_t *c) {
    printf("conn closed!\n");
}

lsquic_stream_ctx_t *on_new_stream(void *stream_if_ctx, lsquic_stream_t *s) {
    printf("new stream!\n");

    lsquic_stream_wantread(s, 1);
}

void on_read(lsquic_stream_t *s, lsquic_stream_ctx_t *h) {


    printf("on_read?\n");

    char temp[512] = {};


    int nr = lsquic_stream_read(s, temp, 512);
    printf("read: %d\n", nr);

    printf("%s\n", temp);

    lsquic_stream_write(s, temp, nr);
    lsquic_stream_flush(s);

}

void on_write    (lsquic_stream_t *s, lsquic_stream_ctx_t *h) {

}

void on_close    (lsquic_stream_t *s, lsquic_stream_ctx_t *h) {
    
}

#include "openssl/ssl.h"

static char s_alpn[0x100];

int
add_alpn (const char *alpn)
{
    size_t alpn_len, all_len;

    alpn_len = strlen(alpn);
    if (alpn_len > 255)
        return -1;

    all_len = strlen(s_alpn);
    if (all_len + 1 + alpn_len + 1 > sizeof(s_alpn))
        return -1;

    s_alpn[all_len] = alpn_len;
    memcpy(&s_alpn[all_len + 1], alpn, alpn_len);
    s_alpn[all_len + 1 + alpn_len] = '\0';
    return 0;
}

static int select_alpn (SSL *ssl, const unsigned char **out, unsigned char *outlen,
                    const unsigned char *in, unsigned int inlen, void *arg) {
    int r;

    r = SSL_select_next_proto((unsigned char **) out, outlen, in, inlen,
                                    (unsigned char *) s_alpn, strlen(s_alpn));
    if (r == OPENSSL_NPN_NEGOTIATED) {
        printf("OK?\n");
        return SSL_TLSEXT_ERR_OK;
    }
    else
    {
        printf("no supported protocol can be selected!\n");
        //LSQ_WARN("no supported protocol can be selected from %.*s",
                                                    //(int) inlen, (char *) in);
        return SSL_TLSEXT_ERR_ALERT_FATAL;
    }
}

SSL_CTX *old_ctx;

// this one is required for servers
struct ssl_ctx_st *get_ssl_ctx(void *peer_ctx, const struct sockaddr *local) {
    printf("getting ssl ctx now\n");

    if (old_ctx) {
        return old_ctx;
    }


    SSL_CTX *ctx = SSL_CTX_new(TLS_method());

    old_ctx = ctx;

    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
    
    SSL_CTX_set_default_verify_paths(ctx);
    SSL_CTX_set_alpn_select_cb(ctx, select_alpn, NULL);

    int a = SSL_CTX_use_certificate_chain_file(ctx, "/home/alexhultman/uWebSockets.js/misc/cert.pem");
    int b = SSL_CTX_use_PrivateKey_file(ctx, "/home/alexhultman/uWebSockets.js/misc/key.pem", SSL_FILETYPE_PEM);

    printf("%d, %d\n", a, b);

    return ctx;
}

int main() {
    /* Allocate per thread, UDP packet buffers */
    buf = us_create_udp_packet_buffer();
    send_buf = us_create_udp_packet_buffer();

	/* Create the event loop */
	struct us_loop_t *loop = us_create_loop(0, on_wakeup, on_pre, on_post, 0);

    /* Create two UDP sockets and bind them to their respective ports */
    server = us_create_udp_socket(loop, on_server_read, 5678);
    printf("Server socket: %p\n", server);

    udp_fd = us_poll_fd(server);

    printf("udp fd: %d\n", udp_fd);

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


add_alpn("echo");

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
