/* Public interfaces for SSL sockets and contexts */

#ifndef LIBUS_NO_SSL

/* An options structure where set options are non-null. Used to initialize an SSL socket context */
struct us_ssl_socket_context_options {
    const char *key_file_name;
    const char *cert_file_name;
    const char *passphrase;
    const char *ca_file_name;
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

#endif
