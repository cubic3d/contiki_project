#include "data.h"
#include <stdio.h>
#include "aodv.h"

int data_send_ping(struct runicast_conn *rc, DataPing *ping) {
    static uint8_t next_hop = 0;
    next_hop = aodv_routing_table_lookup(ping->destination_address);

    if(next_hop == 0) {
        printf("Error: PING could not be sent - no route to node\n");
        return 0;
    }

    static uint8_t buffer[sizeof(DataPing) + 1];

    buffer[0] = PING;
    buffer[1] = ping->source_address;
    buffer[2] = ping->destination_address;

    static linkaddr_t addr;
    addr.u8[0] = next_hop;
    addr.u8[1] = 0;

    packetbuf_copyfrom(buffer, sizeof(buffer));
    data_print_ping("Send", ping);
    return runicast_send(rc, &addr, DATA_MAX_RETRANSMISSIONS);
}

int data_send_ping2(struct runicast_conn *rc, uint8_t destination_address) {
    static DataPing ping;

    ping.source_address = linkaddr_node_addr.u8[0];
    ping.destination_address = destination_address;

    return data_send_ping(rc, &ping);
}

DataPing *data_receive_ping(uint8_t *data) {
    static DataPing ping;

    ping.source_address = data[1];
    ping.destination_address = data[2];

    data_print_ping("Recv", &ping);
    return &ping;
}

void data_print_ping(const char* action, DataPing *ping) {
    printf("%s DATA-PING: Source: %d | Destination: %d\n",
        action,
        ping->source_address,
        ping->destination_address);
}
