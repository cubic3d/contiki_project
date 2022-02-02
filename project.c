#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>

PROCESS(example_broadcast_process, "Broadcast example");
AUTOSTART_PROCESSES(&example_broadcast_process);

static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
    printf("broadcast message received from %d.%d: '%s'\n", 
           from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}

static const struct broadcast_callbacks broadcast_cb = {broadcast_recv};

static struct broadcast_conn broadcast;

PROCESS_THREAD(example_broadcast_process, ev, data) {
    static struct etimer et;
    
    PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
    PROCESS_BEGIN();
    
    broadcast_open(&broadcast, 129, &broadcast_cb);
    
    while(1) {
        unsigned long ticks = CLOCK_SECOND * 2 + random_rand() % (CLOCK_SECOND * 2);
        
        etimer_set(&et, ticks);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        
        packetbuf_copyfrom("Hello", 6);
        
        broadcast_send(&broadcast);
        
        printf("broadcast packet sent\n");
    }
    
    PROCESS_END();
}
