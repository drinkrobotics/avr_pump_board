/*
 * lights.h
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

#ifndef __LIGHTS_H__
#define __LIGHTS_H__

#define COLOR_RED   0xFF0000
#define COLOR_GREEN 0x00FF00
#define COLOR_BLUE  0x0000FF

void lightsInit(void);

// id: (1 - 20), state: (0 or 1)
void lightsSet(uint8_t id, uint8_t state);

void lightsRGB(uint16_t led, uint32_t color);
void lightsDisplayBuffer(void);

#define lightsBusy() (DMA.STATUS & (DMA_CH0BUSY_bm | DMA_CH1BUSY_bm))

#endif // __LIGHTS_H__

