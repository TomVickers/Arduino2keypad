// file USBprotocol.cpp - methods for converting data into plain text strings transferred over USB serial

#include "USBprotocol.h"
#include "KeypadSerial.h"

#define F7_MSG_ALT(s)        (*((s)+0) == 'F' && *((s)+1) == '7' && *((s)+2) == 'A')
#define F7_MSG(s)            (*((s)+0) == 'F' && *((s)+1) == '7')

void USBprotocol::init(void)
{
    count = 0;
    
    // init F7 message structs
    initF7(&msgF7[0]);
    initF7(&msgF7[1]);

    //parseF7("F7 z=FC t=0 c=1 r=1 a=0 s=0 p=0 b=1 1=1234567890123456 2=ABCDEFGHIJKLMNOP", 70);
    parseRecv("F7 z=FC t=0 c=1 r=0 a=1 s=0 p=0 b=1 1=Armed      12:58 2=Welcome Home    ", 70);
    parseRecv("F7A z=00 t=0 c=1 r=1 a=0 s=0 p=1 b=1 1=Disarmed   12:22 2=Welcome Home    ", 70);
}

void USBprotocol::initF7(t_MesgF7 * pMsgF7)
{
    memset((void *)pMsgF7, 0, sizeof(t_MesgF7));

    // constant values
    pMsgF7->type    = 0xF7;
    pMsgF7->keypads = 0xFF;  // send to all keypads
    pMsgF7->addr4   = 0x00;  // unknown value, my alarm panel is observed to send 0x10
    pMsgF7->prog    = 0x00;  // programming mode (not used)
}

uint8_t USBprotocol::parseRecv(const char * msg, const uint8_t len)
{
    if (F7_MSG_ALT(msg))
    {
        return parseF7(msg+4, len-4, &msgF7[1]);
    }
    else if (F7_MSG(msg))
    {
        parseF7(msg+3, len-3, &msgF7[1]);
        return parseF7(msg+3, len-3, &msgF7[0]);
    }

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

// parse F7 message, form is F7[A] z=FC t=0 c=1 r=0 a=0 s=0 p=1 b=1 1=1234567890123456 2=ABCDEFGHIJKLMNOP
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

uint8_t USBprotocol::parseF7(const char * msg, uint8_t len, t_MesgF7 * pMsgF7)
{   
    bool lcd_backlight = false;

    for (uint8_t i=0; i < len && *(msg+i) != '\0'; i++)  // msg pointer starts after 'F7 ' or 'F7A '
    {
        char parm = *(msg+i);
        i += 2;  // move i past parm and '=' 

        switch (parm)
        {
        case 'z':
            pMsgF7->zone = GET_BYTE(*(msg+i), *(msg+i+1)); i += 2;
            break;
        case 't':
            pMsgF7->byte1 = GET_NIBBLE(*(msg+i)); i++;
            break;
        case 'c':
            pMsgF7->byte3 = SET_CHIME(pMsgF7->byte3, GET_BOOL(*(msg+i))); i++;
            break;
        case 'r':
            pMsgF7->byte2 = SET_READY(pMsgF7->byte2, GET_BOOL(*(msg+i))); i++;
            break;
        case 'a':
            pMsgF7->byte3 = SET_ARMED_AWAY(pMsgF7->byte3, GET_BOOL(*(msg+i))); i++;
            break;
        case 's':
            pMsgF7->byte2 = SET_ARMED_STAY(pMsgF7->byte2, GET_BOOL(*(msg+i))); i++;
            break;
        case 'p':
            pMsgF7->byte3 = SET_POWER(pMsgF7->byte3, GET_BOOL(*(msg+i))); i++;
            break;
        case 'b':
            lcd_backlight = GET_BOOL(*(msg+i)); i++;
            break;
        case '1':
            pMsgF7->line1[0] = (*(msg+i) & 0x7f) | (lcd_backlight ? 0x80 : 0x00); i++;
            for (uint8_t j=1; j < 16; j++)
            {
                pMsgF7->line1[j] = *(msg+i) & 0x7f; i++;
            }
            break;
        case '2':
            for (uint8_t j=0; j < 16; j++)
            {
                pMsgF7->line2[j] = *(msg+i) & 0x7f; i++;
            }
            break;
        default:
            break;
        }
    }

    pMsgF7->chksum = 0;

    for (uint8_t i=0; i < 44; i++)
    {
        pMsgF7->chksum += *(((uint8_t *)pMsgF7) + i);
    }

    pMsgF7->chksum = 0x100 - pMsgF7->chksum;

    return 0xF7;
}

const uint8_t * USBprotocol::getF7(void)
{ 
    return (const uint8_t *)&(msgF7[count++ & 0x1]);  // returned mesg alternates between 2 stored messages (which may be the same)
}


#if 0
// debug: print message struct into buf
const char * USBprotocol::printF7(char * buf)
{
    uint8_t idx = 0;
    idx += sprintf(buf+idx, "%02x msg -> kp[%02x]\n", pMsgF7->type, pMsgF7->keypads);
    idx += sprintf(buf+idx, "  zone=%02x, tone=%1x, chime=%c, power=%c\n", pMsgF7->zone, pMsgF7->byte1,
               GET_CHIME(pMsgF7->byte3) ? '1' : '0', GET_POWER(pMsgF7->byte3) ? '1' : '0');
    idx += sprintf(buf+idx, "  ready=%c, armed-away=%c, armed-stay=%c\n", GET_READY(pMsgF7->byte2) ? '1' : '0',
        GET_ARMED_AWAY(pMsgF7->byte3) ? '1' : '0', GET_ARMED_STAY(pMsgF7->byte2) ? '1' : '0');
    idx += sprintf(buf+idx, "  line1='");
    for (uint8_t i=0; i < 16; i++)
        *(buf+idx+i) = *(pMsgF7->line1+i) & 0x7F;
    idx += 16;
    idx += sprintf(buf+idx, "'\n");
    idx += sprintf(buf+idx, "  line2='");
    for (uint8_t i=0; i < 16; i++)
        *(buf+idx+i) = *(pMsgF7->line2+i) & 0x7F;
    idx += 16;
    idx += sprintf(buf+idx, "'\n");
    idx += sprintf(buf+idx, "checksum %02x\n", pMsgF7->chksum);
    return (const char *)buf;
}
#endif

