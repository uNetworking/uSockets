#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

void us_internal_init_socket(struct us_socket *s) {

}

int us_socket_write(struct us_socket *s, const char *data, int length, int msg_more) {
    int written = bsd_send(us_poll_fd(&s->p), data, length, msg_more ? MSG_MORE : 0);

    if (written != length) {
        s->context->loop->data.last_write_failed = 1;
        us_poll_change(&s->p, s->context->loop, LIBUS_SOCKET_READABLE | LIBUS_SOCKET_WRITABLE);
    }

    return written < 0 ? 0 : written;
}

void *us_socket_ext(struct us_socket *s) {
    return s + 1;
}

struct us_socket_context *us_socket_get_context(struct us_socket *s) {
    return s->context;
}

void us_socket_timeout(struct us_socket *s, unsigned int seconds) {
    if (seconds) {
        unsigned short timeout_sweeps = 0.5f + ((float) seconds) / ((float) LIBUS_TIMEOUT_GRANULARITY);
        s->timeout = timeout_sweeps ? timeout_sweeps : 1;
    } else {
        s->timeout = 0;
    }
}

void us_socket_flush(struct us_socket *s) {
    bsd_socket_flush(us_poll_fd((struct us_poll *) s));
}

void us_socket_close(struct us_socket *s) {
    us_socket_context_unlink(s->context, s);
    us_poll_stop((struct us_poll *) s, s->context->loop);
    bsd_close_socket(us_poll_fd((struct us_poll *) s));

    s->context->on_close(s);

    // we signal closed socket by setting its context to null
    s->context = 0;
}

int us_socket_is_shut_down(struct us_socket *s) {
    return us_internal_poll_type(&s->p) == POLL_TYPE_SOCKET_SHUT_DOWN;
}

void us_socket_shutdown(struct us_socket *s) {
    if (!us_socket_is_shut_down(s)) {
        us_internal_poll_set_type(&s->p, POLL_TYPE_SOCKET_SHUT_DOWN);
        bsd_shutdown_socket(us_poll_fd((struct us_poll *) s));
    }
}

int us_internal_socket_is_closed(struct us_socket *s) {
    return s->context != 0;
}
