/* This example, or test, is a moron test where the library is being hammered in all the possible ways randomly over time */

#include <libusockets.h>
#include "helper.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// todo: properly put all of these in various ext data so to test them!
int opened_connections, closed_connections, operations_done;
struct us_socket_context *http_context, *websocket_context;
struct us_listen_socket *listen_socket;

// put in loop ext data
void *long_buffer;
unsigned int long_length = 5 * 1024 * 1024;

// also make sure to have socket ext data
// and context ext data
// and loop ext data

struct http_socket {
    char content[512];
};

struct web_socket {
    char content[128];
};

struct http_context {
    // link to the other context here
    char content[1];
};

// todo: it would be nice to randomly select socket instead of
// using the one responsible for the event
struct us_socket *perform_random_operation(struct us_socket *s) {
    switch (rand() % 5) {
        case 0: {
            // close
            return us_socket_close(s);
        }
        case 1: {
            // adopt
            if (rand() % 2) {
                s = us_socket_context_adopt_socket(websocket_context, s, sizeof(struct web_socket));
            } else {
                s = us_socket_context_adopt_socket(http_context, s, sizeof(struct http_socket));
            }

            return perform_random_operation(s);
        }
        case 2: {
            // write
            us_socket_write(s, (char *) long_buffer, rand() % long_length, 0);
        }
        break;
        case 3: {
            // shutdown
            us_socket_shutdown(s);
        }
        break;
        case 4: {
            // loop wakeup and timeout sweep
            us_socket_timeout(s, 1);
            us_wakeup_loop(us_socket_context_loop(us_socket_get_context(s)));
        }
        break;
    }
    return s;
}

void on_wakeup(struct us_loop *loop) {
    // note: we expose internal functions to trigger a timeout sweep to find bugs
    extern void us_internal_timer_sweep(struct us_loop *loop);
    us_internal_timer_sweep(loop);
}

// maybe use thse to count spurious wakeups?
// that is, if we get tons of pre/post over and over without any events
// that would point towards 100% cpu usage kind of bugs
void on_pre(struct us_loop *loop) {
    printf("PRE\n");

    // reset a boolean here
}

void on_post(struct us_loop *loop) {
    // check if we did perform_random_operation
}

struct us_socket *on_web_socket_writable(struct us_socket *s) {
    // check that we are in fact of type http_socket
    memset(us_socket_ext(s), 0, sizeof(struct web_socket));

    return perform_random_operation(s);
}

struct us_socket *on_http_socket_writable(struct us_socket *s) {
    // check that we are in fact of type http_socket
    memset(us_socket_ext(s), 0, sizeof(struct http_socket));

    return perform_random_operation(s);
}

struct us_socket *on_web_socket_close(struct us_socket *s) {
    // check that we are in fact of type http_socket
    memset(us_socket_ext(s), 0, sizeof(struct web_socket));

    closed_connections++;
    printf("Opened: %d\nClosed: %d\n\n", opened_connections, closed_connections);

    if (closed_connections == 10000) {
        us_listen_socket_close(listen_socket);
    } else {
        return perform_random_operation(s);
    }
    return s;
}

struct us_socket *on_http_socket_close(struct us_socket *s) {
    // check that we are in fact of type http_socket
    memset(us_socket_ext(s), 0, sizeof(struct http_socket));

    closed_connections++;
    printf("Opened: %d\nClosed: %d\n\n", opened_connections, closed_connections);

    if (closed_connections == 10000) {
        us_listen_socket_close(listen_socket);
    } else {
        return perform_random_operation(s);
    }
    return s;
}

struct us_socket *on_web_socket_end(struct us_socket *s) {
    // check that we are in fact of type http_socket
    memset(us_socket_ext(s), 0, sizeof(struct web_socket));

    // we need to close on shutdown
    s = us_socket_close(s);
    return perform_random_operation(s);
}

struct us_socket *on_http_socket_end(struct us_socket *s) {
    // check that we are in fact of type http_socket
    memset(us_socket_ext(s), 0, sizeof(struct http_socket));

    // we need to close on shutdown
    s = us_socket_close(s);
    return perform_random_operation(s);
}

struct us_socket *on_web_socket_data(struct us_socket *s, char *data, int length) {
    // check that we are in fact of type http_socket
    memset(us_socket_ext(s), 0, sizeof(struct web_socket));

