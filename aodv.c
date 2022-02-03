#include "aodv.h"
#include "net/rime/rime.h"

int aodv_send_rreq(struct broadcast_conn *bc, AodvRreq *rreq) {
    static uint8_t buffer[sizeof(AodvRreq) + 1];

    buffer[0] = RREQ;
    buffer[1] = rreq->source_address;
    buffer[2] = rreq->destination_address;
    buffer[3] = rreq->ttl;

    packetbuf_copyfrom(buffer, sizeof(buffer));
    return broadcast_send(bc);
}

AodvRreq *aodv_receive_rreq(uint8_t *data) {
    static AodvRreq rreq;
    rreq.source_address = data[0];
    rreq.destination_address = data[1];
    rreq.ttl = data[2];

    return &rreq;
}
