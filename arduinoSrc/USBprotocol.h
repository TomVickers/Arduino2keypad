// file USBprotocol.h - class describing serial protocol over USB

// if you want to change how your CPU communicates with the Arduino, update this class

#pragma once

#include <Arduino.h>
#include "F7msg.h"

class USBprotocol
{
public:
    USBprotocol(void) {}                            // Class constructor.  Returns: none

    void init(void);                                // init the class

    const char * keyMsg(char * buf, uint8_t bufLen, uint8_t addr, uint8_t len, uint8_t * pData, uint8_t type);
    uint8_t      parseRecv(const char * msg, const uint8_t len);
    const char * printF7(char * buf);

    const uint8_t * getF7(void) { return (const uint8_t *)&msgF7; }

private:
    t_MesgF7 msgF7;

    void initF7(void);
    uint8_t parseF7(const char * msg, uint8_t len);
};


// file USBprotocol.cpp - methods for converting data into plain text strings transferred over USB serial

void USBprotocol::init(void)
{
    initF7();
    parseF7("F7 z=FC b=0 c=1 r=0 a=0 s=0 p=1 x=1 1=1234567890123456 2=ABCDEFGHIJKLMNOP", 70);
}

uint8_t USBprotocol::parseRecv(const char * msg, const uint8_t len)
{
    if (F7_MSG(msg))
    {
        return parseF7(msg, len);
    }
#if 0
    else if (F2_MSG(msg))
    {
        return parseF2(msg, len);
    }
#endif
    return 0x0;
}

// message from keypad
const char * USBprotocol::keyMsg(char * buf, uint8_t bufLen, uint8_t addr, uint8_t len, uint8_t * pData, uint8_t type)
{
    uint8_t idx = 0;
    idx += sprintf(buf+idx, "%s_%2d[%02d] ", type == KEYS_MESG ? "KEYS" : "UNK_", addr, len);
    for (uint8_t i=0; i < len && bufLen - idx > 6; i++)
    {
        idx += sprintf(buf+idx, "0x%02x ", *(pData+i));
    }
    sprintf(buf+idx-1, "\n");
    return (const char *)buf;
}

void USBprotocol::initF7(void)
{
    memset((void *)&msgF7, 0, sizeof(msgF7));  // zero F7 message bytes

    // constant values
    msgF7.type    = 0xF7;
    msgF7.keypads = 0xFF;  // send to all keypads
    msgF7.addr4   = 0x10;  // unknown value, my alarm panel sends this
    msgF7.prog    = 0x02;  // ?????
}

// parse F7 message, form is F7 z=FC t=0 c=1 r=0 a=0 s=0 p=1 b=1 1=1234567890123456 2=ABCDEFGHIJKLMNOP
//   z - zone             (byte arg)
//   t - tone             (nibble arg)
//   c - chime            (bool arg)
//   r - ready            (bool arg)
//   a - arm-away         (bool arg)
//   s - arm-stay         (bool arg)
//   p - power-on         (bool arg)
//   b - lcd-backlight-on (bool arg)
//   1 - line1 text       (16-chars)
//   2 - line2 text       (16-chars)

// BYTE1 tone notes
//   00-03 - low two bits define chime count for each F7 msg (0 none, 1,2,3 chime count per msg)
//   04    - fast pulsing tone (like there is an error, or timeout almost done)
//   05-06 - slow pulsing tone (like when alarm is in arm-delay and it is time to leave)
//   07    - continous tone (not pulsing)
//   bits above bottom 3 don't do anything, 0x40 bit causes incompat. con. error

// BYTE2 notes: bit(0x80) 1 -> ARMED-STAY, bit(0x10) 1 -> READY (1 when ok, 0 when exit delay)
// BYTE3 notes: bit(0x20) 1 -> chime on, bit(0x08) 1 -> ac power ok, bit(0x04) 1 -> ARMED_AWAY

uint8_t USBprotocol::parseF7(const char * msg, uint8_t len)
{   
    bool lcd_backlight = false;

    for (uint8_t i=3; i < len && *(msg+i) != '\0'; i++)  // start after 'F7 '
    {
        char parm = *(msg+i);
        i += 2;  // move i past parm and '=' 

        switch (parm)
        {
        case 'z':
            msgF7.zone = GET_BYTE(*(msg+i), *(msg+i+1)); i += 2;
            break;
        case 't':
            msgF7.byte1 = GET_NIBBLE(*(msg+i)); i++;
            break;
        case 'c':
            msgF7.byte3 = SET_CHIME(msgF7.byte3, GET_BOOL(*(msg+i))); i++;
            break;
        case 'r':
            msgF7.byte2 = SET_READY(msgF7.byte2, GET_BOOL(*(msg+i))); i++;
            break;
        case 'a':
            msgF7.byte3 = SET_ARMED_AWAY(msgF7.byte3, GET_BOOL(*(msg+i))); i++;
            break;
        case 's':
            msgF7.byte2 = SET_ARMED_STAY(msgF7.byte2, GET_BOOL(*(msg+i))); i++;
            break;
        case 'p':
            msgF7.byte3 = SET_POWER(msgF7.byte3, GET_BOOL(*(msg+i))); i++;
            break;
        case 'b':
            lcd_backlight = GET_BOOL(*(msg+i)); i++;
            break;
        case '1':
            msgF7.line1[0] = (*(msg+i) & 0x7f) | (lcd_backlight ? 0x80 : 0x00); i++;
            for (uint8_t j=1; j < 16; j++)
            {
                msgF7.line1[j] = *(msg+i) & 0x7f; i++;
            }
            break;
        case '2':
            for (uint8_t j=0; j < 16; j++)
            {
                msgF7.line2[j] = *(msg+i) & 0x7f; i++;
            }
            break;
        default:
            break;
        }
    }

    msgF7.chksum = 0;

    for (uint8_t i=0; i < 44; i++)
    {
        msgF7.chksum += *(((uint8_t *)&msgF7) + i);
    }

    msgF7.chksum = 0x100 - msgF7.chksum;

    return 0xF7;
}

// debug: print message struct into buf
const char * USBprotocol::printF7(char * buf)
{
    uint8_t idx = 0;
    idx += sprintf(buf+idx, "%02x msg -> kp[%02x]\n", msgF7.type, msgF7.keypads);
    idx += sprintf(buf+idx, "  zone=%02x, tone=%1x, chime=%c, power=%c\n", msgF7.zone, msgF7.byte1,
               GET_CHIME(msgF7.byte3) ? '1' : '0', GET_POWER(msgF7.byte3) ? '1' : '0');
    idx += sprintf(buf+idx, "  ready=%c, armed-away=%c, armed-stay=%c\n", GET_READY(msgF7.byte2) ? '1' : '0',
        GET_ARMED_AWAY(msgF7.byte3) ? '1' : '0', GET_ARMED_STAY(msgF7.byte2) ? '1' : '0');
    idx += sprintf(buf+idx, "  line1='");
    for (uint8_t i=0; i < 16; i++)
        *(buf+idx+i) = *(msgF7.line1+i) & 0x7F;
    idx += 16;
    idx += sprintf(buf+idx, "'\n");
    idx += sprintf(buf+idx, "  line2='");
    for (uint8_t i=0; i < 16; i++)
        *(buf+idx+i) = *(msgF7.line2+i) & 0x7F;
    idx += 16;
    idx += sprintf(buf+idx, "'\n");
    idx += sprintf(buf+idx, "checksum %02x\n", msgF7.chksum);
    return (const char *)buf;
}


