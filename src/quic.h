#ifdef LIBUS_USE_QUIC

/* Experimental QUIC functions */

#include "libusockets.h"

typedef struct {
    char *cert_file_name;
    char *key_file_name;
} us_quic_socket_context_options_t;

typedef struct {

} us_quic_socket_t;

struct us_quic_socket_context_s {

};
struct us_quic_listen_socket_s;

typedef struct us_quic_socket_context_s us_quic_socket_context_t;
typedef struct us_quic_listen_socket_s us_quic_listen_socket_t;

us_quic_socket_context_t *us_create_quic_socket_context(struct us_loop_t *loop, us_quic_socket_context_options_t options);
us_quic_listen_socket_t *us_quic_socket_context_listen(us_quic_socket_context_t *context, char *host, int port);

void us_quic_socket_context_on_data(us_quic_socket_context_t *context, void(*on_data)(us_quic_socket_t *, char *, int));

#endif