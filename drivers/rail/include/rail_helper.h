/*
 * Copyright (C) 2018 Hochschule RheinMain
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef RAIL_HELPER_H
#define RAIL_HELPER_H

#include <stdint.h>
#include <stdbool.h>

#include "rail.h"

#include "ringbuffer.h"

#ifdef __cplusplus
extern "C" {
#endif


#define RAIL_EVENT_MSG_COUNT 10

typedef struct {

    RAIL_Events_t event;
    RAIL_RxPacketHandle_t rx_packet;
    RAIL_RxPacketInfo_t rx_packet_info;
    uint16_t rx_packet_size;
    uint32_t event_count;

} rail_event_msg_t;

typedef struct {
    ringbuffer_t ring_buffer; /* ring buffer for incomming events */
    uint8_t _buffer[sizeof(rail_event_msg_t) * RAIL_EVENT_MSG_COUNT];

} rail_event_queue_t;


void rail_event_queue_init(rail_event_queue_t* queue);

int rail_event_queue_peek(rail_event_queue_t* queue, rail_event_msg_t* event_msg);

int rail_event_queue_poll(rail_event_queue_t* queue, rail_event_msg_t* event_msg);

int rail_event_queue_add(rail_event_queue_t* queue, rail_event_msg_t* event_msg);



#ifdef DEVELHELP
const char *rail_error2str(RAIL_Status_t status);
const char *rail_packetStatus2str(RAIL_RxPacketStatus_t status);

const char *rail_radioState2str(RAIL_RadioState_t state);

const char *rail_event2str(RAIL_Events_t event);
#endif /* DEVELHELP*/



#ifdef __cplusplus
}
#endif


#endif /* RAIL_HELPER_H */
