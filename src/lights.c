/*
 * lights.c
 * avr_pump_board
 *
 * In normal operation, 20 lights are connected via 20 N-Channel MOSFETs to
 * 20 GPIOs of our MCU:
 *
 * Lights L01 - L06: PC0 - PC5
 * Lights L07 - L12: PD0 - PD5
 * Lights L13 - L18: PE0 - PE5
 * Lights L19 - L20: PF0 - PF1
 *
 * Additionally, three GPIOs (also with MOSFETs) are routed to an external
 * connector, to connect a classic single-color 12V RGB-LED strip.
 *
 * With this firmware, a WS2812 RGB LED strip is driven from LR.
 * Connect data with a small pull-up to +5V to PF2/OC0C.
 *
 * LR, LG, LB: PF2 (OC0C), PF3 (OC0D), PF4 (OC1A)
 *
 * WS2812 datasheet:
 * https://cdn-shop.adafruit.com/datasheets/WS2812.pdf
 *
 * Inspired by Assembler implementation by M. Marquardt:
 * https://www.mikrocontroller.net/topic/281135
 *
 * Copyright (c) 2017 Thomas Buck <xythobuz@xythobuz.de>
 * All rights reserved.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "clock.h"
#include "serial.h"
#include "lights.h"

#define LED_COUNT 300
#define LED_FREQ 800000ul // 800kHz as in WS2812 datasheet
#define BITS_PER_BYTE 8
#define COLOR_COMPONENTS 3
#define BUFFERED_BYTES_PER_CH 1

// bit timings
#define LED_BIT_COUNT ((F_CPU / LED_FREQ) - 1ul)
#define LED_BIT_COUNT_0 ((((F_CPU / LED_FREQ) * 1ul) / 3ul) - 1ul)
#define LED_BIT_COUNT_1 ((((F_CPU / LED_FREQ) * 2ul) / 3ul) - 1ul)

// One PWM value byte for each bit: 8bit x 3 (RGB) = 24
#define PWM_BUFFERED_BYTES (COLOR_COMPONENTS * BUFFERED_BYTES_PER_CH)
#define PWM_BUF_SIZE (BITS_PER_BYTE * PWM_BUFFERED_BYTES)
static volatile uint8_t ledBufferA[PWM_BUF_SIZE];
static volatile uint8_t ledBufferB[PWM_BUF_SIZE];

// RGB data buffer
#define RGB_BUF_SIZE ((LED_COUNT * COLOR_COMPONENTS) + 1)
static uint8_t ledBuffer[RGB_BUF_SIZE];
static volatile uint16_t ledBufferPos = 0;

#define DMA_TRANSACTION_INTERRUPT_LEVEL 0x02

void lightsInit(void) {
    // Set LED pins as output
    PORTC.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm;
    PORTD.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm;
    PORTE.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm;
    PORTF.DIRSET = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm;

    // Disable LEDs on init
    PORTC.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm;
    PORTD.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm;
    PORTE.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm | PIN5_bm;
    PORTF.OUTCLR = PIN0_bm | PIN1_bm | PIN2_bm | PIN3_bm | PIN4_bm;

#if 0
    // Enable DMA channels for WS2812 control
    DMA.CTRL = DMA_ENABLE_bm | DMA_DBUFMODE_CH01_gc;

    // Single-Shot transfers in repeat-mode
    DMA.CH0.CTRLA = DMA_CH_SINGLE_bm | DMA_CH_REPEAT_bm;
    DMA.CH1.CTRLA = DMA_CH_SINGLE_bm | DMA_CH_REPEAT_bm;

    // Set transaction complete interrupt level to medium
    DMA.CH0.CTRLB = DMA_TRANSACTION_INTERRUPT_LEVEL << DMA_CH_TRNINTLVL_gp;
    DMA.CH1.CTRLB = DMA_TRANSACTION_INTERRUPT_LEVEL << DMA_CH_TRNINTLVL_gp;

    // Group configuration: Transaction; Increment source address
    DMA.CH0.ADDRCTRL = DMA_CH_SRCRELOAD0_bm | DMA_CH_SRCRELOAD1_bm | DMA_CH_SRCDIR0_bm;
    DMA.CH1.ADDRCTRL = DMA_CH_SRCRELOAD0_bm | DMA_CH_SRCRELOAD1_bm | DMA_CH_SRCDIR0_bm;

    // Trigger group: SYS
    DMA.CH0.TRIGSRC = DMA_CH_TRIGSRC_EVSYS_CH0_gc;
    DMA.CH1.TRIGSRC = DMA_CH_TRIGSRC_EVSYS_CH0_gc;

    // Endless transfers
    DMA.CH0.REPCNT = 0x00;
    DMA.CH1.REPCNT = 0x00;

    // Read from our buffer arrays
    DMA.CH0.SRCADDR0 = ((uint16_t)ledBufferA & 0x00FF);
    DMA.CH0.SRCADDR1 = ((uint16_t)ledBufferA & 0xFF00) >> 8;
    DMA.CH0.SRCADDR2 = 0x00;
    DMA.CH1.SRCADDR0 = ((uint16_t)ledBufferB & 0x00FF);
    DMA.CH1.SRCADDR1 = ((uint16_t)ledBufferB & 0xFF00) >> 8;
    DMA.CH1.SRCADDR2 = 0x00;

    // Write to TimerF0 Output Compare C buffer
    DMA.CH0.DESTADDR0 = ((uint16_t)TCF0_CCCBUF & 0x00FF);
    DMA.CH0.DESTADDR1 = ((uint16_t)TCF0_CCCBUF & 0xFF00) >> 8;
    DMA.CH0.DESTADDR2 = 0;
    DMA.CH1.DESTADDR0 = ((uint16_t)TCF0_CCCBUF & 0x00FF);
    DMA.CH1.DESTADDR1 = ((uint16_t)TCF0_CCCBUF & 0xFF00) >> 8;
    DMA.CH1.DESTADDR2 = 0;

    // Trigger DMA transfer on TimerF0 overflow
    EVSYS.CH0MUX = EVSYS_CHMUX_TCF0_OVF_gc;
    EVSYS.CH0CTRL = 0x00;

    // Enable Compare C, select single-slope PWM mode
    TCF0.CTRLB = TC0_CCCEN_bm | TC_WGMODE_SS_gc;

    // Clear output compare bit while OFF
    TCF0.CTRLC = 0x00;

    // Byte-Mode: upper byte of counter set to zero after each counter clock cycle
    TCF0.CTRLE = TC0_BYTEM_bm;

    // bug in DMA hardware?: interrupt needs to fire
    TCF0.INTCTRLA = 0x03; // high-priority interrupt

    // Pre-load timer with our value calculated from the LED frequency
    TCF0.PERBUF = LED_BIT_COUNT;
    TCF0.PER = LED_BIT_COUNT;

    // clear data buffers
    for (uint16_t i = 0; i < LED_COUNT; i++) {
        ledBuffer[(3 * i) + 0] = 0;
        ledBuffer[(3 * i) + 1] = 0;
        ledBuffer[(3 * i) + 2] = 0;

        if (i < PWM_BUF_SIZE) {
            ledBufferA[i] = 0;
            ledBufferB[i] = 0;
        }
    }

    // end marker
    ledBuffer[RGB_BUF_SIZE - 1] = 0xFF;
#endif
}

ISR(TCF0_OVF_vect) { } // bug in DMA hardware?: interrupt needs to exist

void lightsRGB(uint16_t led, uint32_t color) {
    if (lightsBusy()) {
        serialWriteString(1, "Error: DMA transfer in progress!\n");
        return;
    }

    if (led >= LED_COUNT) {
        serialWriteString(1, "Error: invalid LED number!\n");
        return;
    }

    ledBuffer[(3 * led) + 0] = (color & 0x0000FF) >> 0;
    ledBuffer[(3 * led) + 1] = (color & 0x00FF00) >> 8;
    ledBuffer[(3 * led) + 2] = (color & 0xFF0000) >> 16;
}

static void lightsRGBtoPWM(volatile uint8_t *in, volatile uint8_t *out, uint16_t len) {
    // read len bytes from in and writes (len * 8) bytes to out
    for (uint16_t i = 0; i < len; i++) {
        for (uint8_t b = 0; b < BITS_PER_BYTE; b++) {
            if (in[i] & (1 << (BITS_PER_BYTE - b - 1))) {
                // bit is set
                out[(i * BITS_PER_BYTE) + b] = LED_BIT_COUNT_1;
            } else {
                // bit is cleared
                out[(i * BITS_PER_BYTE) + b] = LED_BIT_COUNT_0;
            }
        }
    }

    // we always write to buffers of size PWM_BUF_SIZE
    // if it isn't filled completely, end with 0xFF as marker
    for (uint16_t i = (len * BITS_PER_BYTE) + 1; i < PWM_BUF_SIZE; i++) {
        out[i] = 0xFF;
    }
}

void lightsDisplayBuffer(void) {
    if (lightsBusy()) {
        serialWriteString(1, "Error: DMA transfer is in progress!\n");
        return;
    }

//#define TIMER_TEST
#ifdef TIMER_TEST
    // Timer test - only put out 800kHz 50% PWM signal
    TCF0.PER = LED_BIT_COUNT * 1;
    TCF0.PERBUF = LED_BIT_COUNT * 1;
    TCF0.CNT = 0;
    TCF0.CCC = LED_BIT_COUNT / 2 * 1;
    TCF0.CTRLA = TC_CLKSEL_DIV1_gc;
    return;
#endif // TIMER_TEST

    serialWriteString(1, "Starting WS2812 output...\n");

    // fill both buffers
    lightsRGBtoPWM(ledBuffer, ledBufferA, PWM_BUFFERED_BYTES);
    lightsRGBtoPWM(ledBuffer + PWM_BUFFERED_BYTES, ledBufferB, PWM_BUFFERED_BYTES);
    ledBufferPos = 2 * PWM_BUFFERED_BYTES;

    // set DMA sizes
    DMA.CH0.TRFCNT = PWM_BUF_SIZE;
    DMA.CH1.TRFCNT = PWM_BUF_SIZE;

    // enable DMA repeat mode
    DMA.CH0.CTRLA |= DMA_CH_REPEAT_bm;
    DMA.CH1.CTRLA |= DMA_CH_REPEAT_bm;

    // Init counter with 1 to avoid HIGH state on first run
    TCF0.CNT = 1;
    TCF0.CCC = 0x00;

    DMA.CH0.CTRLA |= DMA_CH_ENABLE_bm; // Enable DMA0
    TCF0.CTRLA = TC_CLKSEL_DIV1_gc; // Start Timer

    EVSYS.STROBE = 0x01; // strobe our evsys event

#define WAIT_FOR_DMA_TO_FINISH
#ifdef WAIT_FOR_DMA_TO_FINISH
    PORTE.OUTCLR = PIN7_bm;
    if (!lightsBusy()) {
        serialWriteString(1, "DMA finished immediately?!\n");
    } else {
        uint64_t startTime = getSystemTime();
        while (lightsBusy());
        serialWriteString(1, "DMA finished in ");
        serialWriteInt16(1, getSystemTime() - startTime);
        serialWriteString(1, "ms!\n");
    }
    PORTE.OUTSET = PIN7_bm;
#endif // WAIT_FOR_DMA_TO_FINISH
}

static void lightsDMAInterrupt(volatile uint8_t *thisBuf, DMA_CH_t *thisDMA, DMA_CH_t *otherDMA) {
    serialWriteString(1, (thisBuf == ledBufferA) ? "a" : "b");
    if (ledBufferPos > RGB_BUF_SIZE) {
        serialWriteString(1, "f\n");
        goto dma_isr_end;
    } else {
        serialWriteString(1, "c\n");
    }

    uint16_t len = PWM_BUFFERED_BYTES;
    if ((RGB_BUF_SIZE - ledBufferPos) < PWM_BUFFERED_BYTES) {
        len = RGB_BUF_SIZE - ledBufferPos;
    }
    lightsRGBtoPWM(ledBuffer + ledBufferPos, thisBuf, len);
    ledBufferPos += len;

    thisDMA->TRFCNT = len;

    if (ledBufferPos < RGB_BUF_SIZE) {
        goto dma_isr0;
    }

dma_isr_stop:
    otherDMA->CTRLA &= ~DMA_CH_REPEAT_bm;

dma_isr0:
    thisDMA->CTRLB = DMA_CH_TRNIF_bm | (DMA_TRANSACTION_INTERRUPT_LEVEL << DMA_CH_TRNINTLVL_gp);
    return;

dma_isr_end:
    thisDMA->CTRLA &= ~DMA_CH_ENABLE_bm;
    if (otherDMA->CTRLA & DMA_CH_REPEAT_bm) {
        goto dma_isr_stop;
    }

    // wait for end marker
    while (TCF0.CCC != 0xFF);

    // stop timer and force output low
    TCF0.CTRLA = 0x00;
    TCF0.CTRLC = 0x00;

    goto dma_isr0;
}

ISR(DMA_CH0_vect) {
    // DMA Channel A is finished
    lightsDMAInterrupt(ledBufferA, &DMA.CH0, &DMA.CH1);
}

ISR(DMA_CH1_vect) {
    // DMA Channel B is finished
    lightsDMAInterrupt(ledBufferB, &DMA.CH1, &DMA.CH0);
}

void lightsSet(uint8_t id, uint8_t state) {
    if ((id < 1) || (id > 20)) {
        return;
    }
    id--;

    if (id < 5) {
        if (state) {
            PORTC.OUTSET = (1 << id);
        } else {
            PORTC.OUTCLR = (1 << id);
        }
    } else if (id < 10) {
        if (state) {
            PORTD.OUTSET = (1 << (id - 5));
        } else {
            PORTD.OUTCLR = (1 << (id - 5));
        }
    } else if (id < 15) {
        if (state) {
            PORTE.OUTSET = (1 << (id - 10));
        } else {
            PORTE.OUTCLR = (1 << (id - 10));
        }
    } else {
        if (state) {
            PORTF.OUTSET = (1 << (id - 15));
        } else {
            PORTF.OUTCLR = (1 << (id - 15));
        }
    }
}

