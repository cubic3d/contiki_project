#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/uart1.h"
#include "dev/serial-line.h"
#include <stdio.h>

PROCESS(command_handler, "Handle button");
AUTOSTART_PROCESSES(&command_handler);

PROCESS_THREAD(command_handler, ev, data) {
    PROCESS_BEGIN();

    uart1_set_input(serial_line_input_byte);
    serial_line_init();

    for(;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message && data != NULL);

        printf("received command: '%s'\n", (const char*)data);
    }

    PROCESS_END();
}
