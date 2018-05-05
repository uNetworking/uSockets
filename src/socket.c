#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

/*int us_socket_fd(struct us_socket *s) {
    return us_poll_fd(s->p);
}*/

int us_socket_write(struct us_socket *s, const char *data, int length) {

    int written = bsd_send(us_poll_fd(s), data, length, 0);

    // automatically poll for writable here

    return length;
}

void *us_socket_ext(struct us_socket *s) {
    // one socket forward
    return s + 1;
}

int us_socket_type(struct us_socket *s) {
    return 0;
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
