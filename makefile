# makefile
# Copyright (c) 2012 - 2017 Thomas Buck <xythobuz@xythobuz.de>
# All rights reserved.

TARGET = avr_pump_board

SRCS = src/main.c
SRCS += src/clock.c
SRCS += src/interface.c
SRCS += src/lights.c
SRCS += src/pumps.c
SRCS += src/recipe.c
SRCS += src/serial.c

# -----------------------------------------------------------------------------

MCU = atxmega128a1
F_CPU = 32000000

ISPPORT = usb
ISPTYPE = avrisp2

# -----------------------------------------------------------------------------

CARGS = -mmcu=$(MCU)
CARGS += -Iinc
CARGS += -Os
CARGS += -funsigned-char
CARGS += -funsigned-bitfields
CARGS += -fpack-struct
CARGS += -fshort-enums
CARGS += -ffunction-sections
CARGS += -fdata-sections
CARGS += -Wall -Wstrict-prototypes
CARGS += -std=gnu99
CARGS += -DF_CPU=$(F_CPU)
CARGS += -MP -MD

LDARGS = -Wl,-L.,-lm,--relax,--gc-sections

# -----------------------------------------------------------------------------

AVRDUDE = avrdude
AVRGCC = avr-gcc
AVRSIZE = avr-size
AVROBJCOPY = avr-objcopy
RM = rm -rf

# -----------------------------------------------------------------------------

OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

all: $(TARGET).hex $(TARGET).elf $(OBJS)

program: $(TARGET).hex
	$(AVRDUDE) -p $(MCU) -c $(ISPTYPE) -P $(ISPPORT) -e -U $(TARGET).hex

%.hex: %.elf $(OBJS)
	$(AVROBJCOPY) -O ihex $< $@

%.elf: $(OBJS)
	$(AVRGCC) $(CARGS) $(OBJS) --output $@ $(LDARGS)
	$(AVRSIZE) $@

%.o: %.c
	$(AVRGCC) -c $< -o $@ $(CARGS)

clean:
	$(RM) $(OBJS)
	$(RM) $(DEPS)
	$(RM) $(TARGET).elf
	$(RM) $(TARGET).hex

# Always recompile interface (prints compile date)
src/interface.o: FORCE
FORCE:

# -----------------------------------------------------------------------------

-include $(DEPS)

