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

// note: if in a sweep, this can break the iteration if not properly done
void us_socket_timeout(struct us_socket *s, unsigned int seconds) {
    if (seconds) {
        unsigned short timeout_sweeps = seconds / LIBUS_TIMEOUT_GRANULARITY;
        s->timeout = timeout_sweeps ? timeout_sweeps : 1;
    } else {
        s->timeout = 0;
    }

    // setting a timeout when one already is there should not succeed

    //

    /*if (seconds) {
        s->timeout = timeout_sweeps ? timeout_sweeps : 1;

        // link
        s->next = s->context->loop->timeout_list_head;
        s->prev = 0;
        if (s->context->loop->timeout_list_head) {
            s->context->loop->timeout_list_head->prev = s;
        }
        s->context->loop->timeout_list_head = s;
    } else {
        // unlink
        if (s->prev == s->next) {
            s->context->loop->timeout_list_head = 0;
        } else {
            if (s->prev) {
                s->prev->next = s->next;
            } else {
                s->context->loop->timeout_list_head = s->next;
            }
            if (s->next) {
                s->next->prev = s->prev;
            }
        }
    }*/
}
