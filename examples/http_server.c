/* This is a barebone keep-alive HTTP server */

#include <libusockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct http_socket {
	/* How far we have streamed our response */
	int offset;
};

struct http_context {
	/* The shared response */
	char *response;
	int length;
};

/* We don't need any of these */
void on_wakeup(struct us_loop *loop) {

}

void on_pre(struct us_loop *loop) {

}

/* This is not HTTP POST, it is merely an event emitted post loop iteration */
void on_post(struct us_loop *loop) {

}

void on_http_socket_writable(struct us_socket *s) {
	struct http_socket *http_socket = (struct http_socket *) us_socket_ext(s);
	struct http_context *http_context = (struct http_context *) us_socket_context_ext(us_socket_get_context(s));

	/* Stream whatever is remaining of the response */
	http_socket->offset += us_socket_write(s, http_context->response + http_socket->offset, http_context->length - http_socket->offset, 0);
}

void on_http_socket_close(struct us_socket *s) {
	printf("Client disconnected\n");
}

void on_http_socket_end(struct us_socket *s) {
	/* HTTP does not support half-closed sockets */
	us_socket_shutdown(s);
	us_socket_close(s);
}

void on_http_socket_data(struct us_socket *s, char *data, int length) {
	/* Get socket extension and the socket's context's extension */
	struct http_socket *http_socket = (struct http_socket *) us_socket_ext(s);
	struct http_context *http_context = (struct http_context *) us_socket_context_ext(us_socket_get_context(s));

	/* We treat all data events as a request */
	http_socket->offset = us_socket_write(s, http_context->response, http_context->length, 0);

	/* Reset idle timer */
	us_socket_timeout(s, 30);
}

void on_http_socket_open(struct us_socket *s, int is_client) {
	struct http_socket *http_socket = (struct http_socket *) us_socket_ext(s);

	/* Reset offset */
	http_socket->offset = 0;

	/* Timeout idle HTTP connections */
	us_socket_timeout(s, 30);

	printf("Client connected\n");
}

void on_http_socket_timeout(struct us_socket *s) {
	/* Close idle HTTP sockets */
	us_socket_close(s);
}

int main() {
	/* Create the event loop */
	struct us_loop *loop = us_create_loop(1, on_wakeup, on_pre, on_post, 0);

	/* Create a socket context for HTTP */
	struct us_socket_context *http_context = us_create_socket_context(loop, sizeof(struct http_context));

	/* Generate the shared response */
	const char body[] = "<html><body><h1>Why hello there!</h1></body></html>";
	
	struct http_context *http_context_ext = (struct http_context *) us_socket_context_ext(http_context);
	http_context_ext->response = (char *) malloc(128 + sizeof(body) - 1);
	http_context_ext->length = snprintf(http_context_ext->response, 128 + sizeof(body) - 1, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n\r\n%s", sizeof(body) - 1, body);

	/* Set up event handlers */
	us_socket_context_on_open(http_context, on_http_socket_open);
	us_socket_context_on_data(http_context, on_http_socket_data);
	us_socket_context_on_writable(http_context, on_http_socket_writable);
	us_socket_context_on_close(http_context, on_http_socket_close);
	us_socket_context_on_timeout(http_context, on_http_socket_timeout);
	us_socket_context_on_end(http_context, on_http_socket_end);

	/* Start serving HTTP connections */
	struct us_listen_socket *listen_socket = us_socket_context_listen(http_context, 0, 3000, 0, sizeof(struct http_socket));

	if (listen_socket) {
		printf("Listening on port 3000...\n");
		us_loop_run(loop);
	} else {
		printf("Failed to listen!\n");
	}
}
