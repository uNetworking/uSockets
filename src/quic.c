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

/* Socket context */
struct us_quic_socket_context_s {

    struct us_udp_packet_buffer_t *recv_buf;
    struct us_udp_packet_buffer_t *send_buf;
    int outgoing_packets;

    struct us_udp_socket_t *udp_socket;
    struct us_loop_t *loop;
    lsquic_engine_t *engine;

    void(*on_stream_data)(us_quic_stream_t *s, char *data, int length);
    void(*on_stream_headers)();
    void(*on_stream_open)();
    void(*on_stream_close)();
    void(*on_stream_writable)();
    void(*on_open)();
    void(*on_close)();
};

/* Setters */
void us_quic_socket_context_on_stream_data(us_quic_socket_context_t *context, void(*on_stream_data)(us_quic_stream_t *s, char *data, int length)) {
    context->on_stream_data = on_stream_data;
}
void us_quic_socket_context_on_stream_headers(us_quic_socket_context_t *context, void(*on_stream_headers)()) {
    context->on_stream_headers = on_stream_headers;
}
void us_quic_socket_context_on_stream_open(us_quic_socket_context_t *context, void(*on_stream_open)()) {
    context->on_stream_open = on_stream_open;
}
void us_quic_socket_context_on_stream_close(us_quic_socket_context_t *context, void(*on_stream_close)()) {
    context->on_stream_close = on_stream_close;
}
void us_quic_socket_context_on_open(us_quic_socket_context_t *context, void(*on_open)()) {
    context->on_open = on_open;
}
void us_quic_socket_context_on_close(us_quic_socket_context_t *context, void(*on_close)()) {
    context->on_close = on_close;
}
void us_quic_socket_context_on_stream_writable(us_quic_socket_context_t *context, void(*on_stream_writable)()) {
    context->on_stream_writable = on_stream_writable;
}

/* UDP handlers */
void on_udp_socket_writable(struct us_udp_socket_t *s) {
    /* Need context from socket here */
    us_quic_socket_context_t *context = us_udp_socket_user(s);

    /* We just continue now */
    lsquic_engine_send_unsent_packets(context->engine);
}

void on_udp_socket_data(struct us_udp_socket_t *s, struct us_udp_packet_buffer_t *buf, int packets) {
    

    

    /* We need to lookup the context from the udp socket */
    //us_udpus_udp_socket_context(s);
    // do we have udp socket contexts? or do we just have user data?

    us_quic_socket_context_t *context = us_udp_socket_user(s);
    
    /* We just shove it to lsquic */
    for (int i = 0; i < packets; i++) {
        char *payload = us_udp_packet_buffer_payload(buf, i);
        int length = us_udp_packet_buffer_payload_length(buf, i);
        int ecn = us_udp_packet_buffer_ecn(buf, i);
        void *peer_addr = us_udp_packet_buffer_peer(buf, i);

        //printf("Reading UDP of size %d\n", length);


        int ret = lsquic_engine_packet_in(context->engine, payload, length, peer_addr, peer_addr, (void *) 12, 0);
        //printf("Engine returned: %d\n", ret);

    
    }

    lsquic_engine_process_conns(context->engine);

}

/* Return number of packets sent or -1 on error */
int send_packets_out(void *ctx, const struct lsquic_out_spec *specs, unsigned n_specs) {
    us_quic_socket_context_t *context = ctx;

    //printf("Sending UDP\n");

    struct mmsghdr msgs[512] = {};
    unsigned n;

    if (n > 512) {
        printf("more than 512 packets!\n");
        exit(0);
    }

    for (n = 0; n < n_specs; ++n)
    {
        memset(&msgs[n], 0, sizeof(struct mmsghdr));

        msgs[n].msg_hdr.msg_name       = (void *) specs[n].dest_sa;
        msgs[n].msg_hdr.msg_namelen    = sizeof(struct sockaddr_in);
        msgs[n].msg_hdr.msg_iov        = specs[n].iov;
        msgs[n].msg_hdr.msg_iovlen     = specs[n].iovlen;
    }

    /* On Linux, we can directly pass an mmsghr here */
    n = us_udp_socket_send(context->udp_socket, (struct us_udp_packet_buffer_t *) msgs, n);

    //printf("Sent: %d\n", n);

    if (n != n_specs) {
        printf("CANNOT SEND PACKETS!\n");
        exit(0);
    }

    return (int) n;
}

