
#ifndef  RAIL_DRV_H_INC
#define  RAIL_DRV_H_INC

#include <stdint.h>
#include <stdbool.h>

#include "board.h"
#include "periph/spi.h"
#include "periph/gpio.h"
#include "net/netdev2.h"
#include "net/netdev2/ieee802154.h"
#include "net/gnrc/nettype.h"
#include "net/eui64.h" 

#ifdef __cplusplus
extern "C" {
#endif


// states of the radio transceiver

#define RAIL_TRANSCEIVER_STATE_UNINITIALISIED   0x00
#define RAIL_TRANSCEIVER_STATE_IDLE             0x01
#define RAIL_TRANSCEIVER_STATE_SLEEP            0x02
#define RAIL_TRANSCEIVER_STATE_OFF              0x03
#define RAIL_TRANSCEIVER_STATE_RX               0x04
#define RAIL_TRANSCEIVER_STATE_TX               0x05
 
#define RAIL_DEFAULT_PANID         (IEEE802154_DEFAULT_PANID)
#define RAIL_DEFAULT_TXPOWER       (IEEE802154_DEFAULT_TXPOWER)




typedef struct rail_params {

    int tmp;
    uint64_t XTAL_frequency;
    uint64_t BASE_frequency;
    int32_t  max_transit_power;
} rail_params_t;

typedef struct {
    netdev2_ieee802154_t netdev;
    rail_params_t params;

    uint8_t state;              // state of radio
    bool promiscuousMode;
    eui64_t eui;
} rail_t;


void rail_setup(rail_t* dev, const rail_params_t* params);

int rail_init(rail_t* dev);

int rail_tx_prepare(rail_t* dev);

int rail_start_rx(rail_t* dev);

#ifdef __cplusplus
}
#endif

#endif   /* ----- #ifndef RAIL_DRV_H_INC  ----- */
