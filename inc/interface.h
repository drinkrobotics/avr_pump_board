/*
 * interface.h
 * avr_pump_board
 *
 * This file implements the serial ASCII menu interface used as an API for the
 * master device to control the system.
 *
 * Each command consists of one character identifying the action to be
 * executed, prefixed by an arbitrary string set in this module, followed by
 * an optional unnamed parameter and, if the unnamed parameter exists, more
 * optional named parameters. The command ends with a new-line (\n).
 * Only ASCII decimal numbers are supported as parameters.
 *
 * For example, if the prefix is set to "$$":
 *     $$v\n - Show the version information
 *     $$p10d500\n - Run pump 10 for a duration of 500ms
 *
 * The implementation of the methods is done in this module, too.
 * They are then included in the commands[] list.
 *
 * Copyright (c) 2017 Thomas Buck <xythobuz@xythobuz.de>
 * All rights reserved.
 */

#ifndef __INTERFACE_H__
#define __INTERFACE_H__

void interfaceHandler(uint8_t c, uint16_t arg);
void interfaceLoop(void);

#endif // __INTERFACE_H__

