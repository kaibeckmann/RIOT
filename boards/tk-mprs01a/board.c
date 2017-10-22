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
 * @brief       Board specific implementations for Thermokon
 * board  mprs01 model a
 *
 * @author      Kai Beckmann <kai.beckmann@hs-rm.de> 
 *
 * @}
 */

#include "board.h"
#include "cpu.h"


#include "periph/gpio.h"
#include "periph/i2c.h"
#include "periph/adc.h"

#include "panic.h"

#include "bsp_trace.h"

void board_init(void)
{
    /* cpu init */
    cpu_init();


    /*  enable SWO output pin for Energy Profiler TODO ifdef stuff */
    BSP_TraceProfilerSetup();
    /* init led */
    gpio_init(LED0_PIN, GPIO_OUT);

    /* init the button */
    gpio_init(PB0_PIN, GPIO_IN);

    /* init reed contact gpio */
    gpio_init(REED_CONTACT_PIN, GPIO_IN);

    /* init periph onoff / vcc supply  gpio */ 
    gpio_init(PERIPH_VCC_PIN,GPIO_OUT);

    /* init battery measure onof gpio */
    gpio_init(BATT_MEASS_ONOFF_PIN, GPIO_OUT);

    /* init battery measure adc */
    int ret = 0;

    ret = adc_init(BATT_MEASS_LINE);
  //  assert(ret > 0);

    /* init intrnal VDD measure adc */
    ret = adc_init(VDD_MEASS_LINE);
  //  assert(ret > 0);

    /* init internal temp sensor adc */
    ret = adc_init(INTERNAL_TEMP_SENSOR_MEASS_LINE);
  //  assert(ret > 0);
    ret = ret;


    /* init i2c? */

    /* init barometer sensor */

    /* init humility sensor */

    /* init pm? */

    /* init energy harvesting module? */

}
