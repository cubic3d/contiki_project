#include "contiki.h"
#include "net/rime/rime.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include <stdio.h>

PROCESS(button_handler, "Handle button");
AUTOSTART_PROCESSES(&button_handler);

PROCESS_THREAD(button_handler, ev, data) {
    PROCESS_BEGIN();

    SENSORS_ACTIVATE(button_sensor);

    for(;;) {
        PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);

        printf("Button pressed!\n");
    }

    PROCESS_END();
}
