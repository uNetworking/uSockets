/* Public interfaces for SSL sockets and contexts */

#ifndef LIBUS_NO_SSL

/* An options structure where set options are non-null. Used to initialize an SSL socket context */
struct us_ssl_socket_context_options {
    const char *key_file_name;
    const char *cert_file_name;
    const char *passphrase;
};

/* See us_create_socket_context. SSL variant taking SSL options structure */
WIN32_EXPORT struct us_ssl_socket_context *us_create_ssl_socket_context(struct us_loop *loop, int context_ext_size, struct us_ssl_socket_context_options options);

/* */
WIN32_EXPORT void us_ssl_socket_context_free(struct us_ssl_socket_context *context);

/* See us_socket_context */
WIN32_EXPORT void us_ssl_socket_context_on_open(struct us_ssl_socket_context *context, void (*on_open)(struct us_ssl_socket *s, int is_client));
WIN32_EXPORT void us_ssl_socket_context_on_close(struct us_ssl_socket_context *context, void (*on_close)(struct us_ssl_socket *s));
WIN32_EXPORT void us_ssl_socket_context_on_data(struct us_ssl_socket_context *context, void (*on_data)(struct us_ssl_socket *s, char *data, int length));
WIN32_EXPORT void us_ssl_socket_context_on_writable(struct us_ssl_socket_context *context, void (*on_writable)(struct us_ssl_socket *s));
WIN32_EXPORT void us_ssl_socket_context_on_timeout(struct us_ssl_socket_context *context, void (*on_timeout)(struct us_ssl_socket *s));

/* Technically SSL sockets cannot be half-closed, so this callback is never called */
WIN32_EXPORT void us_ssl_socket_context_on_end(struct us_ssl_socket_context *context, void (*on_end)(struct us_ssl_socket *s));

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
WIN32_EXPORT void us_ssl_socket_close(struct us_ssl_socket *s);

#endif
