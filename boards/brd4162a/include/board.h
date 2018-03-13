/*
 * Copyright (C) 2015-2017 Freie Universität Berlin
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
 * @brief       Board specific definitions for the BRD4162A starter kit
 *
 *
 * @author      Kai Beckmann <kai.beckmann@hs-rm.de>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Bas Stottelaar <basstottelaar@gmail.com>
 */

#ifndef BOARD_H
#define BOARD_H

#include "cpu.h"

#include "periph_conf.h"
#include "periph/gpio.h"
#include "periph/spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    Xtimer configuration
 *
 * The timer runs at 250 KHz to increase accuracy.
 * @{
 */
#define XTIMER_HZ           (250000UL)
#define XTIMER_WIDTH        (16)
/** @} */

/**
 * @name    Push button pin definitions
 * @{
 */
#define PB0_PIN             GPIO_PIN(PF, 6)
#define PB1_PIN             GPIO_PIN(PF, 7)
/** @} */

/**
 * @name    LED pin definitions
 * @{
 */
#define LED0_PIN            GPIO_PIN(PF, 4)
#define LED1_PIN            GPIO_PIN(PF, 5)
/** @} */

/**
 * @name    Macros for controlling the on-board LEDs
 * @{
 */
#define LED0_ON             gpio_set(LED0_PIN)
#define LED0_OFF            gpio_clear(LED0_PIN)
#define LED0_TOGGLE         gpio_toggle(LED0_PIN)
#define LED1_ON             gpio_set(LED1_PIN)
#define LED1_OFF            gpio_clear(LED1_PIN)
#define LED1_TOGGLE         gpio_toggle(LED1_PIN)
/** @} */


/**
 * @name    Board controller configuration
 *
 * Define the GPIO pin to enable the BC, to allow serial communication
 * via the USB port.
 * @{
 */
#define BC_PIN              GPIO_PIN(PA, 5)
/** @} */

/**
 * @name    Temperature sensor configuration
 *
 * Connection to the on-board temperature/humidity sensor (Si7021).
 * @{
 */
#ifndef SI7021_ENABLED
#define SI7021_ENABLED          (1)
#endif
#define SI7021_I2C              I2C_DEV(0)

#define SI7021_ENABLE_PIN       GPIO_PIN(PB,10)

#define SI70XX_PARAM_I2C_DEV    SI7021_I2C
/** @} */





/**
 * @brief   Initialize the board (GPIO, sensors, clocks).
 */
void board_init(void);

#ifdef __cplusplus
}
#endif

#endif /* BOARD_H */
/** @} */
