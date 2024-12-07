# pico-coldfire-bdm

An interface to Motorola® Coldfire processors' Background Debug Interface (BDM) using a Raspberry Pi Pico

```
pip install arduino-coldfire-bdm
```

### Usage

**THIS IS IN PROGRESS AND DOES NOT YET WORK**

1. Ensure you have a [working Python 3 installation](https://python.org/download).
2. Do some magic.
3. Connect a Raspberry Pi Pico to a Motorola Coldfire via its debug pins. See below for pin mappings. (Ensure the Coldfire board is powered up independently; the Pico does not supply power to the Coldfire.)
4. Drag the `.uf2` file onto the Ri Pi Pico (`.uf2` is available in the "releases").
5. On the command line (Terminal.app, Command Prompt on Windows, etc), run `pico-coldfire-bdm` (or, if that doesn't work, `python3 -m pico_coldfire_bdm.command_line`) to invoke the command line program. You should see the following help text:

```
usage: arduino-coldfire-bdm [-h] [--dry-run] [--show-commands] [--serial-port SERIAL_PORT] [--baud-rate BAUD_RATE] {dump_memory,trace_execution,load_flash,sram_test} ...

Communicate with an attached Coldfire V3 board (and maybe other versions too) to act as a simple debugger, via an Arduino serial connection.

options:
  -h, --help            show this help message and exit
  --dry-run             If passed, don't actually connect to a serial port; just queue up commands that would have otherwise been sent to an attached Arduino.
  --show-commands       If passed, print out a listing of every command sent to the Pico.
  --serial-port SERIAL_PORT
                        The file path of the serial port to connect to.
  --baud-rate BAUD_RATE
                        The baud rate to use when connecting to the Arduino over serial. Must match what the Arduino expects. Not required with R-Pi Pico.

commands:
  {dump_memory,trace_execution,load_flash,sram_test}
    dump_memory         Dump memory.
    trace_execution     Begin execution and print out the program counter and registers before every instruction.
    load_flash          Erase an attached Flash chip and load in new contents from a file.
    sram_test           Test SRAM attached to the Coldfire. Expects exactly 1MB of RAM, attached via chip-select port 1, mapped at 0x00200000.
```

6. Select the appropriate `--serial-port` to use to connect to your Pico, and run one of the commands.

## What?

A long long time ago (the mid-1990s), Motorola created a series of CPUs derived from the 68k architecture, called the Coldfire. These processors are largely obsolete today, but are still found in certain industrial equipment and embedded devices released around that time; including some vintage synthesizers, like [the Alesis A6 Andromeda](https://www.alesis.com/products/view/a6-andromeda).

This repository contains two things:
 - The Pico code, which allows connecting a $4 R-Pi Pico to a Coldfire processor's Background Debug Mode (BDM) port to send and receive data
 - a Python library to connect to a Pico over USB serial, to allow running high-level debugging commands, like:
   - tracing execution (i.e.: like GDB or LLDB)
   - dumping the contents of memory to a file
   - erasing and re-flashing an attached flash memory chip
   - testing attached RAM chips
   - whatever else you want; it's Python! You can script it.

This library, when paired with a Pico, performs many of the same functions as [PEMicro's _Multilink_ debugging probes](https://www.pemicro.com). This library is free, runs wherever you can run an Arduino and Python code (rather than just on Windows), and requires no drivers. However, this library is way slower, is missing a ton of functions, and has no IDE support.

## Why?

I stole this entire idea and code from [https://github.com/psobot/arduino-coldfire-bdm](https://github.com/psobot/arduino-coldfire-bdm).

He has this reasoning:
I bought a broken Alesis A6 Andromeda, and wanted to try fixing it by re-flashing its firmware without doing any soldering, because I'm bad at soldering. Read about that journey [on my blog](http://blog.petersobot.com/preview/c5fGNB81gvwoJvi7SchKEK/).

## How?

I stole this entire idea and code from [https://github.com/psobot/arduino-coldfire-bdm](https://github.com/psobot/arduino-coldfire-bdm).

He did the hard work:
I read the [MCF5307 data sheet](https://www.nxp.com/docs/en/data-sheet/MCF5307BUM.pdf) (484 pages) very carefully. It documents how to connect to a Coldfire BDM port from first principles.

## Can I use this for other things?

Probably. There are certain assumptions made (especially around interfacing with Flash or RAM) that apply only to the Andromeda, and may not be useful for other devices. Pull requests to make this code more generic would be welcomed.

## What if I brick my device?

Here's the license; I take no responsibility if this software is misused. You should probably read the software carefully first.

```
Copyright (c) 2022 Peter Sobot

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
