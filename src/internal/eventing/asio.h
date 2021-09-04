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

#ifndef ASIO_H
#define ASIO_H

#include "internal/loop_data.h"

#define LIBUS_SOCKET_READABLE 1
#define LIBUS_SOCKET_WRITABLE 2

struct us_loop_t {
    alignas(LIBUS_EXT_ALIGNMENT) struct us_internal_loop_data_t data;

    // a loop is an io_context
    void *io;

    // whether or not we got an io_context as hint or not
    int is_default;
};

// it is no longer valid to cast a pointer to us_poll_t to a pointer of uv_poll_t
struct us_poll_t {
    void *boost_block;

    LIBUS_SOCKET_DESCRIPTOR fd;
    unsigned char poll_type;
    int events;
};

#endif // ASIO_H
