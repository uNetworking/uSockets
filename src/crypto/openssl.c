/*
 * Authored by Alex Hultman, 2018-2019.
 * Intellectual property of third-party.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef LIBUS_USE_OPENSSL

#include "libusockets.h"
#include "internal/internal.h"

/* This module contains the entire OpenSSL implementation
 * of the SSL socket and socket context interfaces. */

#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

/* We do not want to block the loop with tons and tons of CPU-intensive work.
 * Spread it out during many loop iterations, prioritizing already open connections,
 * they are far easier on CPU */
static const int MAX_HANDSHAKES_PER_LOOP_ITERATION = 5;

struct loop_ssl_data {
    char *ssl_read_input, *ssl_read_output;
    unsigned int ssl_read_input_length;
    unsigned int ssl_read_input_offset;
    struct us_socket_t *ssl_socket;

    int last_write_was_msg_more;
    int msg_more;

    // these are used to throttle SSL handshakes per loop iteration
    long long last_iteration_nr;
    int handshake_budget;

    BIO *shared_rbio;
    BIO *shared_wbio;
    BIO_METHOD *shared_biom;
};

struct us_internal_ssl_socket_context_t {
    struct us_socket_context_t sc;

    // this thing can be shared with other socket contexts via socket transfer!
    // maybe instead of holding once you hold many, a vector or set
    // when a socket that belongs to another socket context transfers to a new socket context
    SSL_CTX *ssl_context;
    int is_parent;

    // här måste det vara!
    struct us_internal_ssl_socket_t *(*on_open)(struct us_internal_ssl_socket_t *, int is_client, char *ip, int ip_length);
    struct us_internal_ssl_socket_t *(*on_data)(struct us_internal_ssl_socket_t *, char *data, int length);
    struct us_internal_ssl_socket_t *(*on_close)(struct us_internal_ssl_socket_t *);
};

// same here, should or shouldn't it contain s?
struct us_internal_ssl_socket_t {
    struct us_socket_t s;
    SSL *ssl;
    int ssl_write_wants_read; // we use this for now
};

int passphrase_cb(char *buf, int size, int rwflag, void *u) {
    const char *passphrase = (const char *) u;
    size_t passphrase_length = strlen(passphrase);
    memcpy(buf, passphrase, passphrase_length);
    // put null at end? no?
    return (int) passphrase_length;
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
    int written = us_socket_write(0, loop_ssl_data->ssl_socket, data, length, loop_ssl_data->last_write_was_msg_more);

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
        return -1;
    }

    if ((unsigned int) length > loop_ssl_data->ssl_read_input_length) {
        length = loop_ssl_data->ssl_read_input_length;
    }

    memcpy(dst, loop_ssl_data->ssl_read_input + loop_ssl_data->ssl_read_input_offset, length);

    loop_ssl_data->ssl_read_input_offset += length;
    loop_ssl_data->ssl_read_input_length -= length;
    return length;
}

struct us_internal_ssl_socket_t *ssl_on_open(struct us_internal_ssl_socket_t *s, int is_client, char *ip, int ip_length) {
    struct us_internal_ssl_socket_context_t *context = (struct us_internal_ssl_socket_context_t *) us_socket_context(0, &s->s);

    struct us_loop_t *loop = us_socket_context_loop(0, &context->sc);
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

    return (struct us_internal_ssl_socket_t *) context->on_open(s, is_client, ip, ip_length);
}

struct us_internal_ssl_socket_t *ssl_on_close(struct us_internal_ssl_socket_t *s) {
    struct us_internal_ssl_socket_context_t *context = (struct us_internal_ssl_socket_context_t *) us_socket_context(0, &s->s);

    SSL_free(s->ssl);

    return context->on_close(s);
}

struct us_internal_ssl_socket_t *ssl_on_end(struct us_internal_ssl_socket_t *s) {
    struct us_internal_ssl_socket_context_t *context = (struct us_internal_ssl_socket_context_t *) us_socket_context(0, &s->s);

    // whatever state we are in, a TCP FIN is always an answered shutdown
    return us_internal_ssl_socket_close(s);
}

// this whole function needs a complete clean-up
struct us_internal_ssl_socket_t *ssl_on_data(struct us_internal_ssl_socket_t *s, void *data, int length) {
    // note: this context can change when we adopt the socket!
    struct us_internal_ssl_socket_context_t *context = (struct us_internal_ssl_socket_context_t *) us_socket_context(0, &s->s);

