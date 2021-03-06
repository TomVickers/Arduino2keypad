# Arduino2keypad
This repository is the companion of https://github.com/TomVickers/RPIalarm

This repository contains code to interface a 6160 alarm keypad with an Arduino.  If you are interested in using a 6160 alarm keypad (commonly found connected to a variety of Honeywell alarm systems like the Vista-20p), this code may be useful to you.

As with most things in life, my work on this is simply an extension of the work by others.  I found Mark Kimsal's website (http://getatanc.com/) and github repository (https://github.com/TANC-security/keypad-firmware) very helpful.  Most of what you will find here, I found first on his pages.  I have extended it a bit, and worked out some details of communicating directly with the keypad (his task involved talking to the alarm as a keypad).

The doc directory contains notes, circuit diagrams, etc.  If you find something is wrong (or learn something new), please let me know and I will try to update them as we learn more about this device.

The ArduinoProj directory contains the Arduino project named USB2keybus.  I build it using Arduino software (version 1.8.5) on a Mega 2560.  It will probably run on other Arduino processors with minor changes.  I can also build it on my alarm Raspberry PI using the provided Makefile.  There are some notes in the comments at the top of the Makefile that indicate which packages you must install to enable cross compiling for the Arduino.  You will notice that the project uses a modified version of the SoftwareSerial lib.  All the modifications in my ModSoftwareSerial files are marked with the comment NON_STANDARD (in case you want to port these changes to a different version of SoftwareSerial).

--------------- NOTE: Beta code ------------------------

This code is a work in progress.  A few features are not yet complete, but it appears to be stable.  I am currently using it as a bi-directional parser between a Raspberry Pi 3 USB serial port at 115200 baud and a 6160 keypad.

