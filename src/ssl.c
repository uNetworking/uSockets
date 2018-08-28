#define LIBUS_NO_SSL

#ifndef LIBUS_NO_SSL

#include "libusockets.h"
#include "internal/common.h"

/* This module contains the entire OpenSSL implementation
 * of the SSL socket and socket context interfaces. */

#include <openssl/ssl.h>
#include <openssl/bio.h>

struct loop_ssl_data {
    char *ssl_read_input, *ssl_read_output;
    unsigned int ssl_read_input_length;
    unsigned int ssl_read_input_offset;
    struct us_socket *ssl_socket;

    int last_write_was_msg_more;
    int msg_more;

    BIO *shared_rbio;
    BIO *shared_wbio;
    BIO_METHOD *shared_biom;
};

struct us_ssl_socket_context {
    struct us_socket_context sc;

    // this thing can be shared with other socket contexts via socket transfer!
    // maybe instead of holding once you hold many, a vector or set
    // when a socket that belongs to another socket context transfers to a new socket context
    SSL_CTX *ssl_context;

    // här måste det vara!
    void (*on_open)(struct us_ssl_socket *, int is_client);
    void (*on_data)(struct us_ssl_socket *, char *data, int length);
    void (*on_close)(struct us_ssl_socket *);
};

// same here, should or shouldn't it contain s?
struct us_ssl_socket {
    struct us_socket s;
    SSL *ssl;
    int ssl_write_wants_read; // we use this for now
};

int passphrase_cb(char *buf, int size, int rwflag, void *u) {
    const char *passphrase = (const char *) u;
    int passphrase_length = strlen(passphrase);
    memcpy(buf, passphrase, passphrase_length);
    // put null at end? no?
    return passphrase_length;
}

int BIO_s_custom_create(BIO *bio) {
    BIO_set_init(bio, 1);
    return 1;
}

long BIO_s_custom_ctrl(BIO *bio, int cmd, long num, void *user) {
    switch(cmd) {
    case BIO_CTRL_FLUSH:
        return 1;
    default:
        return 0;
    }
}

int BIO_s_custom_write(BIO *bio, const char *data, int length) {
    struct loop_ssl_data *loop_ssl_data = (struct loop_ssl_data *) BIO_get_data(bio);

    //printf("BIO_s_custom_write\n");

    loop_ssl_data->last_write_was_msg_more = loop_ssl_data->msg_more || length == 16413;
    int written = us_socket_write(loop_ssl_data->ssl_socket, data, length, loop_ssl_data->last_write_was_msg_more);

    if (!written) {
        BIO_set_flags(bio, BIO_get_flags(bio) | BIO_FLAGS_SHOULD_RETRY | BIO_FLAGS_WRITE);
        return -1;
    }

    //printf("BIO_s_custom_write returns: %d\n", ret);

    return written;
}

int BIO_s_custom_read(BIO *bio, char *dst, int length) {
    struct loop_ssl_data *loop_ssl_data = (struct loop_ssl_data *) BIO_get_data(bio);

    //printf("BIO_s_custom_read\n");

    if (!loop_ssl_data->ssl_read_input_length) {
        BIO_set_flags(bio, BIO_get_flags(bio) | BIO_FLAGS_SHOULD_RETRY | BIO_FLAGS_READ);
        return 0;
    }

    if (length > loop_ssl_data->ssl_read_input_length) {
        length = loop_ssl_data->ssl_read_input_length;
    }

    memcpy(dst, loop_ssl_data->ssl_read_input + loop_ssl_data->ssl_read_input_offset, length);

    loop_ssl_data->ssl_read_input_offset += length;
    loop_ssl_data->ssl_read_input_length -= length;
    return length;
}

