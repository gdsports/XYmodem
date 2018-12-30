/*
 * Command Line Interface (CLI) out Serial port for filesystem basic
 * operations. The Adafruit M0/M4 Express/CPX have a SPI Flash chip which is
 * very useful for parameters, data logging, etc. This program provides an easy
 * way to access the files stored in the SPI Flash chip using a terminal
 * program. minicom on Linux/Unix works well. The Arduino Serial monitor also
 * works. TeraTerm or Putty on Windows should work.
 *
 * Test on Adafruit Metro M4 Express.
 *
 * This is much easier to implement and understand than USB Mass Storage or USB
 * Media Transfer Protocol.
 *
 * ## Print file and directory names. dir and ls are synonyms. [dirname] is
 * optional. Defaults to working directory.
 *
 *      dir [dirname], ls [dirname]
 *
 * ## Print working directory. pwd and cd (no parameter) are synonyms.
 *
 *      pwd, cd
 *
 * ## Change working directory. If no dirname is specified, prints the working
 * directory. If full path is not specified the working directory is used.
 *
 *      cd <dirname>
 *
 * ## Make directory. Multiple levels may be specified. E.g. mkdir
 * dir1/dir2/dir3. If full path is not specified the working directory is used.
 *
 *      mkdir <dirname>
 *
 * ## Remove directory and ALL files and subdirectories. Works like "rm -rf" so
 * be careful. If full path is not specified the working directory is used.
 *
 *      rmdir <dirname>
 *
 * ## Remove file. del and rm are synonyms. If full path is not specified the
 * working directory is used.
 *
 *      del <filename>, rm <filename>
 *
 * ## Print file contents. type and cat are synonyms. If full path is not
 * specified the working directory is used. Use the terminal program logging or
 * ASCII file capture to save the contents to a computer file.
 *
 *      type <filename>, cat <filename>
 *
 * ## Read from Serial port and write to a file. Terminate with ^D. If full
 * path is not specified the working directory is used.
 *
 *      capture <filename>
 *
 * E.g. Run "capture params.json". In the terminal program, send the contents
 * of an ASCII file. Use copy and paste or ASCII upload feature of the terminal
 * program. Close the file by pressing ^D. Examine the file using "cat
 * params.json".
 *
 * ## Receive YMODEM batch mode. The sender may send 0 or more files including
 * file names. rb receives and creates the files.
 *
 *    rb
 *
 * ## Receive one file using XMODEM. The XMODEM protocol does not allow the
 * sender to send the filename. Do not use this unless YMODEM is not available.
 * The XMODEM protocol also pads files to multiples of 128 bytes.
 *
 *    rx <filename>
 *
 * ## TODO maybe, not too useful
 *
 *    ren <fromfilename> <tofilename>, mv <fromfilename> <tofilename>
 *
 *    copy <fromfilename> <tofilename>, cp <fromfilename> <tofilename>
 *
 */

#include <xymodem.h>

XYmodem rxymodem;

// CLI globals
char cwd[128+1];     // Current Working Directory
bool CaptureMode = false;
bool XYmodemMode = false;
File CaptureFile;

void setup() {
  // Initialize serial port and wait for it to open before continuing.
  // But do not wait forever!
  Serial.begin(115200);
  while (!Serial && millis() < 2000) {
    delay(100);
  }
  Serial.println("FatFs Command Line Interpreter");
  Serial1.begin(115200);

  if (rxymodem.begin()) {
    Serial.println("Flash filesystem setup failed");
  }
  else {
    Serial.println("Flash filesystem ready for use");
  }

  setup_cli();
}

typedef void (*action_func_t)(char *aLine);

typedef struct {
  char command[7+1];    // max 7 characters plus '\0' terminator
  action_func_t action;
} command_action_t;

