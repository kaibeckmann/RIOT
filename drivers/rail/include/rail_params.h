/*
 * Copyright (C) 2018 Hochschule RheinMain
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef RAIL_PARAMS_H
#define RAIL_PARAMS_H

#include "board.h"
#include "rail_drv.h"

#ifdef __cplusplus
extern "C" {
#endif

#define RAIL_PARAMS_DEFAULT { .tmp = 23, \
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


#endif /* RAIL_PARAMS_H */
