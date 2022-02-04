#include "aodv.h"
#include "net/rime/rime.h"
#include <stdio.h>

// Very primitive static table, inefficient but fine for the project.
// Max nodes supported is 50, but no hashing required and operations are O(1).
// Size of 256 will already cause boot loops on motes.
static AodvRoutingEntry routing_table[AODV_RT_SIZE];

// ID incremented before every RREQ a nodes makes to allow duplicate detection.
static uint8_t node_rreq_number = 0;

int aodv_send_rreq(struct broadcast_conn *bc, AodvRreq *rreq) {
    static uint8_t buffer[sizeof(AodvRreq) + 1];

    buffer[0] = RREQ;
    buffer[1] = (rreq->unknown_sequence_number << 1);
    buffer[2] = rreq->id;
    buffer[3] = rreq->source_address;
    buffer[4] = rreq->source_sequence_number;
    buffer[5] = rreq->destination_address;
    buffer[6] = rreq->destination_sequence_number;
    buffer[7] = rreq->ttl;

    packetbuf_copyfrom(buffer, sizeof(buffer));
    return broadcast_send(bc);
}

int aodv_send_rreq2(struct broadcast_conn *bc, uint8_t destination_address) {
    static AodvRreq rreq;

    // Check if we have a destination route already
    if(routing_table[destination_address].in_use) {
        rreq.destination_sequence_number = routing_table[destination_address].sequence_number;
        rreq.unknown_sequence_number = !routing_table[destination_address].valid_sequence_number;
    } else {
        rreq.destination_sequence_number = 0;
        rreq.unknown_sequence_number = true;
    }

    rreq.id = ++node_rreq_number;
    rreq.source_address = linkaddr_node_addr.u8[0];
    rreq.source_sequence_number = ++routing_table[linkaddr_node_addr.u8[0]].sequence_number;
    rreq.destination_address = destination_address;
    rreq.ttl = AODV_RREQ_TTL;

    return aodv_send_rreq(bc, &rreq);
}

AodvRreq *aodv_receive_rreq(uint8_t *data) {
    static AodvRreq rreq;
    rreq.unknown_sequence_number = data[1] & 0x01;
    rreq.id = data[2];
    rreq.source_address = data[3];
    rreq.source_sequence_number = data[4];
    rreq.destination_address = data[5];
    rreq.destination_sequence_number = data[6];
    rreq.ttl = data[7];

    return &rreq;
}

void aodv_routing_table_init() {
    static uint8_t i;
    for(i = 0; i < AODV_RT_SIZE; i++) {
        routing_table[i].in_use = false;
    }

    // Add self to routing table as a static route and maintain own sequence number in this entry
    routing_table[linkaddr_node_addr.u8[0]].in_use = true;
    routing_table[linkaddr_node_addr.u8[0]].next_hop = linkaddr_node_addr.u8[0];
    routing_table[linkaddr_node_addr.u8[0]].distance = 0;
    // Local sequence number used as a kind of logical clock to determine recency of a route
    routing_table[linkaddr_node_addr.u8[0]].sequence_number = 0;
    routing_table[linkaddr_node_addr.u8[0]].valid_sequence_number = true;
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
