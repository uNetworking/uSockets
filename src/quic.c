#ifdef LIBUS_USE_QUIC

#include "quic.h"

#define _GNU_SOURCE
#include <sys/socket.h>

#include "lsquic.h"
#include "lsquic_types.h"
#include "lsxpack_header.h"

/* This one is really only used to set inet addresses */
#include <netinet/in.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct us_udp_packet_buffer_t *buf;
struct us_udp_packet_buffer_t *send_buf;
int outgoing_packets = 0;
lsquic_engine_t *engine;
int udp_fd;
struct us_udp_socket_t *server;

void on_server_drain(struct us_udp_socket_t *s) {

}

void on_server_data(struct us_udp_socket_t *s, struct us_udp_packet_buffer_t *buf, int packets) {
    // returns how many packets
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
    }

    printf("processing conns\n");
    lsquic_engine_process_conns(engine);
    printf("done processing conns\n");
}

/* Return number of packets sent or -1 on error */
int send_packets_out(void *ctx, const struct lsquic_out_spec *specs, unsigned n_specs) {

    printf("sending packets out\n");

    // We need to make a nice faster interface from lsquic to uSockets send that
    // can immediately take these specs pretty much unchanged
    // should be simple to add a second send function for this in uSockets

    struct mmsghdr msgs[512] = {};

    int sockfd = udp_fd;
    unsigned n;

    for (n = 0; n < n_specs; ++n)
    {
        memset(&msgs[n], 0, sizeof(struct mmsghdr));

        msgs[n].msg_hdr.msg_name       = (void *) specs[n].dest_sa;
        msgs[n].msg_hdr.msg_namelen    = sizeof(struct sockaddr_in);
        msgs[n].msg_hdr.msg_iov        = specs[n].iov;
        msgs[n].msg_hdr.msg_iovlen     = specs[n].iovlen;
    }

    n = sendmmsg(sockfd, msgs, n_specs, 0);
    printf("Sent: %d\n", n);

    if (n != n_specs) {
        printf("CANNOT SEND PACKETS!\n");
        exit(0);
    }

    return (int) n;
}

lsquic_conn_ctx_t *on_new_conn(void *stream_if_ctx, lsquic_conn_t *c) {
    printf("New connection!\n");

    return 0;
}

void on_conn_closed(lsquic_conn_t *c) {
    printf("Connection closed!\n");
}

lsquic_stream_ctx_t *on_new_stream(void *stream_if_ctx, lsquic_stream_t *s) {
    printf("New stream!\n");
    lsquic_stream_wantread(s, 1);

    return 0;
}

// this would be the application logic of the echo server
// this function should emit the quic message to the high level application
void on_read(lsquic_stream_t *s, lsquic_stream_ctx_t *h) {

    printf("lsquick on_read for stream: %p\n", s);

    char temp[4096] = {};

    int nr = lsquic_stream_read(s, temp, 4096);
    printf("read: %d\n", nr);

    printf("%s\n", temp);

    // why do we get tons of zero reads?
    // maybe it doesn't matter, if we can parse this input then we are fine
    lsquic_stream_wantread(s, 0);
    lsquic_stream_wantwrite(s, 1);

    // respond minimally here I guess
    //h3_response(s, h);




    //lsquic_stream_write(s, "Hello QUIC!", 11);







    //exit(0);

    //lsquic_stream_write(s, temp, nr);
    //lsquic_stream_flush(s);
}

#define V(v) (v), strlen(v)

struct header_buf
{
    unsigned    off;
    char        buf[UINT16_MAX];
};

int
header_set_ptr (struct lsxpack_header *hdr, struct header_buf *header_buf,
                const char *name, size_t name_len,
                const char *val, size_t val_len)
{
    if (header_buf->off + name_len + val_len <= sizeof(header_buf->buf))
    {
        memcpy(header_buf->buf + header_buf->off, name, name_len);
        memcpy(header_buf->buf + header_buf->off + name_len, val, val_len);
        lsxpack_header_set_offset2(hdr, header_buf->buf + header_buf->off,
                                            0, name_len, name_len, val_len);
        header_buf->off += name_len + val_len;
        return 0;
    }
    else
        return -1;
}

static int
send_headers2 (struct lsquic_stream *stream, struct lsquic_stream_ctx *st_h,
                    size_t content_len)
{
    char clbuf[0x20];
    struct header_buf hbuf;

    snprintf(clbuf, sizeof(clbuf), "%zd", content_len);

    hbuf.off = 0;
    struct lsxpack_header  headers_arr[4];
    header_set_ptr(&headers_arr[0], &hbuf, V(":status"), V("200"));
    header_set_ptr(&headers_arr[1], &hbuf, V("server"), V("uSockets"));
    header_set_ptr(&headers_arr[2], &hbuf, V("content-type"), V("text/html"));
    header_set_ptr(&headers_arr[3], &hbuf, V("content-length"), V(clbuf));
    lsquic_http_headers_t headers = {
        .count = sizeof(headers_arr) / sizeof(headers_arr[0]),
        .headers = headers_arr,
    };

    return lsquic_stream_send_headers(stream, &headers, 0);
}

