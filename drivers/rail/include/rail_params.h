
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




// c&p from gecko-sdk BSP for EFR32MG1_BRD4154A
#if !defined(RADIO_PA_2P4_INIT)
#define RADIO_PA_2P4_INIT                                    \
  {                                                          \
    PA_SEL_2P4_HP,    /* Power Amplifier mode */             \
    PA_VOLTMODE_DCDC, /* Power Amplifier vPA Voltage mode */ \
    100,              /* Desired output power in dBm * 10 */ \
    0,                /* Output power offset in dBm * 10 */  \
    10,               /* Desired ramp time in us */          \
  }
#endif

#if !defined(RADIO_PA_SUBGIG_INIT)
#define RADIO_PA_SUBGIG_INIT                                    \
    {                                                           \
        PA_SEL_SUBGIG,    /*  Power Amplifier mode */           \
        PA_VOLTMODE_DCDC, /* Power Amplifier vPA Voltage mode */\
        100,              /* Desired output power in dBm * 10 */\
        0,                /* Output power offset in dBm * 10 */ \
        10,               /* Desired ramp time in us */         \
    }
#endif





#ifdef __cplusplus
}
#endif


#endif   /* ----- #ifndef RAIL_PARAMS_H_INC  ----- */
