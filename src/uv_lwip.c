#include <lwip/init.h>
#include "uv_lwip.h"
#include <base/llog.h>
#include <lwip/ip.h>
#include <lwip/ip_addr.h>
#include <lwip/priv/tcp_priv.h>
#include <lwip/tcp.h>
#include <lwip/ip4_frag.h>
#include <lwip/nd6.h>
#include <lwip/ip6_frag.h>
#include <string.h>

static uv_once_t uvl_init_once = UV_ONCE_INIT;

static void addr_from_lwip(void *ip, const ip_addr_t *ip_addr)
{
    if (IP_IS_V6(ip_addr)) {
        LLOG(LLOG_ERROR, "ipv6 not support now");
        return;
    } else {
        memcpy(ip, &ip_addr->u_addr.ip4.addr, 4);
    }
}

static void uvl_client_err_func (void *arg, err_t err)
{

}

static err_t uvl_client_recv_func (void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{

}

static err_t uvl_client_sent_func (void *arg, struct tcp_pcb *tpcb, u16_t len)
{

}

static err_t uvl_listener_accept_func (void *arg, struct tcp_pcb *newpcb, err_t err)
{
    uvl_t *handle = (uvl_t *)arg;

    ASSERT(handle->listener)
    ASSERT(handle->connection_cb)
    ASSERT(handle->waiting_pcb == NULL)

    uv_loop_t *loop = handle->loop;

    handle->waiting_pcb = newpcb;
    handle->connection_cb(handle, 0);

    // not accept?
    if (handle->waiting_pcb != NULL) {
        // send rst
        tcp_abort(newpcb);
    }

    return ERR_OK;
}

static err_t uvl_netif_output_func (struct netif *netif, struct pbuf *p, const ip4_addr_t *ipaddr)
{
    uvl_t *handle = (uvl_t *)netif->state;
    uv_buf_t bufs[UVL_NBUF_LEN];
    int ret;
    int i = 0;

    do {
        bufs[i].base = p->payload;
        bufs[i].len = p->len;
        i += 1;
    } while ((p = p->next));

    ret = handle->output(handle, bufs, i);

    if (ret != 0) {
        LLOG(LLOG_ERROR, "uvl->output %d", ret);
    }

    return ret == 0 ? ERR_OK : ERR_IF;
}

static err_t uvl_netif_init_func (struct netif *netif)
{
    netif->name[0] = 'h';
    netif->name[1] = 'o';
    netif->output = uvl_netif_output_func;
    // netif->output_ip6 = netif_output_ip6_func;

    return ERR_OK;
}


static err_t uvl_netif_input_func(struct pbuf *p, struct netif *inp)
{
    uint8_t ip_version = 0;
    if (p->len > 0) {
        ip_version = (((uint8_t *)p->payload)[0] >> 4);
    }

    switch (ip_version) {
        case 4: {
            return ip4_input(p, inp);
        } break;
        case 6: {
            return ip6_input(p, inp);
        } break;
    }

    pbuf_free(p);
    return ERR_OK;
}

int uvl_read_start(uvl_tcp_t *client, uvl_alloc_cb alloc_cb, uvl_read_cb read_cb)
{
    ASSERT(client->reading == 0)


}

int uvl_accept(uvl_t *handle, uvl_tcp_t *client)
{
    ASSERT(handle->waiting_pcb)
    ASSERT(client->loop == handle->loop)
    ASSERT(client->handle == NULL)

    struct tcp_pcb *newpcb = handle->waiting_pcb;
    uint8_t local_addr[4];
    uint8_t remote_addr[4];

    addr_from_lwip(local_addr, &newpcb->local_ip);
    addr_from_lwip(remote_addr, &newpcb->remote_ip);

    client->handle = handle;

    client->local_addr.sin_family = AF_INET;
    client->local_addr.sin_addr = *((struct in_addr *)local_addr);
    client->local_addr.sin_port = htons(newpcb->local_port);

    client->remote_addr.sin_family = AF_INET;
    client->remote_addr.sin_addr = *((struct in_addr *)remote_addr);
    client->remote_addr.sin_port = htons(newpcb->remote_port);

    tcp_arg(newpcb, client);

    tcp_err(newpcb, uvl_client_err_func);
    tcp_recv(newpcb, uvl_client_recv_func);
    tcp_sent(newpcb, uvl_client_sent_func);

    handle->waiting_pcb = NULL;

    return 0;
}

int uvl_init_lwip(uvl_t *handle)
{
    struct netif *the_netif = (struct netif *)malloc(sizeof(struct netif));
    handle->the_netif = the_netif;

    uv_once(&uvl_init_once, lwip_init);

    // make addresses for netif
    ip4_addr_t addr;
    ip4_addr_t netmask;
    ip4_addr_t gw;
    ip4_addr_set_any(&addr);
    ip4_addr_set_any(&netmask);
    ip4_addr_set_any(&gw);
    if (!netif_add(the_netif, &addr, &netmask, &gw, handle /* state */, uvl_netif_init_func, uvl_netif_input_func)) {
        LLOG(LLOG_ERROR, "netif_add failed");
        goto fail;
    }

    // set netif up
    netif_set_up(the_netif);

    // set netif link up, otherwise ip route will refuse to route
    netif_set_link_up(the_netif);

    // set netif pretend TCP
    netif_set_pretend_tcp(the_netif, 1);

    // set netif default
    netif_set_default(the_netif);

    return 0;
fail:
    return -1;
}

int uvl_tcp_init(uv_loop_t *loop, uvl_tcp_t *client)
{
    client->loop = loop;
    client->handle = NULL;
    client->reading = 0;

    memset(&client->local_addr, 0, sizeof(client->local_addr));
    memset(&client->remote_addr, 0, sizeof(client->remote_addr));

    return 0;
}

int uvl_init(uv_loop_t *loop, uvl_t *handle)
{
    handle->loop = loop;
    handle->output = NULL;
    handle->connection_cb = NULL;

    handle->listener = NULL;
    handle->waiting_pcb = NULL;

    return uvl_init_lwip(handle);
}

int uvl_bind(uvl_t *handle, uvl_output_fn output)
{
    handle->output = output;

    return 0;
}

int uvl_input(uvl_t *handle, const uv_buf_t buf[], unsigned int nbufs)
{

}

int uvl_listen(uvl_t *handle, uvl_connection_cb connection_cb)
{
    handle->connection_cb = connection_cb;

    // init listener
    struct tcp_pcb *l = tcp_new_ip_type(IPADDR_TYPE_V4);
    if (!l) {
        LLOG(LLOG_ERROR, "tcp_new_ip_type failed");
        goto fail;
    }

    // bind listener TODO: multiple netif support ?
    if (tcp_bind_to_netif(l, "ho0") != ERR_OK) {
        LLOG(LLOG_ERROR, "tcp_bind_to_netif failed");
        tcp_close(l);
        goto fail;
    }

    // ensure the listener only accepts connections from this netif
    // tcp_bind_netif(l, the_netif);

    // listen listener
    if (!(handle->listener = tcp_listen(l))) {
        LLOG(LLOG_ERROR, "tcp_listen failed");
        tcp_close(l);
        goto fail;
    }

    tcp_arg(handle->listener, handle);
    // setup listener accept handler
    tcp_accept(handle->listener, uvl_listener_accept_func);

    return 0;
fail:
    return -1;
}
