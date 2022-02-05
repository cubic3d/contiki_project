#include "aodv.h"
#include "net/rime/rime.h"
#include <stdio.h>

// Very primitive static table, inefficient but fine for the project.
// Max nodes supported is 50, but no hashing required and operations are O(1).
// Size of 256 will already cause boot loops on motes.
static AodvRoutingEntry routing_table[AODV_RT_SIZE];

// ID incremented before every RREQ a nodes makes to allow duplicate detection.
static uint8_t node_rreq_number = 0;

// The tuple (node_last_sent_rreq_id, own_address) identifies self sent RREQ which can be dropped
// as long as they occur in AODV_PATH_DISCOVERY_TIME.
// This is simplified to a single entry, where a dynamic list would be a better for of multiple expiring entires.
// Clearing this entry after AODV_PATH_DISCOVERY_TIME is not implemented due to simplicity.
static uint8_t node_last_seen_rreq_id = 0;
static uint8_t node_last_seen_source_address = 0;

int aodv_send_rreq(struct broadcast_conn *bc, AodvRreq *rreq) {
    // Prevent sending requests with expired TTL
    if(rreq->ttl <= 0) {
        return 0;
    }

    // Mark RREQ ID as sent
    node_last_seen_rreq_id = rreq->id;
    node_last_seen_source_address = rreq->source_address;

    static uint8_t buffer[sizeof(AodvRreq) + 1];

    buffer[0] = RREQ;
    buffer[1] = (rreq->unknown_sequence_number << 0);
    buffer[2] = rreq->id;
    buffer[3] = rreq->source_address;
    buffer[4] = rreq->source_sequence_number;
    buffer[5] = rreq->destination_address;
    buffer[6] = rreq->destination_sequence_number;
    buffer[7] = rreq->ttl;

    packetbuf_copyfrom(buffer, sizeof(buffer));
    aodv_print_rreq("Send", rreq);
    return broadcast_send(bc);
}

