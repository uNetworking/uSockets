/* This example, or test, is a moron test where the library is being hammered in all the possible ways randomly over time */

#include <libusockets.h>
const int SSL = 1;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 60

void print_progress(double percentage) {
    static int last_val = -1;
    int val = (int) (percentage * 100);
    if (last_val != -1 && val == last_val) {
        return;
    }
    last_val = val;
    int lpad = (int) (percentage * PBWIDTH);
    int rpad = PBWIDTH - lpad;
    printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
    fflush(stdout);
}

// todo: properly put all of these in various ext data so to test them!
int opened_connections, closed_connections, operations_done;
struct us_socket_context_t *http_context, *websocket_context;
struct us_listen_socket_t *listen_socket;

int opened_clients, opened_servers, closed_clients, closed_servers;

// put in loop ext data
void *long_buffer;
unsigned int long_length = 5 * 1024 * 1024;

// also make sure to have socket ext data
// and context ext data
// and loop ext data

const double pad_should_always_be = 14.652752;

/* We begin smaller than the WS so that resize must fail (opposite of what we have in uWS).
 * This triggers the libuv resize crash that must be fixed. */
struct http_socket {
    double pad_invariant;
    int is_http;
    double post_pad_invariant;
    int is_client;
    char content[128];
};

struct web_socket {
    double pad_invariant;
    int is_http;
    double post_pad_invariant;
    int is_client;
    char content[1024];
};

/* This checks the ext data state according to callbacks */
void assume_state(struct us_socket_t *s, int is_http) {
    struct http_socket *hs = (struct http_socket *) us_socket_ext(SSL, s);

    if (hs->pad_invariant != pad_should_always_be || hs->post_pad_invariant != pad_should_always_be) {
        printf("ERROR: Pad invariant is not correct!\n");
        free((void *) 1);
    }

    if (hs->is_http != is_http) {
        printf("ERROR: State is: %d should be: %d. Terminating now!\n", hs->is_http, is_http);
        free((void *) 1);
    }

    // try and cause havoc (different size)
    if (hs->is_http) {
        memset(hs->content, 0, 128);
    } else {
        memset(hs->content, 0, 1024);
    }
}

struct http_context {
    // link to the other context here
    char content[1];
};

// todo: it would be nice to randomly select socket instead of
// using the one responsible for the event
struct us_socket_t *perform_random_operation(struct us_socket_t *s) {
    switch (rand() % 5) {
        case 0: {
            // close
            return us_socket_close(SSL, s, 0, NULL);
        }
        case 1: {
            // adoption cannot happen if closed!
            if (!us_socket_is_closed(SSL, s)) {
                if (rand() % 2) {
                    s = us_socket_context_adopt_socket(SSL, websocket_context, s, sizeof(struct web_socket));
                    struct http_socket *hs = (struct http_socket *) us_socket_ext(SSL, s);
                    hs->is_http = 0;
                } else {
                    s = us_socket_context_adopt_socket(SSL, http_context, s, sizeof(struct http_socket));
                    struct http_socket *hs = (struct http_socket *) us_socket_ext(SSL, s);
                    hs->is_http = 1;
                }
            }

            return perform_random_operation(s);
        }
        case 2: {
            // write - causes the other end to receive the data (event) and possibly us
            // to receive on writable event - could it be that we get stuck if the other end is closed?
            // no because, if we do not get ack in time we will timeout after some time
            us_socket_write(SSL, s, (char *) long_buffer, rand() % long_length, 0);
        }
        break;
        case 3: {
            // shutdown (on macOS we can get stuck in fin_wait_2 for some weird reason!)
            // if we send fin, the other end sends data but then on writable closes? then fin is not sent?
            // so we need to timeout here to ensure we are closed if no fin is received within 30 seconds
            us_socket_shutdown(SSL, s);
            us_socket_timeout(SSL, s, 16);
        }
        break;
        case 4: {
            /* Triggers all timeouts next iteration */
            us_socket_timeout(SSL, s, 4);
            us_wakeup_loop(us_socket_context_loop(SSL, us_socket_context(SSL, s)));
        }
        break;
    }
    return s;
}

void on_wakeup(struct us_loop_t *loop) {
    // note: we expose internal functions to trigger a timeout sweep to find bugs
    extern void us_internal_timer_sweep(struct us_loop_t *loop);
    
    //us_internal_timer_sweep(loop);
}

// maybe use thse to count spurious wakeups?
// that is, if we get tons of pre/post over and over without any events
// that would point towards 100% cpu usage kind of bugs
void on_pre(struct us_loop_t *loop) {

}

void on_post(struct us_loop_t *loop) {
    // check if we did perform_random_operation
}

struct us_socket_t *on_web_socket_writable(struct us_socket_t *s) {
    assume_state(s, 0);

    return perform_random_operation(s);
}

struct us_socket_t *on_http_socket_writable(struct us_socket_t *s) {
    assume_state(s, 1);

    return perform_random_operation(s);
}

