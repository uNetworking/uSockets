#include "libusockets.h"

#include <stdio.h>
#include <stdlib.h>

struct app_http_socket {
    struct us_socket s;
    int offset;
};

struct app_web_socket {
    struct us_socket s;
    int bunch_of_shit;
};

struct app_http_context {
    struct us_socket_context context;
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

// keep polling for writable and assume the user will fill the buffer again
// if not, THEN change the poll
void on_http_socket_writable(struct us_socket *s) {
    struct app_http_socket *http_socket = (struct app_http_socket *) s;

    //printf("Offset: %d\n", http_socket->offset);
    http_socket->offset += us_socket_write(s, largeHttpBuf + http_socket->offset, largeHttpBufSize - http_socket->offset);
}

void on_http_socket_end(struct us_socket *s) {
    printf("Disconnected!\n");
}

void on_http_socket_data(struct us_socket *s, void *data, int length) {
    //printf("%.*s", length, data);

    // let's just echo it back for now
    //us_socket_write(s, buf, sizeof(buf) - 1);


    // start streaming the response
    struct app_http_socket *http_socket = (struct app_http_socket *) s;

    //printf("Offset: %d\n", http_socket->offset);
    http_socket->offset = us_socket_write(s, largeHttpBuf, largeHttpBufSize);
}

// we can get both socket and its context so user data is not needed!
void on_http_socket_accepted(struct us_socket *s) {
    struct app_http_socket *http_socket = (struct app_http_socket *) s;

    // link this socket into this context (enables timers, etc)
    //us_socket_context_link(s->context, s);

    http_socket->offset = 0;

    //printf("Ansluten!\n");

    // let's expect some data within 10 seconds
    us_socket_timeout(s, 10);
}

void on_http_socket_timeout(struct us_socket *s) {
    printf("Dangit we timed out!\n");

    // start timer again
    us_socket_timeout(s, 2);
}

int main() {

    printf("%d\n", largeHttpBufSize);

    largeHttpBuf = malloc(largeHttpBufSize);
    memcpy(largeHttpBuf, largeBuf, sizeof(largeBuf) - 1);

    printf("Sizeof us_socket: %ld\n", sizeof(struct us_socket));

    // create the loop, and register a wakeup handler
    struct us_loop *loop = us_create_loop(on_wakeup, 0); // shound take pre and post callbacks also!

    // create a context (behavior) for httpsockets
    struct us_socket_context *http_context = us_create_socket_context(loop, sizeof(struct app_http_context));

    http_context->on_accepted = on_http_socket_accepted;
    http_context->on_data = on_http_socket_data;
    http_context->on_writable = on_http_socket_writable;
    http_context->on_end = on_http_socket_end;
    http_context->on_socket_timeout = on_http_socket_timeout;

    // start accepting http sockets
    struct us_listen_socket *listen_socket = us_socket_context_listen(http_context, 0, 3000, 0, sizeof(struct app_http_socket));

    // test a listen socket timeout
    //us_socket_timeout(listen_socket, 2);

    if (listen_socket) {
        printf("Listening!\n");
        us_loop_run(loop);
    }

    printf("falling through!\n");
    return 0;
}
