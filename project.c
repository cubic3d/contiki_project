#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/uart1.h"
#include "dev/serial-line.h"
#include <stdio.h>

PROCESS(init, "Main process and command handler");
AUTOSTART_PROCESSES(&init);


static struct broadcast_conn broadcast;

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
    printf("broadcast message received from %d.%d: '%s'\n", 
           from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}
static const struct broadcast_callbacks broadcast_cb = {broadcast_recv};


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
        char *command = strtok((char*)data, " ");

        if(strcmp(command, "rreq") == 0) {
            // Send RREQ to node with specified ID
            char *id = strtok(NULL, " ");
            printf("Sending RREQ to %s\n", id);
            packetbuf_copyfrom("Hello", 6);
            broadcast_send(&broadcast);
        }
    }

    PROCESS_END();
}
