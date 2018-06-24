#include "libusockets.h"

#include <stdio.h>
#include <stdlib.h>

struct app_http_socket {
    int offset;
};

struct app_http_context {

};

// small response
//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 512\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 512 - 1;

//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 5120\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 5120 - 1;

//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 51200\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 51200 - 1;

//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 512000\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 512000 - 1;

//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 5242880\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 5242880 - 1;

char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 52428800\r\n\r\n";
int largeHttpBufSize = sizeof(largeBuf) + 52428800 - 1;

//large response
//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 104857600\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 104857600 - 1;

char *largeHttpBuf;

// us_wakeup_loop() triggers
void on_wakeup(struct us_loop *loop) {

}

void on_http_socket_writable(struct us_socket *s) {
    struct app_http_socket *http_socket = (struct app_http_socket *) us_socket_ext(s);

    http_socket->offset += us_socket_write(s, largeHttpBuf + http_socket->offset, largeHttpBufSize - http_socket->offset, 0);
}

void on_http_socket_end(struct us_socket *s) {
    printf("Disconnected!\n");
}

void on_http_socket_data(struct us_socket *s, void *data, int length) {
    struct app_http_socket *http_socket = (struct app_http_socket *) us_socket_ext(s);

    http_socket->offset = us_socket_write(s, largeHttpBuf, largeHttpBufSize, 0);
}

// we can get both socket and its context so user data is not needed!
void on_http_socket_accepted(struct us_socket *s) {
    struct app_http_socket *http_socket = (struct app_http_socket *) us_socket_ext(s);

    http_socket->offset = 0;
}

void on_http_socket_timeout(struct us_socket *s) {

}

int main() {

    printf("%d\n", largeHttpBufSize);

    largeHttpBuf = malloc(largeHttpBufSize);
    memcpy(largeHttpBuf, largeBuf, sizeof(largeBuf) - 1);

    // this section reproduces the strange major drop in throughput (8gb/sec -> 5gb/sec)
    char *tmp = malloc(largeHttpBufSize);
    memcpy(tmp, largeHttpBuf, largeHttpBufSize);
    largeHttpBuf = tmp;

    // create the loop, and register a wakeup handler
    struct us_loop *loop = us_create_loop(on_wakeup, on_wakeup, on_wakeup, 0); // shound take pre and post callbacks also!

    // create a context (behavior) for httpsockets
    struct us_socket_context *http_context = us_create_socket_context(loop, sizeof(struct app_http_context));

    us_socket_context_on_open(http_context, on_http_socket_accepted);
    us_socket_context_on_data(http_context, on_http_socket_data);
    us_socket_context_on_writable(http_context, on_http_socket_writable);
    us_socket_context_on_close(http_context, on_http_socket_end);
    us_socket_context_on_timeout(http_context, on_http_socket_timeout);

    // start accepting http sockets
    struct us_listen_socket *listen_socket = us_socket_context_listen(http_context, 0, 3000, 0, sizeof(struct app_http_socket));

    if (listen_socket) {
        printf("Listening!\n");
        us_loop_run(loop);
    }

    printf("falling through!\n");
    return 0;
}
