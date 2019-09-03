/*
 * recipe.c
 * avr_pump_board
 *
 * This module handles the recipe ingredients and their dispension.
 * To ensure the amount of liquids dispensed is accurate, we first transmit
 * all the required pumps and their durations before starting them all at once.
 *
 * Copyright (c) 2017 Thomas Buck <xythobuz@xythobuz.de>
 * All rights reserved.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "config.h"
#include "serial.h"
#include "pumps.h"
#include "recipe.h"

static RecipeIngredient ingredients[RECIPE_MAX_INGREDIENTS];
static uint8_t ingredientCount = 0;

#define FLAG_STATE_PUMP (1 << 0)
#define FLAG_STATE_TIME (1 << 1)
#define FLAG_STATE_DELAY (1 << 2)

static uint8_t statePump = 0;
static uint16_t stateTime = 0;
static uint16_t stateDelay = 0;
static uint8_t state = 0;

void recipeReset(uint16_t arg) {
    ingredientCount = 0;
    statePump = 0;
    stateTime = 0;
    stateDelay = 0;
    state = 0;
}

void recipePump(uint16_t arg) {
    if ((arg < 1) || (arg > 20)) {
        serialWriteString(1, "Error: invalid pump id!\n");
        return;
    }

    if (ingredientCount >= RECIPE_MAX_INGREDIENTS) {
        serialWriteString(1, "Error: too many ingredients in recipe!\n");
        return;
    }

    statePump = arg;
    state |= FLAG_STATE_PUMP;
}

void recipeDuration(uint16_t arg) {
    if (arg == 0) {
        serialWriteString(1, "Error: only positive integer times are allowed!\n");
        return;
    }

    if (ingredientCount >= RECIPE_MAX_INGREDIENTS) {
        serialWriteString(1, "Error: too many ingredients in recipe!\n");
        return;
    }

    stateTime = arg;
    state |= FLAG_STATE_TIME;
}

void recipeDelay(uint16_t arg) {
	stateDelay = arg;
	state |= FLAG_STATE_DELAY;
}

void recipeStore(uint16_t arg) {
    if (ingredientCount >= RECIPE_MAX_INGREDIENTS) {
        serialWriteString(1, "Error: too many ingredients in recipe!\n");
        return;
    }

    if ((!(state & FLAG_STATE_PUMP)) || (!(state & FLAG_STATE_TIME))) {
        serialWriteString(1, "Error: can't store without pump and time!\n");
        return;
    }

    /* search if this pump is already in use */
    uint8_t exists = 0, i;
    for (i = 0; i < ingredientCount; i++) {
        if (ingredients[i].pump == statePump) {
            exists = 1;
            break;
        }
    }

    if (exists) {
        /* entry for this pump already exists -> overwrite */
        ingredients[i].pump = statePump;
        ingredients[i].time = stateTime;
        ingredients[i].delay = stateDelay;
    } else {
        ingredients[ingredientCount].pump = statePump;
        ingredients[ingredientCount].time = stateTime;
        ingredients[ingredientCount].delay = stateDelay;
        ingredientCount++;
    }
}

void recipeGo(uint16_t arg) {
    if (ingredientCount == 0) {
        serialWriteString(1, "Error: no ingredients stored!\n");
        return;
    }

    // Turn on 2nd status LED while dispensing
    PORTE.OUTCLR = PIN7_bm;

    // start dispensing
    pumpsRecipe(ingredients, ingredientCount);

    // wait until dispension is finished
    while (pumpsDispensing());
    PORTE.OUTSET = PIN7_bm;
    recipeReset(0);
}

void recipeList(uint16_t arg) {
    serialWriteString(1, "Stored ");
    serialWriteInt16(1, ingredientCount);
    serialWriteString(1, " ingredients\n");
    for (uint8_t i = 0; i < ingredientCount; i++) {
        serialWriteString(1, "Pump ");
        serialWriteInt16(1, ingredients[i].pump);
        serialWriteString(1, " running for ");
        serialWriteInt16(1, ingredients[i].time);
        serialWriteString(1, "ms after ");
        serialWriteInt16(1, ingredients[i].delay);
        serialWriteString(1, "ms\n");
    }
}

