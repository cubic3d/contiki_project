#ifndef AODV_H
#define AODV_H

#include <stdint.h>
#include "broadcast.h"
#include <stdbool.h>

#define AODV_RREQ_TTL 10
#define AODV_RT_SIZE 50

typedef struct {
    bool in_use;
    uint8_t next_hop;
    uint8_t distance;
    uint8_t sequence_number;
    bool valid_sequence_number;
} AodvRoutingEntry;

typedef enum {
    RREQ,
} AodvType;

typedef struct {
    uint8_t id;
    uint8_t source_address;
    uint8_t source_sequence_number;
    uint8_t destination_address;
    uint8_t destination_sequence_number;
    uint8_t ttl;
    bool unknown_sequence_number;
} AodvRreq;


int aodv_send_rreq(struct broadcast_conn *bc, AodvRreq *rreq);
int aodv_send_rreq2(struct broadcast_conn *bc, uint8_t destination_address);
bool aodv_seen_rreq(AodvRreq *rreq);
AodvRreq *aodv_receive_rreq(uint8_t *data);
void aodv_print_rreq(const char* action, AodvRreq *rreq);

void aodv_routing_table_init();
void aodv_routing_table_print();
void aodv_routing_table_update_prev_hop(uint8_t from, AodvRreq *rreq);
void aodv_routing_table_update_source(uint8_t from, AodvRreq *rreq);

#endif
