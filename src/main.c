#include "libusockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/bio.h>

struct app_http_socket {
    struct us_socket s;
    SSL *ssl;
    BIO *rbio, *wbio;
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


////////////////////////////////////////////

// trick is to not use any of BIO_write or BIO_read ourselves, only internal SSL
// we control the BIO with zero-copy setters and getters

int BIO_s_custom_create(BIO *bio) {
    //printf("BIO_s_custom_create called\n");

    BIO_set_data(bio, 0); // our user data

    BIO_set_init(bio, 1);

    return 1;
}

long BIO_s_custom_ctrl(BIO *bio, int cmd, long num, void *user) {
    //printf("Command: %d\n", cmd);

    switch(cmd) {
    case BIO_CTRL_FLUSH:
        return 1;
    }

//# define BIO_CTRL_RESET          1/* opt - rewind/zero etc */
//# define BIO_CTRL_EOF            2/* opt - are we at the eof */
//# define BIO_CTRL_INFO           3/* opt - extra tit-bits */
//# define BIO_CTRL_SET            4/* man - set the 'IO' type */
//# define BIO_CTRL_GET            5/* man - get the 'IO' type */
//# define BIO_CTRL_PUSH           6/* opt - internal, used to signify change */
//# define BIO_CTRL_POP            7/* opt - internal, used to signify change */
//# define BIO_CTRL_GET_CLOSE      8/* man - set the 'close' on free */
//# define BIO_CTRL_SET_CLOSE      9/* man - set the 'close' on free */
//# define BIO_CTRL_PENDING        10/* opt - is their more data buffered */
//# define BIO_CTRL_FLUSH          11/* opt - 'flush' buffered output */
//# define BIO_CTRL_DUP            12/* man - extra stuff for 'duped' BIO */
//# define BIO_CTRL_WPENDING       13/* opt - number of bytes still to write */
//# define BIO_CTRL_SET_CALLBACK   14/* opt - set callback function */
//# define BIO_CTRL_GET_CALLBACK   15/* opt - set callback function */



    // we do not understand
    return 0;
}

// this is set before SSL_read, and set to 0 before SSL_write!
char *receive_buffer;
int receive_buffer_length;
struct us_socket *receive_socket;

// note: we can share the same BIO pair among all SSL of the same thread!
// only 6 kb of SSL state is needed and 16+16kb of temporary buffers!

int BIO_s_custom_write(BIO *bio, const char *data, int length) {

    // important hack: if we have no cork or buffer and the first chunk is less than 1 SSL block of 16 kb then send direclty (we reached the end)
    // else, use MSG_MORE or cork or any corking or even user space buffering

    // msg_more is something we need here!

    return us_socket_write(receive_socket, data, length, length > 16000);
}

// make sure to reset receive_buffer before ssl_write is called? no?
int BIO_s_custom_read(BIO *bio, char *dst, int length) {
    if (receive_buffer_length == 0) {
        // we need to signal this was not an IO error but merely no more data!
        BIO_set_flags(bio, BIO_get_flags(bio) | BIO_FLAGS_SHOULD_RETRY | BIO_FLAGS_READ);
        return 0;
    }

    // only called by openssl to read from recv buffer?
    // could also be called by ssl_write!

    if (length > receive_buffer_length) {
        length = receive_buffer_length;
    }

    memcpy(dst, receive_buffer, length);

    receive_buffer += length;
    receive_buffer_length -= length;
    return length;
}

BIO_METHOD *BIO_s_custom() {
    BIO_METHOD *biom = BIO_meth_new(/*BIO_TYPE_SOURCE_SINK*/ BIO_TYPE_MEM, "µS BIO type");

    BIO_meth_set_create(biom, BIO_s_custom_create);
    BIO_meth_set_write(biom, BIO_s_custom_write);
    BIO_meth_set_read(biom, BIO_s_custom_read);
    BIO_meth_set_ctrl(biom, BIO_s_custom_ctrl);

    return biom;
}


///////////////////////////////////////////


// us_wakeup_loop() triggers
void on_wakeup(struct us_loop *loop) {

}

// keep polling for writable and assume the user will fill the buffer again
// if not, THEN change the poll
void on_http_socket_writable(struct us_socket *s) {
    struct app_http_socket *http_socket = (struct app_http_socket *) s;

    // if we have things to write in the first place!
    receive_buffer_length = 0;
    receive_socket = s;

    if (http_socket->offset == 0) {
        http_socket->offset += SSL_write(http_socket->ssl, largeHttpBuf, largeHttpBufSize);
    }
}

void on_http_socket_end(struct us_socket *s) {
    //printf("Disconnected!\n");
}

#define BUF_SIZE 10240
char buf[BUF_SIZE];

void on_http_socket_data(struct us_socket *s, void *data, int length) {

    //printf("Got data\n\n");

    // start streaming the response
    struct app_http_socket *http_socket = (struct app_http_socket *) s;

    // kernel -> recv buffer -> SSL -> ssl recv buffer -> application

    // vs.

    // kernel -> recv buffer -> BIO buffer -> SSL -> BIO buffer -> recv buffer -> application
    receive_buffer = data;
    receive_buffer_length = length;
    receive_socket = s;
    //printf("Calling SSL_read\n");
    //size_t bytes_read;

    // något är fel här, debugga skiten
    // se vad för fel det är med SSL_read -> kan vara så att
    // write failar och måste buffras upp?

    // kör SSL_read i debug efter den failat att läsa allt och se var det klämmer!

    int read = 0;

    while (receive_buffer_length) {
        int last_receive_length = receive_buffer_length;
        int t = SSL_read(http_socket->ssl, buf + read, BUF_SIZE - read);
        if (last_receive_length == receive_buffer_length) {
            // what even happens here? spill? why!?
            printf("Receive buffer left: %d our read is: %d\n", receive_buffer_length, read);
            break;
        } else {
            //printf("Strangeness: SSL_read did not read everything in the BIO\n");
        }

        if (t > 0) {
            read += t;
        }
    }

    // reset receive buffer here!
    receive_buffer_length = 0;
    receive_buffer = 0;

    if (read == -1) {
        int err = SSL_get_error(http_socket->ssl, read);

        if (err == SSL_ERROR_WANT_WRITE) {
            printf("SSL_read want to write\n");
        } else if (err == SSL_ERROR_WANT_READ) {
            //printf("SSL_read want to read\n");
            return;
        } else {
            // this is unwanted, treat any of these errors as serious


            if (err == SSL_ERROR_SSL) {
                printf("SSL_ERROR_SSL\n");
            } else if (err == SSL_ERROR_SYSCALL) {
                printf("SSL_ERROR_SYSCALL\n");
            } else {
                printf("Error: %d\n", err);
            }

        }
    } else {

        //printf("Read from openssl %d bytes\n", read);
        //printf("%.*s", read, buf);


        // we should problaby only write 16kb of data a go
        // SSL_write calls BIO_write with 16.02 kb of raw data to send
        // I guess if BIO_write can signal error as soon as we have backpressure it is okay
        // we stream 16kb of data per syscall that is

        // we might want to buffer up the data in a custom buffering BIO for writes?

        // or just set some kind of variable bool buffer = true

        // stream data to openssl
        //printf("Calling SSL_write\n");
        int ssl_written = SSL_write(http_socket->ssl, largeHttpBuf, largeHttpBufSize);
        http_socket->offset = ssl_written;
        //printf("Wrote unencrypted: %d\n", ssl_written);


        // kolla felet här (ska vara wants write)


    }

   }

// we can get both socket and its context so user data is not needed!
void on_http_socket_accepted(struct us_socket *s) {
    //printf("Got new connection\n");

    struct app_http_socket *http_socket = (struct app_http_socket *) s;

    // init ssl layer
    struct app_http_context *http_context = (struct app_http_context *) http_socket->s.context;

    http_socket->ssl = SSL_new(http_context->ssl_context);

    // create bios
    http_socket->rbio = BIO_new(BIO_s_custom());
    http_socket->wbio = BIO_new(BIO_s_custom());

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

    // any buffer large for write
    //ssl_output_buffer_original_length = 1024 * 1024 * 1024;
    //ssl_output_buffer_original = malloc(ssl_output_buffer_original_length);

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

    SSL_load_error_strings();
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


    // create a bunch of sockets for memory usage tests
    /*for (int i = 0; i < 100000; i++) {
        // varje är ca 6kb?
        SSL_new(http_context_->ssl_context);
        //BIO_new(BIO_s_custom());
        //BIO_new(BIO_s_custom());
    }*/


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
