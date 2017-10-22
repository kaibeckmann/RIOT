/*
 * Copyright (C) 2015-2016 Freie Universität Berlin
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
 * @brief       Configuration of CPU peripherals for Thermokon mprs01 model a
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Bas Stottelaar <basstottelaar@gmail.com>
 * @author      Kai Beckmann <kai.beckmann@hs-rm.de>
 */

#ifndef PERIPH_CONF_H
#define PERIPH_CONF_H

#include "cpu.h"

#include "periph_cpu.h"

#include "em_cmu.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO move someplace more generic
#define SILABS_CONCATOR(str,clock)  str ## _ ## clock

#define SILABS_CLOCK_OSCILLATOR(clock) SILABS_CONCATOR(cmuOsc, clock)
#define SILABS_CLOCK_SELECT(clock) SILABS_CONCATOR(cmuSelect, clock)



/**
 * @brief   Clock configuration
 * @{
 */
#ifndef CLOCK_HF
#define CLOCK_HF            HFXO
#endif
#ifndef CLOCK_CORE_DIV
#define CLOCK_CORE_DIV      cmuClkDiv_1
#endif
    // TODO or LFRCO?
#ifndef CLOCK_LFA
#define CLOCK_LFA           LFXO
#endif
#ifndef CLOCK_LFB
#define CLOCK_LFB           LFXO
#endif
#ifndef CLOCK_LFE
#define CLOCK_LFE           LFXO
#endif
/** @} */

#define CLOCK_HFXO_INIT                                                  \
      {                                                                        \
              false,      /*  Low-noise mode for EFR32 */                             \
             false,      /*  Disable auto-start on EM0/1 entry */                    \
             false,      /*  Disable auto-select on EM0/1 entry */                   \
             false,      /*  Disable auto-start and select on RAC wakeup */          \
             _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,                                    \
             0x142,      /*  Steady-state CTUNE for WSTK boards without load caps */ \
             _CMU_HFXOSTEADYSTATECTRL_REGISH_DEFAULT,                               \
             0x20,       /*  Matching errata fix in CHIP_Init() */                   \
             0x7,        /*  Recommended steady-state osc core bias current */       \
             0x6,        /*  Recommended peak detection threshold */                 \
             _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_DEFAULT,                          \
             0xA,        /*  Recommended peak detection timeout  */                  \
             0x4,        /*  Recommended steady timeout */                           \
             _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,                           \
             cmuOscMode_Crystal,                                                    \
        } 


#if !defined(CMU_HFXOINIT_WSTK_DEFAULT)
#define CMU_HFXOINIT_WSTK_DEFAULT                                          \
      {                                                                        \
              false,      /*  Low-noise mode for EFR32 */                             \
             false,      /*  Disable auto-start on EM0/1 entry */                    \
             false,      /*  Disable auto-select on EM0/1 entry */                   \
             false,      /*  Disable auto-start and select on RAC wakeup */          \
             _CMU_HFXOSTARTUPCTRL_CTUNE_DEFAULT,                                    \
             0x142,      /*  Steady-state CTUNE for WSTK boards without load caps */ \
             _CMU_HFXOSTEADYSTATECTRL_REGISH_DEFAULT,                               \
             _CMU_HFXOSTARTUPCTRL_IBTRIMXOCORE_DEFAULT,                             \
             0x7,        /*  Recommended steady-state osc core bias current */       \
             0x6,        /*  Recommended peak detection threshold */                 \
             _CMU_HFXOTIMEOUTCTRL_SHUNTOPTTIMEOUT_DEFAULT,                          \
             0xA,        /*  Recommended peak detection timeout  */                  \
             0x4,        /*  Recommended steady timeout */                           \
             _CMU_HFXOTIMEOUTCTRL_STARTUPTIMEOUT_DEFAULT,                           \
             cmuOscMode_Crystal,                                                    \
           }
#endif
   
#if !defined(EMU_DCDCINIT_WSTK_DEFAULT)
   /*  Use emlib defaults */
#define EMU_DCDCINIT_WSTK_DEFAULT EMU_DCDCINIT_DEFAULT
#endif

#define CLOCK_HFXO_FREQ 38400000UL
#define CLOCK_LFXO_FREQ 32768UL

/**
 * @brief   ADC configuration
 * @{
 */
static const adc_conf_t adc_config[] = {
    {
        ADC0,                               /* device */
        cmuClock_ADC0,                      /* CMU register */
    }
};

