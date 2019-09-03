# Pump Board - AVR Firmware

This repository contains the source code for the firmware running the pumps in our UbaBot cocktail machine.

Unfortunately, the schematics and board layout can not be published. But I will try to describe the hardware sufficiently so you are able to re-create it if you want to.

## Quick Start - Software

You need avr-gcc and the avr-libc to build this project (use WinAVR on Windows, Macports on OS X or your distros packet manager on Linux).

Simply run `make` to create the firmware hex. Run `make program` to upload the firmware using an AVR ISP MkII.

## Hardware Description

The board is based around an AtXMega128A2AU. The power supply and some other details are left up to your needs.
In the current implementation we're limited to 20 pumps. These are driven using a VN750PS-E motor driver IC for each pump, so 20 in total. For the exact pin connections of the 'pump' and 'sense' lines for each IC, check the 'src/pumps.c' file comments.
There are also some MOSFETs to light some LEDs, but we're not using that feature. For these pinouts, check 'src/lights.c'.
USARTC1 is used to talk to the host machine controlling the pump dispensing.

## Protocol

WIP, the code in here in 'src/interface.c' is the documentation. Also take a look at the corresponding python driver in our bartendro fork repo.

## Links

 * [(my own) Serial Port library](https://github.com/xythobuz/avrSerial)
 * [Datasheet VN750PS-E](http://www.st.com/resource/en/datasheet/vn750ps-e.pdf)
 * [Datasheet ATxmega128A1 MCU](http://www.atmel.com/images/Atmel-8067-8-and-16-bit-AVR-Microcontrollers-ATxmega64A1-ATxmega128A1_Datasheet.pdf)
 * [Datasheet ATxmega128A1 CPU Core](http://www.atmel.com/images/doc8077.pdf)

## License

The code in this repository is released under a BSD 2-Clause License.

Copyright (c) 2012 - 2019 Thomas Buck (thomas@xythobuz.de)
All rights reserved.

> Redistribution and use in source and binary forms, with or without
> modification, are permitted provided that the following conditions
> are met:
>
>  - Redistributions of source code must retain the above copyright notice,
>    this list of conditions and the following disclaimer.
>
>  - Redistributions in binary form must reproduce the above copyright
>    notice, this list of conditions and the following disclaimer in the
>    documentation and/or other materials provided with the distribution.
>
> THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
> "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
> TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
> PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
> CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
> EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
> PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
> PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
> LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
> NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
> SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

