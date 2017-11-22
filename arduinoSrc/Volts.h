// file Volts.h - class for handling reading and outputing project voltages

#pragma once

#include <Arduino.h>

#define NUM_VOLTS (3)  // number of voltages to monitor

// v1 & v2 are passed through a voltage divider with a scaling of 0.175 (47k-10k)
// that results in full scale input of (5/0.175) 28.5v, for 100ths of volts, mult by 2850
// using a scale factor of 2850/1024 will give correct number of 100ths of volts

// v3 is passed through a voltage divider with a scaling of 0.5, 10v full scale
// using a scale factor of 1000/1024 will give the correct number of 100ths of volts

static const uint16_t scale[NUM_VOLTS] = { 2850, 2850, 1000 };
static const uint8_t  pin[NUM_VOLTS]   = {   A0,   A1,   A2 };  // analog input pins monitoring rails

class Volts
{
public:
    Volts (void) {}                          // Class constructor.  Returns: none

    void init(void);                         // init the class
    void read(void);                         // read the voltages
    void getMsg(char * buf, uint8_t bufLen); // write the voltage message into the provided buf

private:
    uint8_t rail[NUM_VOLTS];  // voltage rails (in 100ths of volts)

};
