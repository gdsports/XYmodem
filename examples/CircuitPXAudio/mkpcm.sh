#!/bin/bash
# sudo apt-get install espeak audacity
espeak -a 200 -w button1.wav "button 1"
espeak -a 200 -w button2.wav "button 2"
espeak -a 200 -w captch1.wav "cap touch 1"
# Use audacity to convert WAV to RAW (8 bit samples, 11025 samples/sec, mono).
# Rename the RAW files to PCM.
