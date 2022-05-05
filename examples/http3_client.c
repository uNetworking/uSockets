#ifdef LIBUS_USE_QUIC

#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>

/* Experimental HTTP/3 client */
#include <libusockets.h>

/* Quic interface is not exposed under libusockets.h yet */
#include "quic.h"

/* Let's just have this one here for now */
us_quic_socket_context_t *context;

#include <stdio.h>

unsigned long long requests = 0;

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

    printf("CLIENT GOT HTTP RESPONSE!\n");

    print_current_headers();

    /* Make a new stream */
    us_quic_socket_create_stream(us_quic_stream_socket(s));
}

/* And this would be the body of the request */
void on_stream_data(us_quic_stream_t *s, char *data, int length) {
    printf("Body length is: %d\n", length);

    printf("%.*s\n", length, data);
}

void on_stream_writable(us_quic_stream_t *s) {

}

void on_stream_close(us_quic_stream_t *s) {
    printf("Stream closed\n");
}

/* On new connection */
void on_open(us_quic_socket_t *s, int is_client) {
    printf("New QUIC connection! Is client: %d\n", is_client);

    // for now the lib creates a stream by itself here if client
    if (is_client) {
        us_quic_socket_create_stream(s);
    }
}

/* On new stream */
void on_stream_open(us_quic_stream_t *s, int is_client) {
    printf("Stream open is_client: %d!\n", is_client);

    /* The client begins by making a request */
    if (is_client) {
        us_quic_socket_context_set_header(NULL, 0, ":method", 7, "GET", 3);
        //us_quic_socket_context_set_header(NULL, 1, ":path", 5, "/hi", 3);

        us_quic_socket_context_send_headers(NULL, s, 1, 0);
        /* Shutdown writing (send FIN) */
        us_quic_stream_shutdown(s);
    }
}

void on_close(us_quic_socket_t *s) {
    printf("QUIC connection closed!\n");
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

    /* We also establish a client connection that sends requests */
    us_quic_socket_t *connect_socket = us_quic_socket_context_connect(context, "::1", 9004);

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