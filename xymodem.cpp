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

#include <Arduino.h>
#include <xymodem.h>

#define DEBUG_ON 0

#if DEBUG_ON
// Arduino Zero, -DUSB_VID=0x2341 -DUSB_PID=0x804d
#if USB_VID==0x2341 && USB_PID==0x804d
/* Programming port */
#define Debug_Serial Serial
#else
#define Debug_Serial Serial1
#endif

#define dbprint(...) Debug_Serial.print(__VA_ARGS__)
#define dbprintln(...) Debug_Serial.println(__VA_ARGS__)
#else
#define dbprint(...)
#define dbprintln(...)
#endif

/*
 * Start XMODEM receive. rx = receive XMODEM
 */
int XYmodem::start_rx(Stream *port, const char *rx_filename, bool rx_buf_1k, bool useCRC)
{
  YMODEM = false;
  return start(port, NULL, rx_filename, rx_buf_1k, useCRC);
}

/*
 * Start YMODEM receive. YMODEM is also known as batch mode. rb = receive batch.
 */
int XYmodem::start_rb(Stream *port, void *filesys, bool rx_buf_1k, bool useCRC)
{
  YMODEM = true;
  return start(port, filesys, NULL, rx_buf_1k, useCRC);
}

int XYmodem::start(Stream *port, void *filesys, const char *rx_filename, bool rx_buf_1k, bool useCRC)
{
  rx_buf_size = 128;
  if (rx_buf_1k) {
    rx_buf_size = 1024;
  }
  dbprint("rx_buf_size=");
  dbprintln(rx_buf_size);
  if (rx_buf == NULL) {
    rx_buf = (uint8_t*)malloc(rx_buf_size);
    if (rx_buf == NULL) {
      rxmodem.close();
      dbprintln("XYmodem malloc failed");
      return 1;
    }
  }
  CRC_on = useCRC;
  rxmodem_state = BLOCKSTART;
  next_block = 1;
  reply = (CRC_on)? 'C' : NAK;
  this->port = port;
  this->filesys = (FATFILESYS_CLASS *)filesys;
  port->write(reply);
  port->flush();
  next_millis = millis()+ TIMEOUT_LONG;
  if (rx_filename != NULL && *rx_filename != '\0') {
    dbprint("XYmodem starting <"); dbprint(rx_filename); dbprintln('>');
    this->filesys->remove((char *)rx_filename);
    strncpy(this->rx_filename, (char *)rx_buf, sizeof(this->rx_filename)-1);
    this->rx_filename[sizeof(this->rx_filename)-1] = '\0';
    rxmodem = this->filesys->open(this->rx_filename, FILE_WRITE);
    if (rxmodem) {
      return 0;
    }
    else {
      dbprintln("XYmodem open file failed");
      return 1;
    }
  }
  return 0;
}

