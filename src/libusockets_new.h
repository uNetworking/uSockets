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

#include "libusockets.h"

/* This is an experimental new API that is identical for SSL and non-SSL. */

#ifndef LIBUSOCKETS_NEW_H
#define LIBUSOCKETS_NEW_H

/* Todo: make every struct type end with _t */
struct us_new_socket_t;
struct us_new_socket_context_t;

/* Temporary (must match with ssl options) */
struct us_new_socket_context_options_t {
    const char *key_file_name;
    const char *cert_file_name;
    const char *passphrase;
    const char *dh_params_file_name;
};

/* Per-socket-context functions */
inline static struct us_new_socket_t *us_new_socket_context_connect(const int ssl, struct us_new_socket_context_t *c, const char *host, int port, int options, int socket_ext_size) {
#ifdef LIBUS_NO_SSL
    return (struct us_new_socket_t *) us_socket_context_connect((struct us_socket_context *) c, host, port, options, socket_ext_size);
#else
    return ssl ? (struct us_new_socket_t *) us_ssl_socket_context_connect((struct us_ssl_socket_context *) c, host, port, options, socket_ext_size) : (struct us_new_socket_t *) us_socket_context_connect((struct us_socket_context *) c, host, port, options, socket_ext_size);
#endif
}

inline static struct us_new_socket_context_t *us_new_create_socket_context(const int ssl, struct us_loop *loop, int socket_context_ext_size, struct us_new_socket_context_options_t options) {
#ifdef LIBUS_NO_SSL
    return (struct us_new_socket_context_t *) us_create_socket_context(loop, socket_context_ext_size);
#else
    /* Convert between the two structs */
    struct us_ssl_socket_context_options ssl_options = {
        options.key_file_name,
        options.cert_file_name,
        options.passphrase,
        options.dh_params_file_name
    };

    return ssl ? (struct us_new_socket_context_t *) us_create_ssl_socket_context(loop, socket_context_ext_size, ssl_options) : (struct us_new_socket_context_t *) us_create_socket_context(loop, socket_context_ext_size);
#endif
}

inline static struct us_new_socket_context_t *us_new_create_child_socket_context(const int ssl, struct us_new_socket_context_t *c, int socket_context_ext_size) {
#ifdef LIBUS_NO_SSL
    return (struct us_new_socket_context_t *) us_create_child_socket_context((struct us_socket_context *) c, socket_context_ext_size);
#else
    return ssl ? (struct us_new_socket_context_t *) us_create_child_ssl_socket_context((struct us_ssl_socket_context *) c, socket_context_ext_size) : (struct us_new_socket_context_t *) us_create_child_socket_context((struct us_socket_context *) c, socket_context_ext_size);
#endif
}

inline static struct us_new_socket_t *us_new_socket_context_adopt_socket(const int ssl, struct us_new_socket_context_t *c, struct us_new_socket_t *s, int socket_ext_size) {
#ifdef LIBUS_NO_SSL
    return (struct us_new_socket_t *) us_socket_context_adopt_socket((struct us_socket_context *) c, (struct us_socket *) s, socket_ext_size);
#else
    return ssl ? (struct us_new_socket_t *) us_ssl_socket_context_adopt_socket((struct us_ssl_socket_context *) c, (struct us_ssl_socket *) s, socket_ext_size) : (struct us_new_socket_t *) us_socket_context_adopt_socket((struct us_socket_context *) c, (struct us_socket *) s, socket_ext_size);
#endif
}

inline static void us_new_socket_context_free(const int ssl, struct us_new_socket_context_t *c) {
#ifdef LIBUS_NO_SSL
    us_socket_context_free((struct us_socket_context *) c);
#else
    if (ssl) {
        us_ssl_socket_context_free((struct us_ssl_socket_context *) c);
    } else {
        us_socket_context_free((struct us_socket_context *) c);
    }
#endif
}

