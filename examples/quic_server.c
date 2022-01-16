#ifdef LIBUS_USE_QUIC

/* Experimental QUIC server */
#include <libusockets.h>

/* Quic interface is not exposed under libusockets.h yet */
#include "quic.h"

#include <stdio.h>

void on_wakeup(struct us_loop_t *loop) {

}

void on_pre(struct us_loop_t *loop) {

}

void on_post(struct us_loop_t *loop) {

}

void on_server_quic_data(us_quic_socket_t *s, char *data, int length) {
    printf("quic data emitted\n");
}

int main() {
	/* Create the event loop */
	struct us_loop_t *loop = us_create_loop(0, on_wakeup, on_pre, on_post, 0);

    /* SSL cert is always needed for quic */
    us_quic_socket_context_options_t options = {
        .cert_file_name = "",
        .key_file_name = ""
    };

    /* Create quic socket context */
    us_quic_socket_context_t *context = us_create_quic_socket_context(loop, options);

    /* Specify application callbacks */
    us_quic_socket_context_on_data(context, on_server_quic_data);

    /* The listening socket is the actual UDP socket used */
    us_quic_listen_socket_t *listen_socket = us_quic_socket_context_listen(context, "127.0.0.1", 5678);

    /* Run the event loop */
    us_loop_run(loop);
}
#else

#include <stdio.h>

int main() {
    printf("Compile with WITH_QUIC=1 make examples in order to build this example\n");
}

#endif