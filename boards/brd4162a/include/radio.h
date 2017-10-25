/*
 * Copyright (C) 2017 Hochschule RheinMain
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    boards_brd4162a Silicon Labs BRD4162A starter kit
 * @ingroup     boards
 * @brief       Support for the Silicon Labs BRD4162A starter kit
 * @{
 *
 * @file
 * @brief       Board specific definitions for the radio transceicer of the 
 * BRD4162A starter kit
 *
 * @author      Kai Beckmann <kai.beckmann@hs-rm.de>
 **/

#ifndef RADIO_H
#define RADIO_H

#include "cpu.h"

#include "periph_conf.h"
#include "periph/gpio.h"
#include "periph/spi.h"

#ifdef __cplusplus
extern "C" {
#endif
// Packet Trace interface
#ifndef PTI_ENABLED
#define PTI_ENABLED     (1)
#endif



// c&p from gecko-sdk BSP for EFR32MG1_BRD4154A
#if !defined(RADIO_PTI_INIT)
#define RADIO_PTI_INIT                                                     \
  {                                                                        \
    RADIO_PTI_MODE_UART,    /* Simplest output mode is UART mode */        \
    1600000,                /* Choose 1.6 MHz for best compatibility */    \
    6,                      /* WSTK uses location 6 for DOUT */            \
    gpioPortB,              /* Get the port for this loc */                \
    12,                     /* Get the pin, location should match above */ \
    0,                      /* DCLK not used for UART mode */              \
    0,                      /* DCLK not used for UART mode */              \
    0,                      /* DCLK not used for UART mode */              \
    6,                      /* WSTK uses location 6 for DFRAME */          \
    gpioPortB,              /* Get the port for this loc */                \
    13,                     /* Get the pin, location should match above */ \
  }
#endif

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



#ifdef __cplusplus
}
#endif

#endif /* RADIO_H */
/** @} */
