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

    /*struct us_listen_socket *p = us_loop_create_poll(context->loop, sizeof(struct us_listen_socket));
    us_poll_init(p, listen_socket_fd, POLL_TYPE_LISTEN_SOCKET);
    us_poll_start(p, context->loop, LIBUS_SOCKET_READABLE);

    p->socket_size = sizeof(struct us_socket) + socket_ext_size;

    // this is common, should be like us_socket_init(context);
    p->s.context = context;
    p->s.timeout = 0;
    p->s.next = 0;

    us_socket_context_link(context, p);
    return p;*/

    return 0;
}

void us_context_connect(const char *host, int port, int options, int ext_size, void (*cb)(struct us_socket *s), void *user_data) {

    cb(0);
}

void us_socket_context_link(struct us_socket_context *context, struct us_socket *s) {

    // just add it to the context

    context->head = s;
    s->next = 0;
}
