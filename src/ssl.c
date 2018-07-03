#include "libusockets.h"
#include "internal/common.h"

// this is where ssl-specific overrides are implemented
// all non-overrided function should be shared between both ssl and non-ssl

// really this whole file encapsulates the OpenSSL implementation fully, so any SSL library could be used
// by simply linking to a different ssl.c implementation

#include <openssl/ssl.h>
#include <openssl/bio.h>

struct us_ssl_socket_context {
    struct us_socket_context sc;

    // this thing can be shared with other socket contexts via socket transfer!
    // maybe instead of holding once you hold many, a vector or set
    // when a socket that belongs to another socket context transfers to a new socket context
    SSL_CTX *ssl_context;

    // här måste det vara!
    void (*on_open)(struct us_ssl_socket *);
    void (*on_data)(struct us_ssl_socket *, char *data, int length);
    void (*on_close)(struct us_ssl_socket *);
};

// same applies here, a child is never defined but only seen as an extension to the parent

// och varför i helvete kan inte denna vara en jävla us_socket?
struct us_ssl_socket {
    struct us_socket s;
    SSL *ssl;
    BIO *rbio, *wbio;
};

int passphrase_cb(char *buf, int size, int rwflag, void *u) {
    const char *passphrase = (const char *) u;
    int passphrase_length = strlen(passphrase);
    memcpy(buf, passphrase, passphrase_length);
    // put null at end? no?
    return passphrase_length;
}

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
int more_hint = 0;

// note: we can share the same BIO pair among all SSL of the same thread!
// only 6 kb of SSL state is needed and 16+16kb of temporary buffers!

