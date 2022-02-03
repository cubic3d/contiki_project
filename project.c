#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/uart1.h"
#include "dev/serial-line.h"
#include "aodv.h"
#include <stdio.h>
#include <stdlib.h>


// Very primitive static table, inefficient but fine for the project.
// Max nodes supported is 265, but no hashing required and operations are O(1).
static AodvRoutingEntry routing_table[256];


static struct broadcast_conn broadcast;

// Handle incomming broadcast packets.
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
    static uint8_t *data;
    data = (uint8_t *)packetbuf_dataptr();

    // Handle packet depending on its type
    switch(*data) {
        case RREQ:;
            static AodvRreq *rreq;
            rreq = aodv_receive_rreq(&data[1]);

            printf("Received RREQ from %d to %d, TTL: %d\n", rreq->source_address, rreq->destination_address, rreq->ttl);

            // Flood as long as packet is alive
            if(rreq->ttl > 0) {
                rreq->ttl--;
                aodv_send_rreq(&broadcast, rreq);
            }
            break;

        default:
            printf("Received unknown packet type %d", data[0]);
    }
}
static const struct broadcast_callbacks broadcast_cb = {broadcast_recv};


PROCESS(init, "Main process and command handler");
AUTOSTART_PROCESSES(&init);

PROCESS_THREAD(init, ev, data) {
    PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

    PROCESS_BEGIN();

    // Init routing table
    static uint8_t i;
    for(i = 0; i < sizeof(routing_table); i++) {
        routing_table[i].in_use = false;
    }

    // Hook UART1 to serial line API
    uart1_set_input(serial_line_input_byte);
    serial_line_init();

    broadcast_open(&broadcast, 129, &broadcast_cb);

    for(;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message && data != NULL);

        // Extract command by separating received line by " "
        static char *command;
        command = strtok((char*)data, " ");

        if(strcmp(command, "rreq") == 0) {
            // Send RREQ to node with specified ID
            static AodvRreq rreq;
            rreq.source_address = linkaddr_node_addr.u8[0];
            rreq.destination_address = atoi(strtok(NULL, " "));
            rreq.ttl = AODV_RREQ_TTL;

            printf("Sending RREQ to %d\n", rreq.destination_address);
            aodv_send_rreq(&broadcast, &rreq);
        }
    }

    PROCESS_END();
}