int XYmodem::loop(void)
{
  static uint16_t blocksize;
  static uint16_t blocksizenext;
  static uint8_t block;
  int inchar = 0;
  static uint8_t *p;
  static uint8_t datachecksum = 0;
  static uint16_t CRC = 0;
  static uint16_t CRCRx;

  if (rxmodem_state == IDLE) return 0;

  if (millis() > next_millis) {
    port->write(reply);
    port->flush();
    if (reply == NAK || reply == 'C') {
      next_millis = millis() + TIMEOUT_LONG;
      rxmodem_state = BLOCKSTART;
      dbprintln("timeout, send NAK or C");
    }
    else if (reply == CAN) {
      rxmodem_state = IDLE;
      reply = NAK;
      dbprintln("timeout, send CAN");
    }
    return rxmodem_state;
  }
  while (port->available() > 0) {
    inchar = port->read();
    next_millis = millis() + TIMEOUT_SHORT;
    dbprint("inchar=0x"); dbprintln(inchar, HEX);
    switch (rxmodem_state) {
      case IDLE:
        break;
      case BLOCKSTART:
        dbprint("BLOCKSTART 0x"); dbprintln(inchar, HEX);
        switch (inchar) {
          case SOH:
            blocksize = blocksizenext = 128;
            rxmodem_state = BLOCKNUM;
            break;
          case STX:
            blocksize = blocksizenext = 1024;
            if (blocksize > rx_buf_size) {
              reply = NAK;
              rxmodem_state = DATAPURGE;
            }
            else {
              rxmodem_state = BLOCKNUM;
            }
            break;
          case EOT:
            port->write(ACK);
            port->flush();
            next_block = 1;
            if (rxmodem) {
              dbprint("YMODEM="); dbprintln(YMODEM, DEC);
              dbprint("filename="); dbprintln(rx_filename);
              if (YMODEM && (strcmp(rx_filename, "") == 0))
                rxmodem_state = IDLE;
              else
                rxmodem_state = BLOCKSTART;
              rxmodem.close();
            }
            else {
              rxmodem_state = IDLE;
            }
            break;
        }
        break;
      case BLOCKNUM:
        dbprint("BLOCKNUM block=");
        block = inchar;
        dbprintln(block);
        rxmodem_state = BLOCKCHECK;
        break;
      case BLOCKCHECK:
        dbprint("BLOCKCHECK blockchk=0x");
        dbprintln((uint8_t)(inchar ^ block), HEX);
        if ((uint8_t)(inchar ^ block) == 0xFF) {
          if (((block == next_block) || (block == (next_block-1)))) {
            p = rx_buf;
            datachecksum = 0;
            CRC = 0;
            rxmodem_state = DATABLOCK;
          }
          else {
            reply = CAN;
            rxmodem_state = DATAPURGE;
          }
        }
        else {
          reply = NAK;
          rxmodem_state = DATAPURGE;
        }
        break;
      case DATABLOCK:
        dbprintln("DATABLOCK");
        *p++ = inchar;
        if (CRC_on) {
          CRC = updcrc(inchar, CRC);
        }
        else {
          datachecksum += inchar;
        }
        blocksize--;
        if (blocksize == 0) {
          rxmodem_state = DATACHECK;
        }
        else {
          int bytesAvail, bytesIn;
          bytesAvail = port->available();
          dbprint("bytesAvail=");
          dbprintln(bytesAvail);
          if (bytesAvail > 0) {
            bytesIn = port->readBytes((char *)p, min(bytesAvail, blocksize));
            dbprint("blocksize=");
            dbprint(blocksize);
            dbprint(" bytesIn=");
            dbprintln(bytesIn);
            while (bytesIn--) {
              if (CRC_on) {
                CRC = updcrc(*p++, CRC);
              }
              else {
                datachecksum += *p++;
              }
              blocksize--;
            }
            if (blocksize == 0) rxmodem_state = DATACHECK;
          }
        }
        break;
      case DATACHECK:
        if (CRC_on) {
          dbprint("DATACHECK CRC=0x");
          dbprintln(CRC, HEX);
          CRCRx = inchar << 8;
          rxmodem_state = DATACHECKCRC;
        }
        else {
          dbprint("DATACHECK datachecksum=0x");
          dbprintln(datachecksum, HEX);
          if (datachecksum == inchar) {
            dbprintln("Checksum OK");
            port->write(ACK);
            port->flush();
            if (block == next_block) {
              dbprintln("Good block");
              next_block++;
              int bytesOut = min(blocksizenext, rx_file_remaining);
              rxmodem.write(rx_buf, bytesOut);
              rx_file_remaining -= bytesOut;
              dbprint("rx_file_remaining="); dbprint(rx_file_remaining);
              dbprint(" bytesOut="); dbprintln(bytesOut);
              next_millis = millis() + TIMEOUT_LONG;
              rxmodem_state = BLOCKSTART;
            }
            else if (block == 0) {
              // ymodem block 0 file name, file size, etc.
              dbprint("rx file name="); dbprintln((char *)rx_buf);
              if (rx_buf[0] != '\0') {
                filesys->remove((char *)rx_buf);
                strncpy(rx_filename, (char *)rx_buf, sizeof(rx_filename)-1);
                rx_filename[sizeof(rx_filename)-1] = '\0';
                rxmodem = filesys->open(rx_filename, FILE_WRITE);
                if (rxmodem) {
                  rxmodem_state = BLOCKSTART;
                  next_block = 1;
                  reply = (CRC_on)? 'C' : NAK;
                  port->write(reply);
                  port->flush();
                  next_millis = millis()+ TIMEOUT_LONG;
                  dbprintln("rxmodem starting");
                  dbprintln((char *)rx_buf + strlen((const char *)rx_buf)+1);
                  rx_file_remaining = strtoul(
                      (char *)&rx_buf[strlen((const char *)rx_buf)+1], NULL, 10);
                  dbprint("rx_file_remaining=");
                  dbprintln(rx_file_remaining);
                }
                else {
                  dbprintln("rx file open failed");
                }
              }
              else {
                rxmodem_state = IDLE;
              }
            }
          }
          else {
            dbprintln("Checksum bad");
            port->write(NAK);
            port->flush();
            rxmodem_state = BLOCKSTART;
          }
        }
        break;
      case DATACHECKCRC:
        CRCRx |= inchar;
        dbprint("DATACHECK CRCRx=0x");
        dbprintln(CRCRx, HEX);
        if (CRCRx == CRC) {
          dbprintln("CRC OK");
          port->write(ACK);
          port->flush();
          if (block == next_block) {
            dbprintln("Good block");
            next_block++;
            int bytesOut = min(blocksizenext, rx_file_remaining);
            rxmodem.write(rx_buf, bytesOut);
            rx_file_remaining -= bytesOut;
            dbprint("rx_file_remaining="); dbprint(rx_file_remaining);
            dbprint(" bytesOut="); dbprintln(bytesOut);
            next_millis = millis() + TIMEOUT_LONG;
            rxmodem_state = BLOCKSTART;
          }
          else if (block == 0) {
            // ymodem block 0 file name, file size, etc.
            dbprint("rx file name="); dbprintln((char *)rx_buf);
            if (rx_buf[0] != '\0') {
              filesys->remove((char *)rx_buf);
              strncpy(rx_filename, (char *)rx_buf, sizeof(rx_filename)-1);
              rx_filename[sizeof(rx_filename)-1] = '\0';
              rxmodem = filesys->open(rx_filename, FILE_WRITE);
              if (rxmodem) {
                rxmodem_state = BLOCKSTART;
                next_block = 1;
                reply = (CRC_on)? 'C' : NAK;
                port->write(reply);
                port->flush();
                next_millis = millis()+ TIMEOUT_LONG;
                dbprintln("rxmodem starting");
                dbprintln((char *)rx_buf + strlen((const char *)rx_buf)+1);
                rx_file_remaining = strtoul(
                    (char *)&rx_buf[strlen((const char *)rx_buf)+1], NULL, 10);
                dbprint("rx_file_remaining=");
                dbprintln(rx_file_remaining);
              }
              else {
                dbprintln("rx file open failed");
              }
            }
            else {
              rxmodem_state = IDLE;
            }
          }
        }
        else {
          dbprintln("Checksum bad");
          port->write(NAK);
          port->flush();
          rxmodem_state = BLOCKSTART;
        }
        break;
      case DATAPURGE:
        dbprintln("DATAPURGE");
        int bytesAvail = port->available();
        dbprint("bytesAvail=");
        dbprintln(bytesAvail);
        if (bytesAvail > 0) {
          port->readBytes((char *)p, bytesAvail);
        }
        break;
    }
  } // while available()
  return rxmodem_state;
}

