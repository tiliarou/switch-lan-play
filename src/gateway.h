#ifndef _GATEWAY_H_
#define _GATEWAY_H_

#include <uv.h>
#include <uv_lwip.h>
#include "packet.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GATEWAY_BUFFER_SIZE 2000
#define GATEWAY_PROXY_RECV_BUF_SIZE 8192
typedef struct conn_s conn_t;

typedef int (*send_packet_func_t)(void *userdata, const void *data, uint16_t len);
struct gateway;

int gateway_init(
    struct gateway **gateway,
    struct packet_ctx *packet_ctx,
    bool fake_internet,
    struct sockaddr *socks5_proxy_addr,
    const char *username,
    const char *password);
int gateway_close(struct gateway *gateway);
void gateway_on_packet(struct gateway *gateway, const uint8_t *data, int data_len);

#ifdef __cplusplus
}
#endif

#endif // _GATEWAY_H_
