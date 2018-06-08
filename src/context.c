#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

// put this in loop.c?
struct us_socket_context *us_create_socket_context(struct us_loop *loop, int context_ext_size) {
    struct us_socket_context *context = malloc(sizeof(struct us_socket_context) + context_ext_size);

    // us_socket_context_init(loop)
    context->loop = loop;
    context->head = 0;
    context->next = 0;

    // link it to loop
    // us_loop_link(context)
    loop->head = context;

    return context;
}

struct us_listen_socket *us_socket_context_listen(struct us_socket_context *context, const char *host, int port, int options, int socket_ext_size) {

    LIBUS_SOCKET_DESCRIPTOR listen_socket_fd = bsd_create_listen_socket(host, port, options);

    if (listen_socket_fd == LIBUS_SOCKET_ERROR) {
        return 0;
    }

    struct us_poll *p = us_create_poll(context->loop, 0, sizeof(struct us_listen_socket));
    us_poll_init(p, listen_socket_fd, POLL_TYPE_LISTEN_SOCKET);
    us_poll_start(p, context->loop, LIBUS_SOCKET_READABLE);

    struct us_listen_socket *ls = (struct us_listen_socket *) p;
    // this is common, should be like us_internal_socket_init(context);
    ls->s.context = context;
    ls->s.timeout = 0;
    ls->s.next = 0;
    us_socket_context_link(context, &ls->s);

    ls->socket_ext_size = socket_ext_size;

    return ls;
}

void us_context_connect(const char *host, int port, int options, int ext_size, void (*cb)(struct us_socket *s), void *user_data) {

    cb(0);
}

void us_socket_context_link(struct us_socket_context *context, struct us_socket *s) {
    // just add it to the context

    context->head = s;
    s->next = 0;
}
