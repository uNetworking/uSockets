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

#ifndef LOOP_H
#define LOOP_H

// we need to define us_internal_async here still!

struct us_internal_async;
struct us_loop_t;

struct us_internal_async *us_internal_create_async(struct us_loop_t *loop, int fallthrough, unsigned int ext_size);
void us_internal_async_close(struct us_internal_async *a);
void us_internal_async_set(struct us_internal_async *a, void (*cb)(struct us_internal_async *));
void us_internal_async_wakeup(struct us_internal_async *a);

struct us_loop_data {
    struct us_timer *sweep_timer;
    struct us_internal_async *wakeup_async;
    int last_write_failed;
    struct us_socket_context *head;
    struct us_socket_context *iterator;
    char *recv_buf;
    void *ssl_data;
    void (*pre_cb)(struct us_loop_t *);
    void (*post_cb)(struct us_loop_t *);
    struct us_socket *closed_head;
    // we use this to ease the separation between ssl and tcp
    // it doesn't matter if this flips, but it never will
    // it increases by 1 every loop pre
    long long iteration_nr;
};

void us_internal_loop_data_init(struct us_loop_t *loop, void (*wakeup_cb)(struct us_loop_t *loop), void (*pre_cb)(struct us_loop_t *loop), void (*post_cb)(struct us_loop_t *loop));
void us_internal_loop_data_free(struct us_loop_t *loop);
void us_internal_loop_pre(struct us_loop_t *loop);
void us_internal_loop_post(struct us_loop_t *loop);

#endif // LOOP_H
