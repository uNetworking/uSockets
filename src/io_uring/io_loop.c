/*
 * Authored by Alex Hultman, 2018-2021.
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

#ifdef LIBUS_USE_IO_URING

#include "libusockets.h"
#include "internal/internal.h"
#include <stdlib.h>

/* The loop has 2 fallthrough polls */


void us_internal_loop_data_free(struct us_loop_t *loop) {

}

void us_wakeup_loop(struct us_loop_t *loop) {

}

void us_internal_loop_link(struct us_loop_t *loop, struct us_socket_context_t *context) {

}

/* Unlink is called before free */
void us_internal_loop_unlink(struct us_loop_t *loop, struct us_socket_context_t *context) {

}

/* This functions should never run recursively */
void us_internal_timer_sweep(struct us_loop_t *loop) {

}

/* We do not want to block the loop with tons and tons of CPU-intensive work for SSL handshakes.
 * Spread it out during many loop iterations, prioritizing already open connections, they are far
 * easier on CPU */
static const int MAX_LOW_PRIO_SOCKETS_PER_LOOP_ITERATION = 5;

void us_internal_handle_low_priority_sockets(struct us_loop_t *loop) {

}

/* Note: Properly takes the linked list and timeout sweep into account */
void us_internal_free_closed_sockets(struct us_loop_t *loop) {

}

long long us_loop_iteration_number(struct us_loop_t *loop) {

}

/* These may have somewhat different meaning depending on the underlying event library */
void us_internal_loop_pre(struct us_loop_t *loop) {

}

void us_internal_loop_post(struct us_loop_t *loop) {

}

/* Integration only requires the timer to be set up */
void us_loop_integrate(struct us_loop_t *loop) {

}

void *us_loop_ext(struct us_loop_t *loop) {

}

#endif