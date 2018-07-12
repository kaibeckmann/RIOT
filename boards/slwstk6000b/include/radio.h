/*
 * Copyright (C) 2017-2018 Hochschule RheinMain
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    boards_slwstk6000b Silicon Labs wireless starter kit
 * @ingroup     boards
 * @brief       Support for the Silicon Labs slwstk6000b starter kit
 * @{
 *
 * @file
 * @brief       Board specific definitions for the radio transceicer of the
 *              slwstk6000b starter kit
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

// posible values, 2400, 868, 915
#define RAIL_RADIO_BAND 2400

// voltage of the radio PA in mV
// depends on the module TODO
#define RAIL_RADIO_PA_VOLTAGE 1800

// Packet Trace interface
#ifndef RAIL_PTI_ENABLED
#define RAIL_PTI_ENABLED     (1)
#endif

// c&p from gecko-sdk BSP for EFR32MG1_BRD4154A
#if !defined(RAIL_PTI_CONFIG)
#define RAIL_PTI_CONFIG                                                    \
  {                                                                        \
    RAIL_PTI_MODE_UART,     /* Only supported output mode for the WSTK*/   \
    1600000,                /* Choose 1.6 MHz for best the WSTK */         \
    6,                      /* WSTK uses location 6 for DOUT PB12 */       \
    gpioPortB,              /* FRC_DOUT#6 is PB12 */                       \
    12,                     /* FRC_DOUT#6 is PB12 */                       \
    6,                      /* UNUSED IN UART MODE */                      \
    gpioPortB,              /* UNUSED IN UART MODE */                      \
    11,                     /* UNUSED IN UART MODE */                      \
    6,                      /* WSTK uses location 6 for DFRAME */          \
    gpioPortB,              /* RC_DOUT#6 is PB13 */                        \
    13,                     /* RC_DOUT#6 is PB13 */                        \
  }
#endif


#ifdef __cplusplus
}
#endif

#endif /* RADIO_H */
/** @} */
