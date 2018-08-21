/* This is a simple yet efficient HTTP server benchmark */

#include <libusockets.h>

#ifndef LIBUS_NO_SSL
#define us_socket us_ssl_socket
#define us_socket_context us_ssl_socket_context
#define us_socket_write us_ssl_socket_write
#define us_socket_close us_ssl_socket_close
#define us_socket_shutdown us_ssl_socket_shutdown
#define us_socket_context_on_end us_ssl_socket_context_on_end
#define us_socket_context_on_open us_ssl_socket_context_on_open
#define us_socket_context_on_close us_ssl_socket_context_on_close
#define us_socket_context_on_writable us_ssl_socket_context_on_writable
#define us_socket_context_on_data us_ssl_socket_context_on_data
#define us_socket_context_on_timeout us_ssl_socket_context_on_timeout
#define us_socket_ext us_ssl_socket_ext
#define us_socket_context_ext us_ssl_socket_context_ext
#define us_socket_get_context us_ssl_socket_get_context
#define us_socket_context_listen us_ssl_socket_context_listen
#define us_socket_timeout us_ssl_socket_timeout
#define us_socket_context_connect us_ssl_socket_context_connect
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char request[] = "GET / HTTP/1.1\r\n\r\n\r\n";
char *host;
int port;
int connections;

int responses;

struct http_socket {
    /* How far we have streamed our request */
    int offset;
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

    /* Stream whatever is remaining of the request */
    http_socket->offset += us_socket_write(s, request + http_socket->offset, (sizeof(request) - 1) - http_socket->offset, 0);
}

void on_http_socket_close(struct us_socket *s) {

}

void on_http_socket_end(struct us_socket *s) {
    us_socket_close(s);
}

void on_http_socket_data(struct us_socket *s, char *data, int length) {
    /* Get socket extension and the socket's context's extension */
    struct http_socket *http_socket = (struct http_socket *) us_socket_ext(s);
    struct http_context *http_context = (struct http_context *) us_socket_context_ext(us_socket_get_context(s));

    /* We treat all data events as a response */
    http_socket->offset = us_socket_write(s, request, sizeof(request) - 1, 0);

    /* */
    responses++;
}

void on_http_socket_open(struct us_socket *s, int is_client) {
    struct http_socket *http_socket = (struct http_socket *) us_socket_ext(s);

    /* Reset offset */
    http_socket->offset = 0;

    /* Send a request */
    us_socket_write(s, request, sizeof(request) - 1, 0);

    if (--connections) {
        us_socket_context_connect(us_socket_get_context(s), host, port, 0, sizeof(struct http_socket));
    }
    else {
        printf("Running benchmark now...\n");

        us_socket_timeout(s, LIBUS_TIMEOUT_GRANULARITY);
    }
}

void on_http_socket_timeout(struct us_socket *s) {
    /* Print current statistics */
    printf("Req/sec: %f\n", ((float)responses) / LIBUS_TIMEOUT_GRANULARITY);

    responses = 0;
    us_socket_timeout(s, LIBUS_TIMEOUT_GRANULARITY);
}

int main(int argc, char **argv) {

    /* Parse host and port */
    if (argc != 4) {
        printf("Usage: connections host port\n");
        return 0;
    }

    port = atoi(argv[3]);
    host = malloc(strlen(argv[2]) + 1);
    memcpy(host, argv[2], strlen(argv[2]) + 1);
    connections = atoi(argv[1]);

    /* Create the event loop */
    struct us_loop *loop = us_create_loop(1, on_wakeup, on_pre, on_post, 0);

    /* Create a socket context for HTTP */
#ifndef LIBUS_NO_SSL
    struct us_ssl_socket_context_options ssl_options = {};
    struct us_socket_context *http_context = us_create_ssl_socket_context(loop, 0, ssl_options);
#else
    struct us_socket_context *http_context = us_create_socket_context(loop, 0);
#endif

    /* Set up event handlers */
    us_socket_context_on_open(http_context, on_http_socket_open);
    us_socket_context_on_data(http_context, on_http_socket_data);
    us_socket_context_on_writable(http_context, on_http_socket_writable);
    us_socket_context_on_close(http_context, on_http_socket_close);
    us_socket_context_on_timeout(http_context, on_http_socket_timeout);
    us_socket_context_on_end(http_context, on_http_socket_end);

    /* Start making HTTP connections */
    us_socket_context_connect(http_context, host, port, 0, sizeof(struct http_socket));

    us_loop_run(loop);
}