    struct us_loop_t *loop = us_socket_context_loop(0, &context->sc);
    struct loop_ssl_data *loop_ssl_data = (struct loop_ssl_data *) loop->data.ssl_data;

    // note: if we put data here we should never really clear it (not in write either, it still should be available for SSL_write to read from!)
    loop_ssl_data->ssl_read_input = data;
    loop_ssl_data->ssl_read_input_length = length;
    loop_ssl_data->ssl_read_input_offset = 0;
    loop_ssl_data->ssl_socket = &s->s;
    loop_ssl_data->msg_more = 0;

    if (us_internal_ssl_socket_is_shut_down(s)) {

        int ret;
        if ((ret = SSL_shutdown(s->ssl)) == 1) {
            // two phase shutdown is complete here
            //printf("Two step SSL shutdown complete\n");

            return us_internal_ssl_socket_close(s);
        } else if (ret < 0) {

            int err = SSL_get_error(s->ssl, ret);

            if (err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL) {
                // we need to clear the error queue in case these added to the thread local queue
                ERR_clear_error();
            }

        }

        // no further processing of data when in shutdown state
        return s;
    }

    // bug checking: this loop needs a lot of attention and clean-ups and check-ups
    int read = 0;
    restart:
    while (1) {
        int just_read = SSL_read(s->ssl, loop_ssl_data->ssl_read_output + LIBUS_RECV_BUFFER_PADDING + read, LIBUS_RECV_BUFFER_LENGTH - read);

        if (just_read <= 0) {
            int err = SSL_get_error(s->ssl, just_read);

            // as far as I know these are the only errors we want to handle
            if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {

                // clear per thread error queue if it may contain something
                if (err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL) {
                    ERR_clear_error();
                }

                // terminate connection here
                return us_internal_ssl_socket_close(s);
            } else {
                // emit the data we have and exit

                // assume we emptied the input buffer fully or error here as well!
                if (loop_ssl_data->ssl_read_input_length) {
                    return us_internal_ssl_socket_close(s);
                }

                // cannot emit zero length to app
                if (!read) {
                    break;
                }

                s = context->on_data(s, loop_ssl_data->ssl_read_output + LIBUS_RECV_BUFFER_PADDING, read);
                if (us_socket_is_closed(0, &s->s)) {
                    return s;
                }

                break;
            }

        }

        read += just_read;

        // at this point we might be full and need to emit the data to application and start over
        if (read == LIBUS_RECV_BUFFER_LENGTH) {

            // emit data and restart
            s = context->on_data(s, loop_ssl_data->ssl_read_output + LIBUS_RECV_BUFFER_PADDING, read);
            if (us_socket_is_closed(0, &s->s)) {
                return s;
            }

            read = 0;
            goto restart;
        }
    }

    // trigger writable if we failed last write with want read
    if (s->ssl_write_wants_read) {
        s->ssl_write_wants_read = 0;

        // make sure to update context before we call (context can change if the user adopts the socket!)
        context = (struct us_internal_ssl_socket_context_t *) us_socket_context(0, &s->s);

        s = (struct us_internal_ssl_socket_t *) context->sc.on_writable(&s->s); // cast here!
        // if we are closed here, then exit
        if (us_socket_is_closed(0, &s->s)) {
            return s;
        }
    }

    // check this then?
    if (SSL_get_shutdown(s->ssl) & SSL_RECEIVED_SHUTDOWN) {
        //printf("SSL_RECEIVED_SHUTDOWN\n");

        //exit(-2);

        // not correct anyways!
        s = us_internal_ssl_socket_close(s);

        //us_
    }

    return s;
}

/* Lazily inits loop ssl data first time */
void us_internal_init_loop_ssl_data(struct us_loop_t *loop) {
    if (!loop->data.ssl_data) {
        struct loop_ssl_data *loop_ssl_data = malloc(sizeof(struct loop_ssl_data));

        loop_ssl_data->ssl_read_output = malloc(LIBUS_RECV_BUFFER_LENGTH + LIBUS_RECV_BUFFER_PADDING * 2);

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

        // reset handshake budget (doesn't matter what loop nr we start on)
        loop_ssl_data->last_iteration_nr = 0;
        loop_ssl_data->handshake_budget = MAX_HANDSHAKES_PER_LOOP_ITERATION;

        loop->data.ssl_data = loop_ssl_data;
    }
}

