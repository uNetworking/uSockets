#include "libusockets.h"
#include "internal/internal.h"

#include "lwip/tcp.h"
#include "lwip/init.h"
#include "lwip/timeouts.h"
#include "lwip/etharp.h"
#include "netif/ethernet.h"

/* We extend out netif's with extra post data */
struct netif_extension {
    struct netif ni;
    int fd;
};

void us_internal_tcp_check_timeouts() {
    //printf("Yolo!\n");
    sys_check_timeouts();
}

static err_t lwip_netif_output(struct netif *netif, struct pbuf *p) {
    struct netif_extension *lwip_netif_extension = (struct netif_extension *) netif;
    int fd = lwip_netif_extension->fd;

    us_internal_send_packet(fd, p->payload, p->len);
    return ERR_OK;
}

// beror på min MAC
static err_t my_netif_init(struct netif *netif) {
    netif->linkoutput = lwip_netif_output;
    netif->output     = etharp_output;
    netif->mtu        = ETHERNET_MTU;
    netif->flags      = NETIF_FLAG_LINK_UP | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

    // mitt wifi
    unsigned char mac[6] = {0xDC, 0x85, 0xDE, 0x3B, 0x8C, 0x89};

    memcpy(netif->hwaddr, mac, ETH_HWADDR_LEN);
    netif->hwaddr_len = ETH_HWADDR_LEN;
    return ERR_OK;
}

// our write function called from socket layer
int us_internal_tcp_write(struct us_socket_t *s, char *data, int length) {
    // tcp_pcb från us_socket_t!

    // returning length here solves the mailbox issues

    //return length;

    struct tcp_pcb *pcb = s->pcb;

    if (ERR_OK == tcp_write(pcb, data, length, TCP_WRITE_FLAG_COPY)) {
        printf("SENT OFF DATA!\n");

        //tcp_output(pcb);

        return length;
    }

    return 0;
}

err_t tcp_recv_f(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {

    if (!p) {
        printf("Fick vi precis shutdown?\n");
        return ERR_OK;
    }

    //printf("p: %p\n", p);

    struct us_socket_t *s = (struct us_socket_t *) arg;

    s->context->on_data(s, p->payload, p->len);
	tcp_recved(tpcb, p->len);

    pbuf_free(p);

	return ERR_OK;
}

// sockets are generally us_poll's but now they should be holding a pointer to tcp_pdb instead, and it holding as argument, the socket
// and the socket interfaces should be re-implemented, esp write!

// on_open here
err_t accept_fn(void *arg, struct tcp_pcb *newpcb, err_t err) {

    //tcp_backlog_accepted(newpcb);

    if (err == ERR_MEM) {
        printf("ACCEPT RETURNED OUT OF MEMORY!\n");
        return ERR_OK;
    }

    if (err != ERR_OK) {
        printf("Accept called with error: %d\n", err);
    } else {
        printf("Accept called without error with: %p\n", newpcb);
    }

    struct us_socket_context_t *context = (struct us_socket_context_t *) arg;

    // tcp_pcb -> us_socket_t ?

    // create a new socket and link the tcp_pdb to it?

    // struct us_socket_t *s, int is_client, char *ip, int ip_length

    // us_socket_t måste vara helt annan

    struct us_socket_t *s = malloc(sizeof(struct us_socket_t) + 256);

    s->pcb = newpcb;

    s->context = arg;

    context->on_open(s, 0, 0, 0);

	tcp_recv(newpcb, tcp_recv_f);
    tcp_arg(newpcb, s);

	return ERR_OK;
}

// read and process the tcp packet
void us_internal_tcp_process_packet(void *buffer, int count, void *user) {
    // usr is netif
    struct netif *netif = (struct netif *) user;

    // är det denna som fuckar loss?
    //printf("Kommer allokera buffer nu: %d\n", count);
    struct pbuf* p = pbuf_alloc(PBUF_RAW, count, PBUF_POOL);
    //printf("Allokerade buffer!\n");

    if (!p) {
        printf("ALLOCATED NULLPTR!\n");
        exit(0);
    }

    if (pbuf_take(p, buffer, count) != ERR_OK) {
        printf("LALALA?\n");
        exit(0);
    }

    if(netif->input(p, netif) != ERR_OK) {
        printf("CANNOT INSERT PACKET!\n");
        pbuf_free(p);
    } else {
        //printf("PArsed packet!\n");
    }
}

// opakt gemensamt gränssnitt delat med bsd-nätverkandet
// eller bara sepcial-fall för user space?
struct us_listen_socket_t *us_internal_tcp_lwip_listen(struct us_socket_context_t *context, const char *host, int port, int options) {

    printf("us_internal_tcp_lwip_listen\n");

    // om vi vet socket contexten här?

    // använd opaka funktioner från tcp.c? med samma gränssnitt som för bsd nedan?

    // detta är första stället du kan skapa en listen socket med lwip
    struct tcp_pcb *t = tcp_new();
    if (ERR_OK == tcp_bind(t, IP4_ADDR_ANY, 4000)) {
        printf("bound to port 4000\n");
    }
    t = tcp_listen(t);
    tcp_arg(t, context);
    tcp_accept(t, accept_fn);

    // kan vi ge tillbaka detta? är det nog?
    return 1;
}

// us_internal_init_tcp() -> void *
void *us_internal_init_lwip(int fd) {

    printf("us_internal_init_lwip\n");

    struct netif *lwip_netif = malloc(sizeof(struct netif_extension));

    struct netif_extension *lwip_netif_extension = (struct netif_extension *) lwip_netif;
    lwip_netif_extension->fd = fd;

    //struct netif netif;

    ip_addr_t ip;
    ipaddr_aton("192.168.10.203", &ip);

    ip_addr_t gw;
    ipaddr_aton("192.168.10.1", &gw);

    ip_addr_t nm;
    ipaddr_aton("255.255.255.0", &nm);


    lwip_init();

    //tcp_init();

    //tcpip_init();

    printf("add: %p\n", netif_add(lwip_netif, &ip, &nm, &gw, NULL, my_netif_init, ethernet_input));
    lwip_netif->name[0] = 'e';
    lwip_netif->name[1] = '0';
    //netif_create_ip6_linklocal_address(&netif, 1);
    //netif.ip6_autoconfig_enabled = 1;
    //netif_set_status_callback(&netif, my_netif_status_callback);
    netif_set_default(lwip_netif);
    netif_set_up(lwip_netif);

    return lwip_netif;
}