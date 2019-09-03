/*
 * pumps.c
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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/delay.h>

//#define DEBUG_PUMPS

#include "config.h"
#include "serial.h"
#include "clock.h"
#include "lights.h"
#include "pumps.h"

static volatile uint8_t pumpRunning = 0;

static uint16_t pumpLastTime = 0;
static uint8_t pumpLastIndex = 0;

static RecipeIngredient *pumpLastRecipe = NULL;
static uint8_t pumpLastRecipeCount = 0;
static uint16_t pumpCurrentRunTime = 0;

uint8_t pumpsDispensing(void) {
    return pumpRunning;
}

void pumpsInit(void) {
    // All pump pins as output
    PORTA.DIRSET = 0xFF;
    PORTB.DIRSET = 0xFF;
    PORTH.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;

    // All pumps to logic-'0'
    PORTA.OUTCLR = 0xFF;
    PORTB.OUTCLR = 0xFF;
    PORTH.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;

    pumpRunning = 0;
    pumpLastTime = 0;
    pumpLastIndex = 0;
    pumpLastRecipe = NULL;
    pumpLastRecipeCount = 0;
    pumpCurrentRunTime = 0;

    // All sense pins as input
    PORTJ.DIRSET = 0x00;
    PORTK.DIRSET = 0x00;
    PORTQ.DIRCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;

    // External Pull-Ups on sense inputs. Pin goes low on error.
    // Always returns to high when motor input goes low. So interrupting
    // on the falling edge should be sufficient.
    // TODO still problematic...?
    /*
    PORTJ.INT0MASK = 0xFF;
    PORTJ.INTCTRL = 0x03; // high-priority interrupt
    PORTK.INT0MASK = 0xFF;
    PORTK.INTCTRL = 0x03;
    PORTQ.INT0MASK = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm;
    PORTQ.INTCTRL = 0x03;
    */
}

static void pumpSet(uint8_t id, uint8_t state) {
    if ((id < 1) || (id > 20)) {
        serialWriteString(1, "Error: invalid pump id!\n");
        return;
    }
    id--;

    if (id < 8) {
        if (state) {
            PORTA.OUTSET = (1 << id);
        } else {
            PORTA.OUTCLR = (1 << id);
        }
    } else if (id < 16) {
        if (state) {
            PORTB.OUTSET = (1 << (id - 8));
        } else {
            PORTB.OUTCLR = (1 << (id - 8));
        }
    } else {
        if (state) {
            PORTH.OUTSET = (1 << (id - 16));
        } else {
            PORTH.OUTCLR = (1 << (id - 16));
        }
    }

    lightsSet(id, state);
}

void pumpOn(uint16_t arg) {
	pumpSet(arg, 1);
}

void pumpOff(uint16_t arg) {
	pumpSet(arg, 0);
}

static void pumpErrorInterrupt(uint8_t n) {
    serialWriteString(1, "Error: pump driver port ");
    serialWriteInt16(1, n);
    serialWriteString(1, " reports a problem!\n");

    // turn off all pumps when an error is reported
    for (uint8_t i = 1; i <= 20; i++) {
        pumpSet(i, 0);
    }

    pumpRunning = 0;
}

ISR(PORTJ_INT0_vect) {
    pumpErrorInterrupt(0);
}

ISR(PORTK_INT0_vect) {
    pumpErrorInterrupt(1);
}

ISR(PORTQ_INT0_vect) {
    pumpErrorInterrupt(2);
}

void pumpsClean(uint8_t state) {
    if (state && pumpRunning) {
        serialWriteString(1, "Error: can't clean while pumps are running!\n");
        return;
    }

    if ((!state) && (!pumpRunning)) {
        serialWriteString(1, "Error: can't stop cleaning while no pumps are running!\n");
        return;
    }

    pumpRunning = state ? 1 : 0;

    for (uint8_t i = 1; i <= 20; i++) {
        pumpSet(i, state);
        _delay_ms(PUMP_CLEAN_DELAY);
    }
}