lsquic_conn_ctx_t *on_new_conn(void *stream_if_ctx, lsquic_conn_t *c) {
    us_quic_socket_context_t *context = stream_if_ctx;

    context->on_open();

    return 0;
}

void on_conn_closed(lsquic_conn_t *c) {
    //us_quic_socket_context_t *context = ctx;

    //context->on_close();

    printf("QUIC connection closed!\n");
}

lsquic_stream_ctx_t *on_new_stream(void *stream_if_ctx, lsquic_stream_t *s) {

    /* In true usockets style we always want read */
    lsquic_stream_wantread(s, 1);

    us_quic_socket_context_t *context = stream_if_ctx;

    context->on_stream_open();

    return context;
}

//#define V(v) (v), strlen(v)

// header bug is really just an offset buffer - perfect for per context!
// could even use cork buffer or similar
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

/* Static storage should be per context or really per loop */
struct header_buf hbuf;
struct lsxpack_header headers_arr[10];

void us_quic_socket_context_set_header(us_quic_socket_context_t *context, int index, char *key, int key_length, char *value, int value_length) {
    if (header_set_ptr(&headers_arr[index], &hbuf, key, key_length, value, value_length) != 0) {
        printf("CANNOT FORMAT HEADER!\n");
        exit(0);
    }
}

void us_quic_socket_context_send_headers(us_quic_socket_context_t *context, us_quic_stream_t *s, int num) {

    lsquic_http_headers_t headers = {
        .count = num,
        .headers = headers_arr,
    };
    // last here is whether this is eof or not (has body)
    if (lsquic_stream_send_headers(s, &headers, 1)) {// pass 0 if data
        printf("CANNOT SEND HEADERS!\n");
        exit(0);
    }

    /* Reset header offset */
    hbuf.off = 0;
}

// this would be the application logic of the echo server
// this function should emit the quic message to the high level application
void on_read(lsquic_stream_t *s, lsquic_stream_ctx_t *h) {

    us_quic_socket_context_t *context = h;

    //printf("stream is readable\n");

    // I guess you just get the header set here
    void *header_set = lsquic_stream_get_hset(s);
    //printf("Header set is: %p\n", header_set);

    if (header_set) {
        context->on_stream_headers(s);
    }

    // here we emit a new request if we have headers?
    // if only data, we probably don't get headers

    //printf("lsquick on_read for stream: %p\n", s);

    char temp[4096] = {};

    //printf("stream_reading now\n");

    int nr = lsquic_stream_read(s, temp, 4096);

    if (nr == -1) {
        printf("Error in reading!\n");
        exit(0);
        return;
    }

    if (nr == 0) {
        // reached the EOF
        lsquic_stream_close(s);
        //lsquic_stream_wantread(s, 0);
        return;
    }

    //printf("read: %d\n", nr);

    //printf("%s\n", temp);

    // why do we get tons of zero reads?
    // maybe it doesn't matter, if we can parse this input then we are fine
    //lsquic_stream_wantread(s, 0);
    //lsquic_stream_wantwrite(s, 1);


    context->on_stream_data(s, temp, nr);
}

int us_quic_stream_write(us_quic_stream_t *s, char *data, int length) {
    lsquic_stream_t *stream = s;
    int ret = lsquic_stream_write(s, data, length);
    return ret;
}

void on_write (lsquic_stream_t *s, lsquic_stream_ctx_t *h) {

}