int aodv_send_rreq2(struct broadcast_conn *bc, uint8_t destination_address) {
    static AodvRreq rreq;

    // Check if we have a destination route already
    if(routing_table[destination_address].in_use) {
        rreq.destination_sequence_number = routing_table[destination_address].sequence_number;
        rreq.unknown_sequence_number = !routing_table[destination_address].known_sequence_number;
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

bool aodv_seen_rreq(AodvRreq *rreq) {
    return node_last_seen_rreq_id == rreq->id && node_last_seen_source_address == rreq->source_address;
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

    aodv_print_rreq("Recv", &rreq);
    return &rreq;
}

void aodv_print_rreq(const char* action, AodvRreq *rreq) {
    printf("%s RREQ: ID: %d | Source: %d/%d | Destination: %d/%d/%s | TTL: %d\n",
        action,
        rreq->id,
        rreq->source_address,
        rreq->source_sequence_number,
        rreq->destination_address,
        rreq->destination_sequence_number,
        rreq->unknown_sequence_number ? "unknown" : "known  ",
        rreq->ttl);
}

int aodv_send_rrep(struct unicast_conn *uc, AodvRrep *rrep) {
    // Check if we have a route (we should!) and pull address for the hop
    static uint8_t next_hop = 0;
    if(routing_table[rrep->source_address].in_use) {
        next_hop = routing_table[rrep->source_address].next_hop;
    } else {
        printf("Error: RREP could not be sent - no route to node\n");
    }

    static uint8_t buffer[sizeof(AodvRrep) + 1];

    buffer[0] = RREP;
    buffer[1] = rrep->hop_count;
    buffer[2] = rrep->source_address;
    buffer[3] = rrep->destination_address;
    buffer[4] = rrep->destination_sequence_number;

    // Create a link address to use in unicast sender
    static linkaddr_t addr;
    addr.u8[0] = next_hop;
    addr.u8[1] = 0;

    packetbuf_copyfrom(buffer, sizeof(buffer));
    aodv_print_rrep("Send", rrep);
    return unicast_send(uc, &addr);
}

int aodv_send_rrep_as_destination(struct unicast_conn *uc, AodvRreq *rreq) {
    // If the generating node is the destination itself, it MUST increment
    // its own sequence number by one if the sequence number in the RREQ
    // packet is equal to that incremented value.
    // https://www.rfc-editor.org/rfc/rfc3561#section-6.6.1
    // We will use the maximum method described in https://www.rfc-editor.org/rfc/rfc3561#section-6.1
    // since the RFC contradicts itself in quite some places...
    // Also doesn't mention to check the unknown flag...
    if(!rreq->unknown_sequence_number
            && rreq->destination_address > routing_table[linkaddr_node_addr.u8[0]].sequence_number) {
        routing_table[linkaddr_node_addr.u8[0]].sequence_number = rreq->destination_address;
    }

    static AodvRrep rrep;

    rrep.hop_count = 0;
    rrep.source_address = rreq->source_address;
    rrep.destination_address = rreq->destination_address;
    rrep.destination_sequence_number = routing_table[linkaddr_node_addr.u8[0]].sequence_number;

    return aodv_send_rrep(uc, &rrep);
}

int aodv_send_rrep_as_intermediate(struct unicast_conn *uc, AodvRreq *rreq) {
    static AodvRrep rrep;

    rrep.hop_count = routing_table[rreq->destination_address].distance;
    rrep.source_address = rreq->source_address;
    rrep.destination_address = rreq->destination_address;
    rrep.destination_sequence_number = routing_table[rreq->destination_address].sequence_number;

    return aodv_send_rrep(uc, &rrep);
}

AodvRrep *aodv_receive_rrep(uint8_t *data) {
    static AodvRrep rrep;
    
    rrep.hop_count = data[1];
    rrep.source_address = data[2];
    rrep.destination_address = data[3];
    rrep.destination_sequence_number = data[4];

    aodv_print_rrep("Recv", &rrep);
    return &rrep;
}

void aodv_print_rrep(const char* action, AodvRrep *rrep) {
    printf("%s RREP: Hop Count: %d | Source: %d | Destination: %d/%d\n",
        action,
        rrep->hop_count,
        rrep->source_address,
        rrep->destination_address,
        rrep->destination_sequence_number);
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
    routing_table[linkaddr_node_addr.u8[0]].known_sequence_number = true;
}

void aodv_routing_table_print() {
    printf("----------------------------\n");
    printf("%-15s%-15s%-15s%-15s%-15s\n", "Destination", "Next Hop", "Distance", "SN", "SN Known");
    static uint8_t i;
    for(i = 0; i < AODV_RT_SIZE; i++) {
        if(routing_table[i].in_use) {
            printf("%-15d%-15d%-15d%-15d%-15s\n",
            i,
            routing_table[i].next_hop,
            routing_table[i].distance,
            routing_table[i].sequence_number,
            routing_table[i].known_sequence_number ? "yes": "no");
        }
    }
    printf("----------------------------\n");
}

void aodv_routing_table_update_if_required(
        uint8_t to,
        uint8_t via,
        uint8_t distance,
        uint8_t sequence_number,
        bool known_sequence_number) {
    static bool update = false;

    // Determine if we should update the route, trying to be explicit
    if(routing_table[to].in_use) {
        if(routing_table[to].known_sequence_number) {
            // Compare sequence numbers
            // This casts the result into signed integer as per RFC to accomplish smooth rollover
            static int8_t sequence_diff;
            sequence_diff = sequence_number - routing_table[to].sequence_number + 1;

            if(sequence_diff > 0) {
                update = true;
                printf("Updated route to %d via %d, reason: newer sequence number\n", to, via);
            } else if(sequence_diff == 0) {
                // Equal sequence, compare distance and update if new is lower
                if(distance < routing_table[to].distance) {
                    update = true;
                    printf("Updated route to %d via %d, reason: shorter distance\n", to, via);
                } else {
                    update = false;
                }
            } else {
                update = false;
            }
        } else {
            update = true;
            printf("Updated route to %d via %d, reason: unknown sequence number\n", to, via);
        }
    } else {
        update = true;
        printf("Added route to %d via %d, reason: not existent\n", to, via);
    }

    // Update routing table
    if(update) {
        routing_table[to].in_use = true;
        routing_table[to].distance = distance;
        routing_table[to].next_hop = via;
        routing_table[to].sequence_number = sequence_number;
        routing_table[to].known_sequence_number = known_sequence_number;
    }
}

bool aodv_routing_table_has_latest_route(AodvRreq *rreq) {
    // Route must be in use, have a known sequence number and be equal or greater than requested
    // Use signed comparison for smooth rollover according to RFC
    return routing_table[rreq->destination_address].in_use
            && routing_table[rreq->destination_address].known_sequence_number
            && (int8_t)(routing_table[rreq->destination_address].sequence_number - rreq->destination_sequence_number) >= 0;
}