void ssl_on_open(struct us_ssl_socket *s, int is_client) {
    struct us_ssl_socket_context *context = (struct us_ssl_socket_context *) us_socket_get_context(&s->s);

    struct us_loop *loop = us_socket_context_loop(&context->sc);
    struct loop_ssl_data *loop_ssl_data = (struct loop_ssl_data *) loop->data.ssl_data;

    s->ssl = SSL_new(context->ssl_context);
    s->ssl_write_wants_read = 0;
    SSL_set_bio(s->ssl, loop_ssl_data->shared_rbio, loop_ssl_data->shared_wbio);

    BIO_up_ref(loop_ssl_data->shared_rbio);
    BIO_up_ref(loop_ssl_data->shared_wbio);

    if (is_client) {
        SSL_set_connect_state(s->ssl);
    } else {
        SSL_set_accept_state(s->ssl);
    }

    context->on_open(s, is_client);
}

void ssl_on_close(struct us_ssl_socket *s) {
    struct us_ssl_socket_context *context = (struct us_ssl_socket_context *) us_socket_get_context(&s->s);

    SSL_free(s->ssl);

    context->on_close(s);
}

void ssl_on_end(struct us_ssl_socket *s) {
    struct us_ssl_socket_context *context = (struct us_ssl_socket_context *) us_socket_get_context(&s->s);

    // whatever state we are in, a TCP FIN is always an answered shutdown
    //printf("ssl_on_end aka someone sent us a TCP FIN??\n");
    us_ssl_socket_close(s);
}

// this whole function needs a complete clean-up
void ssl_on_data(struct us_ssl_socket *s, void *data, int length) {
    struct us_ssl_socket_context *context = (struct us_ssl_socket_context *) us_socket_get_context(&s->s);

    struct us_loop *loop = us_socket_context_loop(&context->sc);
    struct loop_ssl_data *loop_ssl_data = (struct loop_ssl_data *) loop->data.ssl_data;

    loop_ssl_data->ssl_read_input = data;
    loop_ssl_data->ssl_read_input_length = length;
    loop_ssl_data->ssl_read_input_offset = 0;
    loop_ssl_data->ssl_socket = &s->s;

    if (us_ssl_socket_is_shut_down(s)) {


        if (SSL_shutdown(s->ssl) == 1) {
            // two phase shutdown is complete here
            printf("Two step SSL shutdown complete\n");

            us_ssl_socket_close(s);
        } else {
            //printf("Got data when in shutdown?\n");
        }

        // no further processing of data when in shutdown state
        return;
    }

    int read = 0;
    while (loop_ssl_data->ssl_read_input_length) {
        int just_read = SSL_read(s->ssl, loop_ssl_data->ssl_read_output + read, LIBUS_RECV_BUFFER_LENGTH - read);
        if (just_read > 0) {
            read += just_read;
        } else if (loop_ssl_data->ssl_read_input_length) {
            // serious error we cannot recover from
            printf("Serious error!\n");
            us_ssl_socket_close(s);
            exit(-2);
            return;
        }
    }

    if (read > 0) {

        // note: if we got a shutdown we cannot send anything, so we need to handle shutdown earlier than this

        context->on_data(s, loop_ssl_data->ssl_read_output, read);
        if (us_internal_socket_is_closed(&s->s)) {
            return;
        }

        // if we are closed from here, return!

    } else if (read == 0) {
        // hmmmmmm?
        // ignore any sending of 0 length, wtf
    } else {
        int err = SSL_get_error(s->ssl, read);
        //if (err == SSL_ERROR_ZERO_RETURN) {
            // is this path even needed?
            //goto received_shutdown;
        /*} else */if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
            // terminate connection here
            us_ssl_socket_close(s);
            return;
        }
    }

    // trigger writable if we failed last write with want read
    if (s->ssl_write_wants_read) {
        s->ssl_write_wants_read = 0;

        context->sc.on_writable(&s->s); // cast here!
        // if we are closed here, then exit
        if (us_internal_socket_is_closed(&s->s)) {
            return;
        }
    }

    // check this then?
    if (SSL_get_shutdown(s->ssl) & SSL_RECEIVED_SHUTDOWN) {
        //printf("SSL_RECEIVED_SHUTDOWN\n");

        //exit(-2);

        // not correct anyways!
        us_ssl_socket_close(s);

        //us_
    }
}

