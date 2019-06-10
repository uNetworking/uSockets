/*
 * Authored by Alex Hultman, 2018-2019.
 * Intellectual property of third-party.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http://www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "libusockets.h"
#include "internal/common.h"
#include <stdlib.h>

void us_internal_init_socket(struct us_socket_t *s) {
    // shared nodelay here?
}

void us_socket_remote_address(int ssl, struct us_socket_t *s, char *buf, int *length) {
    struct bsd_addr_t addr;
    if (bsd_socket_addr(us_poll_fd(&s->p), &addr) || *length < bsd_addr_get_ip_length(&addr)) {
        *length = 0;
    } else {
        *length = bsd_addr_get_ip_length(&addr);
        memcpy(buf, bsd_addr_get_ip(&addr), *length);
    }
}

int us_socket_write(int ssl, struct us_socket_t *s, const char *data, int length, int msg_more) {
    if (us_socket_is_closed(ssl, s) || us_socket_is_shut_down(ssl, s)) {
        return 0;
    }

    int written = bsd_send(us_poll_fd(&s->p), data, length, msg_more);

    if (written != length) {
        s->context->loop->data.last_write_failed = 1;
        us_poll_change(&s->p, s->context->loop, LIBUS_SOCKET_READABLE | LIBUS_SOCKET_WRITABLE);
    }

    return written < 0 ? 0 : written;
}

/* This is an experimentation for new SSL-routing */
void *us_socket_ext(int ssl, struct us_socket_t *s) {
    /* We route this call to SSL if so? */
#ifndef LIBUS_NO_SSL
    if (ssl) {
        return us_ssl_socket_ext(s);
    }
#endif

    return s + 1;
}

struct us_socket_context_t *us_socket_context(int ssl, struct us_socket_t *s) {
    return s->context;
}

void us_socket_timeout(int ssl, struct us_socket_t *s, unsigned int seconds) {
    if (seconds) {
        unsigned short timeout_sweeps = 0.5f + ((float) seconds) / ((float) LIBUS_TIMEOUT_GRANULARITY);
        s->timeout = timeout_sweeps ? timeout_sweeps : 1;
    } else {
        s->timeout = 0;
    }
}

void us_socket_flush(int ssl, struct us_socket_t *s) {
    if (!us_socket_is_shut_down(ssl, s)) {
        bsd_socket_flush(us_poll_fd((struct us_poll_t *) s));
    }
}

// if anything, close should return null and people should handle that!
// including us really, however I guess libuv requires it to stick around?
struct us_socket_t *us_socket_close(int ssl, struct us_socket_t *s) {
    if (!us_socket_is_closed(ssl, s)) {
        us_internal_socket_context_unlink(s->context, s);
        us_poll_stop((struct us_poll_t *) s, s->context->loop);
        bsd_close_socket(us_poll_fd((struct us_poll_t *) s));

        // link this socket to the close-list and let it be deleted after this iteration
        s->next = s->context->loop->data.closed_head;
        s->context->loop->data.closed_head = s;

        // signal closed socket by having prev = next
        //s->prev = s->next;

        // signal socket closed by having prev = context
        s->prev = (struct us_socket_t *) s->context;

        return s->context->on_close(s);
    }
    return s;
}

int us_socket_is_shut_down(int ssl, struct us_socket_t *s) {
    return us_internal_poll_type(&s->p) == POLL_TYPE_SOCKET_SHUT_DOWN;
}

void us_socket_shutdown(int ssl, struct us_socket_t *s) {
    // todo: should we emit on_close if calling shutdown on an already half-closed socket?
    // we need more states in that case, we need to track RECEIVED_FIN
    // so far, the app has to track this and call close as needed
    if (!us_socket_is_closed(ssl, s) && !us_socket_is_shut_down(ssl, s)) {
        us_internal_poll_set_type(&s->p, POLL_TYPE_SOCKET_SHUT_DOWN);
        us_poll_change(&s->p, s->context->loop, us_poll_events(&s->p) & LIBUS_SOCKET_READABLE);
        bsd_shutdown_socket(us_poll_fd((struct us_poll_t *) s));
    }
}

int us_socket_is_closed(int ssl, struct us_socket_t *s) {
    // this does not work as flag if only holding 1 single socket in a context
    // we only trigger this bug in a separate context since it holds no listen socket!
    //return s->prev == s->next;

    return s->prev == (struct us_socket_t *) s->context;
}
