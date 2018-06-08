You begin by including `libusockets.h`, this is the only public header and exposes a set of functions. If missing from this manual just have a look in the header.

### User data
Every structure is created with some portion of user data memory. I call them extensions and they are retrieved like so:

`void *us_loop_ext(struct us_loop *)`

You always specify the extension size when creating a new resource:

`... us_create_loop(..., int ext_size)`

### us_loop
This structure represents the event loop. It is a shared per-thread resource from which everything stems.

* us_loop_run blocks the calling thread until no more async operations are scheduled. This drives the entire server and is non-reentrant meaning you never call it more than once a time per thread.

### us_socket_context
This is the context in which a socket lives. It holds shared callbacks for all sockets which belong to this context and thus define the behavior of the sockets. If you're building a web server then HTTP sockets belong to your HTTP socket context and WebSockets belong to your WebSockets socket context.

* us_create_socket_context(...
* us_socket_context_listen(...

Callbacks are set like so:

#### us_socket_context_on_data(...
A socket has data for you to read.
#### us_socket_context_on_writable(...
A socket can be written to again.
#### us_socket_context_on_open(...
A new socket has been allocated and opened
#### us_socket_context_on_close(...
A socket is closed and will be deleted
#### us_socket_context_on_timeout(...
A rough timer set on a socket has timed out. Only one timeout value can be set on any one socket a time.

Set timers with `void us_socket_timeout(struct us_socket, int seconds);`

### us_socket
Here we have the socket. It belongs to a socket context and knows about it:

* us_get_socket_context(struct us_socket *)

A socket always stems from a context and is always created from context listen or context connect.

### Sending data
You send data with a (simplified) BSD-like interface like so:

* int us_socket_write(struct us_socket *s, const char *data, int length, int more)

If all data could not be written you will automatically get a callback on_writable in your socket context and will only need to call the function again with the remaining data.

### SSL variants

Same applies to SSL-versions of sockets. They follow the same design but look like so:

int us_ssl_socket_write(...

Socket contexts and sockets have the us_ssl_ prefix instead of only us_. Use those SSL-prefixed functions that exists and for missing functions you can use the shared ones like for instance us_socket_timeout.
