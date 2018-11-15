/*
 * Copyright 2018 Alex Hultman and contributors.

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

/* A socket context holds shared callbacks and user data extension for associated sockets */
WIN32_EXPORT struct us_socket_context *us_create_socket_context(struct us_loop *loop, int ext_size);

/* Delete resources allocated at creation time. */
WIN32_EXPORT void us_socket_context_free(struct us_socket_context *context);

/* Setters of various async callbacks */
WIN32_EXPORT void us_socket_context_on_open(struct us_socket_context *context, struct us_socket *(*on_open)(struct us_socket *s, int is_client));
WIN32_EXPORT void us_socket_context_on_close(struct us_socket_context *context, struct us_socket *(*on_close)(struct us_socket *s));
WIN32_EXPORT void us_socket_context_on_data(struct us_socket_context *context, struct us_socket *(*on_data)(struct us_socket *s, char *data, int length));
WIN32_EXPORT void us_socket_context_on_writable(struct us_socket_context *context, struct us_socket *(*on_writable)(struct us_socket *s));
WIN32_EXPORT void us_socket_context_on_timeout(struct us_socket_context *context, struct us_socket *(*on_timeout)(struct us_socket *s));

/* Emitted when a socket has been half-closed */
WIN32_EXPORT void us_socket_context_on_end(struct us_socket_context *context, struct us_socket *(*on_end)(struct us_socket *s));

/* Returns user data extension for this socket context */
WIN32_EXPORT void *us_socket_context_ext(struct us_socket_context *context);

/* Listen for connections. Acts as the main driving cog in a server. Will call set async callbacks. */
WIN32_EXPORT struct us_listen_socket *us_socket_context_listen(struct us_socket_context *context, const char *host, int port, int options, int socket_ext_size);

/* listen_socket.c/.h */
WIN32_EXPORT void us_listen_socket_close(struct us_listen_socket *ls);

/* Land in on_open or on_close or return null or return socket */
WIN32_EXPORT struct us_socket *us_socket_context_connect(struct us_socket_context *context, const char *host, int port, int options, int socket_ext_size);

/* Returns the loop for this socket context. */
WIN32_EXPORT struct us_loop *us_socket_context_loop(struct us_socket_context *context);

/* Invalidates passed socket, returning a new resized socket which belongs to a different socket context.
 * Used mainly for "socket upgrades" such as when transitioning from HTTP to WebSocket. */
WIN32_EXPORT struct us_socket *us_socket_context_adopt_socket(struct us_socket_context *context, struct us_socket *s, int ext_size);

/* Create a child socket context which acts much like its own socket context with its own callbacks yet still relies on the
 * parent socket context for some shared resources. Child socket contexts should be used together with socket adoptions and nothing else. */
WIN32_EXPORT struct us_socket_context *us_create_child_socket_context(struct us_socket_context *context, int context_ext_size);
