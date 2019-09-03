/*
 * clock.c
 * avr_pump_board
 *
 * Runs a system-timer with 1ms resolution on Timer1.
 *
 * Copyright (c) 2017 Thomas Buck <xythobuz@xythobuz.de>
 * All rights reserved.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdlib.h>
#include <util/atomic.h>

//#define DEBUG_CLOCK

#ifdef DEBUG_CLOCK
#include "serial.h"
#endif // DEBUG_CLOCK

#include "clock.h"

void initOSCs(void) {
    // Setup system clock source
    // 2MHz * 10 = 20MHz
    _PROTECTED_WRITE(OSC_PLLCTRL, F_CPU / 2000000l);

    // Enable PLL
    _PROTECTED_WRITE(OSC_CTRL, OSC_PLLEN_bm | OSC_RC2MEN_bm | OSC_XOSCEN_bm);

    // No Prescaling: 1 / 1-1
    _PROTECTED_WRITE(CLK_PSCTRL, 0);

    // Wait for PLL to be ready...
    while (!(OSC.STATUS & OSC_PLLRDY_bm));

    // Set PLL as main clock-source
    _PROTECTED_WRITE(CLK_CTRL, 0x04);

    // Disable external clock source
    _PROTECTED_WRITE(OSC_CTRL, OSC_PLLEN_bm | OSC_RC2MEN_bm);
}

// ----------------------------------------------------------------------------

volatile static uint64_t systemTime = 0;

typedef void (*CallbackType)(void);

volatile static uint64_t quickTimeFire = 0;
volatile static CallbackType quickTimeCallback = NULL;

void initSystemTimer(void) {
    // initialize TimerC0 with 32MHz / 64 / 500 = 1kHz
    TCC0.CTRLA = TC0_CLKSEL0_bm | TC0_CLKSEL2_bm; // Prescaler 64
    TCC0.PER = 500; // overflow when counting to 500
    TCC0.INTCTRLB = 0x03; // high interrupt level
}

uint64_t getSystemTime(void) {
    return systemTime;
}

ISR(TCC0_CCA_vect) {
    systemTime++;

    if (quickTimeCallback != NULL) {
        if (systemTime == quickTimeFire) {
            quickTimeCallback();
        }
    }
}

// ----------------------------------------------------------------------------

void quickTimeInit(void) {
    // TODO wanted to use RTC32, but our MCU doesn't have this peripheral! :(
}

void quickTimeFireIn(uint16_t millis, void (*callback)(void)) {
#ifdef DEBUG_CLOCK
    serialWriteString(1, "Debug: fire in ");
    serialWriteInt16(1, millis);
    serialWriteString(1, " at ");
    serialWriteInt16(1, systemTime + millis);
    serialWriteString(1, " now ");
    serialWriteInt16(1, systemTime);
    serialWriteString(1, "\n");
#endif // DEBUG_CLOCK

    // called again from ISR!
    quickTimeFire = systemTime + millis;
    quickTimeCallback = callback;
}

