/*
 * Demonstrate keymacros triggered by CPX buttons and captouch. Turns a CP
 * into an application launcher. As long as the application can be launched
 * from command line, there is no need to install software on the computer.
 *
 * Pressing the left button opens Chrome to Google. Pressing the right button
 * opens Chrome to Youtube. Touching captouch #3 (A4) opens Chrom to Adafruit.
 *
 * This version reads key macros from a file named KEYMACRO.TXT stored in the
 * on-board SPI Flash chip.
 *
 * How to upload KEYMACRO.TXT to the CPX SPIFlash using Linux. /dev/ttyACM2 is
 * the CPX USB serial port. The name is likely different (for example. ttyACM0)
 * on your computer. If the sb command is not present, do "sudo apt-get
 * install lrzsz" to install it.
 *
 * $ sb -k keymacro.txt </dev/ttyACM2 >/dev/ttyACM2
 *
 * Most terminal programs such as TeraTerm and minicom have file transfer
 * support. Look for ymodem or xmodem batch.
 */

#include <Adafruit_CircuitPlayground.h>
#include <xymodem.h>
#include <keymouse_play.h>

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
keymouse_play keyplay;

// To open a one line command window on Windows press Windows logo key + R key.
// GUI-R refers to the Windows logo key.
// To open a one line command window on Ubuntu press ALT-F2. This might also
// work on Raspbian.
//
#if 0
// Ubuntu Linux and maybe Raspbian
const char google[] =
"ALT-F2 ~100 'chromium-browser' ~10 ENTER ~100 'https://www.google.com/' ENTER";

const char youtube[] =
"ALT-F2 ~100 'chromium-browser' ~10 ENTER ~100 'https://www.youtube.com/' ENTER";

const char adafruit[] =
"ALT-F2 ~100 'chromium-browser' ~10 ENTER ~100 'https://adafruit.com/' ENTER";
#else
// Windows
const char google[] =
"GUI-R ~100 'chrome' ~10 ENTER ~100 'https://www.google.com/' ENTER";

const char youtube[] =
"GUI-R ~100 'chrome' ~10 ENTER ~100 'https://www.youtube.com/' ENTER";

const char adafruit[] =
"GUI-R ~100 'chrome' ~10 ENTER ~100 'https://adafruit.com/' ENTER";
#endif

// Key macro strings
char *macros[8] = {NULL};

void load_key_macros(void)
{
  // Read keymacro.txt file
  File MacroFile;
  MacroFile = FATFILESYS.open("keymacro.txt");
  if (MacroFile) {
    char buf[1024];
    dbprintln("Open keymacro.txt");
    for (size_t idx = 0; idx < sizeof(macros)/sizeof(macros[0]); idx++) {
      if (MacroFile.available()) {
        int bytesIn = MacroFile.readBytesUntil('\n', buf, sizeof(buf)-1);
        if (bytesIn > 0) {
          buf[bytesIn] = '\0';
          char *buf_right_size = strdup(buf);
          if (macros[idx]) free(macros[idx]);
          macros[idx] = buf_right_size;
          dbprint("macro["); dbprint(idx);
          dbprint("]="); dbprintln(buf_right_size);
        }
      }
      else {
        macros[idx] = NULL;
      }
    }
    MacroFile.close();
    dbprintln("Close keymacro.txt");
  }
  else {
    dbprintln("keymacro.txt not present");
  }
}

void setup() {
  XMODEM_PORT.begin(115200);
#ifdef Debug_Serial
  Debug_Serial.begin(115200);
#endif
  Keyboard.begin();
  delay(2000);
  dbprintln("Circuit Playground Key Macros");

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

  macros[0] = strdup(youtube);
  macros[1] = strdup(google);
  macros[2] = strdup(adafruit);
  load_key_macros();

  CircuitPlayground.begin();
}

bool leftButtonDown = false;
bool rightButtonDown = false;
bool cap3Touched = false;
uint16_t cap3vector[4] = {208, 208, 208, 208};
uint8_t cap3idx = 0;

void loop() {
  // If file transfer finishes, wait for new file transfer
  if (rxymodem.loop() == 0) {
    load_key_macros();
    rxymodem.start_rb(&XMODEM_PORT, &FATFILESYS, true, true);  // Ymodem 1K CRC
  }

  if (CircuitPlayground.leftButton() && !leftButtonDown) {
    dbprintln("Left button pressed!");
    leftButtonDown = true;
    keyplay.start(macros[0]);
  }
  else if (!CircuitPlayground.leftButton() && leftButtonDown) {
    dbprintln("Left button released!");
    leftButtonDown = false;
  }
  if (CircuitPlayground.rightButton() && !rightButtonDown) {
    dbprintln("right button pressed!");
    rightButtonDown = true;
    keyplay.start(macros[1]);
  }
  else if (!CircuitPlayground.rightButton() && rightButtonDown) {
    dbprintln("right button released!");
    rightButtonDown = false;
  }

  //dbprint("Capsense #3: ");
  // Average the last 4 readings to determine whether a touch occurred.
  uint16_t capReading = CircuitPlayground.readCap(3);
  //dbprint(capReading);
  cap3vector[cap3idx++] = capReading;
  cap3idx &= 0x03;  // Limit cap3idx 0..3
  uint16_t cap3average = (cap3vector[0] + cap3vector[1] + cap3vector[2] + cap3vector[3]) >> 2;
  //dbprint(' ');
  //dbprintln(cap3average);
  if (cap3average > 220 && !cap3Touched) {
    dbprintln("cap3 touched!");
    cap3Touched = true;
    keyplay.start(macros[2]);
  }
  else if (cap3average <= 220 && cap3Touched) {
    dbprintln("cap3 released!");
    cap3Touched = false;
  }
#if 0
  dbprint("Capsense #2: "); dbprintln(CircuitPlayground.readCap(2));
  if (! CircuitPlayground.isExpress()) {  // CPX does not have this captouch pin
    dbprint("Capsense #0: "); dbprintln(CircuitPlayground.readCap(0));
  }
  dbprint("Capsense #1: "); dbprintln(CircuitPlayground.readCap(1));
  dbprint("Capsense #12: "); dbprintln(CircuitPlayground.readCap(12));
  dbprint("Capsense #6: "); dbprintln(CircuitPlayground.readCap(6));
  dbprint("Capsense #9: "); dbprintln(CircuitPlayground.readCap(9));
  dbprint("Capsense #10: "); dbprintln(CircuitPlayground.readCap(10));
#endif

  keyplay.loop();
}
