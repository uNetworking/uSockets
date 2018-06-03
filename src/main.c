#include "libusockets.h"

#include <stdio.h>
#include <stdlib.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>

struct app_http_socket {
    struct us_socket s;
    struct SSL *ssl;
    struct BIO *rbio, *wbio;
    int offset;
};

struct app_web_socket {
    struct us_socket s;
    int bunch_of_shit;
};

struct app_http_context {
    struct us_socket_context context;
    struct SSL_CTX *ssl_context;
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

char *discarder;

// us_wakeup_loop() triggers
void on_wakeup(struct us_loop *loop) {

}

// keep polling for writable and assume the user will fill the buffer again
// if not, THEN change the poll
void on_http_socket_writable(struct us_socket *s) {
    struct app_http_socket *http_socket = (struct app_http_socket *) s;



    void *pp;
    int to_write = BIO_get_mem_data(http_socket->wbio, &pp);

    //printf("We have %d bytes in out buffer\n", to_write);

    //printf("Offset: %d\n", http_socket->offset);
    int written = us_socket_write(s, pp, to_write);

    // now we pop this read from the bio
    // we should not copy out!
    // we need to create a custom BIO with zero-copy
    // target openssl 1.1.0+
    BIO_read(http_socket->wbio, discarder, written);

    http_socket->offset += written;

    //printf("Offset: %d\n", http_socket->offset);
    //http_socket->offset += us_socket_write(s, largeHttpBuf + http_socket->offset, largeHttpBufSize - http_socket->offset);
}

void on_http_socket_end(struct us_socket *s) {
    printf("Disconnected!\n");
}

#define BUF_SIZE 10240
char buf[BUF_SIZE];

void on_http_socket_data(struct us_socket *s, void *data, int length) {
    //printf("%.*s", length, data);

    // let's just echo it back for now
    //us_socket_write(s, buf, sizeof(buf) - 1);


    //printf("Got data\n");



    // start streaming the response
    struct app_http_socket *http_socket = (struct app_http_socket *) s;


    int written = BIO_write(http_socket->rbio, data, length);
    //printf("Wrote %d bytes of %d incoming\n", written, length);

    int read = SSL_read(http_socket->ssl, buf, BUF_SIZE);

    // did we produce any output?
    int write_size = BIO_ctrl_pending(http_socket->wbio);
    if (write_size) {
           printf("write size: %d\n", write_size);

           BIO_read(http_socket->wbio, buf, BUF_SIZE);

           // skriv skiten här!
           us_socket_write(http_socket, buf, write_size);
    }

    if (read == -1) {
        int err = SSL_get_error(http_socket->ssl, read);

        if (err == SSL_ERROR_WANT_WRITE) {
            printf("SSL_read want to write\n");
        } else if (err == SSL_ERROR_WANT_READ) {
            printf("SSL_read want to read\n");
        } else {
            printf("Error: %d\n", err);
        }
    } else {

        http_socket->offset = 0;

        //printf("Read from openssl %d bytes\n", read);
        //printf("%.*s", read, buf);

        //printf("We have %d bytes in out buffer\n", BIO_ctrl_pending(http_socket->wbio));

        // stream data to openssl
        int ssl_written = SSL_write(http_socket->ssl, largeHttpBuf, largeHttpBufSize);
        //printf("wrote ssl: %d\n", ssl_written);

        //printf("We have %d bytes in out buffer\n", BIO_ctrl_pending(http_socket->wbio));

        void *pp;
        int to_write = BIO_get_mem_data(http_socket->wbio, &pp);

        //printf("We have %d bytes in out buffer\n", to_write);

        //printf("Offset: %d\n", http_socket->offset);
        http_socket->offset = us_socket_write(s, pp, to_write);

        // now we pop this read from the bio
        // we should not copy out!
        // we need to create a custom BIO with zero-copy
        // target openssl 1.1.0+
        BIO_read(http_socket->wbio, discarder, http_socket->offset);
        //BIO_seek(http_socket->wbio, http_socket->offset);
    }

   }

// we can get both socket and its context so user data is not needed!
void on_http_socket_accepted(struct us_socket *s) {
    struct app_http_socket *http_socket = (struct app_http_socket *) s;

    // init ssl layer
    struct app_http_context *http_context = (struct app_http_context *) http_socket->s.context;

    http_socket->ssl = SSL_new(http_context->ssl_context);

    // create bios
    http_socket->rbio = BIO_new(BIO_s_mem());
    http_socket->wbio = BIO_new(BIO_s_mem());

    //
    SSL_set_accept_state(http_socket->ssl);
    SSL_set_bio(http_socket->ssl, http_socket->rbio, http_socket->wbio);


    http_socket->offset = 0;
}

void on_http_socket_timeout(struct us_socket *s) {
    printf("Dangit we timed out!\n");

    // start timer again
    us_socket_timeout(s, 2);
}

// everything should be hidden like us_loop, only give pointers to user data

// ssl layer


// should this extend a regular socket context with a middle layer?
/*struct us_ssl_socket_context {
    struct SSL_CTX *ssl_context;
};

struct us_ssl_socket_context *us_create_ssl_socket_context(struct us_loop *loop, int context_size) {
    struct us_ssl_socket_context *context = malloc(context_size);

    // init the context
    context->ssl_context = SSL_CTX_new(SSLv23_method());

    return context;
}

// listen
void us_ssl_socket_context_listen(struct us_ssl_socket_context *context) {

}*/

// password callback
int passwordCallback(char *buf, int size, int rwflag, void *u) {
    printf("password cb\n");
    char *password = "1234";
    int length = 4;
    //std::string *password = (std::string *) u;
    //int length = std::min<int>(size, (int) password->length());
    memcpy(buf, password, length);
    buf[length] = '\0';
    return length;
}

int main() {

    discarder = malloc(1024 * 1024 * 1024);

    printf("%d\n", largeHttpBufSize);

    largeHttpBuf = malloc(largeHttpBufSize);
    memcpy(largeHttpBuf, largeBuf, sizeof(largeBuf) - 1);

    printf("Sizeof us_socket: %ld\n", sizeof(struct us_socket));

    // create the loop, and register a wakeup handler
    struct us_loop *loop = us_create_loop(on_wakeup, 0); // shound take pre and post callbacks also!


    // ssl is its own context
    //struct us_ssl_socket_context *ssl_context = us_create_ssl_socket_context(loop, sizeof(struct us_ssl_socket_context));


    // create a context (behavior) for httpsockets
    struct us_socket_context *http_context = us_create_socket_context(loop, sizeof(struct app_http_context));

    // init SSL context
    struct app_http_context *http_context_ = (struct app_http_context *) http_context;

    SSL_library_init();

    // init ssl context
    http_context_->ssl_context = SSL_CTX_new(SSLv23_server_method());

    printf("Context: %ld\n", http_context_->ssl_context);


    SSL_CTX_set_options(http_context_->ssl_context, SSL_OP_NO_SSLv3);

    if (/*keyFilePassword.length()*/ 1) {
        //SSL_CTX_set_default_passwd_cb_userdata(context.context, context.password.get());
        SSL_CTX_set_default_passwd_cb(http_context_->ssl_context, passwordCallback);
    }

    if (SSL_CTX_use_certificate_chain_file(http_context_->ssl_context, /*certChainFileName.c_str()*/ "/home/alexhultman/uWebSockets/misc/ssl/cert.pem") != 1) {
        return -1;
    } else if (SSL_CTX_use_PrivateKey_file(http_context_->ssl_context, /*keyFileName.c_str()*/ "/home/alexhultman/uWebSockets/misc/ssl/key.pem", SSL_FILETYPE_PEM) != 1) {
        return -1;
    }

    printf("passed ssl contenxt init\n");

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