static const adc_chan_conf_t adc_channel_config[] = {
    {
        0,                                  /* device index */
        adcPosSelTEMP,                      /* internal temp sensor see RefMan 24.3.8.9 page 772 */
        adcRef1V25,                         /* channel reference */
        adcAcqTime8                         /* acquisition time */
    },
    {
        0,                                  /* device index */
        adcPosSelAVDD,                      /* internal route of VDD to adc */
        adcRef5V,                           /* channel reference */
        adcAcqTime8                         /* acquisition time */
    },
    {
        0,                                  /* device index */
        adcPosSelAPORT4XCH29,               /* use PB13 as input */
        adcRef5V,                           /* internal 5V ref voltage */
        adcAcqTime8                         /* acquisition time -> 8 clock cycles */
    }
};

#define ADC_NUMOF           (3U)
/** @} */

/**
 * @brief   DAC configuration
 */
#define DAC_NUMOF           (0U)

/**
 * @brief   I2C configuration
 * @{
 */
static const i2c_conf_t i2c_config[] = {
    {
        I2C0,                               /* device */
        GPIO_PIN(PF, 6),                   /* SDA pin */
        GPIO_PIN(PF, 7),                   /* SCL pin */
        I2C_ROUTELOC0_SDALOC_LOC30 |
            I2C_ROUTELOC0_SCLLOC_LOC30,     /* AF location */
        cmuClock_I2C0,                      /* CMU register */
        I2C0_IRQn                           /* IRQ base channel */
    }
};

#define I2C_NUMOF           (1U)
#define I2C_0_ISR           isr_i2c0
/** @} */

/**
 * @brief   PWM configuration
 * @{
 */
static const pwm_chan_conf_t pwm_channel_config[] = {
    /* no available channels */
};

static const pwm_conf_t pwm_config[] = {
    /* no available timers */
};

#define PWM_NUMOF                    (0U)
/** @} */

/**
 * @brief   RTC configuration
 */
#define RTC_NUMOF           (1U)

/**
 * @brief   RTT configuration
 * @{
 */
#define RTT_NUMOF           (1U)

#define RTT_MAX_VALUE       (0xFFFFFFFF)
#define RTT_FREQUENCY       (1U)
/** @} */

/**
 * @brief   SPI configuration
 * @{
 */
 
//static const spi_dev_t spi_config[] = {
//    {
//        USART1,                             /* device */
//        GPIO_PIN(PC, 6),                    /* MOSI pin */
//        GPIO_PIN(PC, 7),                    /* MISO pin */
//        GPIO_PIN(PC, 8),                    /* CLK pin */
//        USART_ROUTELOC0_RXLOC_LOC11 |
//            USART_ROUTELOC0_TXLOC_LOC11 |
//            USART_ROUTELOC0_CLKLOC_LOC11,   /* AF location */
//        cmuClock_USART1,                    /* CMU register */
//        USART1_RX_IRQn                      /* IRQ base channel */
//    }
//};

#define SPI_NUMOF           (0U)
#define SPI_0_EN            0
/** @} */

/**
 * @brief   Timer configuration
 * @{
 */
static const timer_conf_t timer_config[] = {
    {
        {
            TIMER0,             /* lower numbered timer */
            cmuClock_TIMER0     /* pre-scaler bit in the CMU register */
        },
        {
            TIMER1,             /* higher numbered timer, this is the one */
            cmuClock_TIMER1     /* pre-scaler bit in the CMU register */
        },
        TIMER1_IRQn             /* IRQn of the higher numbered timer */
    }
};

#define TIMER_NUMOF         (1U)
#define TIMER_0_ISR         isr_timer1
#define TIMER_0_MAX_VALUE   (0xffff)
/** @} */

/**
 * @brief   UART configuration
 * @{
 */
static const uart_conf_t uart_config[] = {
    {
        USART0,                             /* device */
        GPIO_PIN(PC, 9),                    /* RX pin */
        GPIO_PIN(PC, 8),                    /* TX pin */
        USART_ROUTELOC0_RXLOC_LOC13 |
            USART_ROUTELOC0_TXLOC_LOC13,     /* AF location */
        cmuClock_USART0,                    /* CMU register */
        USART0_RX_IRQn                      /* IRQ base channel */
    }
//    ,
//    {
//        LEUART0,                            /* device */
//        GPIO_PIN(PD, 11),                   /* RX pin */
//        GPIO_PIN(PD, 10),                   /* TX pin */
//        LEUART_ROUTELOC0_RXLOC_LOC18 |
//            LEUART_ROUTELOC0_TXLOC_LOC18,   /* AF location */
//        cmuClock_LEUART0,                   /* CMU register */
//        LEUART0_IRQn                        /* IRQ base channel */
//    }
};

#define UART_NUMOF          (1U)
#define UART_0_ISR_RX       isr_usart0_rx
//#define UART_1_ISR_RX       isr_usart1_rx
//#define UART_2_ISR_RX       isr_leuart0
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CONF_H */
/** @} */
