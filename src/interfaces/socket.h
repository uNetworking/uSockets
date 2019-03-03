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

/* Public interfaces for sockets */

/* Write up to length bytes of data. Returns actual bytes written. Will call the on_writable callback of active socket context on failure to write everything off in one go.
 * Set hint msg_more if you have more immediate data to write. */
WIN32_EXPORT int us_socket_write(struct us_socket *s, const char *data, int length, int msg_more);

/* Set a low precision, high performance timer on a socket. A socket can only have one single active timer at any given point in time. Will remove any such pre set timer */
WIN32_EXPORT void us_socket_timeout(struct us_socket *s, unsigned int seconds);

/* Return the user data extension of this socket */
WIN32_EXPORT void *us_socket_ext(struct us_socket *s);

/* Return the socket context of this socket */
WIN32_EXPORT struct us_socket_context *us_socket_get_context(struct us_socket *s);

/* Withdraw any msg_more status and flush any pending data */
WIN32_EXPORT void us_socket_flush(struct us_socket *s);

/* Shuts down the connection by sending FIN and/or close_notify */
WIN32_EXPORT void us_socket_shutdown(struct us_socket *s);

/* Returns whether the socket has been shut down or not */
WIN32_EXPORT int us_socket_is_shut_down(struct us_socket *s);

/* Returns whether this socket has been closed. Only valid if memory has not yet been released. */
WIN32_EXPORT int us_socket_is_closed(struct us_socket *s);

/* Immediately closes the socket */
WIN32_EXPORT struct us_socket *us_socket_close(struct us_socket *s);

/* Copy remote (IP) address of socket, or fail with zero length. */
WIN32_EXPORT void us_socket_remote_address(struct us_socket *s, char *buf, int *length);