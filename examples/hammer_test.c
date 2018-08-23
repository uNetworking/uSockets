/* This example, or test, is a moron test where the library is being hammered in all the possible ways randomly over time */

#define LIBUS_NO_SSL

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
#include <time.h>

// todo: properly put all of these in various ext data so to test them!
int opened_connections, closed_connections, operations_done;
struct us_socket_context *http_context;
struct us_listen_socket *listen_socket;

void *long_buffer;
int long_length = 5 * 1024 * 1024;

// todo: if write failed, it needs to be called again with THE SAME content!
// otherwise SSL errors!

// add socket adoption as one case! (calls perform_random_operation again)
// also make sure to have socket ext data
// and context ext data
// and loop ext data

void perform_random_operation(struct us_socket *s) {
    switch (rand() % 4) {
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
        // if not complete, we need to queue this up for the writable event!
            us_socket_write(s, (char *) long_buffer, long_length, 0);
        }
        break;

        // case 4: adopt socket (makes use of the socket's context's data to point to the other context)
        // case 5: do whatever via the wakeup event (make use of a socket collection in the loop data)
    }
}

struct http_socket {

};

struct http_context {
    // link to the other context here
};

void on_wakeup(struct us_loop *loop) {
    // test these also
}

void on_pre(struct us_loop *loop) {
    printf("PRE\n");
}

void on_post(struct us_loop *loop) {

}

void on_http_socket_writable(struct us_socket *s) {
    perform_random_operation(s);
}

void on_http_socket_close(struct us_socket *s) {
    closed_connections++;
    printf("Opened: %d\nClosed: %d\n\n", opened_connections, closed_connections);

    if (closed_connections == 10000) {
        us_listen_socket_close(listen_socket);
    } else {
        perform_random_operation(s);
    }
}

void on_http_socket_end(struct us_socket *s) {
    // we need to close on shutdown
    us_socket_close(s);
    perform_random_operation(s);
}

void on_http_socket_data(struct us_socket *s, char *data, int length) {
    //printf("Fick data: <%.*s>\n", length, data);
    perform_random_operation(s);
}

void on_http_socket_open(struct us_socket *s, int is_client) {
    opened_connections++;
    printf("Opened: %d\nClosed: %d\n\n", opened_connections, closed_connections);

    if (opened_connections % 2 == 0 && opened_connections < 10000) {
        us_socket_context_connect(http_context, "localhost", 3000, 0, sizeof(struct http_socket));
    }

    perform_random_operation(s);
}

void on_http_socket_timeout(struct us_socket *s) {
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
#else
    http_context = us_create_socket_context(loop, sizeof(struct http_context));
#endif

    struct us_socket_CONTEXT *websocket_context = us_create_child_socket_context(http_context);

    us_socket_context_on_open(http_context, on_http_socket_open);
    us_socket_context_on_data(http_context, on_http_socket_data);
    us_socket_context_on_writable(http_context, on_http_socket_writable);
    us_socket_context_on_close(http_context, on_http_socket_close);
    us_socket_context_on_timeout(http_context, on_http_socket_timeout);
    us_socket_context_on_end(http_context, on_http_socket_end);

    us_socket_context_on_open(websocket_context, on_http_socket_open);
    us_socket_context_on_data(websocket_context, on_http_socket_data);
    us_socket_context_on_writable(websocket_context, on_http_socket_writable);
    us_socket_context_on_close(websocket_context, on_http_socket_close);
    us_socket_context_on_timeout(websocket_context, on_http_socket_timeout);
    us_socket_context_on_end(websocket_context, on_http_socket_end);

    listen_socket = us_socket_context_listen(http_context, 0, 3000, 0, sizeof(struct http_socket));

    if (listen_socket) {
        printf("Running hammer test\n");
        us_socket_context_connect(http_context, "localhost", 3000, 0, sizeof(struct http_socket));
        us_loop_run(loop);
    } else {
        printf("Cannot listen to port 3000!\n");
    }

    us_socket_context_free(http_context);
    us_loop_free(loop);
    free(long_buffer);
    printf("Done, shutting down now\n");
}
