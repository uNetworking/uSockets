/* Public interfaces for contexts */

/* A socket context holds shared callbacks and user data extension for associated sockets */
struct us_socket_context *us_create_socket_context(struct us_loop *loop, int ext_size);

/* Setters of various async callbacks */
void us_socket_context_on_open(struct us_socket_context *context, void (*on_open)(struct us_socket *s));
void us_socket_context_on_close(struct us_socket_context *context, void (*on_close)(struct us_socket *s));
void us_socket_context_on_data(struct us_socket_context *context, void (*on_data)(struct us_socket *s, char *data, int length));
void us_socket_context_on_writable(struct us_socket_context *context, void (*on_writable)(struct us_socket *s));
void us_socket_context_on_timeout(struct us_socket_context *context, void (*on_timeout)(struct us_socket *s));

/* Emitted when a socket has been half-closed */
void us_socket_context_on_end(struct us_socket_context *context, void (*on_end)(struct us_socket *s));

/* Returns user data extension for this socket context */
void *us_socket_context_ext(struct us_socket_context *context);

/* Listen for connections. Acts as the main driving cog in a server. Will call set async callbacks. */
struct us_listen_socket *us_socket_context_listen(struct us_socket_context *context, const char *host, int port, int options, int socket_ext_size);

/* Begin an async socket connection. Will call set async callbacks */
void us_context_connect(const char *host, int port, int options, int ext_size, void (*cb)(struct us_socket *), void *user_data);

/* (Explicitly) associate a socket with this socket context. A socket can only belong to one single socket context at any one time */
void us_socket_context_link(struct us_socket_context *context, struct us_socket *s);

/* */
void us_socket_context_unlink(struct us_socket_context *context, struct us_socket *s);

/* */
struct us_loop *us_socket_context_loop(struct us_socket_context *context);

// you need a way to "move" a socket from one context of other kind (http -> websocket)
// you need to make sure the shared SSL_context is up-referenced so that removing the old context does not mess up the new
// or, simply say that a socket always belongs to where it was created but can be transformed into other contexts
struct us_socket *us_socket_context_transform_socket(struct us_socket_context *context, struct us_socket *s);