static void pumpHandleRecipeState(void) {
    // turn off the current pump
    pumpSet(pumpLastRecipe[pumpLastIndex].pump, 0);
    pumpCurrentRunTime = pumpLastRecipe[pumpLastIndex].time;
    pumpLastRecipe[pumpLastIndex].time = 0;

#ifdef DEBUG_PUMPS
    serialWriteString(1, "Debug: turning off pump ");
    serialWriteInt16(1, pumpLastRecipe[pumpLastIndex].pump);
    serialWriteString(1, "\n");
#endif // DEBUG_PUMPS

    // determine next pump
    pumpLastTime = 0xFFFF;
    pumpLastIndex = RECIPE_MAX_INGREDIENTS;
    for (uint8_t i = 0; i < pumpLastRecipeCount; i++) {
        if (pumpLastRecipe[i].time > 0) {
            // turn off other pumps with the same timestamp
            if (pumpLastRecipe[i].time <= pumpCurrentRunTime) {
                pumpSet(pumpLastRecipe[i].pump, 0);

#ifdef DEBUG_PUMPS
                serialWriteString(1, "Debug: also turning off pump ");
                serialWriteInt16(1, pumpLastRecipe[i].pump);
                serialWriteString(1, "\n");
#endif // DEBUG_PUMPS

                pumpLastRecipe[i].time = 0;
            } else if (pumpLastRecipe[i].time < pumpLastTime) {
                pumpLastTime = pumpLastRecipe[i].time;
                pumpLastIndex = i;
            }
        }
    }

    // set up timer for next pump
    if (pumpLastIndex < RECIPE_MAX_INGREDIENTS) {
#ifdef DEBUG_PUMPS
        serialWriteString(1, "Debug: next pump to turn off: ");
        serialWriteInt16(1, pumpLastRecipe[pumpLastIndex].pump);
        serialWriteString(1, " in ");
        serialWriteInt16(1, pumpLastRecipe[pumpLastIndex].time - pumpCurrentRunTime);
        serialWriteString(1, "\n");
#endif // DEBUG_PUMPS

        quickTimeFireIn(pumpLastRecipe[pumpLastIndex].time - pumpCurrentRunTime, pumpHandleRecipeState);
    } else {
        pumpRunning = 0;

#ifdef DEBUG_PUMPS
        serialWriteString(1, "Debug: Done!\n");
#endif // DEBUG_PUMPS
    }
}

void pumpsRecipe(RecipeIngredient *recipe, uint8_t ingredients) {
    if (pumpRunning) {
        serialWriteString(1, "Error: can't dispense recipe while pumps are running!\n");
        return;
    }

    if (ingredients < 1) {
        serialWriteString(1, "Error: can't dispense empty recipe!\n");
        return;
    }

    pumpLastRecipe = recipe;
    pumpLastRecipeCount = ingredients;
    pumpLastTime = 0xFFFF;
    pumpLastIndex = RECIPE_MAX_INGREDIENTS;
    for (uint8_t i = 0; i < ingredients; i++) {
        if ((recipe[i].pump < 1) || (recipe[i].pump > 20)) {
            serialWriteString(1, "Error: invalid pump in recipe!\n");
            return;
        }
        if (recipe[i].time < 1) {
            serialWriteString(1, "Error: invalid time in recipe!\n");
            return;
        }

		// select the pump with the lowest duration as the one we turn off
		// first, on our next "interrupt"
        if ((recipe[i].time + recipe[i].delay) < pumpLastTime) {
            pumpLastTime = recipe[i].time + recipe[i].delay;
            pumpLastIndex = i;
        }
    }

	// TODO add support for delay, currently all are turned on at the same time!!

    if (pumpLastIndex >= RECIPE_MAX_INGREDIENTS) {
        serialWriteString(1, "Error: no valid next pump found!\n");
        return;
    }

    pumpCurrentRunTime = 0;

    // initialize our timer used for this
    quickTimeInit();

    pumpRunning = 1;

#ifdef DEBUG_PUMPS
    serialWriteString(1, "Debug: next pump to turn off: ");
    serialWriteInt16(1, recipe[pumpLastIndex].pump);
    serialWriteString(1, " in ");
    serialWriteInt16(1, recipe[pumpLastIndex].time);
    serialWriteString(1, "\nDebug: turning on all pumps\n");
#endif // DEBUG_PUMPS

    for (uint8_t i = 0; i < ingredients; i++) {
        pumpSet(recipe[i].pump, 1); // turn on all pumps in recipe
    }

    quickTimeFireIn(pumpLastTime, pumpHandleRecipeState);
}

