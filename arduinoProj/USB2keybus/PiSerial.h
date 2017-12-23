// file PiSerial.h - Serial class for handling serial com with Raspberry PI

#pragma once

#define SERIAL_RX_BUFFER_SIZE  256    // increase default size for arduino serial recv buffer

#include <Arduino.h>

#define PI_SERIAL_BAUD      115200   // baud rate for USB serial port

static const uint8_t PI_SERIAL_MSG_BUF_SIZE = 128;  // max length of recv'd msg

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
