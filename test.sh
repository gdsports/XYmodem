#!/bin/bash
IDEVER="1.8.8"
TEENSYVER="1.45"
WORKDIR="/tmp/autobuild_$$"
mkdir -p ${WORKDIR}
# Install Ardino IDE with teensyduino in work directory
TARFILE="${HOME}/Downloads/arduino-${IDEVER}-teensyduino-${TEENSYVER}.tbz"
if [ -f ${TARFILE} ]
then
    tar xf ${TARFILE} -C ${WORKDIR}
else
    exit -1
fi
# Create portable sketchbook and library directories
IDEDIR="${WORKDIR}/arduino-${IDEVER}"
LIBDIR="${IDEDIR}/portable/sketchbook/libraries"
mkdir -p "${LIBDIR}"
export PATH="${IDEDIR}:${PATH}"
cd ${IDEDIR}
which arduino
# Configure board package
BOARD="teensy:avr:teensy36"
arduino --pref "compiler.warning_level=none" \
        --pref "custom_usb=teensy36_serialhid" \
        --pref "custom_keys=teensy36_en-us" \
        --pref "custom_opt=teensy36_o2std" \
        --pref "custom_speed=teensy36_180" \
        --pref "update.check=false" \
        --pref "editor.external=true" --save-prefs
arduino --board "${BOARD}" --save-prefs
CC="arduino --verify --board ${BOARD}"
cd ${LIBDIR}
ln -s ~/Sync/Adafruit_SPIFlash/ .
ln -s ~/Sync/Adafruit_QSPI/ .
if [ -d ${HOME}/Sync/keymouse_t3/ ]
then
    ln -s ${HOME}/Sync/keymouse_t3/
else
    git clone https://github.com/gdsports/keymouse_t3
fi
if [ -d ${HOME}/Sync/XYmodem/ ]
then
    ln -s ${HOME}/Sync/XYmodem/
else
    git clone https://github.com/gdsports/XYmodem
fi
cd XYmodem/examples
find . -name '*.ino' -print0 | grep -zv Circuit | xargs -0 -n 1 $CC >/tmp/t36_$$.txt 2>&1
#
arduino --install-boards "arduino:samd"
arduino --pref "boardsmanager.additional.urls=https://adafruit.github.io/arduino-board-index/package_adafruit_index.json" --save-prefs
arduino --install-boards "adafruit:samd"
BOARD="adafruit:samd:adafruit_metro_m4"
CC="arduino --verify --board ${BOARD}"
find . -name '*.ino' -print0 | grep -zv Circuit | xargs -0 -n 1 $CC >/tmp/m4_$$.txt 2>&1
#
BOARD="adafruit:samd:adafruit_circuitplayground_m0"
CC="arduino --verify --board ${BOARD}"
find . -name '*.ino' -print0 | xargs -0 -n 1 $CC >/tmp/cpxm0_$$.txt 2>&1
#
BOARD="adafruit:samd:adafruit_itsybitsy_m0"
CC="arduino --verify --board ${BOARD}"
find . -name '*.ino' -print0 | grep -zv Circuit | xargs -0 -n 1 $CC >/tmp/ibm0_$$.txt 2>&1
