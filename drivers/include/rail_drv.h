/*
 * Copyright (C) 2018 Hochschule RheinMain
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef RAIL_DRV_H
#define RAIL_DRV_H

#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "periph/spi.h"
#include "periph/gpio.h"
#include "net/netdev.h"
#include "net/netdev/ieee802154.h"
#include "net/gnrc/nettype.h"
#include "net/eui64.h"

#include "rail.h"

#ifdef __cplusplus
extern "C" {
#endif


/* states of the radio transceiver */
/* TODO 
    - use enum?
    - use !
*/
#define RAIL_TRANSCEIVER_STATE_UNINITIALIZED    0x00
#define RAIL_TRANSCEIVER_STATE_IDLE             0x01
#define RAIL_TRANSCEIVER_STATE_SLEEP            0x02
#define RAIL_TRANSCEIVER_STATE_OFF              0x03
#define RAIL_TRANSCEIVER_STATE_RX               0x04
#define RAIL_TRANSCEIVER_STATE_TX               0x05


#define RAIL_DEFAULT_PANID         (IEEE802154_DEFAULT_PANID)
#define RAIL_DEFAULT_TXPOWER       (IEEE802154_DEFAULT_TXPOWER)

enum rail_transceiver_config_frequency {
    RAIL_TRANSCEIVER_FREQUENCY_2P4GHZ,
    RAIL_TRANSCEIVER_FREQUENCY_868MHZ,
    RAIL_TRANSCEIVER_FREQUENCY_912MHZ
};


typedef struct rail_params {
    enum rail_transceiver_config_frequency freq;

} rail_params_t;

typedef struct {
    netdev_ieee802154_t netdev;
    rail_params_t params;

    /* handle of the RAIL driver blob instance */
    RAIL_Handle_t rhandle;
    /* config of the RAIL driver blob */
    RAIL_Config_t rconfig;

    uint8_t state;              /* state of radio transceiver */

    RAIL_RxPacketHandle_t lastRxPacketHandle;

    bool promiscuousMode;
    eui64_t eui;
} rail_t;


void rail_setup(rail_t *dev, const rail_params_t *params);

int rail_init(rail_t *dev);


int rail_start_rx(rail_t *dev);

int rail_transmit_frame(rail_t *dev, uint8_t *data_ptr, size_t data_length);

#ifdef DEVELHELP
const char *rail_error2str(RAIL_Status_t status);
const char *rail_packetStatus2str(RAIL_RxPacketStatus_t status);

const char *rail_radioState2str(RAIL_RadioState_t state);
#endif /* DEVELHELP*/

#ifdef __cplusplus
}
#endif

#endif /* RAIL_DRV_H */
