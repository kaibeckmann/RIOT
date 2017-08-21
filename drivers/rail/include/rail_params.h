
#ifndef  RAIL_PARAMS_H_INC
#define  RAIL_PARAMS_H_INC

#include "board.h"
#include "rail_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RAIL_PARAMS_DEFAULT {.tmp = 23, \
                             .XTAL_frequency = 38400000UL, \
                             .BASE_frequency = 2450000000UL, \
                             .max_transit_power = 0 } 

static const rail_params_t rail_params[] = {
#ifdef RAIL_PARAMS_BOARD
    RAIL_PARAMS_BOARD,
#else
    RAIL_PARAMS_DEFAULT,
#endif
};

#ifdef __cplusplus
}
#endif


#endif   /* ----- #ifndef RAIL_PARAMS_H_INC  ----- */
