#include <libusockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* We don't need any of these */
void on_wakeup(struct us_loop_t *loop) {

}

void on_pre(struct us_loop_t *loop) {

}

/* This is not HTTP POST, it is merely an event emitted post loop iteration */
void on_post(struct us_loop_t *loop) {

}

struct us_socket_t *on_http_socket_writable(struct us_socket_t *s) {
	return s;
}

struct us_socket_t *on_http_socket_close(struct us_socket_t *s, int code, void *reason) {
	printf("Client disconnected\n");
	return s;
}

struct us_socket_t *on_http_socket_end(struct us_socket_t *s) {
	/* HTTP does not support half-closed sockets */
	us_socket_shutdown(0, s);
	return us_socket_close(0, s, 0, NULL);
}

struct us_socket_t *on_http_socket_data(struct us_socket_t *s, char *data, int length) {

	us_socket_write(0, s, "Hello short message!", 20, 0);
	return s;
}

struct us_socket_t *on_http_socket_open(struct us_socket_t *s, int is_client, char *ip, int ip_length) {

	printf("Client connected\n");
	return s;
}

struct us_socket_t *on_http_socket_timeout(struct us_socket_t *s) {
	return s;
}

int main() {
	/* Create the event loop */
	struct us_loop_t *loop = us_create_loop(0, on_wakeup, on_pre, on_post, 0);

	/* Create a socket context for HTTP */
	struct us_socket_context_options_t options = {};

	struct us_socket_context_t *http_context = us_create_socket_context(0, loop, 0, options);

	if (!http_context) {
		printf("Could not load SSL cert/key\n");
		exit(0);
	}

	/* Set up event handlers */
	us_socket_context_on_open(0, http_context, on_http_socket_open);
	us_socket_context_on_data(0, http_context, on_http_socket_data);
	us_socket_context_on_writable(0, http_context, on_http_socket_writable);
	us_socket_context_on_close(0, http_context, on_http_socket_close);
	us_socket_context_on_timeout(0, http_context, on_http_socket_timeout);
	us_socket_context_on_end(0, http_context, on_http_socket_end);

	/* Start serving HTTP connections */
	struct us_listen_socket_t *listen_socket = us_socket_context_listen(0, http_context, 0, 3000, 0, 0);

	if (listen_socket) {
		printf("Listening on port 3000...\n");
		us_loop_run(loop);
	} else {
		printf("Failed to listen!\n");
	}
}
