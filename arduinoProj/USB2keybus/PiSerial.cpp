// file PiSerial.cpp - Serial class for handling serial com with Raspberry PI

#include "PiSerial.h"

void PiSerial::clearCmd(void)
{
    cmdRecvd = false;
    bufIdx = 0;
    msgBuf[0] = '\0';
}

void PiSerial::init(void)
{
    // init USB serial connection to Raspberry PI
    Serial.begin(PI_SERIAL_BAUD);  
    sprintf(msgBuf, "\nUSB2keybus initialized, USB rx buf size %d\n", SERIAL_RX_BUFFER_SIZE);
    Serial.println(msgBuf);
    clearCmd();
}

void PiSerial::write(const char * buf)
{
    Serial.print(buf);
}

// read from the RPI serial port.  Returns: true if complete command recvd
bool PiSerial::read(void)
{
    while (!cmdRecvd && Serial.available() && bufIdx < PI_SERIAL_MSG_BUF_SIZE-1)
    {
        char c = Serial.read();
        if (c == '\n' || c == '\r')  // strip either type of line termination
        {
            if (bufIdx > 0) // don't create zero length commands
            {
                if (msgBuf[0] == 'F')  // commands always start with 'F'
                {
                    cmdRecvd = true;
                }
                else
                {
                    Serial.println("ERROR: garbled command\n");
                    clearCmd();
                }
            }
        }
        else
        {
            msgBuf[bufIdx++] = c;
        }
    }

    if (bufIdx >= PI_SERIAL_MSG_BUF_SIZE-1)
    {
        Serial.println("ERROR: buf overflow\n");
        clearCmd();
    }
    else
    {
        msgBuf[bufIdx] = '\0';  // keep current buffer null terminated
    }
    return cmdRecvd;
}

const char * PiSerial::getMsg(uint8_t * size)
{
    *size = cmdRecvd ? bufIdx : 0;
    return (const char *)msgBuf;
}