void on_write (lsquic_stream_t *s, lsquic_stream_ctx_t *h) {

    printf("Sending response in on_write now\n");

/*
    static struct lsxpack_header packed_headers[2] = {{}, {}};

    // set status 200
    lsxpack_header_set_qpack_idx(&packed_headers[0], 25, "", 0);
    // no content length
    lsxpack_header_set_qpack_idx(&packed_headers[1], 4, "", 0);

    static lsquic_http_headers_t headers = {
        .count = 1,
        .headers = packed_headers,
    };

    printf("Sending headers: %d\n", lsquic_stream_send_headers(s, &headers, 0));*/

    send_headers2(s, h, 11);

    printf("write: %d\n", lsquic_stream_write(s, "Hello QUIC!", 11));


    lsquic_stream_shutdown(s, 1);
    //lsquic_stream_flush(s);

    lsquic_stream_wantwrite(s, 0);
    lsquic_stream_wantread(s, 1);
}

void on_close (lsquic_stream_t *s, lsquic_stream_ctx_t *h) {
    
}

#include "openssl/ssl.h"

static char s_alpn[0x100];

int add_alpn (const char *alpn)
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

static int select_alpn(SSL *ssl, const unsigned char **out, unsigned char *outlen,
                    const unsigned char *in, unsigned int inlen, void *arg) {
    int r;

    printf("select_alpn\n");

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

int server_name_cb(SSL *s, int *al, void *arg) {
    printf("yolo SNI server_name_cb\n");

    SSL_set_SSL_CTX(s, old_ctx);

    printf("existing name is: %s\n", SSL_get_servername(s, TLSEXT_NAMETYPE_host_name));

    SSL_set_tlsext_host_name(s, "YOLO NAME!");

    printf("set name is: %s\n", SSL_get_servername(s, TLSEXT_NAMETYPE_host_name));


    return SSL_TLSEXT_ERR_OK;
}

// this one is required for servers
struct ssl_ctx_st *get_ssl_ctx(void *peer_ctx, const struct sockaddr *local) {
    printf("getting ssl ctx now\n");

    if (old_ctx) {
        return old_ctx;
    }


    SSL_CTX *ctx = SSL_CTX_new(TLS_method());

    old_ctx = ctx;

    //SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    //SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
    
    //SSL_CTX_set_default_verify_paths(ctx);
    
    // probably cannot use this when http is in use?
    // alpn is needed
    SSL_CTX_set_alpn_select_cb(ctx, select_alpn, NULL);

    // sni is needed
    SSL_CTX_set_tlsext_servername_callback(ctx, server_name_cb);
 //long SSL_CTX_set_tlsext_servername_arg(SSL_CTX *ctx, void *arg);

    int a = SSL_CTX_use_certificate_chain_file(ctx, "/home/alexhultman/uWebSockets.js/misc/cert.pem");
    int b = SSL_CTX_use_PrivateKey_file(ctx, "/home/alexhultman/uWebSockets.js/misc/key.pem", SSL_FILETYPE_PEM);

    printf("%d, %d\n", a, b);

    return ctx;
}

SSL_CTX *sni_lookup(void *lsquic_cert_lookup_ctx, const struct sockaddr *local, const char *sni) {
    printf("simply returning old ctx in sni\n");
    return old_ctx;
}

int log_buf_cb(void *logger_ctx, const char *buf, size_t len) {
    printf("%.*s\n", len, buf);
    return 0;
}

us_quic_socket_context_t *us_create_quic_socket_context(struct us_loop_t *loop, us_quic_socket_context_options_t options) {

    /* Create two UDP sockets and bind them to their respective ports */
    server = us_create_udp_socket(loop, buf, on_server_data, on_server_drain, "172.25.224.221", 9004);
    printf("Server socket: %p\n", server);

    udp_fd = us_poll_fd((struct us_poll_t *)server);

    printf("udp fd: %d\n", udp_fd);

    /* Init lsquic engine */
    if (0 != lsquic_global_init(/*LSQUIC_GLOBAL_CLIENT|*/LSQUIC_GLOBAL_SERVER)) {
        exit(EXIT_FAILURE);
    }

    static struct lsquic_stream_if stream_callbacks = {
        .on_close = on_close,
        .on_conn_closed = on_conn_closed,
        .on_write = on_write,
        .on_read = on_read,
        .on_new_stream = on_new_stream,
        .on_new_conn = on_new_conn
    };

    //memset(&stream_callbacks, 13, sizeof(struct lsquic_stream_if));


    add_alpn("h3");

    struct lsquic_engine_api engine_api = {
        .ea_packets_out     = send_packets_out,
        .ea_packets_out_ctx = (void *) server,  /* For example */
        .ea_stream_if       = &stream_callbacks,
        .ea_stream_if_ctx   = server,

        .ea_get_ssl_ctx = get_ssl_ctx,
        
        // lookup certificate
        .ea_lookup_cert = sni_lookup,
        .ea_cert_lu_ctx = 13,

        // these are zero anyways
        .ea_hsi_ctx = 0,
        .ea_hsi_if = 0,
    };

    //printf("log: %d\n", lsquic_set_log_level("debug"));

    static struct lsquic_logger_if logger = {
        .log_buf = log_buf_cb,
    };



    lsquic_logger_init(&logger, 0, LLTS_NONE);

    /* Create an engine in server mode with HTTP behavior: */
    engine = lsquic_engine_new(LSENG_SERVER | LSENG_HTTP, &engine_api);

    printf("Engine: %p\n", engine);

    return 0;
}

void us_quic_socket_context_on_data(us_quic_socket_context_t *context, void(*on_data)(us_quic_socket_t *, char *, int)) {

}

us_quic_listen_socket_t *us_quic_socket_context_listen(us_quic_socket_context_t *context, char *host, int port) {
    /* Allocate per thread, UDP packet buffers */
    buf = us_create_udp_packet_buffer();
    send_buf = us_create_udp_packet_buffer();

    return 0;
}

#endif
