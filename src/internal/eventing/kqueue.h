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

#ifndef KQUEUE_H
#define KQUEUE_H

#include "internal/loop.h"

#include <sys/event.h>
#define LIBUS_SOCKET_READABLE -EVFILT_READ
#define LIBUS_SOCKET_WRITABLE -EVFILT_WRITE

struct us_loop_t {
    // common data
    alignas(LIBUS_EXT_ALIGNMENT) struct us_loop_data data;

    // epoll extensions
    int num_polls;
    int num_fd_ready;
    int fd_iterator;
    int kqfd;
    struct kevent ready_events[1024];
};

struct us_poll_t {
    struct {
        int fd : 28;
        unsigned int poll_type : 4;
    } state;
};

#endif // KQUEUE_H
