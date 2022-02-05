#ifndef DATA_H
#define DATA_H

#include <stdint.h>
#include "runicast.h"

#define DATA_MAX_RETRANSMISSIONS 3

typedef enum {
    PING,
} DataType;

typedef struct {
    uint8_t source_address;
    uint8_t destination_address;
} DataPing;

int data_send_ping(struct runicast_conn *rc, DataPing *ping);
int data_send_ping2(struct runicast_conn *rc, uint8_t destination_address);
DataPing *data_receive_ping(uint8_t *data);
void data_print_ping(const char* action, DataPing *ping);

#endif