struct us_socket_t *on_web_socket_close(struct us_socket_t *s, int code, void *reason) {
    assume_state(s, 0);

    struct web_socket *ws = (struct web_socket *) us_socket_ext(SSL, s);

    if (ws->is_client) {
        closed_clients++;
    } else {
        closed_servers++;
    }

    closed_connections++;

    print_progress((double) closed_connections / 10000);

    if (closed_connections == 10000) {
        if (opened_clients != 5000) {
            printf("VA I HELVETE STÄNGER LISTEN INNAN ÖPPNAT ALLA CLIENTS! %d\n", opened_clients);
            exit(1);
        }

        us_listen_socket_close(SSL, listen_socket);
    } else {
        return perform_random_operation(s);
    }
    return s;
}

struct us_socket_t *on_http_socket_close(struct us_socket_t *s, int code, void *reason) {
    assume_state(s, 1);

    struct http_socket *hs = (struct http_socket *) us_socket_ext(SSL, s);

    if (hs->is_client) {
        closed_clients++;
    } else {
        closed_servers++;
    }

    print_progress((double) closed_connections / 10000);

    closed_connections++;
    
    if (closed_connections == 10000) {
        if (opened_clients != 5000) {
            printf("VA I HELVETE STÄNGER LISTEN INNAN ÖPPNAT ALLA CLIENTS! %d\n", opened_clients);
            exit(1);
        }
        us_listen_socket_close(SSL, listen_socket);
    } else {
        return perform_random_operation(s);
    }
    return s;
}

/* Same as below */
struct us_socket_t *on_web_socket_end(struct us_socket_t *s) {
    assume_state(s, 0);

    // we need to close on shutdown
    s = us_socket_close(SSL, s, 0, NULL);
    return perform_random_operation(s);
}

struct us_socket_t *on_http_socket_end(struct us_socket_t *s) {
    assume_state(s, 1);

    /* Getting a FIN and we just close down */
    s = us_socket_close(SSL, s, 0, NULL);

    /* I guess we do this, to extra check that following actions are ignored,
     * we really should just return s here, but to stress the lib we do this */
    return perform_random_operation(s);
}

struct us_socket_t *on_web_socket_data(struct us_socket_t *s, char *data, int length) {
    assume_state(s, 0);

    if (length == 0) {
        printf("ERROR: Got data event with no data\n");
        exit(-1);
    }

    return perform_random_operation(s);
}

struct us_socket_t *on_http_socket_data(struct us_socket_t *s, char *data, int length) {
    assume_state(s, 1);

    if (length == 0) {
        printf("ERROR: Got data event with no data\n");
        exit(-1);
    }

    return perform_random_operation(s);
}

struct us_socket_t *on_web_socket_open(struct us_socket_t *s, int is_client, char *ip, int ip_length) {
    // fail here, this can never happen!
    printf("ERROR: on_web_socket_open called!\n");
    exit(-2);
}

/* This one drives progress, or well close actually drives progress but this allows sockets to close */
struct us_socket_t *next_connection() {

    if (opened_clients == 5000) {
        printf("ERROR! next_connection called when already having made all!\n");
        free((void *) -1);
    }

    struct us_socket_t *connection_socket;
    if (!(connection_socket = us_socket_context_connect(SSL, http_context, "127.0.0.1", 3000, NULL, 0, sizeof(struct http_socket)))) {
        /* This one we do not deal with, so just exit */
        printf("FAILED TO START CONNECTION, WILL EXIT NOW\n");
        exit(1);
    }

    /* Typically, in real applications you would want to us_socket_timeout the connection_socket
     * to track a maximal connection timeout. However, because this test relies on perfect sync
     * between number of server side sockets and number of client side sockets, we cannot use this
     * feature since it is possible we send a SYN, timeout and close the client socket while
     * the listen side accepts the SYN and opens up while the client side is missing, causing the
     * test to fail. */

    return connection_socket;
}

struct us_socket_t *on_http_socket_connect_error(struct us_socket_t *s, int code) {
    /* On macOS it is common for a connect to end up here. Even though we have no real timeout
     * it seems the system has a default connect timeout and we end up here. */
    next_connection();

    return s;
}

struct us_socket_t *on_web_socket_connect_error(struct us_socket_t *s, int code) {
    printf("ERROR: WebSocket can never get connect errors!\n");
    exit(1);

    return s;
}

struct us_socket_t *on_http_socket_open(struct us_socket_t *s, int is_client, char *ip, int ip_length) {
    struct http_socket *hs = (struct http_socket *) us_socket_ext(SSL, s);
    hs->is_http = 1;
    hs->pad_invariant = pad_should_always_be;
    hs->post_pad_invariant = pad_should_always_be;
    hs->is_client = is_client;

    assume_state(s, 1);

    opened_connections++;
    if (is_client) {
        opened_clients++;
    } else {
        opened_servers++;
    }

    if (is_client && opened_clients < 5000) {
        next_connection();
    }

    return perform_random_operation(s);
}

