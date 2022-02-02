/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */
/**
 * \file
 *         Best-effort single-hop unicast example
 * \author
 *         Adam Dunkels <adam@sics.se>
 */
#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>

/*---------------------------------------------------------------------------*/
PROCESS(example_unicast_process, "Unicast example");
AUTOSTART_PROCESSES(&example_unicast_process);

/*---------------------------------------------------------------------------*/
/* 
 * Define callbacks function
 * Called when a packet has been received by the broadcast module
 */
static void
unicast_recv(struct unicast_conn *c, const linkaddr_t *from) {
    printf("unicast message received from %d.%d: '%s'\n",
           from->u8[0], from->u8[1], (char*)packetbuf_dataptr());
}

static const struct unicast_callbacks unicast_cb = {unicast_recv};

/*---------------------------------------------------------------------------*/
/* Unicast connection */
static struct unicast_conn uc;

PROCESS_THREAD(example_unicast_process, ev, data) {
    static struct etimer et;
    
    PROCESS_EXITHANDLER(unicast_close(&uc);)
    PROCESS_BEGIN();
    
    /* 
     * Set up a unicast connection
     * Arguments: channel (146) and callbacks function
     */
    unicast_open(&uc, 146, &unicast_cb);
    
    /*
     * Send unicast packet to node "1.0" every 2 seconds
     * e.g. "2.0" --> "1.0", "3.0" --> "1.0"
     */
    while(1) {
        /* Delay 2 seconds */
        etimer_set(&et, CLOCK_SECOND * 2);
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
        
        /* Copy data to the packet buffer */
        packetbuf_copyfrom("Hello", 6);
        
        /* Set RIME address of destination node (1.0) */
        linkaddr_t addr;
        addr.u8[0] = 1;
        addr.u8[1] = 0;
        
        /* linkaddr_node_addr: RIME address of the current node */
        /* Sender's address must be different from destination address */
        if (!linkaddr_cmp(&addr, &linkaddr_node_addr)) {
            unicast_send(&uc, &addr);
        }
    }
    
    PROCESS_END();
}
/*---------------------------------------------------------------------------*/
