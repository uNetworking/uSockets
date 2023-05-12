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

#ifdef LIBUS_USE_IO_URING

#include "libusockets.h"
#include "internal/internal.h"
#include "internal.h"
#include <stdlib.h>
#include <string.h>

       #include <sys/types.h>
       #include <sys/socket.h>
       #include <netdb.h>

/* Shared with SSL */

void us_listen_socket_close(int ssl, struct us_listen_socket_t *ls) {

}

void us_socket_context_close(int ssl, struct us_socket_context_t *context) {

}

void us_internal_socket_context_unlink_listen_socket(struct us_socket_context_t *context, struct us_listen_socket_t *ls) {

}

void us_internal_socket_context_unlink_socket(struct us_socket_context_t *context, struct us_socket_t *s) {

}

struct us_socket_t *us_socket_context_adopt_socket(int ssl, struct us_socket_context_t *context, struct us_socket_t *s, int ext_size) {
    //printf("adopting socket now\n");
    s->context = context;
    return s;
}

/* We always add in the top, so we don't modify any s.next */
void us_internal_socket_context_link_listen_socket(struct us_socket_context_t *context, struct us_listen_socket_t *ls) {

}

/* We always add in the top, so we don't modify any s.next */
void us_internal_socket_context_link_socket(struct us_socket_context_t *context, struct us_socket_t *s) {
    s->context = context;
    s->next = context->head_sockets;
    s->prev = 0;
    if (context->head_sockets) {
        context->head_sockets->prev = s;
    }
    context->head_sockets = s;
}

struct us_loop_t *us_socket_context_loop(int ssl, struct us_socket_context_t *context) {
    return context->loop;
}

/* Not shared with SSL */

/* Lookup userdata by server name pattern */
void *us_socket_context_find_server_name_userdata(int ssl, struct us_socket_context_t *context, const char *hostname_pattern) {
    return 0;
}

/* Get userdata attached to this SNI-routed socket, or nullptr if default */
void *us_socket_server_name_userdata(int ssl, struct us_socket_t *s) {
    return 0;
}

/* Add SNI context */
void us_socket_context_add_server_name(int ssl, struct us_socket_context_t *context, const char *hostname_pattern, struct us_socket_context_options_t options, void *user) {

}

/* Remove SNI context */
void us_socket_context_remove_server_name(int ssl, struct us_socket_context_t *context, const char *hostname_pattern) {

}

/* I don't like this one - maybe rename it to on_missing_server_name? */

/* Called when SNI matching fails - not if a match could be made.
 * You may modify the context by adding/removing names in this callback.
 * If the correct name is added immediately in the callback, it will be used */
void us_socket_context_on_server_name(int ssl, struct us_socket_context_t *context, void (*cb)(struct us_socket_context_t *, const char *hostname)) {

}

/* Todo: get native context from SNI pattern */

void *us_socket_context_get_native_handle(int ssl, struct us_socket_context_t *context) {
    return 0;
}

/* Options is currently only applicable for SSL - this will change with time (prefer_low_memory is one example) */
struct us_socket_context_t *us_create_socket_context(int ssl, struct us_loop_t *loop, int context_ext_size, struct us_socket_context_options_t options) {
    struct us_socket_context_t *context = malloc(context_ext_size + sizeof(struct us_socket_context_t));

    context->loop = loop;

    context->head_sockets = 0;
    context->head_listen_sockets = 0;
    context->iterator = 0;
    context->next = 0;

    /* Begin at 0 */
    context->timestamp = 0;
    context->long_timestamp = 0;
    context->global_tick = 0;

    us_internal_loop_link(loop, context);

    return context;
}

void us_socket_context_free(int ssl, struct us_socket_context_t *context) {

}

struct us_listen_socket_t *us_socket_context_listen(int ssl, struct us_socket_context_t *context, const char *host, int port, int options, int socket_ext_size) {

    struct us_listen_socket_t *listen_s = malloc(sizeof(struct us_listen_socket_t));

    listen_s->context = context;
    listen_s->socket_ext_size = socket_ext_size;

    // some variables we need
    int portno = port;//strtol("3000", NULL, 10);
    struct sockaddr_in serv_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // setup socket
    int sock_listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    bsd_socket_nodelay(sock_listen_fd, 1);

