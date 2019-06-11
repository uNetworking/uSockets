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

/* Public interfaces for SSL sockets and contexts */

// this file now becomes internal!

#ifndef LIBUS_NO_SSL

// these should end in _t just like regular sockets!
struct us_ssl_socket;
struct us_ssl_socket_context;

/* An options structure where set options are non-null. Used to initialize an SSL socket context */
/*struct us_ssl_socket_context_options {
    const char *key_file_name;
    const char *cert_file_name;
    const char *passphrase;
    const char *dh_params_file_name;
    int ssl_prefer_low_memory_usage;
};*/

/* See us_create_socket_context. SSL variant taking SSL options structure */
WIN32_EXPORT struct us_ssl_socket_context *us_create_ssl_socket_context(struct us_loop_t *loop, int context_ext_size, struct us_socket_context_options_t options);

/* */
WIN32_EXPORT void us_ssl_socket_context_free(struct us_ssl_socket_context *context);

/* See us_socket_context */
WIN32_EXPORT void us_ssl_socket_context_on_open(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_open)(struct us_ssl_socket *s, int is_client, char *ip, int ip_length));
WIN32_EXPORT void us_ssl_socket_context_on_close(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_close)(struct us_ssl_socket *s));
WIN32_EXPORT void us_ssl_socket_context_on_data(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_data)(struct us_ssl_socket *s, char *data, int length));
WIN32_EXPORT void us_ssl_socket_context_on_writable(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_writable)(struct us_ssl_socket *s));
WIN32_EXPORT void us_ssl_socket_context_on_timeout(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_timeout)(struct us_ssl_socket *s));
WIN32_EXPORT void us_ssl_socket_context_on_end(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_end)(struct us_ssl_socket *s));

/* See us_socket_context */
WIN32_EXPORT struct us_listen_socket_t *us_ssl_socket_context_listen(struct us_ssl_socket_context *context, const char *host, int port, int options, int socket_ext_size);

/* */
WIN32_EXPORT struct us_ssl_socket *us_ssl_socket_context_connect(struct us_ssl_socket_context *context, const char *host, int port, int options, int socket_ext_size);

/* See us_socket */
WIN32_EXPORT int us_ssl_socket_write(struct us_ssl_socket *s, const char *data, int length, int msg_more);

/* See us_socket */
WIN32_EXPORT void us_ssl_socket_timeout(struct us_ssl_socket *s, unsigned int seconds);

/* */
WIN32_EXPORT void *us_ssl_socket_context_ext(struct us_ssl_socket_context *s);

/* */
WIN32_EXPORT struct us_ssl_socket_context *us_ssl_socket_get_context(struct us_ssl_socket *s);

/* Return the user data extension of this socket */
WIN32_EXPORT void *us_ssl_socket_ext(struct us_ssl_socket *s);

/* */
WIN32_EXPORT int us_ssl_socket_is_shut_down(struct us_ssl_socket *s);

/* */
WIN32_EXPORT void us_ssl_socket_shutdown(struct us_ssl_socket *s);

/* */
WIN32_EXPORT struct us_ssl_socket *us_ssl_socket_close(struct us_ssl_socket *s);

/* Invalidates passed socket, returning a new resized socket which belongs to a different socket context.
 * Used mainly for "socket upgrades" such as when transitioning from HTTP to WebSocket. */
WIN32_EXPORT struct us_ssl_socket *us_ssl_socket_context_adopt_socket(struct us_ssl_socket_context *context, struct us_ssl_socket *s, int ext_size);

/* Create a child socket context which acts much like its own socket context with its own callbacks yet still relies on the
 * parent socket context for some shared resources. Child socket contexts should be used together with socket adoptions and nothing else. */
WIN32_EXPORT struct us_ssl_socket_context *us_create_child_ssl_socket_context(struct us_ssl_socket_context *context, int context_ext_size);

WIN32_EXPORT struct us_loop_t *us_ssl_socket_context_loop(struct us_ssl_socket_context *context);

#endif