struct us_socket_t *on_web_socket_timeout(struct us_socket_t *s) {
    assume_state(s, 0);

    return perform_random_operation(s);
}

struct us_socket_t *on_http_socket_timeout(struct us_socket_t *s) {

    /* If we timed out before being established, we should always cancel connecting and connect again */
    if (!us_socket_is_established(SSL, s)) {

        if (s != (struct us_socket_t *) listen_socket) {
            /* Connect sockets are not allowed to timeout since, in this hammer_test
             * we can get a connection server-side yet no connection client side due
             * to sending a SYN, the closing due to timeout, yet having the server side
             * eventually accept and open its side causing out of sync */
            printf("CONNECTION TIMEOUT!!! CANNOT HAPPEN!!\n");
            exit(1);

            /* It would be perfectly valid to perform the following if we did not care for
             * having number of opened sockets server side synced with number of opened sockets
             * client side (the following is typically what you would do) */
            us_socket_close_connecting(SSL, s);
            next_connection();
        }

        /* Okay, so this is the listen_socket */
        static time_t last_time;

        if (last_time && time(0) - last_time == 0) {
            printf("TIMER IS FIRING TOO FAST!!!\n");
            exit(1);
        }

        last_time = time(0);

        print_progress((double) closed_connections / 10000);
        us_socket_timeout(SSL, s, 16);
        return s;
    }

    /* Assume this established socket is in the state of http */
    assume_state(s, 1);

    /* An established socket has timed out */
    if (us_socket_is_shut_down(SSL, s)) {
        /* If we have sent FIN but not received a FIN on the other side,
         * we can actually end up stuck in FIN_WAIT_2 without seeing any
         * progress (FIN_WAIT_2 is not a state kqueue will report as we
         * still can get data). macOS does not seem to send FIN for closed
         * sockets in all cases. */
        return us_socket_close(SSL, s, 0, 0);
    }

    return perform_random_operation(s);
}

int main() {
    srand(time(0));
    long_buffer = calloc(long_length, 1);

    struct us_loop_t *loop = us_create_loop(0, on_wakeup, on_pre, on_post, 0);

    // us_loop_on_wakeup()
    // us_loop_on_pre()
    // us_loop_on_post()
    // us_loop_on_poll()
    // us_loop_on_timer()


    // these are ignored for non-SSL
    struct us_socket_context_options_t options = {};
    options.key_file_name = "/home/alexhultman/uWebSockets.js/misc/key.pem";
    options.cert_file_name = "/home/alexhultman/uWebSockets.js/misc/cert.pem";
    options.passphrase = "1234";

    http_context = us_create_socket_context(SSL, loop, sizeof(struct http_context), options);


    us_socket_context_on_open(SSL, http_context, on_http_socket_open);
    us_socket_context_on_data(SSL, http_context, on_http_socket_data);
    us_socket_context_on_writable(SSL, http_context, on_http_socket_writable);
    us_socket_context_on_close(SSL, http_context, on_http_socket_close);
    us_socket_context_on_timeout(SSL, http_context, on_http_socket_timeout);
    us_socket_context_on_end(SSL, http_context, on_http_socket_end);
    us_socket_context_on_connect_error(SSL, http_context, on_http_socket_connect_error);

    websocket_context = us_create_child_socket_context(SSL, http_context, sizeof(struct http_context));

    us_socket_context_on_open(SSL, websocket_context, on_web_socket_open);
    us_socket_context_on_data(SSL, websocket_context, on_web_socket_data);
    us_socket_context_on_writable(SSL, websocket_context, on_web_socket_writable);
    us_socket_context_on_close(SSL, websocket_context, on_web_socket_close);
    us_socket_context_on_timeout(SSL, websocket_context, on_web_socket_timeout);
    us_socket_context_on_end(SSL, websocket_context, on_web_socket_end);
    us_socket_context_on_connect_error(SSL, websocket_context, on_web_socket_connect_error);

    listen_socket = us_socket_context_listen(SSL, http_context, "127.0.0.1", 3000, 0, sizeof(struct http_socket));

    /* We use the listen socket as a way to check so that timeout stamps don't
     * deviate from wallclock time - let's use 16 seconds and check that we have
     * at least 1 second diff since last trigger (allows iteration lag of 15 seconds) */
    us_socket_timeout(SSL, (struct us_socket_t *) listen_socket, 16);

    if (listen_socket) {
        printf("Running hammer test\n");
        print_progress(0);
        next_connection();

        us_loop_run(loop);
    } else {
        printf("Cannot listen to port 3000!\n");
    }

    us_socket_context_free(SSL, websocket_context);
    us_socket_context_free(SSL, http_context);
    us_loop_free(loop);
    free(long_buffer);
    print_progress(1);
    printf("\n");

    if (opened_clients == 5000 && closed_clients == 5000 && opened_servers == 5000 && closed_servers == 5000) {
        printf("ALL GOOD\n");
        return 0;
    } else {
        printf("MISMATCHING! FAILED!\n");
        return 1;
    }
}