int BIO_s_custom_write(BIO *bio, const char *data, int length) {

    // important hack: if we have no cork or buffer and the first chunk is less than 1 SSL block of 16 kb then send direclty (we reached the end)
    // else, use MSG_MORE or cork or any corking or even user space buffering

    // msg_more is something we need here!

    //printf("length: %d\n", length);

    //printf("length: %d\n", length);

    return us_socket_write(receive_socket, data, length, /*length > 16000*/more_hint);
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

void *us_ssl_socket_context_ext(struct us_ssl_socket_context *context) {
    return context + 1;
}

struct us_ssl_socket_context *us_ssl_socket_get_context(struct us_ssl_socket *s) {
    return (struct us_ssl_socket_context *) s->s.context;
}

int us_ssl_socket_write(struct us_ssl_socket *s, const char *data, int length, int hint_more) {
    // if we have things to write in the first place!
    receive_buffer_length = 0;
    receive_socket = &s->s;

    more_hint = hint_more;

    return SSL_write(s->ssl, data, length);
}

void ssl_on_open(struct us_ssl_socket *s) {
    struct us_ssl_socket_context *context = (struct us_ssl_socket_context *) us_socket_get_context(&s->s);

    s->ssl = SSL_new(context->ssl_context);
    SSL_set_accept_state(s->ssl);

    s->rbio = BIO_new(BIO_s_custom());
    s->wbio = BIO_new(BIO_s_custom());
    SSL_set_bio(s->ssl, s->rbio, s->wbio);

    context->on_open(s);
}

void ssl_on_close(struct us_ssl_socket *s) {
    struct us_ssl_socket_context *context = (struct us_ssl_socket_context *) us_socket_get_context(&s->s);

    // do ssl close stuff

    context->on_close(s);
}

#define BUF_SIZE 10240
char buf[BUF_SIZE];

void ssl_on_data(struct us_ssl_socket *s, void *data, int length) {
    struct us_ssl_socket_context *context = (struct us_ssl_socket_context *) us_socket_get_context(&s->s);

    // måste sättas per-context! eller iaf per tråd!
    receive_buffer = data;
    receive_buffer_length = length;
    receive_socket = &s->s;

    int read = 0;
    int err;

    while (receive_buffer_length) {
        int last_receive_length = receive_buffer_length;
        err = SSL_read(s->ssl, buf + read, BUF_SIZE - read);
        if (last_receive_length == receive_buffer_length) {
            // terminate the connection because something is seriously wrong here!
        }

        if (err > 0) {
            read += err;
        }
    }

    if (read == -1) {
        int err = SSL_get_error(s->ssl, read);

        if (err == SSL_ERROR_WANT_WRITE) {
            printf("SSL_read want to write\n");
        } else if (err == SSL_ERROR_WANT_READ) {
            printf("SSL_read want to read\n");
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
        if (read > 0) {
            context->on_data(s, buf, read);
        }
    }
}

struct us_ssl_socket_context *us_create_ssl_socket_context(struct us_loop *loop, int context_ext_size, struct us_ssl_socket_context_options options) {

    // should not be needed in openssl 1.1.0+
    SSL_library_init();

    struct us_ssl_socket_context *context = (struct us_ssl_socket_context *) us_create_socket_context(loop, sizeof(struct us_ssl_socket_context) + context_ext_size);

    // this is a server?
    context->ssl_context = SSL_CTX_new(TLS_server_method());

    SSL_CTX_set_options(context->ssl_context, SSL_OP_NO_SSLv3);

    if (options.passphrase) {
        SSL_CTX_set_default_passwd_cb_userdata(context->ssl_context, (void *) options.passphrase);
        SSL_CTX_set_default_passwd_cb(context->ssl_context, passphrase_cb);
    }

    if (options.cert_file_name) {
        if (SSL_CTX_use_certificate_chain_file(context->ssl_context, options.cert_file_name) != 1) {
            return 0;
        }
    }

    if (options.key_file_name) {
        if (SSL_CTX_use_PrivateKey_file(context->ssl_context, options.key_file_name, SSL_FILETYPE_PEM) != 1) {
            return 0;
        }
    }

    return context;
}

struct us_listen_socket *us_ssl_socket_context_listen(struct us_ssl_socket_context *context, const char *host, int port, int options, int socket_ext_size) {
    return us_socket_context_listen(&context->sc, host, port, options, sizeof(struct us_ssl_socket) + socket_ext_size);
}

void us_ssl_socket_context_on_open(struct us_ssl_socket_context *context, void (*on_open)(struct us_ssl_socket *s)) {
    us_socket_context_on_open(&context->sc, (void (*)(struct us_socket *)) ssl_on_open);
    context->on_open = on_open;
}

void us_ssl_socket_context_on_close(struct us_ssl_socket_context *context, void (*on_close)(struct us_ssl_socket *s)) {
    us_socket_context_on_close((struct us_socket_context *) context, (void (*)(struct us_socket *)) ssl_on_close);
    context->on_close = on_close;
}

void us_ssl_socket_context_on_data(struct us_ssl_socket_context *context, void (*on_data)(struct us_ssl_socket *s, char *data, int length)) {
    us_socket_context_on_data((struct us_socket_context *) context, (void (*)(struct us_socket *, char *, int)) ssl_on_data);
    context->on_data = on_data;
}

void us_ssl_socket_context_on_writable(struct us_ssl_socket_context *context, void (*on_writable)(struct us_ssl_socket *s)) {
    us_socket_context_on_writable((struct us_socket_context *) context, (void (*)(struct us_socket *)) on_writable);
}

void us_ssl_socket_context_on_timeout(struct us_ssl_socket_context *context, void (*on_timeout)(struct us_ssl_socket *s)) {
    us_socket_context_on_timeout((struct us_socket_context *) context, (void (*)(struct us_socket *)) on_timeout);
}

void us_ssl_socket_timeout(struct us_ssl_socket *s, unsigned int seconds) {
    us_socket_timeout((struct us_socket *) s, seconds);
}

void *us_ssl_socket_ext(struct us_ssl_socket *s) {
    return s + 1;
}