int make_full_pathname(char *name, char *pathname, size_t pathname_len)
{
  if (name == NULL || name == '\0') return -1;
  if (pathname == NULL || pathname_len == 0) return -1;

  // if dir name starts with '/', it is a full pathname so make it.
  // else form full pathname with current working directory.
  if (*name == '/') {
    strncpy(pathname, name, pathname_len);
    pathname[pathname_len-1] = '\0';
  }
  else {
    strcpy(pathname, cwd);
    if (cwd[strlen(cwd)-1] == '/') {
      if (strlen(cwd) + strlen(name) >= pathname_len) {
        Serial.println("pathname too long");
        return -2;
      }
    }
    else {
      if (strlen(cwd) + 1 + strlen(name) >= pathname_len) {
        Serial.println("pathname too long");
        return -2;
      }
      strcat(pathname, "/");
    }
    strcat(pathname, name);
  }
  return 0;
}

void remove_file(char *aLine) {
  char *filename = strtok(NULL, " \t");
  char pathname[128+1];
  if (make_full_pathname(filename, pathname, sizeof(pathname)) != 0) return;
  // Delete a file with the remove command.  For example create a test2.txt file
  // inside /test/foo and then delete it.
  if (!FATFILESYS.remove(pathname)) {
    Serial.println("Error, couldn't delete file!");
    return;
  }
}

void change_dir(char *aLine) {
  char *dirname = strtok(NULL, " \t");
  char pathname[128+1];

  if (make_full_pathname(dirname, pathname, sizeof(pathname)) != 0) return;
  if ((strcmp(pathname, "/") != 0) && !FATFILESYS.exists(pathname)) {
    Serial.println("Directory does not exist.");
    return;
  }
  File d = FATFILESYS.open(pathname);
  if (d.isDirectory()) {
    strcpy(cwd, pathname);
  }
  else {
    Serial.println("Not a directory");
  }
  d.close();
}

void make_dir(char *aLine) {
  char *dirname = strtok(NULL, " \t");
  char pathname[128+1];

  if (make_full_pathname(dirname, pathname, sizeof(pathname)) != 0) return;
  // Check if directory exists and create it if not there.
  // Note you should _not_ add a trailing slash (like '/test/') to directory names!
  // You can use the same exists function to check for the existance of a file too.
  if (!FATFILESYS.exists(pathname)) {
    // Use mkdir to create directory (note you should _not_ have a trailing slash).
    if (!FATFILESYS.mkdir(pathname)) {
      Serial.println("Error, failed to create directory!");
      return;
    }
  }
}

void remove_dir(char *aLine) {
  // Delete a directory with the rmdir command.  Be careful as
  // this will delete EVERYTHING in the directory at all levels!
  // I.e. this is like running a recursive delete, rm -rf, in
  // unix filesystems!
  char *dirname = strtok(NULL, " \t");
  char pathname[128+1];

  if (make_full_pathname(dirname, pathname, sizeof(pathname)) != 0) return;
  if (!FATFILESYS.rmdir(pathname)) {
    Serial.println("Error, couldn't delete test directory!");
    return;
  }
  // Check that test is really deleted.
  if (FATFILESYS.exists(pathname)) {
    Serial.println("Error, test directory was not deleted!");
    return;
  }
}

void print_dir(char *aLine) {
  File dir = FATFILESYS.open(cwd);
  if (!dir) {
    Serial.println("Directory open failed");
    return;
  }
  if (!dir.isDirectory()) {
    Serial.println("Not directory");
    dir.close();
    return;
  }
  File child = dir.openNextFile();
  while (child) {
    // Print the file name and mention if it's a directory.
    Serial.print(child.name());
    Serial.print(" "); Serial.print(child.size(), DEC);
    if (child.isDirectory()) {
      Serial.print(" <DIR>");
    }
    Serial.println();
    // Keep calling openNextFile to get a new file.
    // When you're done enumerating files an unopened one will
    // be returned (i.e. testing it for true/false like at the
    // top of this while loop will fail).
    child = dir.openNextFile();
  }
}

void print_file(char *aLine) {
  char *filename = strtok(NULL, " \t");
  char pathname[128+1];

  if (make_full_pathname(filename, pathname, sizeof(pathname)) != 0) return;
  File readFile = FATFILESYS.open(pathname, FILE_READ);
  if (!readFile) {
    Serial.println("Error, failed to open file for reading!");
    return;
  }
  readFile.setTimeout(0);
  while (readFile.available()) {
    char buf[512];
    size_t bytesIn = readFile.readBytes(buf, sizeof(buf));
    if (bytesIn > 0) {
      Serial.write(buf, bytesIn);
    }
    else {
      break;
    }
  }

  // Close the file when finished reading.
  readFile.close();
}

