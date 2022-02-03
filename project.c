#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/uart1.h"
#include "dev/serial-line.h"
#include <stdio.h>

PROCESS(command_receiver, "Receive commands over serial");
AUTOSTART_PROCESSES(&command_receiver);

PROCESS_THREAD(command_receiver, ev, data) {
    PROCESS_BEGIN();

    uart1_set_input(serial_line_input_byte);
    serial_line_init();

    for(;;) {
        PROCESS_WAIT_EVENT();

        if(ev == serial_line_event_message && data != NULL) {
            printf("received command: '%s'\n", (const char*)data);
        }
    }

    PROCESS_END();
}
