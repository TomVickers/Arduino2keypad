// file PiSerial.h - Serial class for handling serial com with Raspberry PI

#pragma once

#include <Arduino.h>

// raspberry PI serial config
#define PI_SERIAL_BAUD  115200

static const uint8_t PI_SERIAL_MSG_BUF_SIZE = 80;  // max length of recv'd msg

class PiSerial
{
public:
    PiSerial(void) {}                       // Class constructor.  Returns: none

    void init(void);                        // init the PiSerial class
    bool read(void);                        // poll the Pi for serial input
    void write(const char * buf);           // write buf to serial out
    void clearCmd(void);                    // clear the current command buf
    const char * getMsg(uint8_t * size);    // get serial message (if any)

private:
    char msgBuf[PI_SERIAL_MSG_BUF_SIZE];

    uint8_t bufIdx;
    bool    cmdRecvd;
};
