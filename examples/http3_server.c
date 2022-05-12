#ifdef LIBUS_USE_QUIC

#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>

/* Experimental HTTP/3 server. You can use quiche-client --no-verify https://[::1]:9004/ */
#include <libusockets.h>

/* Quic interface is not exposed under libusockets.h yet */
#include "quic.h"

/* Let's just have this one here for now */
us_quic_socket_context_t *context;

#include <stdio.h>

/* Loop callbacks not used in this example */
void on_wakeup(struct us_loop_t *loop) {}
void on_pre(struct us_loop_t *loop) {}
void on_post(struct us_loop_t *loop) {}

void print_current_headers() {
    /* Iterate the headers and print them */
    for (int i = 0, more = 1; more; i++) {
        char *name, *value;
        int name_length, value_length;
        if (more = us_quic_socket_context_get_header(context, i, &name, &name_length, &value, &value_length)) {
            printf("header %.*s = %.*s\n", name_length, name, value_length, value);
        }
    }
}

/* This would be a request */
void on_stream_headers(us_quic_stream_t *s) {

    //printf("==== HTTP/3 request ====\n");

    //print_current_headers();

    /* Write headers */
    us_quic_socket_context_set_header(context, 0, ":status", 7, "200", 3);
    //us_quic_socket_context_set_header(context, 1, "content-length", 14, "11", 2);
    //us_quic_socket_context_set_header(context, 2, "content-type", 12, "text/html", 9);
    us_quic_socket_context_send_headers(context, s, 1, 1);

    /* Write body and shutdown (unknown if content-length must be present?) */
    us_quic_stream_write(s, "Hello quic!", 11);

    /* Every request has its own stream, so we conceptually serve requests like in HTTP 1.0 */
    us_quic_stream_shutdown(s);
}

/* And this would be the body of the request */
void on_stream_data(us_quic_stream_t *s, char *data, int length) {
    printf("Body length is: %d\n", length);
}

void on_stream_writable(us_quic_stream_t *s) {

}

void on_stream_close(us_quic_stream_t *s) {
    //printf("Stream closed\n");
}

/* On new connection */
void on_open(us_quic_socket_t *s, int is_client) {
    printf("Connection established!\n");
}

/* On new stream */
void on_stream_open(us_quic_stream_t *s, int is_client) {
    //printf("Stream opened!\n");
}

void on_close(us_quic_socket_t *s) {
    printf("Disconnected!\n");
}

int main() {
	/* Create the event loop */
	struct us_loop_t *loop = us_create_loop(0, on_wakeup, on_pre, on_post, 0);

    /* SSL cert is always needed for quic */
    us_quic_socket_context_options_t options = {
        .cert_file_name = "",
        .key_file_name = ""
    };

    /* Create quic socket context (assumes h3 for now) */
    context = us_create_quic_socket_context(loop, options);

    /* Specify application callbacks */
    us_quic_socket_context_on_stream_data(context, on_stream_data);
    us_quic_socket_context_on_stream_open(context, on_stream_open);
    us_quic_socket_context_on_stream_close(context, on_stream_close);
    us_quic_socket_context_on_stream_writable(context, on_stream_writable);
    us_quic_socket_context_on_stream_headers(context, on_stream_headers);
    us_quic_socket_context_on_open(context, on_open);
    us_quic_socket_context_on_close(context, on_close);

    /* The listening socket is the actual UDP socket used */
    us_quic_listen_socket_t *listen_socket = us_quic_socket_context_listen(context, "::1", 9004);

    /* Run the event loop */
    us_loop_run(loop);

    printf("Falling through!\n");
}
#else

#include <stdio.h>

int main() {
    printf("Compile with WITH_QUIC=1 make examples in order to build this example\n");
}

#endif