inline static struct us_listen_socket *us_new_socket_context_listen(const int ssl, struct us_new_socket_context_t *c, const char *host, int port, int options, int socket_ext_size) {
#ifdef LIBUS_NO_SSL
    return us_socket_context_listen((struct us_socket_context *) c, host, port, options, socket_ext_size);
#else
    return ssl ? us_ssl_socket_context_listen((struct us_ssl_socket_context *) c, host, port, options, socket_ext_size) : us_socket_context_listen((struct us_socket_context *) c, host, port, options, socket_ext_size);
#endif
}

inline static struct us_loop *us_new_socket_context_loop(const int ssl, struct us_new_socket_context_t *c) {
#ifdef LIBUS_NO_SSL
    return us_socket_context_loop((struct us_socket_context *) c);
#else
    return ssl ? us_ssl_socket_context_loop((struct us_ssl_socket_context *) c) : us_socket_context_loop((struct us_socket_context *) c);
#endif
}

inline static void us_new_socket_context_on_data(const int ssl, struct us_new_socket_context_t *c, struct us_new_socket_t *(*on_data)(struct us_new_socket_t *s, char *data, int length)) {
#ifdef LIBUS_NO_SSL
    us_socket_context_on_data((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s, char *data, int length)) on_data);
#else
    if (ssl) {
        us_ssl_socket_context_on_data((struct us_ssl_socket_context *) c, (struct us_ssl_socket *(*)(struct us_ssl_socket *s, char *data, int length)) on_data);
    } else {
        us_socket_context_on_data((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s, char *data, int length)) on_data);
    }
#endif
}

inline static void us_new_socket_context_on_timeout(const int ssl, struct us_new_socket_context_t *c, struct us_new_socket_t *(*on_timeout)(struct us_new_socket_t *s)) {
#ifdef LIBUS_NO_SSL
    us_socket_context_on_timeout((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s)) on_timeout);
#else
    if (ssl) {
        us_ssl_socket_context_on_timeout((struct us_ssl_socket_context *) c, (struct us_ssl_socket *(*)(struct us_ssl_socket *s)) on_timeout);
    } else {
        us_socket_context_on_timeout((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s)) on_timeout);
    }
#endif
}

inline static void us_new_socket_context_on_end(const int ssl, struct us_new_socket_context_t *c, struct us_new_socket_t *(*on_end)(struct us_new_socket_t *s)) {
#ifdef LIBUS_NO_SSL
    us_socket_context_on_end((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s)) on_end);
#else
    if (ssl) {
        us_ssl_socket_context_on_end((struct us_ssl_socket_context *) c, (struct us_ssl_socket *(*)(struct us_ssl_socket *s)) on_end);
    } else {
        us_socket_context_on_end((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s)) on_end);
    }
#endif
}

inline static void us_new_socket_context_on_writable(const int ssl, struct us_new_socket_context_t *c, struct us_new_socket_t *(*on_writable)(struct us_new_socket_t *s)) {
#ifdef LIBUS_NO_SSL
    us_socket_context_on_writable((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s)) on_writable);
#else
    if (ssl) {
        us_ssl_socket_context_on_writable((struct us_ssl_socket_context *) c, (struct us_ssl_socket *(*)(struct us_ssl_socket *s)) on_writable);
    } else {
        us_socket_context_on_writable((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s)) on_writable);
    }
#endif
}

inline static void us_new_socket_context_on_close(const int ssl, struct us_new_socket_context_t *c, struct us_new_socket_t *(*on_close)(struct us_new_socket_t *s)) {
#ifdef LIBUS_NO_SSL
    us_socket_context_on_close((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s)) on_close);
#else
    if (ssl) {
        us_ssl_socket_context_on_close((struct us_ssl_socket_context *) c, (struct us_ssl_socket *(*)(struct us_ssl_socket *s)) on_close);
    } else {
        us_socket_context_on_close((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s)) on_close);
    }
#endif
}

inline static void us_new_socket_context_on_open(const int ssl, struct us_new_socket_context_t *c, struct us_new_socket_t *(*on_open)(struct us_new_socket_t *s, int is_client)) {
#ifdef LIBUS_NO_SSL
    us_socket_context_on_open((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s, int is_client)) on_open);
