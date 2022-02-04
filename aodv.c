#include "aodv.h"
#include "net/rime/rime.h"
#include <stdio.h>

// Very primitive static table, inefficient but fine for the project.
// Max nodes supported is 50, but no hashing required and operations are O(1).
// Size of 256 will already cause boot loops on motes.
static AodvRoutingEntry routing_table[AODV_RT_SIZE];

int aodv_send_rreq(struct broadcast_conn *bc, AodvRreq *rreq) {
    static uint8_t buffer[sizeof(AodvRreq) + 1];

    buffer[0] = RREQ;
    buffer[1] = rreq->source_address;
    buffer[2] = rreq->destination_address;
    buffer[3] = rreq->ttl;

    packetbuf_copyfrom(buffer, sizeof(buffer));
    return broadcast_send(bc);
}

AodvRreq *aodv_receive_rreq(uint8_t *data) {
    static AodvRreq rreq;
    rreq.source_address = data[0];
    rreq.destination_address = data[1];
    rreq.ttl = data[2];

    return &rreq;
}

void aodv_routing_table_init() {
    static uint8_t i;
    for(i = 0; i < AODV_RT_SIZE; i++) {
        routing_table[i].in_use = false;
    }
}

void aodv_routing_table_print() {
    printf("----------------------------\n");
    printf("%-15s%-15s%-15s\n", "Destination", "Next Hop", "Distance");
    static uint8_t i;
    for(i = 0; i < AODV_RT_SIZE; i++) {
        if(routing_table[i].in_use) {
            printf("%-15d%-15d%-15d\n", i, routing_table[i].next_hop, routing_table[i].distance);
        }
    }
    printf("----------------------------\n");
}

void aodv_routing_table_update(uint8_t from, AodvRreq *rreq) {
    // Update routing table based on the RREQ, if it's not our own
    if(rreq->source_address != linkaddr_node_addr.u8[0]) {
        routing_table[rreq->source_address].in_use = true;
        routing_table[rreq->source_address].next_hop = from;
        routing_table[rreq->source_address].distance = AODV_RREQ_TTL - rreq->ttl + 1;
    }

    // Update routing table based on the packet source
    routing_table[from].in_use = true;
    routing_table[from].next_hop = from;
    routing_table[from].distance = 1;
}
