# libusockets.h
This is the only header you include. Following documentation has been extracted from this header. It may be outdated, go read the header directly for up-to-date documentation.

These interfaces are "alpha" and subject to change.

# Loop
```c
/* Public interfaces for loops */

/* Returns a new event loop with user data extension */
WIN32_EXPORT struct us_loop *us_create_loop(int default_hint, void (*wakeup_cb)(struct us_loop *loop), void (*pre_cb)(struct us_loop *loop), void (*post_cb)(struct us_loop *loop), unsigned int ext_size);

/* */
WIN32_EXPORT void us_loop_free(struct us_loop *loop);

/* Returns the loop user data extension */
WIN32_EXPORT void *us_loop_ext(struct us_loop *loop);

/* Blocks the calling thread and drives the event loop until no more non-fallthrough polls are scheduled */
WIN32_EXPORT void us_loop_run(struct us_loop *loop);

/* Signals the loop from any thread to wake up and execute its wakeup handler from the loop's own running thread.
 * This is the only fully thread-safe function and serves as the basis for thread safety */
WIN32_EXPORT void us_wakeup_loop(struct us_loop *loop);

/* Hook up timers in existing loop */
WIN32_EXPORT void us_loop_integrate(struct us_loop *loop);
```

# Polls
```c
/* Public interfaces for polls */

/* A fallthrough poll does not keep the loop running, it falls through */
WIN32_EXPORT struct us_poll *us_create_poll(struct us_loop *loop, int fallthrough, unsigned int ext_size);

/* After stopping a poll you must manually free the memory */
WIN32_EXPORT void us_poll_free(struct us_poll *p, struct us_loop *loop);

/* Associate this poll with a socket descriptor and poll type */
WIN32_EXPORT void us_poll_init(struct us_poll *p, LIBUS_SOCKET_DESCRIPTOR fd, int poll_type);

/* Start, change and stop polling for events */
WIN32_EXPORT void us_poll_start(struct us_poll *p, struct us_loop *loop, int events);
WIN32_EXPORT void us_poll_change(struct us_poll *p, struct us_loop *loop, int events);
WIN32_EXPORT void us_poll_stop(struct us_poll *p, struct us_loop *loop);

/* Return what events we are polling for */
WIN32_EXPORT int us_poll_events(struct us_poll *p);

/* Returns the user data extension of this poll */
WIN32_EXPORT void *us_poll_ext(struct us_poll *p);

/* Get associated socket descriptor from a poll */
WIN32_EXPORT LIBUS_SOCKET_DESCRIPTOR us_poll_fd(struct us_poll *p);

/* Resize an active poll */
WIN32_EXPORT struct us_poll *us_poll_resize(struct us_poll *p, struct us_loop *loop, unsigned int ext_size);
```

# High cost timers
```c
/* Public interfaces for timers */

/* Create a new high precision, low performance timer. May fail and return null */
WIN32_EXPORT struct us_timer *us_create_timer(struct us_loop *loop, int fallthrough, unsigned int ext_size);

/* */
WIN32_EXPORT void us_timer_close(struct us_timer *timer);

/* Arm a timer with a delay from now and eventually a repeat delay.
 * Specify 0 as repeat delay to disable repeating. Specify both 0 to disarm. */
WIN32_EXPORT void us_timer_set(struct us_timer *timer, void (*cb)(struct us_timer *t), int ms, int repeat_ms);

/* Returns the loop for this timer */
WIN32_EXPORT struct us_loop *us_timer_loop(struct us_timer *t);
```

# Sockets
```c
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
```

# Socket contexts

```c
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
```

# SSL
```c
/* An options structure where set options are non-null. Used to initialize an SSL socket context */
struct us_ssl_socket_context_options {
    const char *key_file_name;
    const char *cert_file_name;
    const char *passphrase;
    const char *dh_params_file_name;
};

/* See us_create_socket_context. SSL variant taking SSL options structure */
WIN32_EXPORT struct us_ssl_socket_context *us_create_ssl_socket_context(struct us_loop *loop, int context_ext_size, struct us_ssl_socket_context_options options);

/* */
WIN32_EXPORT void us_ssl_socket_context_free(struct us_ssl_socket_context *context);

/* See us_socket_context */
WIN32_EXPORT void us_ssl_socket_context_on_open(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_open)(struct us_ssl_socket *s, int is_client));
WIN32_EXPORT void us_ssl_socket_context_on_close(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_close)(struct us_ssl_socket *s));
WIN32_EXPORT void us_ssl_socket_context_on_data(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_data)(struct us_ssl_socket *s, char *data, int length));
WIN32_EXPORT void us_ssl_socket_context_on_writable(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_writable)(struct us_ssl_socket *s));
WIN32_EXPORT void us_ssl_socket_context_on_timeout(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_timeout)(struct us_ssl_socket *s));
WIN32_EXPORT void us_ssl_socket_context_on_end(struct us_ssl_socket_context *context, struct us_ssl_socket *(*on_end)(struct us_ssl_socket *s));

/* See us_socket_context */
WIN32_EXPORT struct us_listen_socket *us_ssl_socket_context_listen(struct us_ssl_socket_context *context, const char *host, int port, int options, int socket_ext_size);

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

WIN32_EXPORT struct us_loop *us_ssl_socket_context_loop(struct us_ssl_socket_context *context);
```
