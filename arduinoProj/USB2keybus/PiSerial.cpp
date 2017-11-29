// file PiSerial.cpp - Serial class for handling serial com with Raspberry PI

#include "PiSerial.h"

void PiSerial::clearCmd(void)
{
    cmdRecvd = false;
    bufIdx = 0;
    memset(msgBuf, 0, PI_SERIAL_MSG_BUF_SIZE);
}

void PiSerial::init(void)
{
    clearCmd();

    // init USB serial connection to Raspberry PI
    Serial.begin(PI_SERIAL_BAUD);  
    Serial.println("\nUSB2keybus initialized");
}

void PiSerial::write(const char * buf)
{
    Serial.print(buf);
}

// read from the RPI serial port.  Returns: true if complete command recvd
bool PiSerial::read(void)
{
    if (!cmdRecvd && Serial.available() && bufIdx < PI_SERIAL_MSG_BUF_SIZE-1)
    {
        do {
            char c = Serial.read();

            if (c == '\n') // current command has been terminated
            {
                cmdRecvd = true;
            }
            else if (c != '\r')  // ignore CRs
            {
                msgBuf[bufIdx++] = c;
            }
        } while (!cmdRecvd && Serial.available() && bufIdx < PI_SERIAL_MSG_BUF_SIZE-1);

        if (bufIdx >= PI_SERIAL_MSG_BUF_SIZE-1)
        {
            Serial.println("ERROR: command buffer overflow");
            clearCmd();
        }
    }
    return cmdRecvd;
}

const char * PiSerial::getMsg(uint8_t * size)
{
    *size = cmdRecvd ? bufIdx : 0;
    return (const char *)msgBuf;
}
