// file Volts.h - class for handling reading and outputing project voltages

#pragma once

#include <Arduino.h>

#define NUM_VOLTS (3)  // number of voltages to monitor

// voltages v0-v2 are passed through a voltage divider with a scaling of 0.175 (47k-10k)
// that results in full scale input of (5/0.175) 28.5v. 
// Using a scale factor of 2850/1024 will give correct number of 100ths of volts

static const uint16_t scale[NUM_VOLTS] = { 2850, 2850, 2850 };
static const uint8_t  pin[NUM_VOLTS]   = {   A0,   A1,   A2 };  // analog input pins monitoring rails

class Volts
{
public:
    Volts (void) {}                          // Class constructor.  Returns: none

    void init(void);                         // init the class
    void read(void);                         // read the voltages
    void getMsg(char * buf, uint8_t bufLen); // write the voltage message into the provided buf

private:
    uint16_t rail[NUM_VOLTS];  // voltage rails (in 100ths of volts)

};