void on_close (lsquic_stream_t *s, lsquic_stream_ctx_t *h) {
    //printf("STREAM CLOSED!\n");
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
        printf("OPENSSL_NPN_NEGOTIATED\n");
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

    if (!SSL_get_servername(s, TLSEXT_NAMETYPE_host_name)) {
        SSL_set_tlsext_host_name(s, "YOLO NAME!");
        printf("set name is: %s\n", SSL_get_servername(s, TLSEXT_NAMETYPE_host_name));
    }


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

    SSL_CTX_set_min_proto_version(ctx, TLS1_3_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
    
    //SSL_CTX_set_default_verify_paths(ctx);
    
    // probably cannot use this when http is in use?
    // alpn is needed
    SSL_CTX_set_alpn_select_cb(ctx, select_alpn, NULL);

    // sni is needed
    SSL_CTX_set_tlsext_servername_callback(ctx, server_name_cb);
 //long SSL_CTX_set_tlsext_servername_arg(SSL_CTX *ctx, void *arg);

    int a = SSL_CTX_use_certificate_chain_file(ctx, "/home/alexhultman/uWebSockets.js/misc/cert.pem");
    int b = SSL_CTX_use_PrivateKey_file(ctx, "/home/alexhultman/uWebSockets.js/misc/key.pem", SSL_FILETYPE_PEM);

    printf("loaded cert and key? %d, %d\n", a, b);

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

int us_quic_stream_shutdown(us_quic_stream_t *s) {
    lsquic_stream_t *stream = s;

    int ret = lsquic_stream_shutdown(s, 1);
    if (ret != 0) {
        printf("cannot shutdown stream!\n");
        exit(0);
    }

    return 0;
}

// header of header set
struct header_set_hd {
    int offset;
};

// let's just store last header set here
struct header_set_hd *last_hset;

// just a shitty marker for now
struct processed_header {
    void *name, *value;
    int name_length, value_length;
};

int us_quic_socket_context_get_header(us_quic_socket_context_t *context, int index, char **name, int *name_length, char **value, int *value_length) {

    if (index < last_hset->offset) {

        struct processed_header *pd = (last_hset + 1);

        pd = pd + index;

        *name = pd->name;
        *value = pd->value;
        *value_length = pd->value_length;
        *name_length = pd->name_length;

        return 1;
    }

    return 0;

}

// header set callbacks
void *hsi_create_header_set(void *hsi_ctx, lsquic_stream_t *stream, int is_push_promise) {

    //printf("hsi_create_header_set\n");

    void *hset = malloc(1024);
    memset(hset, 0, sizeof(struct header_set_hd));

    // hsi_ctx is set in engine creation below

    // I guess we just return whatever here, what we return here is gettable via the stream

    // gettable via lsquic_stream_get_hset

    // return user defined header set

    return hset;
}

void hsi_discard_header_set(void *hdr_set) {
    // this is pretty much the destructor of above constructor

    printf("hsi_discard_header!\n");
}

// one header set allocates one 8kb buffer from a linked list of available buffers


// 8kb of preallocated heap for headers
char header_decode_heap[1024 * 8];
int header_decode_heap_offset = 0;

struct lsxpack_header *hsi_prepare_decode(void *hdr_set, struct lsxpack_header *hdr, size_t space) {

    //printf("hsi_prepare_decode\n");

    if (!hdr) {
        hdr = malloc(sizeof(struct lsxpack_header));
        memset(hdr, 0, sizeof(struct lsxpack_header));
        hdr->buf = malloc(space);
        lsxpack_header_prepare_decode(hdr, hdr->buf, 0, space);
    } else {
        hdr->val_len = space;
        hdr->buf = realloc(hdr->buf, space);
    }

    return hdr;
}

int hsi_process_header(void *hdr_set, struct lsxpack_header *hdr) {

    // I guess this is the emitting of the header to app space

    //printf("hsi_process_header: %p\n", hdr);

    struct header_set_hd *hd = hdr_set;
    struct processed_header *proc_hdr = hd + 1;

    if (!hdr) {
        //printf("end of headers!\n");

        last_hset = hd;

        // mark end, well we can also just read the offset!
        //memset(&proc_hdr[hd->offset], 0, sizeof(struct processed_header));

        return 0;
    }

    /*if (hdr->hpack_index) {
        printf("header has hpack index: %d\n", hdr->hpack_index);
    }

    if (hdr->qpack_index) {
        printf("header has qpack index: %d\n", hdr->qpack_index);
    }*/

    proc_hdr[hd->offset].value = &hdr->buf[hdr->val_offset];
    proc_hdr[hd->offset].name = &hdr->buf[hdr->name_offset];
    proc_hdr[hd->offset].value_length = hdr->val_len;
    proc_hdr[hd->offset].name_length = hdr->name_len;

    //printf("header %.*s = %.*s\n", hdr->name_len, &hdr->buf[hdr->name_offset], hdr->val_len, &hdr->buf[hdr->val_offset]);

    hd->offset++;

    return 0;
}

extern us_quic_socket_context_t *context;

void timer_cb(struct us_timer_t *t) {
    printf("Processing conns from timer\n");
    lsquic_engine_process_conns(context->engine);
}

// this will be for both client and server, but will be only for either h3 or raw quic
us_quic_socket_context_t *us_create_quic_socket_context(struct us_loop_t *loop, us_quic_socket_context_options_t options) {


    // every _listen_ call creates a new udp socket that feeds inputs to the engine in the context
    // every context has its own send buffer and udp send socket (not bound to any port or ip?)

    // or just make it so that once you listen, it will listen on that port for input, and the context will use
    // the first udp socket for output as it doesn't matter which one is used

    /* Holds all callbacks */
    us_quic_socket_context_t *context = malloc(sizeof(us_quic_socket_context_t));

    context->loop = loop;
    context->udp_socket = 0;

    /* Allocate per thread, UDP packet buffers */
    context->recv_buf = us_create_udp_packet_buffer();
    context->send_buf = us_create_udp_packet_buffer();

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

    static struct lsquic_hset_if hset_if = {
        .hsi_discard_header_set = hsi_discard_header_set,
        .hsi_create_header_set = hsi_create_header_set,
        .hsi_prepare_decode = hsi_prepare_decode,
        .hsi_process_header = hsi_process_header
    };


    add_alpn("h3");

    struct lsquic_engine_api engine_api = {
        .ea_packets_out     = send_packets_out,
        .ea_packets_out_ctx = (void *) context,  /* For example */
        .ea_stream_if       = &stream_callbacks,
        .ea_stream_if_ctx   = context,

        .ea_get_ssl_ctx = get_ssl_ctx,
        
        // lookup certificate
        .ea_lookup_cert = sni_lookup,
        .ea_cert_lu_ctx = 13,

        // these are zero anyways
        .ea_hsi_ctx = 0,
        .ea_hsi_if = &hset_if,
    };

    //printf("log: %d\n", lsquic_set_log_level("debug"));

    static struct lsquic_logger_if logger = {
        .log_buf = log_buf_cb,
    };



    lsquic_logger_init(&logger, 0, LLTS_NONE);

    /* Create an engine in server mode with HTTP behavior: */
    context->engine = lsquic_engine_new(LSENG_SERVER | LSENG_HTTP, &engine_api);

    //printf("Engine: %p\n", context->engine);

    // start a timer to handle connections
    struct us_timer_t *delayTimer = us_create_timer(loop, 0, 0);
    us_timer_set(delayTimer, timer_cb, 1000, 1000);

    return context;
}

us_quic_listen_socket_t *us_quic_socket_context_listen(us_quic_socket_context_t *context, char *host, int port) {
    /* We literally do create a listen socket */
    context->udp_socket = us_create_udp_socket(context->loop, context->recv_buf, on_udp_socket_data, on_udp_socket_writable, host, port, context);
    return context->udp_socket;
}

#endif
