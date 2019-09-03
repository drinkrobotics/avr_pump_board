/*
 * interface.c
 * avr_pump_board
 *
 * This file implements the serial ASCII menu interface used as an API for the
 * master device to control the system.
 *
 * Each command consists of one character identifying the action to be
 * executed, prefixed by an arbitrary string set in this module, followed by
 * an optional unnamed parameter. The command ends with a new-line (\n).
 * Only ASCII decimal numbers are supported as parameters.
 *
 * For example, if the prefix is set to "$$":
 *     $$v\n - Show the version information
 *     $$p10\n - Set pump 10 as state for the next command
 *
 * The implementation of the methods is done in this module, too.
 * They are then included in the commands[] list.
 *
 * Copyright (c) 2017 Thomas Buck <xythobuz@xythobuz.de>
 * All rights reserved.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

#include "config.h"
#include "serial.h"
#include "recipe.h"
#include "pumps.h"
#include "lights.h"
#include "interface.h"

// ----------------------------------------------------------------------------
// Implementation of interface functions
// optional parameters are given as parameter - if they exist

#define printHelp(c, desc) serialWriteString(1, "  " COMMAND_PREFIX c "  - " desc "\n")
static void methodHelp(uint16_t arg) {
    serialWriteString(1, "Available commands:\n");
    printHelp("h", "Print this help text");
    printHelp("v", "Print version information");
    printHelp("r", "Reset recipe list");
    printHelp("pX", "Set pump X for current recipe ingredient");
    printHelp("dX", "Set duration to X milliseconds for current recipe ingredient");
    printHelp("wX", "Wait for X milliseconds before starting this recipe ingredient");
    printHelp("s", "Store current recipe ingredient and go to next one");
    printHelp("g", "Go and dispense currently entered recipe");
    printHelp("l", "List currently entered recipe ingredients");
    printHelp("cX", "Start or stop cleaning cycle for all pumps (0 or 1)");
    printHelp("nX", "Turn on pump X");
    printHelp("fX", "Turn off pump X");
    printHelp("q", "Debug helper");
}

static void methodVersion(uint16_t arg) {
    serialWriteString(1, TARGET_ID " firmware " VERSION_ID "\n");
    serialWriteString(1, "by " AUTHOR_ID " - build date:\n");
    serialWriteString(1, __DATE__ " - " __TIME__ "\n");
}

static void methodClean(uint16_t arg) {
    if (arg) {
        pumpsClean(1);
    } else {
        pumpsClean(0);
    }
}

static void methodDebug(uint16_t arg) {
    serialWriteString(1, "Refreshing RGB LEDs...\n");
    lightsDisplayBuffer();
}

// ----------------------------------------------------------------------------

typedef void (*InterfaceMethod)(uint16_t arg);

#define MAX_CHARS_PER_COMMAND 3

typedef struct {
    uint8_t chars[MAX_CHARS_PER_COMMAND];
    InterfaceMethod callback;
} InterfaceCommand;

InterfaceCommand commands[] = {
    { { 'h', 'H', '?' }, methodHelp },
    { { 'v', 'V',  0  }, methodVersion },
    { { 'r', 'R',  0  }, recipeReset },
    { { 'p', 'P',  0  }, recipePump },
    { { 'd', 'D',  0  }, recipeDuration },
    { { 'w', 'W',  0  }, recipeDelay },
    { { 's', 'S',  0  }, recipeStore },
    { { 'g', 'G',  0  }, recipeGo },
    { { 'l', 'L',  0  }, recipeList },
    { { 'c', 'C',  0  }, methodClean },
    { { 'n', 'N',  0  }, pumpOn },
    { { 'f', 'F',  0  }, pumpOff },
    { { 'q',  0,   0  }, methodDebug }
};
static const uint8_t commandCount = sizeof(commands) / sizeof(InterfaceCommand);

#define STATE_RESET 0
#define STATE_READING 1
static uint8_t state = STATE_RESET;

#define PREFIX_LEN ((sizeof(COMMAND_PREFIX) / sizeof(char)) - 1)
#define MAX_PARAMETER_LEN 6
#define BUF_LEN (MAX_PARAMETER_LEN + PREFIX_LEN + 10)
static uint8_t lineBuffer[BUF_LEN];
static uint8_t lineBufferLen = 0;

void interfaceHandler(uint8_t c, uint16_t arg) {
    for (uint8_t i = 0; i < commandCount; i++) {
        for (uint8_t j = 0; j < MAX_CHARS_PER_COMMAND; j++) {
            if (commands[i].chars[j] == c) {
                commands[i].callback(arg);
                return;
            }
        }
    }

    serialWriteString(1, "Error: unknown command!\n");
}

static uint16_t convertAsciiToInt(uint8_t *s, uint8_t l) {
    uint16_t res = 0;
    for (uint8_t i = 0, n = l - 1; i < l; i++, n--) {
        uint16_t x = 1;
        for (uint8_t j = 0; j < n; j++) {
            x *= 10;
        }
        res += (s[i] - '0') * x;
    }
    return res;
}

static void interfaceHandleLine(void) {
    if (lineBufferLen < (PREFIX_LEN + 1)) {
        return;
    }

    for (uint8_t i = 0; i < PREFIX_LEN; i++) {
        if (lineBuffer[i] != COMMAND_PREFIX[i]) {
            serialWriteString(1, "Error: invalid command prefix!\n");
            return;
        }
    }

    char c = lineBuffer[PREFIX_LEN];
    uint8_t n = PREFIX_LEN + 1;
    if (n >= lineBufferLen) {
        interfaceHandler(c, 0);
    } else {
        // There are some bytes left - probably parameters
        uint8_t digitBuffer[MAX_PARAMETER_LEN];
        uint8_t digitIndex = 0;
        while (n < lineBufferLen) {
            if ((lineBuffer[n] < '0') || (lineBuffer[n] > '9')) {
                serialWriteString(1, "Error: non-ASCII-digit parameter!\n");
                return;
            } else {
                digitBuffer[digitIndex] = lineBuffer[n];
                if (digitIndex < (MAX_PARAMETER_LEN - 1)) {
                    digitIndex++;
                } else {
                    serialWriteString(1, "Error: parameter is too long!\n");
                    return;
                }
            }
            n++;
        }
        interfaceHandler(c, convertAsciiToInt(digitBuffer, digitIndex));
    }
}

void interfaceLoop(void) {
    if (state == STATE_RESET) {
        serialWriteString(1, COMMANDLINE_STRING);
        state = STATE_READING;
        lineBufferLen = 0;
    } else if (state == STATE_READING) {
        if (serialHasChar(1)) {
            uint8_t c = serialGet(1);

#ifndef DISABLE_SERIAL_ECHO
            serialWrite(1, c);
#endif

            if ((c != '\r') && (c != '\n')) {
                lineBuffer[lineBufferLen] = c;
                if (lineBufferLen < (BUF_LEN - 1)) {
                    lineBufferLen++;
                } else {
                    serialWriteString(1, "Error: command line buffer will overflow!\n");
                }
            } else if (c == '\n') {
                interfaceHandleLine();
                state = STATE_RESET;
            }
        }
    } else {
        serialWriteString(1, "Error: Invalid State!\n");
        state = STATE_RESET;
    }
}

