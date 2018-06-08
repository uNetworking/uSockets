#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

int us_socket_write(struct us_socket *s, const char *data, int length, int msg_more) {

    /*if (msg_more) {
        printf("MSG_MORE send of size: %d\n", length);
    } else {
        printf("Regular send of size: %d\n", length);
    }*/

    // MSG_NOSIGNAL for linux and freebsd

    // Linux specific hack for SSL speedup: msg_more
    int written = bsd_send(us_poll_fd((struct us_poll *) s), data, length, msg_more ? MSG_MORE : 0);

    if (written == -1) {
        return 0;
        printf("WRITE FAILED!\n");
    }

    // automatically poll for writable here
    if (written != length) {
        // only do this if at the first segment!
        //if (length == 104857646) {
            us_poll_change((struct us_poll *) s, s->context->loop, LIBUS_SOCKET_READABLE | LIBUS_SOCKET_WRITABLE);
        //}
    }

    return written;
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
