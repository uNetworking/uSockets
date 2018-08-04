/* This is a basic TCP echo server. */

#include <libusockets.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Our socket extension */
struct echo_socket {
	char *backpressure;
	int length;
};

/* Our socket context extension */
struct echo_context {

};

/* Loop wakeup handler */
void on_wakeup(struct us_loop *loop) {

}

/* Loop pre iteration handler */
void on_pre(struct us_loop *loop) {

}

/* Loop post iteration handler */
void on_post(struct us_loop *loop) {

}

/* Socket writable handler */
void on_echo_socket_writable(struct us_socket *s) {
	struct echo_socket *es = (struct echo_socket *) us_socket_ext(s);

	/* Continue writing out our backpressure */
	int written = us_socket_write(s, es->backpressure, es->length, 0);
	if (written != es->length) {
		char *new_buffer = (char *) malloc(es->length - written);
		memcpy(new_buffer, es->backpressure, es->length - written);
		free(es->backpressure);
		es->backpressure = new_buffer;
		es->length -= written;
	} else {
		free(es->backpressure);
		es->length = 0;
	}

	/* Client is not boring */
	us_socket_timeout(s, 30);
}

/* Socket closed handler */
void on_echo_socket_close(struct us_socket *s) {
	struct echo_socket *es = (struct echo_socket *) us_socket_ext(s);

	printf("Client disconnected\n");

	free(es->backpressure);
}

/* Socket half-closed handler */
void on_echo_socket_end(struct us_socket *s) {
	us_socket_shutdown(s);
	us_socket_close(s);
}

/* Socket data handler */
void on_echo_socket_data(struct us_socket *s, char *data, int length) {
	struct echo_socket *es = (struct echo_socket *) us_socket_ext(s);

	/* Print the data we received */
	printf("Client sent <%.*s>\n", length, data);

	/* Send it back or buffer it up */
	int written = us_socket_write(s, data, length, 0);
	if (written != length) {
		char *new_buffer = (char *) malloc(es->length + length - written);
		memcpy(new_buffer, es->backpressure, es->length);
		memcpy(new_buffer + es->length, data + written, length - written);
		free(es->backpressure);
		es->backpressure = new_buffer;
		es->length += length - written;
	}

	/* Client is not boring */
	us_socket_timeout(s, 30);
}

/* Socket opened handler */
void on_echo_socket_open(struct us_socket *s) {
	struct echo_socket *es = (struct echo_socket *) us_socket_ext(s);

	/* Initialize the new socket's extension */
	es->backpressure = 0;
	es->length = 0;

	/* Start a timeout to close the socekt if boring */
	us_socket_timeout(s, 30);

	printf("Client connected\n");
}

/* Socket timeout handler */
void on_echo_socket_timeout(struct us_socket *s) {
	printf("Client was idle for too long\n");
	us_socket_close(s);
}

int main() {
	/* The event loop */
	struct us_loop *loop = us_create_loop(1, on_wakeup, on_pre, on_post, 0);

	/* Socket context */
	struct us_socket_context *echo_context = us_create_socket_context(loop, sizeof(struct echo_context));

	/* Registering event handlers */
	us_socket_context_on_open(echo_context, on_echo_socket_open);
	us_socket_context_on_data(echo_context, on_echo_socket_data);
	us_socket_context_on_writable(echo_context, on_echo_socket_writable);
	us_socket_context_on_close(echo_context, on_echo_socket_close);
	us_socket_context_on_timeout(echo_context, on_echo_socket_timeout);
	us_socket_context_on_end(echo_context, on_echo_socket_end);

	/* Start accepting echo sockets */
	struct us_listen_socket *listen_socket = us_socket_context_listen(echo_context, 0, 3000, 0, sizeof(struct echo_socket));

	if (listen_socket) {
		printf("Listening on port 3000...\n");
		us_loop_run(loop);
	} else {
		printf("Failed to listen!\n");
	}
}
