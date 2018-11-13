# libusockets.h
This is the only header you include. Following documentation has been extracted from this header. It may be outdated, go read the header directly for up-to-date documentation.

These interfaces are "alpha" and subject to change.

# Loop
```c
/* Public interfaces for loops */

/* Returns a new event loop with user data extension */
struct us_loop *us_create_loop(void (*wakeup_cb)(struct us_loop *loop), int ext_size);

/* Returns the loop user data extension */
void *us_loop_ext(struct us_loop *loop);

/* Blocks the calling thread and drives the event loop until no more non-fallthrough polls are scheduled */
void us_loop_run(struct us_loop *loop);
```

# Polls
```c
/* Public interfaces for polls */

/* A fallthrough poll does not keep the loop running, it falls through */
struct us_poll *us_create_poll(struct us_loop *loop, int fallthrough, int ext_size);

/* Associate this poll with a socket descriptor and poll type */
void us_poll_init(struct us_poll *p, LIBUS_SOCKET_DESCRIPTOR fd, int poll_type);

/* Start, change and stop polling for events */
void us_poll_start(struct us_poll *p, struct us_loop *loop, int events);
void us_poll_change(struct us_poll *p, struct us_loop *loop, int events);
void us_poll_stop(struct us_poll *p, struct us_loop *loop);

/* Returns the user data extension of this poll */
void *us_poll_ext(struct us_poll *p);

/* Get associated socket descriptor from a poll */
LIBUS_SOCKET_DESCRIPTOR us_poll_fd(struct us_poll *p);
```

# High cost timers
```c
/* Public interfaces for timers */

/* Create a new high precision, low performance timer. May fail and return null */
struct us_timer *us_create_timer(struct us_loop *loop, int fallthrough, int ext_size);

/* Arm a timer with a delay from now and eventually a repeat delay.
 * Specify 0 as repeat delay to disable repeating. Specify both 0 to disarm. */
void us_timer_set(struct us_timer *timer, void (*cb)(struct us_timer *t), int ms, int repeat_ms);
```

# Sockets
```c
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
```

# Socket contexts

```c
/* Public interfaces for contexts */

/* A socket context holds shared callbacks and user data extension for associated sockets */
struct us_socket_context *us_create_socket_context(struct us_loop *loop, int ext_size);

/* Setters of various async callbacks */
void us_socket_context_on_open(struct us_socket_context *context, void (*on_open)(struct us_socket *s));
void us_socket_context_on_close(struct us_socket_context *context, void (*on_close)(struct us_socket *s));
void us_socket_context_on_data(struct us_socket_context *context, void (*on_data)(struct us_socket *s, char *data, int length));
void us_socket_context_on_writable(struct us_socket_context *context, void (*on_writable)(struct us_socket *s));
void us_socket_context_on_timeout(struct us_socket_context *context, void (*on_timeout)(struct us_socket *s));

/* Returns user data extension for this socket context */
void *us_socket_context_ext(struct us_socket_context *context);

/* Listen for connections. Acts as the main driving cog in a server. Will call set async callbacks. */
struct us_listen_socket *us_socket_context_listen(struct us_socket_context *context, const char *host, int port, int options, int socket_ext_size);

/* Begin an async socket connection. Will call set async callbacks */
void us_context_connect(const char *host, int port, int options, int ext_size, void (*cb)(struct us_socket *), void *user_data);

/* (Explicitly) associate a socket with this socket context. A socket can only belong to one single socket context at any one time */
void us_socket_context_link(struct us_socket_context *context, struct us_socket *s);
```

# SSL
```c
/* Public interfaces for SSL sockets and contexts */

/* An options structure where set options are non-null. Used to initialize an SSL socket context */
struct us_ssl_socket_context_options {
    const char *key_file_name;
    const char *cert_file_name;
    const char *passphrase;
    const char *ca_file_name;
};

/* See us_create_socket_context. SSL variant taking SSL options structure */
struct us_ssl_socket_context *us_create_ssl_socket_context(struct us_loop *loop, int context_ext_size, struct us_ssl_socket_context_options options);

/* See us_socket_context */
void us_ssl_socket_context_on_open(struct us_ssl_socket_context *context, void (*on_open)(struct us_ssl_socket *s));
void us_ssl_socket_context_on_close(struct us_ssl_socket_context *context, void (*on_close)(struct us_ssl_socket *s));
void us_ssl_socket_context_on_data(struct us_ssl_socket_context *context, void (*on_data)(struct us_ssl_socket *s, char *data, int length));
void us_ssl_socket_context_on_writable(struct us_ssl_socket_context *context, void (*on_writable)(struct us_ssl_socket *s));
void us_ssl_socket_context_on_timeout(struct us_ssl_socket_context *context, void (*on_timeout)(struct us_ssl_socket *s));

/* See us_socket_context */
struct us_listen_socket *us_ssl_socket_context_listen(struct us_ssl_socket_context *context, const char *host, int port, int options, int socket_ext_size);

/* See us_socket */
int us_ssl_socket_write(struct us_ssl_socket *s, const char *data, int length);

/* See us_socket */
void us_ssl_socket_timeout(struct us_ssl_socket *s, unsigned int seconds);
```
