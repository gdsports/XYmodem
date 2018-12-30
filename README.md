# XMODEM and YMODEM for Arduino

XMODEM and YMODEM file transfer protocols for Arduino, Adafruit, and Teensy.

* Reliable method to transfer binary files into SPI Flash and SD.
* Supports YMODEM 1K blocks, batch mode, and CRC.
* Supports XMODEM 1K blocks and CRC.
* Works with SD and Adafruit SPI and QSPI Flash FAT file systems.
* Only receive is implemented so far.
* Tested boards: Teensy 3.6 with SD card, Adafruit Metro M4, Adafruit
Circuit Playground Express.

## Examples

### rxymodem

Simple demo of receiving files using YMODEM in continous receive mode.

### fatfscli

A simple command line interface to FAT file systems. This is very useful for
managing SPI Flash. Unlike SD card, SPI Flash cannot be removed and managed on
a computer. Any terminal program such as TeraTerm and minicom may be used. The
Arduino serial monitor is not recommended. If it must be used, change the
line ending to CR.

#### Print file and directory names.
dir and ls are synonyms. [dirname] is optional. Defaults to working directory.

     dir [dirname], ls [dirname]

#### Print working directory.
pwd and cd (no parameter) are synonyms.

     pwd, cd

#### Change working directory.
If no dirname is specified, prints the working directory. If full path
is not specified the working directory is used.

     cd <dirname>

#### Make directory.
Multiple levels may be specified. E.g. mkdir
dir1/dir2/dir3. If full path is not specified the working directory is used.

     mkdir <dirname>

#### Remove directory and ALL files and subdirectories.

Works like "rm -rf" so be careful. If full path is not specified the
working directory is used.

     rmdir <dirname>

#### Remove file.
del and rm are synonyms. If full path is not specified the working
directory is used. No wildcards.

     del <filename>, rm <filename>

#### Print file contents.
type and cat are synonyms. If full path is not
specified the working directory is used. Use the terminal program logging or
ASCII file capture to save the contents to a computer file.

     type <filename>, cat <filename>

#### Read from Serial port and write to a file.
Terminate with ^D. If full
path is not specified the working directory is used.

     capture <filename>

E.g. Run "capture params.json". In the terminal program, send the contents
of an ASCII file. Use copy and paste or ASCII upload feature of the terminal
program. Close the file by pressing ^D. Examine the file using "cat
params.json".

#### Receive YMODEM batch mode.
The sender may send 0 or more files including
file names. rb receives and creates the files.

     rb

#### Receive one file using XMODEM.
The XMODEM protocol does not allow the
sender to send the filename. Do not use XMODEM unless YMODEM is not available.
The XMODEM protocol also pads files to multiples of 128 bytes.

     rx <filename>

### CircuitPlaygroundExpress

Demonstrate using a CPX as USB keyboard macro board. The key macro processor is
provided by the [keymouse_t3](https://github.com/gdsports/keymouse_t3) library.
The key macros are read from a file named KEYMACRO.TXT stored in SPI Flash.

This library makes it possible to send the KEYMACRO.TXT file from a computer
using YMODEM. The user does not need to install and use the Arduino IDE to
change the key macros. The only program require is a terminal program such as
TeraTerm or minicom.
