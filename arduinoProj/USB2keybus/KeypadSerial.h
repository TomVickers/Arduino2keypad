// file KeypadSerial.h - a class for handling com with alarm keypad

#pragma once

#include <Arduino.h>
#include "ModSoftwareSerial.h"

// i/o pins for software serial 
#define RX_PIN (12)
#define TX_PIN (11)

// responses from requestData func
#define NO_MESG    (0)
#define KEYS_MESG  (1)

#define KP_SERIAL_BAUD        (4800)    // baud rate for keypad communication
#define KP_SERIAL_MAX_KEYPADS    (8)    // max number of keypads in alarm circuit
#define KP_SERIAL_READ_BUF_SIZE (64)    // size of read buffer

// polling states during keypad polling
enum {
    NOT_POLLING  = 0,
    POLL_STATE_1 = 1,  // sent first  0x00
    POLL_STATE_2 = 2,  // send second 0x00
    POLL_STATE_3 = 3,  // sent third  0x00
    POLL_STATE_4 = 4   // read bitmask from keypad
};

class KeypadSerial
{
public:
    KeypadSerial(void);              // Class constructor.  Returns: none

    void    init(void);              // init the class
    bool    poll(void);
    void    write(const uint8_t * msg, const uint8_t size);
    bool    read(uint8_t * c, uint32_t timeout);
    void    getMsg(char * buf, uint8_t bufLen);
    uint8_t requestData(uint8_t kp);

    // return keypad address for keypad kp
    uint8_t getAddr(uint8_t kp)         { return kp < numKeypads ? keypadAddr[kp] : 0; }

    // return the number of keypads that responded to the poll request
    uint8_t getNumKeypads(void)         { return numKeypads; }

    // return the number of keys returned by keypad
    uint8_t getKeyCount(void)           { return recvMsgLen > 3 ? recvMsgLen - 3 : 0; }

    // return pointer to array of keys returned by keypad
    uint8_t * getKeys(void)             { return &readBuf[2]; }  // keys start at byte 2

    // return length of data received from keypad
    uint8_t getRecvMsgLen(void)         { return recvMsgLen; }

    // return pointer to array of data read from keypad
    uint8_t * getRecvMsg(void)          { return readBuf; }

    static inline void pinChangeIsr(void) __attribute__((__always_inline__));
    static KeypadSerial * pKeypadSerial;

private:
    bool    parsePollResp(uint8_t);
    void    write0(void);
    void    beforeWrite(void);
    void    afterWrite(void);

    static inline void delay_us(uint32_t us) __attribute__((__always_inline__));

    SoftwareSerial softSerial;

    uint8_t pollState;
    uint8_t numKeypads;
    uint8_t recvMsgLen;
    uint8_t keypadAddr[KP_SERIAL_MAX_KEYPADS];
    uint8_t readBuf[KP_SERIAL_READ_BUF_SIZE];
};