/* Called by loop free, clears any loop ssl data */
void us_internal_free_loop_ssl_data(struct us_loop_t *loop) {
    struct loop_ssl_data *loop_ssl_data = (struct loop_ssl_data *) loop->data.ssl_data;

    if (loop_ssl_data) {
        free(loop_ssl_data->ssl_read_output);

        BIO_free(loop_ssl_data->shared_rbio);
        BIO_free(loop_ssl_data->shared_wbio);

        BIO_meth_free(loop_ssl_data->shared_biom);

        free(loop_ssl_data);
    }
}

// we ignore reading data for ssl sockets that are
// in init state, if our so called budget for doing
// so won't allow it. here we actually use
// the kernel buffering to our advantage
int ssl_ignore_data(struct us_internal_ssl_socket_t *s) {

    // fast path just checks for init
    if (!SSL_in_init(s->ssl)) {
        return 0;
    }

    // this path is run for all ssl sockets that are in init and just got data event from polling

    struct us_loop_t *loop = s->s.context->loop;
    struct loop_ssl_data *loop_ssl_data = loop->data.ssl_data;

    // reset handshake budget if new iteration
    if (loop_ssl_data->last_iteration_nr != us_loop_iteration_number(loop)) {
        loop_ssl_data->last_iteration_nr = us_loop_iteration_number(loop);
        loop_ssl_data->handshake_budget = MAX_HANDSHAKES_PER_LOOP_ITERATION;
    }

    if (loop_ssl_data->handshake_budget) {
        loop_ssl_data->handshake_budget--;
        return 0;
    }

    // ignore this data event
    return 1;
}

/* Per-context functions */
struct us_internal_ssl_socket_context_t *us_internal_create_child_ssl_socket_context(struct us_internal_ssl_socket_context_t *context, int context_ext_size) {
    struct us_socket_context_options_t options = {0};

    struct us_internal_ssl_socket_context_t *child_context = (struct us_internal_ssl_socket_context_t *) us_create_socket_context(0, context->sc.loop, sizeof(struct us_internal_ssl_socket_context_t) + context_ext_size, options);

    // I think this is the only thing being shared
    child_context->ssl_context = context->ssl_context;
    child_context->is_parent = 0;

    return child_context;
}

struct us_internal_ssl_socket_context_t *us_internal_create_ssl_socket_context(struct us_loop_t *loop, int context_ext_size, struct us_socket_context_options_t options) {

    us_internal_init_loop_ssl_data(loop);

    struct us_socket_context_options_t no_options = {0};

    struct us_internal_ssl_socket_context_t *context = (struct us_internal_ssl_socket_context_t *) us_create_socket_context(0, loop, sizeof(struct us_internal_ssl_socket_context_t) + context_ext_size, no_options);

    context->ssl_context = SSL_CTX_new(TLS_method());
    context->is_parent = 1;
    // only parent ssl contexts may need to ignore data
    context->sc.ignore_data = (int (*)(struct us_socket_t *)) ssl_ignore_data;

