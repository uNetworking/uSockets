#include "libusockets.h"

#define USE_SSL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// small response
// fast in ssl
char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 512\r\n\r\n";
int largeHttpBufSize = sizeof(largeBuf) + 512 - 1;

// fast in ssl
//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 5120\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 5120 - 1;

// fast in ssl
//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 51200\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 51200 - 1;

// fast in ssl
//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 512000\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 512000 - 1;

// fast in ssl
//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 5242880\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 5242880 - 1;

// fast in ssl
//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 52428800\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 52428800 - 1;

//large response
//char largeBuf[] = "HTTP/1.1 200 OK\r\nContent-Length: 104857600\r\n\r\n";
//int largeHttpBufSize = sizeof(largeBuf) + 104857600 - 1;

char *largeHttpBuf;

struct http_socket_ext {
    int offset;
};

struct http_context_ext {

};

void on_https_socket_writable(struct us_ssl_socket *s) {
    // update offset!
    us_ssl_socket_write(s, largeHttpBuf, largeHttpBufSize);
}

// keep polling for writable and assume the user will fill the buffer again
// if not, THEN change the poll
void on_http_socket_writable(struct us_socket *s) {

}

void on_http_socket_end(struct us_socket *s) {
    //printf("Disconnected!\n");
}

void on_https_socket_end(struct us_ssl_socket *s) {
    //printf("Disconnected!\n");
}

void on_http_socket_accepted(struct us_socket *s) {
    printf("HTTP socket accepted!\n");
}

void on_http_socket_data(struct us_socket *s, char *data, int length) {
    us_socket_write(s, largeHttpBuf, largeHttpBufSize, 0);
}

void on_https_socket_accepted(struct us_ssl_socket *s) {
    printf("HTTPS socket accepted!\n");
}

void on_https_socket_data(struct us_ssl_socket *s, char *data, int length) {
    us_ssl_socket_write(s, largeHttpBuf, largeHttpBufSize);
}

void on_http_socket_timeout(struct us_socket *s) {
    printf("Dangit we timed out!\n");

    // start timer again
    us_socket_timeout(s, 2);
}

void on_https_socket_timeout(struct us_ssl_socket *s) {
    printf("Dangit we timed out!\n");

    // start timer again
    us_ssl_socket_timeout(s, 2);
}

void timer_cb(struct us_timer *t) {
    printf("Tick!\n");

    //us_wakeup_loop(us_timer_loop(t));
}

void on_wakeup(struct us_loop *loop) {
    printf("Tock!\n");
}

// arg1 ska vara storleken på responsen, kör ett script över alla storlekar
int main() {
    // allocate some garbage data to send
    printf("%d\n", largeHttpBufSize);
    largeHttpBuf = malloc(largeHttpBufSize);
    memcpy(largeHttpBuf, largeBuf, sizeof(largeBuf) - 1);

    // create a loop
    struct us_loop *loop = us_create_loop(on_wakeup, 0); // shound take pre and post callbacks also!

    // start a timer
    struct us_timer *t = us_create_timer(loop, 0, 0);
    us_timer_set(t, timer_cb, 1000, 1000);

    // create either an SSL socket context or a regular socket context
#ifdef USE_SSL
    struct us_ssl_socket_context_options options = {};
    options.key_file_name = "/home/alexhultman/uWebSockets/misc/ssl/key.pem";
    options.cert_file_name = "/home/alexhultman/uWebSockets/misc/ssl/cert.pem";
    options.passphrase = "1234";

    struct us_ssl_socket_context *http_context = us_create_ssl_socket_context(loop, sizeof(struct http_context_ext), options);

    us_ssl_socket_context_on_open(http_context, on_https_socket_accepted);
    us_ssl_socket_context_on_close(http_context, on_https_socket_end);
    us_ssl_socket_context_on_data(http_context, on_https_socket_data);
    us_ssl_socket_context_on_writable(http_context, on_https_socket_writable);
    us_ssl_socket_context_on_timeout(http_context, on_https_socket_timeout);

    struct us_listen_socket *listen_socket = us_ssl_socket_context_listen(http_context, 0, 3000, 0, sizeof(struct http_socket_ext));
#else
    struct us_socket_context *http_context = us_create_socket_context(loop, sizeof(struct http_context_ext));

    us_socket_context_on_open(http_context, on_http_socket_accepted);
    us_socket_context_on_close(http_context, on_http_socket_end);
    us_socket_context_on_data(http_context, on_http_socket_data);
    us_socket_context_on_writable(http_context, on_http_socket_writable);
    us_socket_context_on_timeout(http_context, on_http_socket_timeout);

    struct us_listen_socket *listen_socket = us_socket_context_listen(http_context, 0, 3000, 0, sizeof(struct http_socket_ext));
#endif

    if (listen_socket) {
        printf("Listening!\n");
        us_loop_run(loop);
    }

    printf("falling through!\n");
    return 0;
}
