/*
 * main.c
 * avr_pump_board
 *
 * Copyright (c) 2017 Thomas Buck <xythobuz@xythobuz.de>
 * All rights reserved.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <util/delay.h>

#include "clock.h"
#include "pumps.h"
#include "lights.h"
#include "serial.h"
#include "interface.h"

int main(void) {
    // Status LEDs on PE6 and PE7
    PORTE.DIRSET = PIN6_bm | PIN7_bm; // LEDs as Outputs

    // Enable both LEDs after reset
    PORTE.OUTCLR = PIN6_bm | PIN7_bm;

    // Initialize hardware
    initOSCs();
    initSystemTimer();
    pumpsInit();
    lightsInit();

    // FTDI FT232RL on PC6 (Rx) and PC7 (Tx) / USARTC1 / UART id 1
    PORTC.DIRCLR = PIN6_bm; // Rx as Input
    PORTC.DIRSET = PIN7_bm; // Tx as Output
    PORTC.OUTSET = PIN7_bm; // Set to logic '1'
    serialInit(1, BAUD(38400, F_CPU));

    // Enable all interrupt levels
    PMIC.CTRL |= PMIC_LOLVLEN_bm | PMIC_MEDLVLEN_bm | PMIC_HILVLEN_bm;
    sei();

    // Print Welcome Message with some version info
    serialWriteString(1, "\n");
    interfaceHandler('v', 0);
    serialWriteString(1, "ready!\n");

    // 4-bit active-low DIP switch for hardware ID on-board
    PORTH.DIRCLR = PIN4_bm | PIN5_bm | PIN6_bm | PIN7_bm;
    uint8_t id = ((~PORTH.IN) & 0xF0) >> 4;
    serialWriteString(1, "Hardware ID: ");
    serialWriteInt16(1, id);
    serialWriteString(1, "\n");

    // Disable LEDs after init
    PORTE.OUTSET = PIN6_bm | PIN7_bm;

#if 0
    _delay_ms(500); // some time for uart tx buffer to clear
    lightsRGB(0, COLOR_RED);
    lightsRGB(1, COLOR_GREEN);
    lightsRGB(2, COLOR_BLUE);
    lightsDisplayBuffer();
#endif

    // Main-Loop
    uint64_t lastBeat = 0;
    for(;;) {
        // Wait for and handle incoming commands
        interfaceLoop();

        // blink heart-beat LED every 500ms
        if ((getSystemTime() - lastBeat) > 500) {
            lastBeat = getSystemTime();
            PORTE.OUTTGL = PIN6_bm;
        }
    }

    return 0; // never reached
}

