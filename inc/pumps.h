/*
 * pumps.h
 * avr_pump_board
 *
 * This module handles the GPIOs used to control the pumps and sense their
 * error-state, using the 20 connected VN750PS-E high-power drivers.
 *
 * Pumps P01 - P08: PA0 - PA7
 * Pumps P09 - P16: PB0 - PB7
 * Pumps P17 - P20: PH0 - PH3
 *
 * Sense S01 - S08: PJ0 - PJ7
 * Sense S09 - S16: PK0 - PK7
 * Sense S17 - S20: PQ0 - PQ3
 *
 * Copyright (c) 2017 Thomas Buck <xythobuz@xythobuz.de>
 * All rights reserved.
 */

#ifndef __PUMPS_H__
#define __PUMPS_H__

#include "recipe.h"

void pumpsInit(void);
void pumpsClean(uint8_t state);

void pumpsRecipe(RecipeIngredient *recipe, uint8_t ingredients);
uint8_t pumpsDispensing(void);

void pumpOn(uint16_t arg);
void pumpOff(uint16_t arg);

#endif // __PUMPS_H__

