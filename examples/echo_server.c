#include <libusockets.h>
#include <stdio.h>

/* Our socket extension */
struct echo_socket {

};

/* Our socket context extension */
struct echo_context {

};

/* Loop wakeup handler */
void on_wakeup(struct us_loop *loop) {

}

/* Socket writable handler */
void on_echo_socket_writable(struct us_socket *s) {
    struct echo_socket *es = (struct echo_socket *) us_socket_ext(s);

    //http_socket->offset += us_socket_write(s, largeHttpBuf + http_socket->offset, largeHttpBufSize - http_socket->offset, 0);
}

/* Socket closed handler */
void on_echo_socket_close(struct us_socket *s) {
    printf("Client disconnected\n");
}

/* Socket half-closed handler */
void on_echo_socket_end(struct us_socket *s) {
    us_socket_shutdown(s);
    us_socket_close(s);
}

/* Socket data handler */
void on_echo_socket_data(struct us_socket *s, char *data, int length) {
    struct echo_socket *es = (struct echo_socket *) us_socket_ext(s);

    // print the data being sent

    //http_socket->offset = us_socket_write(s, largeHttpBuf, largeHttpBufSize, 0);
}

/* Socket opened handler */
void on_echo_socket_open(struct us_socket *s) {
    struct echo_socket *es = (struct echo_socket *) us_socket_ext(s);

    //http_socket->offset = 0;

    printf("Client connected\n");
}

/* Socket timeout handler */
void on_echo_socket_timeout(struct us_socket *s) {

}

int main() {
    /* The event loop */
    struct us_loop *loop = us_create_loop(1, on_wakeup, on_wakeup, on_wakeup, 0);

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
        printf("Listening on port 3000\n");
        us_loop_run(loop);
    }
}
