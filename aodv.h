#ifndef AODV_H
#define AODV_H

#include <stdint.h>
#include "broadcast.h"

#define AODV_RREQ_TTL 10

typedef enum {
    RREQ,
} AodvType;

typedef struct {
    uint8_t source_address;
    uint8_t destination_address;
    uint8_t ttl;
} AodvRreq;


int send_rreq(struct broadcast_conn *bc, AodvRreq *rreq);
AodvRreq *receive_rreq(uint8_t *data);

#endif
