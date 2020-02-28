#ifndef LIBUS_PACKET_H
#define LIBUS_PACKET_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <arpa/inet.h> //htons
#include <linux/if_packet.h>
//#include <net/ethernet.h> /* the L2 protocols */
#include <linux/if_ether.h>

// packet in/out and tcp parsing has to happen in a separate thread where received data is put in per-socket
// buffers and the mainthread loop is woken up to iterate over those buffers, user callbacks has to run in main thread

// for now we run everything on the main thread though

#define ETHERNET_MTU 1500

/* Interfaces for connecting with socket context, loop and tcp layer */
void us_internal_tcp_process_packet(void *payload, int length, void *user);
void *us_internal_init_lwip();
struct us_listen_socket_t *us_internal_tcp_lwip_listen(struct us_socket_context_t *context, const char *host, int port, int options);
int us_internal_tcp_write(struct us_socket_t *s, char *data, int length);

/* Create and return the packet socket, FD */
static inline int us_internal_get_packet_socket() {
    return socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, htons(ETH_P_ALL));
}

static inline void us_internal_send_packet(int fd, char *data, int length) {
    struct ethhdr *eh = data;

    struct msghdr msg = {};

    struct sockaddr_ll sin = {};

    //ip_addr_t ip;
    //ipaddr_aton("192.168.168.30", &ip);

    sin.sll_family = AF_PACKET;
    sin.sll_protocol = eh->h_proto;

    memcpy(sin.sll_addr, eh->h_source, 6);

    //sin.sll_addr = eh->eh.h_source;
    sin.sll_halen = 6;
    sin.sll_ifindex = 3; // vilket index ska vi egentligen ha? 0?

    // cat /sys/class/net/wlp3s0/ifindex

    struct iovec message;
    message.iov_base = data;
    message.iov_len = length;

    msg.msg_iov = &message;
    msg.msg_iovlen = 1;

    msg.msg_name = &sin;
    msg.msg_namelen = sizeof(struct sockaddr_ll);

    sendmsg(fd, &msg, 0);
}

/* Tend to the packet socket when it is either readable or writable */
static inline void us_internal_handle_packet_socket(int fd, void *user) {
    //printf("Pakcet socket is ready!\n");

    char buffer[5000];
    /*if (setsockopt(fd, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) == -1) {
        perror("setsockopt");
        exit(1);
    }*/
    struct sockaddr_ll src_addr;
    socklen_t src_addr_len = sizeof(src_addr);
    ssize_t count = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&src_addr, &src_addr_len);

    if (count != -1) {
        //printf("Received packet of length: %ld\n", count);

        // pass this to tcp
        us_internal_tcp_process_packet(buffer, count, user);
    }
}

#endif