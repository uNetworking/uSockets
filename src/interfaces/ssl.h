/* Public interfaces for SSL sockets and contexts */

/* An options structure where set options are non-null. Used to initialize an SSL socket context */
struct us_ssl_socket_context_options {
    const char *key_file_name;
    const char *cert_file_name;
    const char *passphrase;
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
