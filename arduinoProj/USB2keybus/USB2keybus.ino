// USB2keybus.ino - implement adapter between alarm processor serial port and keybus protocol keypad

#include "PiSerial.h"  // declare first to define SERIAL_RX_BUFFER_SIZE
#include <Arduino.h>
#include "KeypadSerial.h"
#include "USBprotocol.h"
#include "Volts.h"

#define PRINT_BUF_SIZE   (128)
static char pBuf[PRINT_BUF_SIZE];  // sprintf buffer

static const uint32_t KP_POLL_PERIOD =  330;  // how often to poll keypad (ms)
static const uint32_t KP_F7_PERIOD   = 4000;  // how often to send F7 status message (ms)
static const uint32_t VOLT_PERIOD    = 5000;  // how often to sample the voltage rails (ms)
static const uint32_t MIN_TX_GAP     =   50;  // allow at least this many ms between transmits
static const uint32_t READ_KEY_DELAY =   40;  // delay between keypad poll response and keypad read

PiSerial     piSerial;     // piSerial class
KeypadSerial kpSerial;     // keypadSerial class
USBprotocol  usbProtocol;  // protocol class for converting msgs to/from USB serial
Volts        volts;        // voltage monitoring class

uint32_t kpF7time;       // global, last time F7 message sent
uint32_t kpPollTime;     // global, last time keypad was polled
uint32_t voltTime;       // global, last time volt message sent
uint32_t lastSendTime;   // global, last time message sent to keypad

bool     keyPadRead;     // if true, in keypad read mode
uint8_t  keyPad;         // next keypad to read
uint8_t  numKeyPads;     // number of keypads that responded to poll

// ------------------------------------------ setup -----------------------------------------

void setup(void)
{
    usbProtocol.init();     // init class
    piSerial.init();        // init class
    kpSerial.init();        // init class
    volts.init();           // init class

    uint32_t ms = millis();
  
    kpF7time = ms;
    kpPollTime = ms;
    voltTime = ms;
    lastSendTime = ms;

    keyPadRead = false;
    keyPad = 0;
    numKeyPads = 0;
}

// ---------------------------------------- main loop ---------------------------------------

void loop(void)
{
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

        uint8_t msgType = usbProtocol.parseRecv(piMsg, piMsgSize);

        if (msgType == 0xF7)
        {
            kpF7time = 0;  // always update keypad as soon as new F7 message arrives
        }
        else if (msgType == 0)
        {
            // unknown console message
            sprintf(pBuf, "ERR_FMT: garble/bad msg format '%s'\n", piMsg);
            piSerial.write(pBuf);
        }
        piSerial.clearCmd();                       // mark command as processed
    }

    uint32_t ms = millis();  // milliseconds since start of run

    if (keyPadRead)  // we are in keypad read mode
    {
        if (ms - kpPollTime > READ_KEY_DELAY)  // after waiting the appropriate time after polling, read the keypad data
        {
            // read the next keypad, send message to USB serial
            lastSendTime = kpPollTime = ms;

            uint8_t msgType = kpSerial.requestData(keyPad);

            if (msgType == KEYS_MESG)       // if true, key presses were returned for this keypad
            {
                piSerial.write(usbProtocol.keyMsg(pBuf, PRINT_BUF_SIZE,
                    kpSerial.getAddr(keyPad), kpSerial.getKeyCount(), kpSerial.getKeys(), msgType));
            }
            else if (msgType != NO_MESG)  // we received some other type of message
            {
                piSerial.write(usbProtocol.keyMsg(pBuf, PRINT_BUF_SIZE, 
                    kpSerial.getAddr(keyPad), kpSerial.getRecvMsgLen(), kpSerial.getRecvMsg(), msgType));
            }

            if (++keyPad >= numKeyPads)  // this was the last keypad with data
            {
                keyPad = numKeyPads = 0;
                keyPadRead = false;  // end keypad read mode
            }
            else
            {
                // still in keyPadRead mode
            }
        }
    }
    else // not in a keypad read cycle, check if time to poll keypad, send F7 msg or send volt msg
    {
        if (ms - lastSendTime > MIN_TX_GAP)  // min time gap between any type of msg
        {
            if (ms - kpPollTime > KP_POLL_PERIOD)
            {
                lastSendTime = kpPollTime = ms;
                if (kpSerial.poll())
                {
                    keyPadRead = true;
                    keyPad = 0;  // start with first keypad that responded
                    numKeyPads = kpSerial.getNumKeypads();
                }
            }
            else if (ms - kpF7time > KP_F7_PERIOD)
            {
                // time to send a F7 status message to keypad
                lastSendTime = kpF7time = ms;
                kpSerial.write(usbProtocol.getF7(), usbProtocol.getF7size());
            }
            else if (ms - voltTime > VOLT_PERIOD)    // if time to sample voltage rails
            {
                lastSendTime = voltTime = ms;
#if 0
                volts.read();
                volts.getMsg(pBuf, PRINT_BUF_SIZE);  // generate volts msg
                piSerial.write(pBuf);
#endif
            }
        }
    }
}

