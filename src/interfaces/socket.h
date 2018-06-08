/* Public interfaces for sockets */

/* Write up to length bytes of data. Returns actual bytes written. Will call the on_writable callback of active socket context on failure to write everything off in one go.
 * Set hint msg_more if you have more immediate data to write. */
int us_socket_write(struct us_socket *s, const char *data, int length, int msg_more);

/* Set a low precision, high performance timer on a socket. A socket can only have one single active timer at any given point in time. Will remove any such pre set timer */
void us_socket_timeout(struct us_socket *s, unsigned int seconds);

/* Return the user data extension of this socket */
void *us_socket_ext(struct us_socket *s);

/* Return the socket context of this socket */
void *us_socket_get_context(struct us_socket *s);
