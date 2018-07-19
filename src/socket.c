#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

void us_internal_init_socket(struct us_socket *s) {

}

int us_socket_write(struct us_socket *s, const char *data, int length, int msg_more) {

    int written = bsd_send(us_poll_fd(&s->p), data, length, msg_more ? MSG_MORE : 0);

    if (written != length) {
        s->context->loop->data.last_write_failed = 1;

        // if not already polling for writable (check poll type!)
        us_poll_change(&s->p, s->context->loop, LIBUS_SOCKET_READABLE | LIBUS_SOCKET_WRITABLE);
    }

    if (written < 0) {
        return 0;
    }

    return written;
}

void *us_socket_ext(struct us_socket *s) {
    return s + 1;
}

struct us_socket_context *us_socket_get_context(struct us_socket *s) {
    return s->context;
}

void us_socket_timeout(struct us_socket *s, unsigned int seconds) {
    if (seconds) {
        unsigned short timeout_sweeps = seconds / LIBUS_TIMEOUT_GRANULARITY;
        s->timeout = timeout_sweeps ? timeout_sweeps : 1;
    } else {
        s->timeout = 0;
    }
}

void us_socket_flush(struct us_socket *s) {
    bsd_socket_flush(us_poll_fd((struct us_poll *) s));
}
