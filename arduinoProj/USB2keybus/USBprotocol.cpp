// file F7msg.cpp - class for handling F7 message

#include "F7msg.h"

void F7msg::init(void)
{
    memset((void *)&mesg, 0, sizeof(mesg));  // zero message bytes

    // constant values
    mesg.type    = 0xF7;
    mesg.keypads = 0xFF;  // send to all keypads
    mesg.addr4   = 0x10;  // unknown value, my alarm panel sends this
    mesg.prog    = 0x02;  // ?????
}

// debug: print message struct into buf
char * F7msg::print(char * buf)
{
    uint8_t idx = 0;
    idx += sprintf(buf+idx, "%02x mesg -> kp[%02x]\n", mesg.type, mesg.keypads);
    idx += sprintf(buf+idx, "  zone=%02x, beep=%02x, chime=%c, power=%c\n", mesg.zone, mesg.byte1, 
               GET_CHIME(mesg.byte3) ? '1' : '0', GET_POWER(mesg.byte3) ? '1' : '0');
    idx += sprintf(buf+idx, "  ready=%c, armed-away=%c, armed-stay=%c\n", GET_READY(mesg.byte2) ? '1' : '0', 
        GET_ARMED_AWAY(mesg.byte3) ? '1' : '0', GET_ARMED_STAY(mesg.byte2) ? '1' : '0');
    idx += sprintf(buf+idx, "  line1='");
    for (uint8_t i=0; i < 16; i++)
        *(buf+idx+i) = *(mesg.line1+i) & 0x7F;
    idx += 16;
    idx += sprintf(buf+idx, "'\n");
    idx += sprintf(buf+idx, "  line2='");
    for (uint8_t i=0; i < 16; i++)
        *(buf+idx+i) = *(mesg.line2+i) & 0x7F;
    idx += 16;
    idx += sprintf(buf+idx, "'\n");
    idx += sprintf(buf+idx, "checksum %02x\n", mesg.chksum);
    return buf;
}

// Form of serial F7 message, hex=2-upper-case-digits, bool=1-digit(0|1) 
// F7 z=zone(hex) b=beep(hex) c=chime(bool) r=ready a=armed-away(bool) s=armed-stay(bool) p=power(bool)
//    x=led_backlight(bool) 1=(16 chars line 1) 2=(16 chars line 2)

// BYTE1 beep notes
//   00-03 - low two bits define chime count for each F7 msg (0 none, 1,2,3 chime count per msg)
//   04    - fast constant beep (like there is an error)
//   05-06 - slow constant beep (like when alarm is armed and it is time to leave)
//   07    - continous tone (not pulsing)
//   bits above bottom 3 don't do anything, 0x40 bit causes incompat. con. error

// BYTE2 notes: bit(0x80) 1 -> ARMED-STAY, bit(0x10) 1 -> READY (1 when ok, 0 when exit delay)
// BYTE3 notes: bit(0x20) 1 -> chime on, bit(0x08) 1 -> ac power ok, bit(0x04) 1 -> ARMED_AWAY

// f7msg.parse("F7 z=FC b=0 c=1 r=0 a=0 s=0 p=1 x=1 1=1234567890123456 2=ABCDEFGHIJKLMNOP", 70); 

// parse input ascii F7 message
void F7msg::parse(const char input[], uint8_t size)
{
    bool lcd_backlight = false;

    for (uint8_t i=3; i < size && input[i] != '\0'; i++)  // start after 'F7 '
    {
        char parm = input[i];
        i += 2;  // move i past parm and '=' 

        switch (parm)
        {
        case 'z':
            mesg.zone = GET_BYTE(input[i], input[i+1]); i += 2;
            break;
        case 'b':
            mesg.byte1 = GET_NIBBLE(input[i]); i++;   // post increment inside the macro gets mangled
            break;
        case 'c':
            mesg.byte3 = SET_CHIME(mesg.byte3, GET_BOOL(input[i++]));
            break;
        case 'r':
            mesg.byte2 = SET_READY(mesg.byte2, GET_BOOL(input[i++]));
            break;
        case 'a':
            mesg.byte3 = SET_ARMED_AWAY(mesg.byte3, GET_BOOL(input[i++]));
            break;
        case 's':
            mesg.byte2 = SET_ARMED_STAY(mesg.byte2, GET_BOOL(input[i++]));
            break;
        case 'p':
            mesg.byte3 = SET_POWER(mesg.byte3, GET_BOOL(input[i++]));
            break;
        case 'x':
            lcd_backlight = GET_BOOL(input[i++]);
            break;
        case '1':
            mesg.line1[0] = (input[i++] & 0x7f) | (lcd_backlight ? 0x80 : 0x00); 
            for (uint8_t j=1; j < 16; j++)
            {
                mesg.line1[j] = input[i++] & 0x7f;
            }
            break;
        case '2':
            for (uint8_t j=0; j < 16; j++)
            {
                mesg.line2[j] = input[i++] & 0x7f;
            }
            break;
        default:
            break;
        }
    }

    mesg.chksum = 0;

    for (uint8_t i=0; i < 44; i++)
    {
        mesg.chksum += *(((uint8_t *)&mesg) + i);
    }
        
    mesg.chksum = 0x100 - mesg.chksum;
}

const uint8_t * F7msg::get(void)
{
    return (const uint8_t *)&mesg;
}
