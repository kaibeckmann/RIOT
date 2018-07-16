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


/**
 * @name    Channel configuration
 * @{
 */
#define RAIL_868MHZ_MIN_CHANNEL           (IEEE802154_CHANNEL_MIN_SUBGHZ)
#define RAIL_868MHZ_MAX_CHANNEL           (IEEE802154_CHANNEL_MIN_SUBGHZ) /* 868MHz has only ONE channel! */
#define RAIL_868MHZ_DEFAULT_CHANNEL       (IEEE802154_CHANNEL_MIN_SUBGHZ)
#define RAIL_912MHZ_MIN_CHANNEL           (IEEE802154_CHANNEL_MIN_SUBGHZ)
#define RAIL_912MHZ_MAX_CHANNEL           (IEEE802154_CHANNEL_MAX_SUBGHZ)
#define RAIL_912MHZ_DEFAULT_CHANNEL       (IEEE802154_DEFAULT_SUBGHZ_CHANNEL)
/* Page 2 is O-QPSK 100 kbit/s (channel 0), or 250 kbit/s (channels 1-10) */
#define RAIL_SUBGHZ_DEFAULT_PAGE          (IEEE802154_DEFAULT_SUBGHZ_PAGE)
#define RAIL_2P4GH_MIN_CHANNEL           (IEEE802154_CHANNEL_MIN)
#define RAIL_2P4GH_MAX_CHANNEL           (IEEE802154_CHANNEL_MAX)
#define RAIL_2P4GH_DEFAULT_CHANNEL       (IEEE802154_DEFAULT_CHANNEL)
/* Only page 0 is supported in the 2.4 GHz band */

/** @} */

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
#define RAIL_DEFAULT_CSMA_TRIES     5

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
    /* config for CSMA */
    RAIL_CsmaConfig_t csma_config;

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
