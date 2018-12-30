/*
MIT License

Copyright (c) 2018 gdsports625@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef _XYMODEM_H_
#define _XYMODEM_H_

#if defined(ADAFRUIT_ITSYBITSY_M0) || defined(ADAFRUIT_CIRCUITPLAYGROUND_M0)
#define ADAFRUIT_SPIFLASH 1
#define FATFILESYS fatfs
#define FATFILESYS_CLASS Adafruit_W25Q16BV_FatFs
#define chipSelect
#include <SPI.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_SPIFlash_FatFs.h>

// Include the FatFs library header to use its low level functions
// directly.  Specifically the f_fdisk and f_mkfs functions are used
// to partition and create the filesystem.
#include "utility/ff.h"

// Configuration of the flash chip pins and flash fatfs object.
// You don't normally need to change these if using a Feather/Metro
// M0 express board.
#define FLASH_TYPE     SPIFLASHTYPE_W25Q16BV  // Flash chip type.
                                              // If you change this be
                                              // sure to change the fatfs
                                              // object type below to match.

#if defined(ADAFRUIT_ITSYBITSY_M0)
#define FLASH_SS       SS1                    // Flash chip SS pin.
#define FLASH_SPI_PORT SPI1                   // What SPI port is Flash on?
#define NEOPIN         40
#else
#if !defined(SS1)
  #define FLASH_SS       SS                    // Flash chip SS pin.
  #define FLASH_SPI_PORT SPI                   // What SPI port is Flash on?
  #define NEOPIN         8
#else
  #define FLASH_SS       SS1                    // Flash chip SS pin.
  #define FLASH_SPI_PORT SPI1                   // What SPI port is Flash on?
  #define NEOPIN         40
#endif
#endif
extern Adafruit_SPIFlash flash;
extern Adafruit_W25Q16BV_FatFs fatfs;
#define XMODEM_PORT Serial

#elif defined(ADAFRUIT_METRO_M4_EXPRESS)
#define ADAFRUIT_SPIFLASH 1
#define FATFILESYS fatfs
#define FATFILESYS_CLASS Adafruit_W25Q16BV_FatFs
#define chipSelect
#include <Adafruit_SPIFlash.h>
#include <Adafruit_SPIFlash_FatFs.h>

// Include the FatFs library header to use its low level functions
// directly.  Specifically the f_fdisk and f_mkfs functions are used
// to partition and create the filesystem.
#include "utility/ff.h"

#include "Adafruit_QSPI_GD25Q.h"
#define FLASH_TYPE     SPIFLASHTYPE_W25Q16BV  // Flash chip type.
extern Adafruit_QSPI_GD25Q flash;
extern Adafruit_W25Q16BV_FatFs fatfs;
#define XMODEM_PORT Serial

// Teensy 3.6
#elif defined(__MK66FX1M0__)
#include <SD.h>
#define FATFILESYS SD
#define FATFILESYS_CLASS SDClass
#define chipSelect BUILTIN_SDCARD
#define XMODEM_PORT Serial

// Arduino Zero
#elif defined(ARDUINO_SAMD_ZERO)
#include <SD.h>
#define FATFILESYS SD
#define FATFILESYS_CLASS SDClass
#define chipSelect
#define XMODEM_PORT SerialUSB

#else
#include <SD.h>
#define FATFILESYS SD
#define FATFILESYS_CLASS SDClass
#define chipSelect
#define XMODEM_PORT Serial
#endif

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

class XYmodem {
  public:
    int start_rx(Stream *port, const char *rx_filename, bool rx_buf_1k, bool useCRC);
    int start_rb(Stream *port, void *filesys, bool rx_buf_1k, bool useCRC);
    int begin(void);
    int loop(void);
  private:
    const uint32_t TIMEOUT_LONG=3000;
    const uint32_t TIMEOUT_SHORT=1000;
    File rxmodem;
    enum rxmodem_t {
      IDLE, BLOCKSTART, BLOCKNUM, BLOCKCHECK, DATABLOCK,
      DATACHECK, DATACHECKCRC, DATAPURGE
    };
    rxmodem_t rxmodem_state = IDLE;
    char rx_filename[128+1];
    uint8_t next_block;
    uint8_t *rx_buf = NULL;
    uint16_t rx_buf_size = 128;
    uint32_t rx_file_remaining;
    uint32_t next_millis = 0;
    uint8_t reply;
    bool CRC_on = false;
    bool YMODEM = false;
    Stream *port;
    FATFILESYS_CLASS *filesys;

  private:
    int start(Stream *port, void *filesys, const char *rx_filename, bool rx_buf_1k, bool useCRC);
    void format_flash();
    uint16_t updcrc(uint8_t c, uint16_t crc);
};

#endif /* _XYMODEM_H_ */
