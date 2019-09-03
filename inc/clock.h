/*
 * clock.h
 * avr_pump_board
 *
 * Copyright (c) 2017 Thomas Buck <xythobuz@xythobuz.de>
 * All rights reserved.
 */

#ifndef __CLOCK_H__
#define __CLOCK_H__

void initOSCs(void);

void initSystemTimer(void);
uint64_t getSystemTime(void);

void quickTimeInit(void);
void quickTimeFireIn(uint16_t millis, void (*callback)(void));

#endif // __CLOCK_H__

