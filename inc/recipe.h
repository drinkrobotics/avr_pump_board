/*
 * recipe.h
 * avr_pump_board
 *
 * This module handles the recipe ingredients and their dispension.
 * To ensure the amount of liquids dispensed is accurate, we first transmit
 * all the required pumps and their durations before starting them all at once.
 *
 * Copyright (c) 2017 Thomas Buck <xythobuz@xythobuz.de>
 * All rights reserved.
 */

#ifndef __RECIPE_H__
#define __RECIPE_H__

typedef struct {
    uint8_t pump;
    uint16_t time;
    uint16_t delay;
} RecipeIngredient;

void recipeReset(uint16_t arg);
void recipePump(uint16_t arg);
void recipeDuration(uint16_t arg);
void recipeDelay(uint16_t arg);
void recipeStore(uint16_t arg);
void recipeGo(uint16_t arg);
void recipeList(uint16_t arg);

#endif // __RECIPE_H__