/* Lazily inits loop ssl data first time */
void us_internal_init_loop_ssl_data(struct us_loop *loop) {
    if (!loop->data.ssl_data) {
        struct loop_ssl_data *loop_ssl_data = malloc(sizeof(struct loop_ssl_data));

        loop_ssl_data->ssl_read_output = malloc(LIBUS_RECV_BUFFER_LENGTH);

        OPENSSL_init_ssl(0, NULL);

        loop_ssl_data->shared_biom = BIO_meth_new(BIO_TYPE_MEM, "µS BIO");
        BIO_meth_set_create(loop_ssl_data->shared_biom, BIO_s_custom_create);
        BIO_meth_set_write(loop_ssl_data->shared_biom, BIO_s_custom_write);
        BIO_meth_set_read(loop_ssl_data->shared_biom, BIO_s_custom_read);
        BIO_meth_set_ctrl(loop_ssl_data->shared_biom, BIO_s_custom_ctrl);

        loop_ssl_data->shared_rbio = BIO_new(loop_ssl_data->shared_biom);
        loop_ssl_data->shared_wbio = BIO_new(loop_ssl_data->shared_biom);
        BIO_set_data(loop_ssl_data->shared_rbio, loop_ssl_data);
        BIO_set_data(loop_ssl_data->shared_wbio, loop_ssl_data);

        loop->data.ssl_data = loop_ssl_data;
    }
}

/* Called by loop free, clears any loop ssl data */
void us_internal_free_loop_ssl_data(struct us_loop *loop) {
    struct loop_ssl_data *loop_ssl_data = (struct loop_ssl_data *) loop->data.ssl_data;

    if (loop_ssl_data) {
        free(loop_ssl_data->ssl_read_output);

        BIO_free(loop_ssl_data->shared_rbio);
        BIO_free(loop_ssl_data->shared_wbio);

        BIO_meth_free(loop_ssl_data->shared_biom);

        free(loop_ssl_data);
    }
}

/* Per-context functions */
struct us_ssl_socket_context *us_create_ssl_socket_context(struct us_loop *loop, int context_ext_size, struct us_ssl_socket_context_options options) {

    us_internal_init_loop_ssl_data(loop);

    struct us_ssl_socket_context *context = (struct us_ssl_socket_context *) us_create_socket_context(loop, sizeof(struct us_ssl_socket_context) + context_ext_size);

    // this is a server?
    context->ssl_context = SSL_CTX_new(TLS_method());

    // options
    SSL_CTX_set_read_ahead(context->ssl_context, 1);
    //SSL_CTX_set_mode(context->ssl_context, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    SSL_CTX_set_mode(context->ssl_context, SSL_MODE_ENABLE_PARTIAL_WRITE);

    //SSL_CTX_set_mode(context->ssl_context, SSL_MODE_RELEASE_BUFFERS);
    SSL_CTX_set_options(context->ssl_context, SSL_OP_NO_SSLv3);

