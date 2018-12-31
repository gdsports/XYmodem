/*
 * Demonstrate sounds triggered by CPX buttons and captouch.
 *
 * Pressing the left button plays button1.pcm.
 * Pressing the right button plays button2.pcm.
 * Touching captouch #3 (A4) play captch1.pcm.
 *
 * The PCM audio files are stored in the on-board SPI Flash chip.
 *
 * How to upload files to the CPX SPIFlash using Linux. /dev/ttyACM2 is the CPX
 * USB serial port. The name is likely different (for example. ttyACM0) on your
 * computer. If the sb command is not present, do "sudo apt-get install lrzsz"
 * to install it.
 *
 * $ sb -k *.pcm </dev/ttyACM2 >/dev/ttyACM2
 *
 * Most terminal programs such as TeraTerm and minicom have file transfer
 * support. Look for ymodem or xmodem batch.
 */

#include <Adafruit_CircuitPlayground.h>
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

void playPCM(const char *pcmfile, uint16_t sampleRate)
{
  File PCM_File;
  PCM_File = FATFILESYS.open(pcmfile);
  if (PCM_File) {
    dbprintln("Open PCM file");
    size_t pcmdata_len = PCM_File.size();
    uint8_t *pcmdata =(uint8_t*) malloc(pcmdata_len);
    if (pcmdata) {
      uint32_t bytesIn = PCM_File.read(pcmdata, pcmdata_len);
      if (bytesIn > 0) {
        CircuitPlayground.speaker.playSound(pcmdata, bytesIn, sampleRate);
      }
      free(pcmdata);
    }
    else {
      uint8_t *pcmdata =(uint8_t*) malloc(2048);
      if (pcmdata) {
        while (pcmdata_len > 0) {
          uint32_t bytesIn = PCM_File.read(pcmdata, min(pcmdata_len, sizeof(pcmdata)));
          if (bytesIn > 0) {
            CircuitPlayground.speaker.playSound(pcmdata, bytesIn, sampleRate);
            pcmdata_len -= bytesIn;
          }
        }
        free(pcmdata);
      }
      else {
        dbprintln("pcm file malloc failed");
      }
    }
    PCM_File.close();
    dbprintln("Close PCM file");
  }
  else {
    dbprintln("PCM file not present");
  }
}

void setup() {
  XMODEM_PORT.begin(115200);
  CircuitPlayground.begin();
#ifdef Debug_Serial
  Debug_Serial.begin(115200);
#endif

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
  rxymodem.start_rb(&XMODEM_PORT, &FATFILESYS, true, true);  // Ymodem 1K CRC
}

bool leftButtonDown = false;
bool rightButtonDown = false;
bool cap3Touched = false;
uint16_t cap3vector[4] = {208, 208, 208, 208};
uint8_t cap3idx = 0;

void loop() {
  // If file transfer finishes, wait for new file transfer
  if (rxymodem.loop() == 0) {
    rxymodem.start_rb(&XMODEM_PORT, &FATFILESYS, true, true);  // Ymodem 1K CRC
  }

  if (CircuitPlayground.leftButton() && !leftButtonDown) {
    dbprintln("Left button pressed!");
    leftButtonDown = true;
    playPCM("button1.pcm", 11025);
  }
  else if (!CircuitPlayground.leftButton() && leftButtonDown) {
    dbprintln("Left button released!");
    leftButtonDown = false;
  }
  if (CircuitPlayground.rightButton() && !rightButtonDown) {
    dbprintln("right button pressed!");
    rightButtonDown = true;
    playPCM("button2.pcm", 11025);
  }
  else if (!CircuitPlayground.rightButton() && rightButtonDown) {
    dbprintln("right button released!");
    rightButtonDown = false;
  }

  // Average the last 4 readings to determine whether a touch occurred.
  uint16_t capReading = CircuitPlayground.readCap(3);
  cap3vector[cap3idx++] = capReading;
  cap3idx &= 0x03;  // Limit cap3idx 0..3
  uint16_t cap3average = (cap3vector[0] + cap3vector[1] + cap3vector[2] + cap3vector[3]) >> 2;
  if (cap3average > 220 && !cap3Touched) {
    dbprintln("cap3 touched!");
    cap3Touched = true;
    playPCM("captch1.pcm", 11025);
  }
  else if (cap3average <= 220 && cap3Touched) {
    dbprintln("cap3 released!");
    cap3Touched = false;
  }
}