void capture_file(char *aLine) {
  char *filename = strtok(NULL, " \t");
  char pathname[128+1];

  if (make_full_pathname(filename, pathname, sizeof(pathname)) != 0) return;
  CaptureFile = FATFILESYS.open(pathname, FILE_WRITE);
  if (!CaptureFile) {
    Serial.println("Error, failed to open file!");
    return;
  }
  CaptureMode = true;
}

void print_working_dir(char *aLine) {
  Serial.println(cwd);
}

void recv_xmodem(char *aLine) {
  char *filename = strtok(NULL, " \t");

  rxymodem.start_rx(&XMODEM_PORT, filename, true, true);
  XYmodemMode = true;
}

void recv_ymodem(char *aLine) {
  rxymodem.start_rb(&XMODEM_PORT, &FATFILESYS, true, true);
  XYmodemMode = true;
}

const command_action_t commands[] = {
  // Name of command user types, function that implements the command.
  {"dir", print_dir},
  {"ls", print_dir},
  {"pwd", print_working_dir},
  {"cd", change_dir},
  {"mkdir", make_dir},
  {"rmdir", remove_dir},
  {"del", remove_file},
  {"rm", remove_file},
  {"type", print_file},
  {"cat", print_file},
  {"capture", capture_file},
  {"rx", recv_xmodem},
  {"rb", recv_ymodem},
  {"help", print_commands},
  {"?", print_commands},
};

// force lower case
void toLower(char *s) {
  while (*s) {
    if (isupper(*s)) {
      *s += 0x20;
    }
    s++;
  }
}

void print_commands(char *aLine) {
  Serial.print(commands[0].command);
  for (size_t i = 1; i < sizeof(commands)/sizeof(commands[0]); i++) {
    Serial.print(','); Serial.print(commands[i].command);
  }
  Serial.println();
}

void execute(char *aLine) {
  if (aLine == NULL || *aLine == '\0') return;
  char *cmd = strtok(aLine, " \t");
  if (cmd == NULL || *cmd == '\0') return;
  toLower(cmd);
  for (size_t i = 0; i < sizeof(commands)/sizeof(commands[0]); i++) {
    if (strcmp(cmd, commands[i].command) == 0) {
      commands[i].action(aLine);
      return;
    }
  }
  Serial.println("command not found");
}

void setup_cli() {
  Serial.setTimeout(0);
  strcpy(cwd, "/");
  Serial.print("$ ");
}

uint8_t bytesIn;
char aLine[80+1];

void loop_cli() {
  if (XYmodemMode){
    if (rxymodem.loop() == 0) {
      XYmodemMode = false;
      Serial.println();
      Serial.print("$ ");
    }
  }
  else {
    if (Serial.available() > 0) {
      int b = Serial.read();
      if (CaptureMode) {
        if (b == 0x04) {   // ^D end of input
          CaptureMode = false;
          // Close the file when finished reading.
          CaptureFile.close();
          Serial.print("$ ");
        }
        if (b != -1) {
          CaptureFile.print((char)b);
        }
      }
      else {
        if (b != -1) {
          switch (b) {
            case '\n':
              break;
            case '\r':
              Serial.println();
              aLine[bytesIn] = '\0';
              execute(aLine);
              bytesIn = 0;
              if (!CaptureMode) Serial.print("$ ");
              break;
            case '\b':  // backspace
              if (bytesIn > 0) {
                bytesIn--;
                Serial.print((char)b); Serial.print(' '); Serial.print((char)b);
              }
              break;
            case 0x03:  // ^C
              Serial.println("^C");
              bytesIn = 0;
              Serial.print("$ ");
              break;
            default:
              Serial.print((char)b);
              aLine[bytesIn++] = (char)b;
              if (bytesIn >= sizeof(aLine)-1) {
                aLine[bytesIn] = '\0';
                execute(aLine);
                bytesIn = 0;
                if (!CaptureMode && !XYmodemMode) Serial.print("$ ");
              }
              break;
          }
        }
      }
    }
  }
}

void loop() {
  loop_cli();
}
