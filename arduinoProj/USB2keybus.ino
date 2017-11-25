// USB2keybus.ino - implement adapter between alarm processor serial port and keybus protocol keypad

#include <Arduino.h>
#include "PiSerial.h"
#include "KeypadSerial.h"
#include "F7msg.h"
#include "Volts.h"

#define PRINT_BUF_SIZE   (128)
static char pBuf[PRINT_BUF_SIZE];  // sprintf buffer

static const uint32_t KP_POLL_PERIOD =  330;  // how often to poll keypad (ms)
static const uint32_t KP_F7_PERIOD   = 4000;  // how often to send F7 status message (ms)
static const uint32_t VOLT_PERIOD    = 5000;  // how often to sample the voltage rails (ms)
static const uint32_t MIN_TX_GAP     =   50;  // allow at least this many ms between transmits

PiSerial     piSerial;     // piSerial class
KeypadSerial kpSerial;     // keypadSerial class
USBprotocol  usbProtocol;  // protocol class for converting msgs to/from USB serial
Volts        volts;        // voltage monitoring class

uint32_t kpF7time;       // global, last time F7 message sent
uint32_t kpPollTime;     // global, last time keypad was polled
uint32_t voltTime;       // global, last time volt message sent
uint32_t lastSendTime;   // global, last time message sent to keypad

// ------------------------------------------ setup -----------------------------------------

void setup()
{
    usbProtocol.init();     // init class
    piSerial.init();        // init class
    kpSerial.init();        // init class
    volts.init();           // init class

    kpF7time = 0;
    kpPollTime = 0;
    voltTime = 0;
    lastSendTime = 0;
}

// ---------------------------------------- main loop ---------------------------------------

// debug (known good F7 message from working alarm)
static const uint8_t recF7msg[] =
    { 0xF7, 0x00, 0x00, 0xFF,   // F7, 2 zeros, keypads that should accept mesg
      0x10, 0xFC, 0x00, 0x00,   // addr4, zone, BYTE1, BYTE2
      0x28, 0x02, 0x00, 0x00,   // BYTE3, prog, prompt pos, unknown
      0xC3, 0x4F, 0x4D, 0x4D,   // message 'COMM' (first byte high bit set for backlight on)
      0x2E, 0x20, 0x46, 0x41,   // message '. FA'
      0x49, 0x4C, 0x55, 0x52,   // message 'ILUR'
      0x45, 0x20, 0x20, 0x20,   // message 'E   '
      0x20, 0x20, 0x20, 0x20,   // message '    '
      0x20, 0x20, 0x20, 0x20,   // message '    '
      0x20, 0x20, 0x20, 0x20,   // message '    '
      0x20, 0x20, 0x20, 0x20,   // message '    '
      0x72, 0x00, 0x00, 0x00 }; // checksum, 3-bytes of pad
      
void loop()
{
extern uint8_t dbg1;
extern uint8_t dbg2;

    uint8_t k = 0;

    if (kpSerial.read(&k, 0)) // if we have unhandled chars from keypad, consume them
    {
        sprintf(pBuf, "WARN: unhandled keypad char %02x\n", k);
        piSerial.write(pBuf);
    }

    if (piSerial.read())    // read any data available from console serial port
    {
        uint8_t piMsgSize = 0;
        const char * piMsg = piSerial.getMsg(&piMsgSize);

        usbProtocol.parse(piMsg, piMsgSize);
#if 0
        if (F7_MSG(piMsg))
        {
            f7msg.parse(piMsg, piMsgSize);         // parse the recvd message
            piSerial.write(f7msg.print(pBuf));     // debug: write back mesg received
        }
        else // unknown console message
        {
            sprintf(pBuf, "ERR: msg format '%s'\n", piMsg);
            piSerial.write(pBuf);
        }
#endif
        piSerial.clearCmd();                       // mark command as processed
    }

    uint32_t ms = millis();  // milliseconds since start of run

    if (ms - lastSendTime > MIN_TX_GAP &&
        ms - kpPollTime > KP_POLL_PERIOD)    // time to poll keypad?
    {
        lastSendTime = kpPollTime = ms;
        if (kpSerial.poll())                       // poll for keypads
        {
            for (uint8_t kp=0; kp < kpSerial.getNumKeypads(); kp++) // loop over each responding keypad
            {
                _delay_ms(50);  // delay about 50ms before requesting data

                uint8_t msgType = kpSerial.requestData(kp);

                if (msgType == KEYS_MESG)       // if true, key presses were returned for this keypad
                {
                    piSerial.write(keyMsg(pBuf, PRINT_BUF_SIZE,
                        kpSerial.getAddr(kp), kpSerial.getKeyCount(), kpSerial.getKeys()), msgType);
                }
                else if (msgType != NO_MESG)  // we received some other type of message
                {
                    piSerial.write(keyMsg(pBuf, PRINT_BUF_SIZE,
                        kpSerial.getAddr(kp), kpSerial.getRecvMsgLen(), kpSerial.getRecvMsg()), msgType);
                }

if (dbg1 != dbg2)
{
  sprintf(pBuf, "DBG: kp%d checksum failed, calc 0x%02x, recv 0x%02x\n", kpSerial.getAddr(kp), dbg1, dbg2);
  piSerial.write(pBuf);
}
            }
        }
    }
    else if (ms - lastSendTime > MIN_TX_GAP &&
             ms -  kpF7time > KP_F7_PERIOD)       // time to send a F7 status message to keypad?
    {
        lastSendTime = kpF7time = ms;
        kpSerial.write(f7msg.get(), F7_MSG_SIZE);
        //kpSerial.write(recF7msg, F7_MSG_SIZE);
        // does not look like keypad acks F7 mesg
    }
    else if (ms - voltTime > VOLT_PERIOD)    // if time to sample voltage rails
    {
        voltTime = ms;
#if 0
        volts.read();
        volts.getMsg(pBuf, PRINT_BUF_SIZE);  // generate volts msg
        piSerial.write(pBuf);
#endif
    }
}
^M
