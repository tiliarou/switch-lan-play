#include "lan-play.h"
#include <assert.h>

int send_payloads(
    struct packet_ctx *self,
    const struct payload *payload
)
{
    uint8_t *buf = self->buffer;
    const struct payload *part = payload;
    uint16_t total_len = 0;

    while (part) {
        if (buf - (uint8_t *)self->buffer + part->len >= self->buffer_len) {
            LLOG(LLOG_ERROR, "send_payloads too large wanted: %d buffer_len: %d", buf - (uint8_t *)self->buffer + part->len, self->buffer_len);
            LLOG(LLOG_DEBUG, "send_payloads buffer: %p", self->buffer);
            assert(0);
            return -1;
        }
        memcpy(buf, part->ptr, part->len);
        buf += part->len;
        total_len += part->len;

        part = part->next;
    }

    self->upload_packet++;
    self->upload_byte += total_len;

    // print_hex(self->buffer, total_len);
    // printf("total len %d\n", total_len);
    return lan_play_send_packet(self->arg, self->buffer, total_len);
}

int send_ether_ex(
    struct packet_ctx *arg,
    const void *dst,
    const void *src,
    uint16_t type,
    const struct payload *payload
)
{
    uint8_t buffer[ETHER_HEADER_LEN];
    struct payload part;

    part.ptr = buffer;
    part.len = ETHER_HEADER_LEN;
    part.next = payload;

    CPY_MAC(buffer + ETHER_OFF_DST, dst);
    CPY_MAC(buffer + ETHER_OFF_SRC, src);
    WRITE_NET16(buffer, ETHER_OFF_TYPE, type);

    return send_payloads(arg, &part);
}
int send_ether(
    struct packet_ctx *arg,
    const void *dst,
    uint16_t type,
    const struct payload *payload
)
{
    return send_ether_ex(
        arg,
        dst,
        arg->mac,
        type,
        payload
    );
}

void print_packet(const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
    eprintf("Packet length: %d\n", pkthdr->len);
    eprintf("Number of bytes: %d\n", pkthdr->caplen);
    eprintf("Recieved time: %s", ctime((const time_t *)&pkthdr->ts.tv_sec));

    uint32_t i;
    for (i=0; i<pkthdr->len; ++i) {
        eprintf(" %02x", packet[i]);
        if ( (i + 1) % 16 == 0 ) {
            eprintf("\n");
        }
    }

    eprintf("\n\n");
}

static u_char AnyMac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
int packet_init(
    struct packet_ctx *self,
    struct lan_play *arg,
    void *buffer,
    size_t buffer_len,

    void *ip,
    void *subnet_net,
    void *subnet_mask,
    time_t arp_ttl)
{
    self->arg = arg;
    self->buffer = buffer;
    self->buffer_len = buffer_len;

    CPY_IPV4(self->ip, ip);
    CPY_IPV4(self->subnet_net, subnet_net);
    CPY_IPV4(self->subnet_mask, subnet_mask);

    self->mac = AnyMac;

    self->identification = 0;
    arp_list_init(self->arp_list);
    self->arp_ttl = arp_ttl;

    self->upload_packet = 0;
    self->upload_byte = 0;
    self->download_packet = 0;
    self->download_byte = 0;

    return 0;
}


int packet_close(struct packet_ctx *self)
{
    return 0; // nothing to release
}

void parse_ether(const u_char *packet, uint16_t len, struct ether_frame *ether)
{
    CPY_MAC(ether->dst, packet + ETHER_OFF_DST);
    CPY_MAC(ether->src, packet + ETHER_OFF_SRC);
    ether->raw = packet;
    ether->raw_len = len;
    ether->type = READ_NET16(packet, ETHER_OFF_TYPE);
    ether->payload = packet + ETHER_OFF_END;
}

int process_ether(struct packet_ctx *arg, const u_char *packet, uint16_t len)
{
    struct ether_frame ether;
    parse_ether(packet, len, &ether);

    if (CMP_MAC(ether.src, arg->mac)) {
        return 0;
    }

    switch (ether.type) {
        case ETHER_TYPE_ARP:
            return process_arp(arg, &ether);
        case ETHER_TYPE_IPV4:
            return process_ipv4(arg, &ether);
        default:
            return 0; // just ignore them
    }
}

void packet_set_mac(struct packet_ctx *arg, const uint8_t *mac)
{
    arg->mac = mac;
}

void get_packet(struct packet_ctx *self, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
    if (pkthdr->len >= 65536) {
        print_packet(pkthdr, packet);
        return;
    }
    self->download_packet++;
    self->download_byte += pkthdr->len;
    if (process_ether(self, packet, pkthdr->len) != 0) {
        print_packet(pkthdr, packet);
    }
}

uint16_t payload_total_len(const struct payload *payload)
{
    const struct payload *part = payload;
    uint16_t total_len = 0;

    while (part) {
        total_len += part->len;
        part = part->next;
    }

    return total_len;
}

void payload_print_hex(const struct payload *payload)
{
    const struct payload *part = payload;
    int i = 0;
    int j;

    while (part) {
        for (j = 0; j < part->len; j++) {
            eprintf(" %02x", part->ptr[j]);
            if ( ++i % 16 == 0 ) {
                eprintf("\n");
            }
        }

        part = part->next;
    }

    eprintf("\n\n");
}
