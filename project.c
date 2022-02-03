#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/uart1.h"
#include "dev/serial-line.h"
#include <stdio.h>
#include <stdlib.h>

#define AODV_RREQ_TTL 10

typedef enum {
    RREQ,
} AodvType;


static struct broadcast_conn broadcast;

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
    static uint8_t *data;
    data = (uint8_t *)packetbuf_dataptr();

    // Handle packet depending on its type
    switch(*data) {
        case RREQ:
            printf("TTL: %d\n", data[3]);
            break;
        default:
            printf("Received unknown packet type %d", data[0]);
    }
}
static const struct broadcast_callbacks broadcast_cb = {broadcast_recv};


static int send_rreq(uint8_t source_address, uint8_t destination_address, uint8_t ttl) {
    static uint8_t buffer[4];

    buffer[0] = RREQ;
    buffer[1] = source_address;
    buffer[2] = destination_address;
    buffer[3] = ttl;

    packetbuf_copyfrom(buffer, sizeof(buffer));
    return broadcast_send(&broadcast);
}


PROCESS(init, "Main process and command handler");
AUTOSTART_PROCESSES(&init);

PROCESS_THREAD(init, ev, data) {
    PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

    PROCESS_BEGIN();

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
            static char *id;
            id = strtok(NULL, " ");
            printf("Sending RREQ to %s\n", id);
            send_rreq(linkaddr_node_addr.u8[0], atoi(id), AODV_RREQ_TTL);
        }
    }

    PROCESS_END();
}
