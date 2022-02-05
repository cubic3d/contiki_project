/*
This project does not fully implement https://www.rfc-editor.org/rfc/rfc3561 but leans
on the basic concepts from the lecture. Counter/Number/Address sizes are chosen to be limited
the datatype, since it's sufficient as a demonstration.
*/

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


static struct broadcast_conn broadcast;
static struct unicast_conn unicast;

// Handle incomming broadcast packets.
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
    static uint8_t *data;
    data = (uint8_t *)packetbuf_dataptr();

    // Handle packet depending on its type
    switch(*data) {
        case RREQ:;
            static AodvRreq *rreq;
            rreq = aodv_receive_rreq(data);

            aodv_routing_table_update_prev_hop(from->u8[0], rreq);

            // Drop RREQ if already seen
            if(aodv_seen_rreq(rreq)) {
                break;
            }

            aodv_routing_table_update_source(from->u8[0], rreq);

            // Check if we are the destination and should respond
            if(rreq->destination_address == linkaddr_node_addr.u8[0]) {
                aodv_send_rrep_as_destination(&unicast, rreq);
                break;
            }

            // Check if we have a route to the destination and can respond on behalf
            if(aodv_routing_table_has_latest_route(rreq)) {
                aodv_send_rrep_as_intermediate(&unicast, rreq);
                break;
            }

            // Flood as long as packet is alive
            rreq->ttl--;
            aodv_send_rreq(&broadcast, rreq);
            break;

        default:
            printf("Received unknown packet type %d", data[0]);
    }
}
static const struct broadcast_callbacks broadcast_cb = {broadcast_recv};

// Handle incomming unicast packets.
static void
unicast_recv(struct unicast_conn *c, const linkaddr_t *from) {
    static uint8_t *data;
    data = (uint8_t *)packetbuf_dataptr();

    // Handle packet depending on its type
    switch(*data) {
        case RREP:;
            static AodvRrep *rrep;
            rrep = aodv_receive_rrep(data);
            break;

        default:
            printf("Received unknown packet type %d", data[0]);
    }
}
static const struct unicast_callbacks unicast_cb = {unicast_recv};


PROCESS(init, "Main process and command handler");
AUTOSTART_PROCESSES(&init);

PROCESS_THREAD(init, ev, data) {
    PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
    PROCESS_EXITHANDLER(unicast_close(&unicast);)

    PROCESS_BEGIN();

    aodv_routing_table_init();

    // Hook UART1 to serial line API
    uart1_set_input(serial_line_input_byte);
    serial_line_init();

    broadcast_open(&broadcast, 129, &broadcast_cb);
    unicast_open(&unicast, 146, &unicast_cb);

    for(;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message && data != NULL);

        // Extract command by separating received line by " "
        static char *command;
        command = strtok((char*)data, " ");

        if(strcmp(command, "rreq") == 0) {
            // Send RREQ to node with specified ID
            static uint8_t destination_address;
            destination_address = atoi(strtok(NULL, " "));

            aodv_send_rreq2(&broadcast, destination_address);
        } else if(strcmp(command, "pt") == 0) {
            aodv_routing_table_print();
        }
    }

    PROCESS_END();
}
