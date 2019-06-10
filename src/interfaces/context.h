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

/* Public interfaces for contexts */

// this comes from ssl
struct us_socket_context_options_t {
    const char *key_file_name;
    const char *cert_file_name;
    const char *passphrase;
    const char *dh_params_file_name;
    int ssl_prefer_low_memory_usage;
};

/* A socket context holds shared callbacks and user data extension for associated sockets */
WIN32_EXPORT struct us_socket_context_t *us_create_socket_context(int ssl, struct us_loop_t *loop, int ext_size, struct us_socket_context_options_t options);

/* Delete resources allocated at creation time. */
WIN32_EXPORT void us_socket_context_free(int ssl, struct us_socket_context_t *context);

/* Setters of various async callbacks */
WIN32_EXPORT void us_socket_context_on_open(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_open)(struct us_socket_t *s, int is_client, char *ip, int ip_length));
WIN32_EXPORT void us_socket_context_on_close(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_close)(struct us_socket_t *s));
WIN32_EXPORT void us_socket_context_on_data(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_data)(struct us_socket_t *s, char *data, int length));
WIN32_EXPORT void us_socket_context_on_writable(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_writable)(struct us_socket_t *s));
WIN32_EXPORT void us_socket_context_on_timeout(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_timeout)(struct us_socket_t *s));

/* Emitted when a socket has been half-closed */
WIN32_EXPORT void us_socket_context_on_end(int ssl, struct us_socket_context_t *context, struct us_socket_t *(*on_end)(struct us_socket_t *s));

/* Returns user data extension for this socket context */
WIN32_EXPORT void *us_socket_context_ext(int ssl, struct us_socket_context_t *context);

/* Listen for connections. Acts as the main driving cog in a server. Will call set async callbacks. */
WIN32_EXPORT struct us_listen_socket_t *us_socket_context_listen(int ssl, struct us_socket_context_t *context, const char *host, int port, int options, int socket_ext_size);

/* listen_socket.c/.h */
WIN32_EXPORT void us_listen_socket_close(int ssl, struct us_listen_socket_t *ls);

/* Land in on_open or on_close or return null or return socket */
WIN32_EXPORT struct us_socket_t *us_socket_context_connect(int ssl, struct us_socket_context_t *context, const char *host, int port, int options, int socket_ext_size);

/* Returns the loop for this socket context. */
WIN32_EXPORT struct us_loop_t *us_socket_context_loop(int ssl, struct us_socket_context_t *context);

/* Invalidates passed socket, returning a new resized socket which belongs to a different socket context.
 * Used mainly for "socket upgrades" such as when transitioning from HTTP to WebSocket. */
WIN32_EXPORT struct us_socket_t *us_socket_context_adopt_socket(int ssl, struct us_socket_context_t *context, struct us_socket_t *s, int ext_size);

/* Create a child socket context which acts much like its own socket context with its own callbacks yet still relies on the
 * parent socket context for some shared resources. Child socket contexts should be used together with socket adoptions and nothing else. */
WIN32_EXPORT struct us_socket_context_t *us_create_child_socket_context(int ssl, struct us_socket_context_t *context, int context_ext_size);
