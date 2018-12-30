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

XYmodem rxymodem;

void setup()
{
  XMODEM_PORT.begin(115200);
#ifdef Debug_Serial
  Debug_Serial.begin(115200);
#endif
  delay(2000);

  dbprint("Initializing Flash file system...");
  if (rxymodem.begin()) {
    dbprintln("initialization failed!");
    return;
  }
  dbprintln("initialization done.");

  // One big difference between Xmodem and Ymodem. Ymodem sends the filename
  // and size for 1 or more files (also known as batch mode). The file size
  // is important because Xmodem pads all files to multiples of 128 bytes.
  // Ymodem tranferred files should not be padded.
  // 1K = 1024 bytes blocks instead of 128 byte blocks.
  // CRC = use 16-bit CRC instead of 1 byte checksum
  //rxymodem.start_rx(&XMODEM_PORT, "junk.dat", true, true);  // Xmodem 1K CRC
  rxymodem.start_rb(&XMODEM_PORT, &FATFILESYS, true, true);  // Ymodem 1K CRC
}

void loop()
{
  if (rxymodem.loop() == 0) {
    //rxymodem.start_rx(&XMODEM_PORT, "morejunk.dat", true, true);  // Xmodem 1K CRC
    rxymodem.start_rb(&XMODEM_PORT, &FATFILESYS, true, true);  // Ymodem 1K CRC
  }
}