    // options
    SSL_CTX_set_read_ahead(context->ssl_context, 1);
    SSL_CTX_set_mode(context->ssl_context, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    //SSL_CTX_set_mode(context->ssl_context, SSL_MODE_ENABLE_PARTIAL_WRITE);

    // this lowers performance a bit in benchmarks
    if (options.ssl_prefer_low_memory_usage) {
       SSL_CTX_set_mode(context->ssl_context, SSL_MODE_RELEASE_BUFFERS);
    }

    //SSL_CTX_set_mode(context->ssl_context, SSL_MODE_RELEASE_BUFFERS);
    SSL_CTX_set_options(context->ssl_context, SSL_OP_NO_SSLv3);
    SSL_CTX_set_options(context->ssl_context, SSL_OP_NO_TLSv1);

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

    if (options.ca_file_name) {
        STACK_OF(X509_NAME) *ca_list;
        ca_list = SSL_load_client_CA_file(options.ca_file_name);
        if(ca_list == NULL) {
            return 0;
        }
        SSL_CTX_set_client_CA_list(context->ssl_context, ca_list);
        if (SSL_CTX_load_verify_locations(context->ssl_context, options.ca_file_name, NULL) != 1) {
            return 0;
        }
        SSL_CTX_set_verify(context->ssl_context, SSL_VERIFY_PEER, NULL);
    }

    if (options.dh_params_file_name) {
        /* Set up ephemeral DH parameters. */
        DH *dh_2048 = NULL;
        FILE *paramfile;
        paramfile = fopen(options.dh_params_file_name, "r");

        if (paramfile) {
            dh_2048 = PEM_read_DHparams(paramfile, NULL, NULL, NULL);
            fclose(paramfile);
        } else {
            return 0;
        }

        if (dh_2048 == NULL) {
            return 0;
        }

        if (SSL_CTX_set_tmp_dh(context->ssl_context, dh_2048) != 1) {
            return 0;
        }

        /* OWASP Cipher String 'A+' (https://www.owasp.org/index.php/TLS_Cipher_String_Cheat_Sheet) */
        if (SSL_CTX_set_cipher_list(context->ssl_context, "DHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-RSA-AES128-GCM-SHA256") != 1) {
            return 0;
        }
    }

    return context;
}

void us_internal_ssl_socket_context_free(struct us_internal_ssl_socket_context_t *context) {
    if (context->is_parent) {
        SSL_CTX_free(context->ssl_context);
    }

    us_socket_context_free(0, &context->sc);
}

struct us_listen_socket_t *us_internal_ssl_socket_context_listen(struct us_internal_ssl_socket_context_t *context, const char *host, int port, int options, int socket_ext_size) {
    return us_socket_context_listen(0, &context->sc, host, port, options, sizeof(struct us_internal_ssl_socket_t) - sizeof(struct us_socket_t) + socket_ext_size);
}

struct us_internal_ssl_socket_t *us_internal_ssl_socket_context_connect(struct us_internal_ssl_socket_context_t *context, const char *host, int port, int options, int socket_ext_size) {
    return (struct us_internal_ssl_socket_t *) us_socket_context_connect(0, &context->sc, host, port, options, sizeof(struct us_internal_ssl_socket_t) - sizeof(struct us_socket_t) + socket_ext_size);
}

void us_internal_ssl_socket_context_on_open(struct us_internal_ssl_socket_context_t *context, struct us_internal_ssl_socket_t *(*on_open)(struct us_internal_ssl_socket_t *s, int is_client, char *ip, int ip_length)) {
    us_socket_context_on_open(0, &context->sc, (struct us_socket_t *(*)(struct us_socket_t *, int, char *, int)) ssl_on_open);
    context->on_open = on_open;
}

void us_internal_ssl_socket_context_on_close(struct us_internal_ssl_socket_context_t *context, struct us_internal_ssl_socket_t *(*on_close)(struct us_internal_ssl_socket_t *s)) {
    us_socket_context_on_close(0, (struct us_socket_context_t *) context, (struct us_socket_t *(*)(struct us_socket_t *)) ssl_on_close);
    context->on_close = on_close;
}

void us_internal_ssl_socket_context_on_data(struct us_internal_ssl_socket_context_t *context, struct us_internal_ssl_socket_t *(*on_data)(struct us_internal_ssl_socket_t *s, char *data, int length)) {
    us_socket_context_on_data(0, (struct us_socket_context_t *) context, (struct us_socket_t *(*)(struct us_socket_t *, char *, int)) ssl_on_data);
    context->on_data = on_data;
}

void us_internal_ssl_socket_context_on_writable(struct us_internal_ssl_socket_context_t *context, struct us_internal_ssl_socket_t *(*on_writable)(struct us_internal_ssl_socket_t *s)) {
    us_socket_context_on_writable(0, (struct us_socket_context_t *) context, (struct us_socket_t *(*)(struct us_socket_t *)) on_writable);
}

void us_internal_ssl_socket_context_on_timeout(struct us_internal_ssl_socket_context_t *context, struct us_internal_ssl_socket_t *(*on_timeout)(struct us_internal_ssl_socket_t *s)) {
    us_socket_context_on_timeout(0, (struct us_socket_context_t *) context, (struct us_socket_t *(*)(struct us_socket_t *)) on_timeout);
}

void us_internal_ssl_socket_context_on_end(struct us_internal_ssl_socket_context_t *context, struct us_internal_ssl_socket_t *(*on_end)(struct us_internal_ssl_socket_t *)) {
    us_socket_context_on_end(0, (struct us_socket_context_t *) context, (struct us_socket_t *(*)(struct us_socket_t *)) ssl_on_end);
}

void *us_internal_ssl_socket_context_ext(struct us_internal_ssl_socket_context_t *context) {
    return context + 1;
}

/* Per socket functions */

int us_internal_ssl_socket_write(struct us_internal_ssl_socket_t *s, const char *data, int length, int msg_more) {
    if (us_socket_is_closed(0, &s->s) || us_internal_ssl_socket_is_shut_down(s)) {
        return 0;
    }

    struct us_internal_ssl_socket_context_t *context = (struct us_internal_ssl_socket_context_t *) us_socket_context(0, &s->s);

    struct us_loop_t *loop = us_socket_context_loop(0, &context->sc);
    struct loop_ssl_data *loop_ssl_data = (struct loop_ssl_data *) loop->data.ssl_data;

    // it makes literally no sense to touch this here! it should start at 0 and ONLY be set and reset by the on_data function!
    // the way is is now, triggering a write from a read will essentially delete all input data!
    // what we need to do is to check if this ever is non-zero and print a warning



    loop_ssl_data->ssl_read_input_length = 0;


    loop_ssl_data->ssl_socket = &s->s;
    loop_ssl_data->msg_more = msg_more;
    loop_ssl_data->last_write_was_msg_more = 0;
    //printf("Calling SSL_write\n");
    int written = SSL_write(s->ssl, data, length);
    //printf("Returning from SSL_write\n");
    loop_ssl_data->msg_more = 0;

    if (loop_ssl_data->last_write_was_msg_more && !msg_more) {
        us_socket_flush(0, &s->s);
    }

    if (written > 0) {
        return written;
    } else {
        int err = SSL_get_error(s->ssl, written);
        if (err == SSL_ERROR_WANT_READ) {
            // here we need to trigger writable event next ssl_read!
            s->ssl_write_wants_read = 1;
        } else if (err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL) {
            // these two errors may add to the error queue, which is per thread and must be cleared
            ERR_clear_error();

            // all errors here except for want write are critical and should not happen
        }

        return 0;
    }
}

void *us_internal_ssl_socket_ext(struct us_internal_ssl_socket_t *s) {
    return s + 1;
}

int us_internal_ssl_socket_is_shut_down(struct us_internal_ssl_socket_t *s) {
    return us_socket_is_shut_down(0, &s->s) || SSL_get_shutdown(s->ssl) & SSL_SENT_SHUTDOWN;
}

void us_internal_ssl_socket_shutdown(struct us_internal_ssl_socket_t *s) {
    if (!us_socket_is_closed(0, &s->s) && !us_internal_ssl_socket_is_shut_down(s)) {
        struct us_internal_ssl_socket_context_t *context = (struct us_internal_ssl_socket_context_t *) us_socket_context(0, &s->s);
        struct us_loop_t *loop = us_socket_context_loop(0, &context->sc);
        struct loop_ssl_data *loop_ssl_data = (struct loop_ssl_data *) loop->data.ssl_data;

        // also makes no sense to touch this here!
        // however the idea is that if THIS socket is not the same as ssl_socket then this data is not for me
        // but this is not correct as it is currently anyways, any data available should be properly reset
        loop_ssl_data->ssl_read_input_length = 0;


        // essentially we need two of these: one for CURRENT CALL and one for CURRENT SOCKET WITH DATA
        // if those match in the BIO function then you may read, if not then you may not read
        // we need ssl_read_socket to be set in on_data and checked in the BIO
        loop_ssl_data->ssl_socket = &s->s;


        loop_ssl_data->msg_more = 0;

        // sets SSL_SENT_SHUTDOWN no matter what (not actually true if error!)
        int ret = SSL_shutdown(s->ssl);
        if (ret == 0) {
            ret = SSL_shutdown(s->ssl);
        }

        if (ret < 0) {

            int err = SSL_get_error(s->ssl, ret);
            if (err == SSL_ERROR_SSL || err == SSL_ERROR_SYSCALL) {
                // clear
                ERR_clear_error();
            }

            // we get here if we are shutting down while still in init
            us_socket_shutdown(0, &s->s);
        }
    }
}

struct us_internal_ssl_socket_t *us_internal_ssl_socket_close(struct us_internal_ssl_socket_t *s) {
    return (struct us_internal_ssl_socket_t *) us_socket_close(0, (struct us_socket_t *) s);
}

struct us_internal_ssl_socket_t *us_internal_ssl_socket_context_adopt_socket(struct us_internal_ssl_socket_context_t *context, struct us_internal_ssl_socket_t *s, int ext_size) {
    // todo: this is completely untested
    return (struct us_internal_ssl_socket_t *) us_socket_context_adopt_socket(0, &context->sc, &s->s, sizeof(struct us_internal_ssl_socket_t) - sizeof(struct us_socket_t) + ext_size);
}

#endif