    if (length == 0) {
        printf("ERROR: Got data event with no data\n");
        exit(-1);
    }

    //printf("Fick data: <%.*s>\n", length, data);
    return perform_random_operation(s);
}

struct us_socket *on_http_socket_data(struct us_socket *s, char *data, int length) {
    // check that we are in fact of type http_socket
    memset(us_socket_ext(s), 0, sizeof(struct http_socket));

    if (length == 0) {
        printf("ERROR: Got data event with no data\n");
        exit(-1);
    }

    //printf("Fick data: <%.*s>\n", length, data);
    return perform_random_operation(s);
}

struct us_socket *on_web_socket_open(struct us_socket *s, int is_client) {
    // fail here, this can never happen!
    printf("ERROR: on_web_socket_open called!\n");
    exit(-2);
}

struct us_socket *on_http_socket_open(struct us_socket *s, int is_client) {
    // check that we are in fact of type http_socket
    memset(us_socket_ext(s), 0, sizeof(struct http_socket));

    opened_connections++;
    printf("Opened: %d\nClosed: %d\n\n", opened_connections, closed_connections);

    if (is_client && opened_connections < 10000) {
        us_socket_context_connect(http_context, "localhost", 3000, 0, sizeof(struct http_socket));
    }

    return perform_random_operation(s);
}

struct us_socket *on_web_socket_timeout(struct us_socket *s) {
    // check that we are in fact of type http_socket
    memset(us_socket_ext(s), 0, sizeof(struct web_socket));

    return perform_random_operation(s);
}

struct us_socket *on_http_socket_timeout(struct us_socket *s) {
    // check that we are in fact of type http_socket
    memset(us_socket_ext(s), 0, sizeof(struct http_socket));

    return perform_random_operation(s);
}

int main() {
    srand(time(0));
    long_buffer = calloc(long_length, 1);

    struct us_loop *loop = us_create_loop(1, on_wakeup, on_pre, on_post, 0);

    // us_loop_on_wakeup()
    // us_loop_on_pre()
    // us_loop_on_post()
    // us_loop_on_poll()
    // us_loop_on_timer()

#ifndef LIBUS_NO_SSL
    struct us_ssl_socket_context_options ssl_options = {};
    ssl_options.key_file_name = "/home/alexhultman/uWebSockets/misc/ssl/key.pem";
    ssl_options.cert_file_name = "/home/alexhultman/uWebSockets/misc/ssl/cert.pem";
    ssl_options.passphrase = "1234";

    http_context = us_create_ssl_socket_context(loop, sizeof(struct http_context), ssl_options);
#else
    http_context = us_create_socket_context(loop, sizeof(struct http_context));
#endif

    us_socket_context_on_open(http_context, on_http_socket_open);
    us_socket_context_on_data(http_context, on_http_socket_data);
    us_socket_context_on_writable(http_context, on_http_socket_writable);
    us_socket_context_on_close(http_context, on_http_socket_close);
    us_socket_context_on_timeout(http_context, on_http_socket_timeout);
    us_socket_context_on_end(http_context, on_http_socket_end);

    websocket_context = us_create_child_socket_context(http_context, sizeof(struct http_context));

    us_socket_context_on_open(websocket_context, on_web_socket_open);
    us_socket_context_on_data(websocket_context, on_web_socket_data);
    us_socket_context_on_writable(websocket_context, on_web_socket_writable);
    us_socket_context_on_close(websocket_context, on_web_socket_close);
    us_socket_context_on_timeout(websocket_context, on_web_socket_timeout);
    us_socket_context_on_end(websocket_context, on_web_socket_end);

    listen_socket = us_socket_context_listen(http_context, 0, 3000, 0, sizeof(struct http_socket));

    if (listen_socket) {
        printf("Running hammer test\n");
        us_socket_context_connect(http_context, "localhost", 3000, 0, sizeof(struct http_socket));
        us_loop_run(loop);
    } else {
        printf("Cannot listen to port 3000!\n");
    }

    us_socket_context_free(websocket_context);
    us_socket_context_free(http_context);
    us_loop_free(loop);
    free(long_buffer);
    printf("Done, shutting down now\n");
}
