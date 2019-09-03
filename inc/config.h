/*
 * config.h
 * avr_pump_board
 *
 * Copyright (c) 2017 Thomas Buck <xythobuz@xythobuz.de>
 * All rights reserved.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define TARGET_ID "avr_pump_board"
#define VERSION_ID "v3.0"
#define AUTHOR_ID "xythobuz.de"

//#define DISABLE_SERIAL_ECHO
#define COMMANDLINE_STRING "?> "
#define COMMAND_PREFIX ""

// doesn't need to be larger than pump count
#define RECIPE_MAX_INGREDIENTS 20

// switch all pumps in the span of 1000ms when cleaning
#define PUMP_CLEAN_DELAY (1000 / 20)

#endif // __CONFIG_H__

