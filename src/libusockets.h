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

#ifndef LIBUSOCKETS_H
#define LIBUSOCKETS_H

/* 512kb shared receive buffer */
#define LIBUS_RECV_BUFFER_LENGTH 524288
/* A timeout granularity of 4 seconds means give or take 4 seconds from set timeout */
#define LIBUS_TIMEOUT_GRANULARITY 4
/* 32 byte padding of receive buffer ends */
#define LIBUS_RECV_BUFFER_PADDING 32
/* Guaranteed alignment of extension memory */
#define LIBUS_EXT_ALIGNMENT 16

/* Define what a socket descriptor is based on platform */
#ifdef _WIN32
#define NOMINMAX
#include <WinSock2.h>
#define LIBUS_SOCKET_DESCRIPTOR SOCKET
#define WIN32_EXPORT __declspec(dllexport)
#define alignas(x) __declspec(align(x))
#else
#include <stdalign.h>
#define LIBUS_SOCKET_DESCRIPTOR int
#define WIN32_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* I guess these are listening options */
enum {
    OPTION_REUSE_PORT
};

/* Library types publicly available */
struct us_socket;
struct us_timer;
struct us_socket_context;
struct us_loop;
struct us_ssl_socket_context;
struct us_ssl_socket;

/* These are public interfaces sorted by subject */
#include "interfaces/timer.h"
#include "interfaces/context.h"
#include "interfaces/loop.h"
#include "interfaces/poll.h"
#include "interfaces/ssl.h"
#include "interfaces/socket.h"

#ifdef __cplusplus
}
#endif

/* Decide what eventing system to use */
#if !defined(LIBUS_USE_EPOLL) && !defined(LIBUS_USE_LIBUV)
#ifdef _WIN32
#define LIBUS_USE_LIBUV
#else
#define LIBUS_USE_EPOLL
#endif
#endif

#endif // LIBUSOCKETS_H
