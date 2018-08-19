/* This example, or test, is a moron test where the library is being hammered in all the possible ways randomly over time */

#ifndef LIBUS_NO_SSL
#define LIBUS_SOCKET us_ssl_socket
#define LIBUS_SOCKET_CONTEXT us_ssl_socket_context
#else
#define LIBUS_SOCKET us_socket
#define LIBUS_SOCKET_CONTEXT us_socket_context
#endif

#include <libusockets.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int opened_connections, closed_connections, operations_done;
struct LIBUS_SOCKET_CONTEXT *http_context;
struct us_listen_socket *listen_socket;

void *long_buffer;
int long_length = 5 * 1024 * 1024;

void perform_random_operation(struct LIBUS_SOCKET *s) {
    switch (rand() % 1) {
#ifndef LIBUS_NO_SSL
        case 0: {
            us_ssl_socket_close(s);
        }
        break;
        case 1: {
            us_ssl_socket_write(s, "short", 5, 0);
        }
        break;
        case 2: {
            us_ssl_socket_shutdown(s);
        }
        break;
        case 3: {
            us_ssl_socket_write(s, (char *) long_buffer, long_length, 0);
        }
        break;
#else
        case 0: {
            us_socket_close(s);
        }
        break;
        case 1: {
            us_socket_write(s, "short", 5, 0);
        }
        break;
        case 2: {
            us_socket_shutdown(s);
        }
        break;
        case 3: {
            us_socket_write(s, (char *) long_buffer, long_length, 0);
        }
        break;
#endif
    }
}

struct http_socket {

};

struct http_context {

};

void on_wakeup(struct us_loop *loop) {
    // test these also
}

void on_pre(struct us_loop *loop) {
    printf("PRE\n");
}

void on_post(struct us_loop *loop) {

}

void on_http_socket_writable(struct LIBUS_SOCKET *s) {
    perform_random_operation(s);
}

void on_http_socket_close(struct LIBUS_SOCKET *s) {
    closed_connections++;
    printf("Opened: %d\nClosed: %d\n\n", opened_connections, closed_connections);

    if (closed_connections == 10000) {
        us_listen_socket_close(listen_socket);
    } else {
        perform_random_operation(s);
    }
}

void on_http_socket_end(struct LIBUS_SOCKET *s) {
    // we need to close on shutdown
#ifndef LIBUS_NO_SSL
     us_ssl_socket_close(s);
#else
     us_socket_close(s);
#endif
    perform_random_operation(s);
}

void on_http_socket_data(struct LIBUS_SOCKET *s, char *data, int length) {
    perform_random_operation(s);
}

void on_http_socket_open(struct LIBUS_SOCKET *s, int is_client) {
    opened_connections++;
    printf("Opened: %d\nClosed: %d\n\n", opened_connections, closed_connections);

    if (opened_connections % 2 == 0 && opened_connections < 10000) {
#ifndef LIBUS_NO_SSL
        us_ssl_socket_context_connect(http_context, "localhost", 3000, 0, sizeof(struct http_socket));
#else
        us_socket_context_connect(http_context, "localhost", 3000, 0, sizeof(struct http_socket));
#endif
    }

    perform_random_operation(s);
}

void on_http_socket_timeout(struct LIBUS_SOCKET *s) {
    perform_random_operation(s);
}

// todo: a separate thread which hammers the wakeup which tries to mess up as much as possible
int main() {
    srand(time(0));
    long_buffer = calloc(long_length, 1);

    struct us_loop *loop = us_create_loop(1, on_wakeup, on_pre, on_post, 0);

#ifndef LIBUS_NO_SSL
    struct us_ssl_socket_context_options ssl_options = {};
    ssl_options.key_file_name = "/home/alexhultman/uWebSockets/misc/ssl/key.pem";
    ssl_options.cert_file_name = "/home/alexhultman/uWebSockets/misc/ssl/cert.pem";
    ssl_options.passphrase = "1234";

    http_context = us_create_ssl_socket_context(loop, sizeof(struct http_context), ssl_options);

    us_ssl_socket_context_on_open(http_context, on_http_socket_open);
    us_ssl_socket_context_on_data(http_context, on_http_socket_data);
    us_ssl_socket_context_on_writable(http_context, on_http_socket_writable);
    us_ssl_socket_context_on_close(http_context, on_http_socket_close);
    us_ssl_socket_context_on_timeout(http_context, on_http_socket_timeout);
    us_ssl_socket_context_on_end(http_context, on_http_socket_end);

    listen_socket = us_ssl_socket_context_listen(http_context, 0, 3000, 0, sizeof(struct http_socket));

    us_ssl_socket_context_connect(http_context, "localhost", 3000, 0, sizeof(struct http_socket));
#else
    http_context = us_create_socket_context(loop, sizeof(struct http_context));

    us_socket_context_on_open(http_context, on_http_socket_open);
    us_socket_context_on_data(http_context, on_http_socket_data);
    us_socket_context_on_writable(http_context, on_http_socket_writable);
    us_socket_context_on_close(http_context, on_http_socket_close);
    us_socket_context_on_timeout(http_context, on_http_socket_timeout);
    us_socket_context_on_end(http_context, on_http_socket_end);

    listen_socket = us_socket_context_listen(http_context, 0, 3000, 0, sizeof(struct http_socket));

    us_socket_context_connect(http_context, "localhost", 3000, 0, sizeof(struct http_socket));
#endif

    if (listen_socket) {
        printf("Running hammer test\n");
        us_loop_run(loop);
    } else {
        printf("Cannot listen to port 3000!\n");
    }

#ifndef LIBUS_NO_SSL
    us_ssl_socket_context_free(http_context);
#else
    us_socket_context_free(http_context);
#endif

    us_loop_free(loop);
    free(long_buffer);
    printf("Done, shutting down now\n");
}
