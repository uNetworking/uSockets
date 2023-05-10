#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "liburing.h"

#define MAX_CONNECTIONS     4096
#define BACKLOG             512
#define MAX_MESSAGE_LEN     2048
#define BUFFERS_COUNT       MAX_CONNECTIONS

void add_accept(struct io_uring *ring, int fd, struct sockaddr *client_addr, socklen_t *client_len, unsigned flags);
void add_socket_read(struct io_uring *ring, int fd, unsigned gid, size_t size, unsigned flags);
void add_socket_write(struct io_uring *ring, int fd, __u16 bid, size_t size, unsigned flags);
void add_provide_buf(struct io_uring *ring, __u16 bid, unsigned gid);

// 8 byte aligned pointer offset
enum pointer_tags {
    SOCKET_READ,
    SOCKET_WRITE,
    LISTEN_SOCKET_ACCEPT,
    SOCKET_CONNECT,

};

#include <netinet/tcp.h>

/*void bsd_socket_nodelay(int fd, int enabled) {
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *) &enabled, sizeof(enabled));
}*/

struct us_timer_t {
    struct us_loop_t *loop;
};

struct us_loop_t {
    struct io_uring ring;

    // let's say there is only one context for now
    //struct us_socket_context_t *context_head;
};

struct us_socket_context_t {
    struct us_loop_t *loop;
    struct us_socket_t *(*on_open)(struct us_socket_t *, int is_client, char *ip, int ip_length);
    struct us_socket_t *(*on_data)(struct us_socket_t *, char *data, int length);
    struct us_socket_t *(*on_writable)(struct us_socket_t *);
    struct us_socket_t *(*on_close)(struct us_socket_t *, int code, void *reason);
};

struct us_listen_socket_t {
    struct us_socket_context_t *context;
    int socket_ext_size;
};

struct us_socket_t {
    struct us_socket_context_t *context;
    int dd;

    char sendBuf[16 * 1024];
};