#ifndef AODV_H
#define AODV_H

#include <stdint.h>
#include "broadcast.h"
#include <stdbool.h>

#define AODV_RREQ_TTL 10

typedef struct {
    bool in_use;
    uint8_t destination;
    uint8_t next_hop;
    uint8_t distance;
} AodvRoutingEntry;

typedef enum {
    RREQ,
} AodvType;

typedef struct {
    uint8_t source_address;
    uint8_t destination_address;
    uint8_t ttl;
} AodvRreq;


int aodv_send_rreq(struct broadcast_conn *bc, AodvRreq *rreq);
AodvRreq *aodv_receive_rreq(uint8_t *data);

#endif