    // these are going to be extended
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

void us_ssl_socket_context_free(struct us_ssl_socket_context *context) {
    SSL_CTX_free(context->ssl_context);

    us_socket_context_free(&context->sc);
}

struct us_listen_socket *us_ssl_socket_context_listen(struct us_ssl_socket_context *context, const char *host, int port, int options, int socket_ext_size) {
    return us_socket_context_listen(&context->sc, host, port, options, sizeof(struct us_ssl_socket) + socket_ext_size);
}

struct us_ssl_socket *us_ssl_socket_context_connect(struct us_ssl_socket_context *context, const char *host, int port, int options, int socket_ext_size) {
    return (struct us_ssl_socket *) us_socket_context_connect(&context->sc, host, port, options, sizeof(struct us_ssl_socket) + socket_ext_size);
}

void us_ssl_socket_context_on_open(struct us_ssl_socket_context *context, void (*on_open)(struct us_ssl_socket *s, int is_client)) {
    us_socket_context_on_open(&context->sc, (void (*)(struct us_socket *, int)) ssl_on_open);
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

void us_ssl_socket_context_on_end(struct us_ssl_socket_context *context, void (*on_end)(struct us_ssl_socket *)) {
    us_socket_context_on_end((struct us_socket_context *) context, (void (*)(struct us_socket *)) ssl_on_end);
}

void *us_ssl_socket_context_ext(struct us_ssl_socket_context *context) {
    return context + 1;
}

/* Per socket functions */
struct us_ssl_socket_context *us_ssl_socket_get_context(struct us_ssl_socket *s) {
    return (struct us_ssl_socket_context *) s->s.context;
}

int us_ssl_socket_write(struct us_ssl_socket *s, const char *data, int length, int msg_more) {
    if (us_internal_socket_is_closed(&s->s) || us_ssl_socket_is_shut_down(s)) {
        return 0;
    }

    struct us_ssl_socket_context *context = (struct us_ssl_socket_context *) us_socket_get_context(&s->s);

    struct us_loop *loop = us_socket_context_loop(&context->sc);
    struct loop_ssl_data *loop_ssl_data = (struct loop_ssl_data *) loop->data.ssl_data;

    loop_ssl_data->ssl_read_input_length = 0;
    loop_ssl_data->ssl_socket = &s->s;
    loop_ssl_data->msg_more = msg_more;
    loop_ssl_data->last_write_was_msg_more = 0;
    int written = SSL_write(s->ssl, data, length);
    loop_ssl_data->msg_more = 0;

    if (loop_ssl_data->last_write_was_msg_more && !msg_more) {
        us_socket_flush(&s->s);
    }

    if (written > 0) {
        return written;
    } else {
        int err = SSL_get_error(s->ssl, written);
        if (err == SSL_ERROR_WANT_READ) {
            // here we need to trigger writable event next ssl_read!
            s->ssl_write_wants_read = 1;
        } else {
            // all errors here except for want write are critical and should not happen
        }

        return 0;
    }
}

void us_ssl_socket_timeout(struct us_ssl_socket *s, unsigned int seconds) {
    us_socket_timeout((struct us_socket *) s, seconds);
}

void *us_ssl_socket_ext(struct us_ssl_socket *s) {
    return s + 1;
}

int us_ssl_socket_is_shut_down(struct us_ssl_socket *s) {
    return us_socket_is_shut_down(&s->s) || SSL_get_shutdown(s->ssl) & SSL_SENT_SHUTDOWN;
}

void us_ssl_socket_shutdown(struct us_ssl_socket *s) {
    if (!us_internal_socket_is_closed(&s->s) && !us_ssl_socket_is_shut_down(s)) {
        struct us_ssl_socket_context *context = (struct us_ssl_socket_context *) us_socket_get_context(&s->s);
        struct us_loop *loop = us_socket_context_loop(&context->sc);
        struct loop_ssl_data *loop_ssl_data = (struct loop_ssl_data *) loop->data.ssl_data;

        loop_ssl_data->ssl_read_input_length = 0;
        loop_ssl_data->ssl_socket = &s->s;
        loop_ssl_data->msg_more = 0;

        // sets SSL_SENT_SHUTDOWN no matter what (not actually true if error!)
        int ret = SSL_shutdown(s->ssl);
        if (ret == 0) {
            ret = SSL_shutdown(s->ssl);
        }

        if (ret < 0) {
            // we get here if we are shutting down while still in init
            us_socket_shutdown(&s->s);
        }
    }
}

void us_ssl_socket_close(struct us_ssl_socket *s) {
    us_socket_close((struct us_socket *) s);
}

#endif