inline uint16_t XYmodem::updcrc(uint8_t c, uint16_t crc)
{
  int count;

  crc = crc ^ (uint16_t) c << 8;
  count = 8;
  do {
    if (crc & 0x8000) {
      crc = crc << 1 ^ 0x1021;
    }
    else {
      crc = crc << 1;
    }
  } while (--count);
  return crc;
}

#if defined(ADAFRUIT_SPIFLASH)
void XYmodem::format_flash() {
  // Partition the flash with 1 partition that takes the entire space.
  Serial.println("Partitioning flash with 1 primary partition...");
  DWORD plist[] = {100, 0, 0, 0};  // 1 primary partition with 100% of space.
  uint8_t buf[512] = {0};          // Working buffer for f_fdisk function.
  FRESULT r = f_fdisk(0, plist, buf);
  if (r != FR_OK) {
    Serial.print("Error, f_fdisk failed with error code: "); Serial.println(r, DEC);
    while(1);
  }
  Serial.println("Partitioned flash!");

  // Make filesystem.
  Serial.println("Creating and formatting FAT filesystem (this takes ~60 seconds)...");
  r = f_mkfs("", FM_ANY, 0, buf, sizeof(buf));
  if (r != FR_OK) {
    Serial.print("Error, f_mkfs failed with error code: "); Serial.println(r, DEC);
    while(1);
  }
  Serial.println("Formatted flash!");

  // Finally test that the filesystem can be mounted.
  if (!FATFILESYS.begin()) {
    Serial.println("Error, failed to mount newly formatted filesystem!");
    while(1);
  }
}
#endif

int XYmodem::begin()
{
#if defined(ADAFRUIT_ITSYBITSY_M0) || defined(ADAFRUIT_CIRCUITPLAYGROUND_M0)
  Serial.println("M0 SPI Flash");
  // Initialize flash library and check its chip ID.
  if (!flash.begin(FLASH_TYPE)) {
    Serial.println("Error, failed to initialize flash chip!");
    return -1;
  }
  Serial.print("Flash chip JEDEC ID: 0x"); Serial.println(flash.GetJEDECID(), HEX);

  // Call fatfs activate to make it the active chip that receives low level fatfs
  // callbacks. This is necessary before making any manual fatfs function calls
  // (like the f_fdisk and f_mkfs functions further below).  Be sure to call
  // activate before you call any fatfs functions yourself!
  FATFILESYS.activate();

#elif defined(ADAFRUIT_METRO_M4_EXPRESS)
  Serial.println("M4 QSPI Flash");
  // Initialize flash library and check its chip ID.
  if (!flash.begin()) {
    Serial.println("Error, failed to initialize flash chip!");
    return -1;
  }
  flash.setFlashType(FLASH_TYPE);
  // Call fatfs activate to make it the active chip that receives low level fatfs
  // callbacks. This is necessary before making any manual fatfs function calls
  // (like the f_fdisk and f_mkfs functions further below).  Be sure to call
  // activate before you call any fatfs functions yourself!
  FATFILESYS.activate();

#endif

  // Mount the filesystem. Format it if it fails.
  if (!FATFILESYS.begin(chipSelect)) {
    Serial.println("Error, failed to mount filesystem!");
#if defined(ADAFRUIT_SPIFLASH)
    // Wait for user to send OK to continue.
    Serial.setTimeout(30000);  // Increase timeout to print message less frequently.
    do {
      Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
      Serial.println("This sketch will ERASE ALL DATA on the flash chip and format it with a new filesystem!");
      Serial.println("Type OK (all caps) and press enter to continue.");
      Serial.println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    }
    while (!Serial.find((char *)"OK"));
    format_flash();

    Serial.println("Flash chip successfully formatted with new empty filesystem!");
#else
    return -2;
#endif
  }
  return 0;
}
