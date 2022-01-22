#ifdef LIBUS_USE_QUIC

/* Experimental QUIC functions */

#include "libusockets.h"

typedef struct {
    char *cert_file_name;
    char *key_file_name;
} us_quic_socket_context_options_t;

typedef struct {

} us_quic_socket_t;

struct us_quic_socket_context_s;
struct us_quic_listen_socket_s;
struct us_quic_stream_s;

typedef struct us_quic_socket_context_s us_quic_socket_context_t;
typedef struct us_quic_listen_socket_s us_quic_listen_socket_t;
typedef struct us_quic_stream_s us_quic_stream_t;

int us_quic_stream_write(us_quic_stream_t *s, char *data, int length);
int us_quic_stream_shutdown(us_quic_stream_t *s);

void us_quic_socket_context_set_header(us_quic_socket_context_t *context, int index, char *key, int key_length, char *value, int value_length);
void us_quic_socket_context_send_headers(us_quic_socket_context_t *context, us_quic_stream_t *s, int num);

us_quic_socket_context_t *us_create_quic_socket_context(struct us_loop_t *loop, us_quic_socket_context_options_t options);
us_quic_listen_socket_t *us_quic_socket_context_listen(us_quic_socket_context_t *context, char *host, int port);

void us_quic_socket_context_on_stream_data(us_quic_socket_context_t *context, void(*on_stream_data)(us_quic_stream_t *s, char *data, int length));
void us_quic_socket_context_on_stream_headers(us_quic_socket_context_t *context, void(*on_stream_headers)());
void us_quic_socket_context_on_stream_open(us_quic_socket_context_t *context, void(*on_stream_open)());
void us_quic_socket_context_on_stream_close(us_quic_socket_context_t *context, void(*on_stream_close)());
void us_quic_socket_context_on_open(us_quic_socket_context_t *context, void(*on_open)());
void us_quic_socket_context_on_close(us_quic_socket_context_t *context, void(*on_close)());
void us_quic_socket_context_on_stream_writable(us_quic_socket_context_t *context, void(*on_stream_writable)());

#endif