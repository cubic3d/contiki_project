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

// Handle incomming broadcast packets.
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
    static uint8_t *data;
    data = (uint8_t *)packetbuf_dataptr();

    // Handle packet depending on its type
    switch(*data) {
        case RREQ:;
            static AodvRreq *rreq;
            rreq = aodv_receive_rreq(data);

            printf("Received RREQ from %d to %d, TTL: %d\n", rreq->source_address, rreq->destination_address, rreq->ttl);

            aodv_routing_table_update(from->u8[0], rreq);

            // Flood as long as packet is alive
            rreq->ttl--;
            aodv_send_rreq(&broadcast, rreq);
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

    aodv_routing_table_init();

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
            static uint8_t destination_address;
            destination_address = atoi(strtok(NULL, " "));

            printf("Sending RREQ for %d\n", destination_address);
            aodv_send_rreq2(&broadcast, destination_address);
        } else if(strcmp(command, "pt") == 0) {
            aodv_routing_table_print();
        }
    }

    PROCESS_END();
}