#else
    if (ssl) {
        us_ssl_socket_context_on_open((struct us_ssl_socket_context *) c, (struct us_ssl_socket *(*)(struct us_ssl_socket *s, int is_client)) on_open);
    } else {
        us_socket_context_on_open((struct us_socket_context *) c, (struct us_socket *(*)(struct us_socket *s, int is_client)) on_open);
    }
#endif
}

inline static void *us_new_socket_context_ext(const int ssl, struct us_new_socket_context_t *c) {
#ifdef LIBUS_NO_SSL
    return us_socket_context_ext((struct us_socket_context *) c);
#else
    return ssl ? us_ssl_socket_context_ext((struct us_ssl_socket_context *) c) : us_socket_context_ext((struct us_socket_context *) c);
#endif
}

/* Per-socket functions */
inline static struct us_new_socket_context_t *us_new_socket_context(const int ssl, struct us_new_socket_t *s) {
#ifdef LIBUS_NO_SSL
    return (struct us_new_socket_context_t *) us_socket_get_context((struct us_socket *) s);
#else
    return ssl ? (struct us_new_socket_context_t *) us_ssl_socket_get_context((struct us_ssl_socket *) s) : (struct us_new_socket_context_t *) us_socket_get_context((struct us_socket *) s);
#endif
}

inline static void us_new_socket_timeout(const int ssl, struct us_new_socket_t *s, unsigned int seconds) {
#ifdef LIBUS_NO_SSL
    us_socket_timeout((struct us_socket *) s, seconds);
#else
    if (ssl) {
        us_ssl_socket_timeout((struct us_ssl_socket *) s, seconds);
    } else {
        us_socket_timeout((struct us_socket *) s, seconds);
    }
#endif
}

inline static void us_new_socket_shutdown(const int ssl, struct us_new_socket_t *s) {
#ifdef LIBUS_NO_SSL
    us_socket_shutdown((struct us_socket *) s);
#else
    if (ssl) {
        us_ssl_socket_shutdown((struct us_ssl_socket *) s);
    } else {
        us_socket_shutdown((struct us_socket *) s);
    }
#endif
}

inline static int us_new_socket_write(const int ssl, struct us_new_socket_t *s, const char *data, int length, int msg_more) {
#ifdef LIBUS_NO_SSL
    return us_socket_write((struct us_socket *) s, data, length, msg_more);
#else
    return ssl ? us_ssl_socket_write((struct us_ssl_socket *) s, data, length, msg_more) : us_socket_write((struct us_socket *) s, data, length, msg_more);
#endif
}

inline static void *us_new_socket_ext(const int ssl, struct us_new_socket_t *s) {
#ifdef LIBUS_NO_SSL
    return us_socket_ext((struct us_socket *) s);
#else
    return ssl ? us_ssl_socket_ext((struct us_ssl_socket *) s) : us_socket_ext((struct us_socket *) s);
#endif
}

inline static struct us_new_socket_t *us_new_socket_close(const int ssl, struct us_new_socket_t *s) {
#ifdef LIBUS_NO_SSL
    return (struct us_new_socket_t *) us_socket_close((struct us_socket *) s);
#else
    return ssl ? (struct us_new_socket_t *) us_ssl_socket_close((struct us_ssl_socket *) s) : (struct us_new_socket_t *) us_socket_close((struct us_socket *) s);
#endif
}

inline static int us_new_socket_is_closed(const int ssl, struct us_new_socket_t *s) {
    /* There is no SSL variant of this function */
    return us_socket_is_closed((struct us_socket *) s);
}

inline static int us_new_socket_is_shut_down(const int ssl, struct us_new_socket_t *s) {
#ifdef LIBUS_NO_SSL
    return us_socket_is_shut_down((struct us_socket *) s);
#else
    return ssl ? us_ssl_socket_is_shut_down((struct us_ssl_socket *) s) : us_socket_is_shut_down((struct us_socket *) s);
#endif
}

#endif
