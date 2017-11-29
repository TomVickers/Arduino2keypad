// file Volts.cpp - class for handling reading and outputing project voltages

#include "Volts.h"

// init
void Volts::init(void)
{
    for (uint8_t i=0; i < NUM_VOLTS; i++)
    {
        rail[i] = 0;
    }
}

// read the voltage rails. Returns: none
void Volts::read(void)
{
    int32_t v;

    for (uint8_t i=0; i < NUM_VOLTS; i++)
    {
        v = analogRead(pin[i]) * scale[i];
        rail[i] = (v >> 10) & 0x0FFFF;  // rail value is in 100ths of volts
    }
}

// generate voltages mesg in text string for sending to USB serial
// FIXME - part of this function should be moved to the USBprotocol class
void Volts::getMsg(char * buf, uint8_t bufLen)
{
    uint8_t idx = 0;
    idx += sprintf(buf+idx, "VOLTS[%02d] ", 3);
    for (uint8_t i=0; i < NUM_VOLTS && bufLen - idx > 6; i++)
    {
        idx += sprintf(buf+idx, "0x%04x ", rail[i]);
    }
    sprintf(buf+idx-1, "\n");
}

