#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/uart1.h"
#include "dev/serial-line.h"
#include <stdio.h>

PROCESS(button_handler, "Handle button");
AUTOSTART_PROCESSES(&button_handler);

PROCESS_THREAD(button_handler, ev, data) {
    PROCESS_BEGIN();

    uart1_set_input(serial_line_input_byte);
    serial_line_init();

    for(;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

        printf("Button pressed!\n");
    }

    PROCESS_END();
}
