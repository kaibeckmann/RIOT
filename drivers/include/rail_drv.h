
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

#ifdef __cplusplus
extern "C" {
#endif


// states of the radio transceiver

#define RAIL_STATE_IDLE     0x01


typedef struct rail_params {

    int tmp;
} rail_params_t;

typedef struct {
    netdev2_ieee802154_t netdev;
    rail_params_t params;

    uint8_t state;              // state of radio
} rail_t;


void rail_setup(rail_t* dev, const rail_params_t* params);

#ifdef __cplusplus
}
#endif

#endif   /* ----- #ifndef RAIL_DRV_H_INC  ----- */