    const int val = 1;
    setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    setsockopt(sock_listen_fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    // bind and listen
    if (bind(sock_listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding socket...\n");
        exit(1);
    }
    if (listen(sock_listen_fd, BACKLOG) < 0) {
        perror("Error listening on socket...\n");
        exit(1);
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(&context->loop->ring);
    io_uring_prep_multishot_accept_direct(sqe, sock_listen_fd, NULL, NULL, 0);
    io_uring_sqe_set_flags(sqe, 0);

    io_uring_sqe_set_data(sqe, (char *)listen_s + LISTEN_SOCKET_ACCEPT);

    return listen_s;
}

struct us_listen_socket_t *us_socket_context_listen_unix(int ssl, struct us_socket_context_t *context, const char *path, int options, int socket_ext_size) {
    return 0;
}


struct us_socket_t *us_socket_context_connect(int ssl, struct us_socket_context_t *context, const char *host, int port, const char *source_host, int options, int socket_ext_size) {
    

    struct us_socket_t *s = malloc(sizeof(struct us_socket_t) + socket_ext_size);
    s->context = context;

    s->timeout = 255;
    s->long_timeout = 255;

    us_internal_socket_context_link_socket(context, s);


    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char port_string[16];
    snprintf(port_string, 16, "%d", port);

    if (getaddrinfo(host, port_string, &hints, &result) != 0) {
        return NULL;
    }

    LIBUS_SOCKET_DESCRIPTOR fd = bsd_create_socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bsd_socket_nodelay(fd, 1);
    if (fd == LIBUS_SOCKET_ERROR) {
        freeaddrinfo(result);
        return NULL;
    }

    if (source_host) {
        struct addrinfo *interface_result;
        if (!getaddrinfo(source_host, NULL, NULL, &interface_result)) {
            int ret = bind(fd, interface_result->ai_addr, (socklen_t) interface_result->ai_addrlen);
            freeaddrinfo(interface_result);
            if (ret == LIBUS_SOCKET_ERROR) {
                bsd_close_socket(fd);
                freeaddrinfo(result);
                return NULL;
            }
        }
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(&context->loop->ring);
    io_uring_prep_connect(sqe, fd, result->ai_addr, (socklen_t) result->ai_addrlen);

        static int num_sockets;

    // register this file add
    io_uring_register_files_update(&context->loop->ring, num_sockets, &fd, 1);



    s->dd = num_sockets++;


    struct iovec iovecs = {s->sendBuf, 16 * 1024};
    //printf("register: %d\n", io_uring_register_buffers_update_tag(&context->loop->ring, s->dd, &iovecs, 0, 1));


    io_uring_sqe_set_data(sqe, (char *)s + SOCKET_CONNECT);


    freeaddrinfo(result);
    
    
    
    return s;
}

struct us_socket_t *us_socket_context_connect_unix(int ssl, struct us_socket_context_t *context, const char *server_path, int options, int socket_ext_size) {
    return 0;
}

struct us_socket_context_t *us_create_child_socket_context(int ssl, struct us_socket_context_t *context, int context_ext_size) {
    struct us_socket_context_options_t opt = {0};
    return us_create_socket_context(ssl, context->loop, context_ext_size, opt);
}

void us_socket_context_on_open(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_open)(struct us_socket_t *s, int is_client, char *ip, int ip_length)) {
    context->on_open = on_open;
}

void us_socket_context_on_close(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_close)(struct us_socket_t *s, int code, void *reason)) {
    context->on_close = on_close;
}

void us_socket_context_on_data(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_data)(struct us_socket_t *s, char *data, int length)) {
    context->on_data = on_data;
}

void us_socket_context_on_writable(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_writable)(struct us_socket_t *s)) {

}

void us_socket_context_on_long_timeout(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_long_timeout)(struct us_socket_t *)) {
    context->on_socket_long_timeout = on_long_timeout;
}

void us_socket_context_on_timeout(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_timeout)(struct us_socket_t *)) {
    context->on_socket_timeout = on_timeout;
}

void us_socket_context_on_end(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_end)(struct us_socket_t *)) {

}

void us_socket_context_on_connect_error(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_connect_error)(struct us_socket_t *s, int code)) {

}

void *us_socket_context_ext(int ssl, struct us_socket_context_t *context) {
    return context + 1;
}

#endif