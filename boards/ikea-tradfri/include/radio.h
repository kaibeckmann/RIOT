/*
 * Copyright (C) 2018 Hochschule RheinMain
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_ikea-tradfri
 * @{
 *
 * @file
 * @brief       Board specific definitions for the radio transceiver of the
 *              Ikea Tradfi module
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

/* only 2.4 GHz possible */
#define RAIL_RADIO_BAND 2400

/* voltage of the radio PA in mV
   depends on the module, tradfi modules from lamps are connected to external
   dc-dc
*/
#define RAIL_RADIO_PA_VOLTAGE 3300


#ifdef __cplusplus
}
#endif

#endif /* RADIO_H */
/** @} */
