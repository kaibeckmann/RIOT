/*
 * Copyright (C) 2017 Hochschule RheinMain
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_tk-mprs01a
 * @{
 *
 * @file
 * @brief       Config for board specific gpio
 *
 * @author      Kai Beckmann <kai.beckmann@hs-rm.de>
 */

#ifndef GPIO_PARAMS_H
#define GPIO_PARAMS_H

#include "board.h"
#include "saul/periph.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief    GPIO pin configuration
 */
static const  saul_gpio_params_t saul_gpio_params[] =
{
    {
        .name = "LED",
        .pin = LED0_PIN,
        .mode = GPIO_OUT
    },
    {
        .name = "Button",
        .pin = PB0_PIN,
        .mode = GPIO_IN_PU
    },
};

#ifdef __cplusplus
}
#endif

#endif /* GPIO_PARAMS_H */
/** @} */
