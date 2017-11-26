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
    //const char * printF7(char * buf);

    const uint8_t * getF7(void);
    const uint8_t   getF7size(void) { return F7_MSG_SIZE; }

private:
    uint8_t count;
    t_MesgF7 msgF7[2];  // 2 F7 mesgs, primary and alternate

    void initF7(t_MesgF7 * pMsgF7);
    uint8_t parseF7(const char * msg, uint8_t len, t_MesgF7 * pMsgF7);
};

