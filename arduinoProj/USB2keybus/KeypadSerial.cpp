// file KeypadSerial.cpp - Software serial class for handling com with alarm keypad

#include "KeypadSerial.h"

// Keypad communication appears to be mostly 8E1 @4800, but some special handling is required
// Check the comments below for details.

#define ONE_BIT_DELAY              delay_us(208)  // ~one bit delay @4800 baud
#define ONE_BYTE_DELAY             delay_us(2030) // ~one byte delay @4800 baud
#define DELAY_BETWEEN_POLL_WRITES  delay_us(1015) // measured delay between polling writes
#define LOW_BEFORE_WRITE_DELAY     delay_us(4060) // time to drop transmit before regular writes

uint8_t dbg1 = 0;  // FIXME - debug, remove me
uint8_t dbg2 = 0;  // FIXME - debug, remove me

KeypadSerial * KeypadSerial::pKeypadSerial = NULL;  // pointer to class for ISR

// class constructor
KeypadSerial::KeypadSerial(void) : softSerial(RX_PIN, TX_PIN, true) {}

// init the class
void KeypadSerial::init(void)
{
    pKeypadSerial = this;              // setup class pointer for ISR
    pollState = NOT_POLLING;           // not currently polling
    softSerial.begin(KP_SERIAL_BAUD);  // set baud rate
    softSerial.setParity(true);        // enable even parity
    afterWrite();                      // normal state of the transmit line should be high
}

// microsecond delay function that supports interrupts during the delay
inline void KeypadSerial::delay_us(uint32_t us)
{
    // _delay_loop_2 delays for about 0.25us at 16MHz.  This code may work for slower
    // clock speeds, but the timing resolution will be less and that may affect your results
    
    for (uint32_t s = micros(); micros() - s < us; )
        _delay_loop_2(1);
}

// The before/after write functions manage the state of the transmit line to keypad

// normal state of the transmit line to the keypad is high, but moves low before
// a write starts (high start bit).  
void KeypadSerial::beforeWrite(void)
{
    softSerial.tx_pin_write(LOW);      // set transmit low before we start writing
    LOW_BEFORE_WRITE_DELAY;            // hold transmit low before write for ~4ms
}

// restore high transmit after write
void KeypadSerial::afterWrite(void)
{
    softSerial.tx_pin_write(HIGH);
}

// parse the keypad byte of the poll response to see which keypads responded
// although unlikely, up to 8 keypads could respond with data in the same poll cycle
bool KeypadSerial::parsePollResp(uint8_t resp)
{
    numKeypads = 0;

    if (resp != 0xFF) // 0xFF indicates no keypad responded
    {
        for (uint8_t i=0; i < 8 && numKeypads < KP_SERIAL_MAX_KEYPADS; i++)
        {
            if (((resp >> i) & 0x01) == 0) // keypad 16+i responded
            {
                keypadAddr[numKeypads++] = 16 + i;
            }
        }
        return (numKeypads > 0);
    }
    return false;
}

// this func holds transmit high for one byte (a 0x00 written inverted)
//   but does not disable pin interrupts during the byte output
//   however, the recv called by pinChangeIsr during the 3rd write will extend
//   the length of the 3rd high transmit.  This is ok as the keypad has already responded
void KeypadSerial::write0(void)
{
    softSerial.tx_pin_write(HIGH);     // set transmit high
    ONE_BYTE_DELAY;                    // hold transmit high for 1 byte (10 bits: start + 8 bits + parity)
    softSerial.tx_pin_write(LOW);      // set transmit low
    DELAY_BETWEEN_POLL_WRITES;         // delay needed between polling writes
}

// Poll the keypad for data.  Return: true if data provided
bool KeypadSerial::poll(void)
{
    // poll the keypads to see what addresses respond
    //  - use transmit like a clock to signal keypads when to respond
    //  - keep transmit low for greater than 10ms (my alarm uses 13ms)
    //  - then high for one word (1st), and low for ~1ms
    //  - then high for one word (2nd), and low for ~1ms
    //  - then high for one word (3rd), and low for ~1ms
    //  - should recv responses from keypads when transmit is high
    //  - keypad responses during polling change pollState in pinChangeIsr

    softSerial.setParity(false);       // turn off parity for this transaction
    uint8_t pollResp = 0xFF;           // init poll response
    pollState = POLL_STATE_1;          // set pollState to initial value

    softSerial.tx_pin_write(LOW);      // set transmit low
    _delay_ms(13);                     // keep low for > 10 ms to signal keypad
    write0();                          // after write, should be at POLL_STATE_2 if keypad responded
    write0();                          // after write, should be at POLL_STATE_3 if keypad responded
    write0();                          // after write, should be at POLL_STATE_4 if keypad responded
    afterWrite();                      // restore transmit line level

    if (pollState == POLL_STATE_4)     // should be at POLL_STATE_4 if keypad responded to each write0
    {
        read(&pollResp, 10);           // which keypads replied?
    }
    pollState = NOT_POLLING;           // done polling (response or no)
    softSerial.setParity(true);        // restore parity after this transaction

    return parsePollResp(pollResp);    // return true if we got a response from any keypads
}

// write sequence of bytes to keypad
void KeypadSerial::write(uint8_t * msg, uint8_t size)
{
    beforeWrite();  // set transmit low before we start writing (about 4ms)

    for (uint8_t i=0; i < size; i++)
    {
        softSerial.write(*(msg + i));
        ONE_BIT_DELAY;
    }

    afterWrite();   // restore transmit line level
}

// return one char read. timeout is in milliseconds. for non-blocking read, give timeout of zero
bool KeypadSerial::read(uint8_t * c, uint32_t timeout)
{
    uint32_t start = millis();
    do 
    {
        if (softSerial.available())
        {
            *c = softSerial.read();
            return true;
        }
    } while (millis() - start < timeout);
    
    *c = 0;
    return false;  // timeout, char not available
}

// send F6 message to keypad to request data. Return mesg type
uint8_t KeypadSerial::requestData(uint8_t kp)
{
    uint8_t msgType = NO_MESG;

    recvMsgLen = 0;

    if (kp >= numKeypads || kp >= KP_SERIAL_MAX_KEYPADS)
    {
        return NO_MESG;  // invalid kp number
    }

    beforeWrite();                     // set transmit low before we start writing
    softSerial.write(0xF6);            // tell keypad to send data
    ONE_BIT_DELAY;                     // one bit delay
    softSerial.write(keypadAddr[kp]);  // address keypad we want to hear from 
    ONE_BIT_DELAY;                     // one bit delay
    afterWrite();                      // restore transmit line level

    uint8_t calcChksum = 0;

    if (read(&readBuf[0], 10) && read(&readBuf[1], 10))  // if we recv a message
    {
        calcChksum = readBuf[0] + readBuf[1]; // add first two bytes to checksum

        // second byte of message is either the length (key message) or a message type

        if (readBuf[1] == 0x87)  // msgType 0x87 unknown, sent on power-up, total length 9, 7 bytes after type
        {
            for (uint8_t i=2; i < 8; i++)  // read bytes up to checksum
            {
                read(&readBuf[i], 10);
                calcChksum += readBuf[i];
            }
            read(&readBuf[8], 10);  // read last mesg byte (checksum)
            recvMsgLen = 9;
            msgType = readBuf[1];
        }
        else if (readBuf[1] <= 16) // assume this is a key message
        {
            for (uint8_t i=0; i < readBuf[1]; i++) // 2nd byte of keys mesg is length
            {
                if (read(&readBuf[i+2], 10) && i < readBuf[1]-1)
                {
                    calcChksum += readBuf[i+2];
                }
            }
            recvMsgLen = readBuf[1]+2;  // remain_bytes + header + length
            msgType = KEYS_MESG;
        } 
        else  // some other type of message, unknown length, read however many bytes are provided
        {
            // read input until we hit size of read buf or read times out
            for (recvMsgLen=2; recvMsgLen < KP_SERIAL_READ_BUF_SIZE && read(&readBuf[recvMsgLen], 10); recvMsgLen++)
                ;
            for (uint8_t i=2; i < recvMsgLen-1; i++)  // assume last byte read is mesg checksum
                calcChksum += readBuf[i];
            msgType = readBuf[1];
        }

        calcChksum = 0x100 - calcChksum;
        
        ONE_BIT_DELAY;
        ONE_BIT_DELAY;  // appears that a two bit delay is needed before dropping transmit for ack

        if ((readBuf[0] & 0x3F) == keypadAddr[kp] &&   // if correct keypad responded to our query
             calcChksum == readBuf[recvMsgLen-1])      // and the checksum is correct
        {
            beforeWrite();                  // set transmit low before we start writing
            softSerial.write(readBuf[0]);   // send keypad mesg ack
            ONE_BIT_DELAY;
            afterWrite();                   // restore transmit line level
            return msgType;
        }
        else
        {
dbg1 = calcChksum;
dbg2 = readBuf[recvMsgLen-1];
        }
    }
    return NO_MESG;  // no message recv for this keypad, or bad checksum
}

// Pin Change INTerrupts ---------------------------------------------------------------------------

inline void KeypadSerial::pinChangeIsr(void)  // declared static
{
    if (pKeypadSerial->softSerial.rx_pin_read()) // low->high change (high start bit)
    {
        if (pKeypadSerial->pollState == NOT_POLLING || pKeypadSerial->pollState == POLL_STATE_3)
        {
            pKeypadSerial->softSerial.recv();  // recv byte
        }
        if (pKeypadSerial->pollState != NOT_POLLING) // we are currently polling keypad
        {
            pKeypadSerial->pollState++;  // bump pollState when pin changes from low to high
        }
    }
    else
    {
        // do nothing when pin changes from high->low
    }
}

#if defined(PCINT0_vect)
ISR(PCINT0_vect)             // pin change on D8-D13 GPIO pins
{
    KeypadSerial::pinChangeIsr();
}
#endif

#if defined(PCINT1_vect)
ISR(PCINT1_vect, ISR_ALIASOF(PCINT0_vect));
#endif

#if defined(PCINT2_vect)
ISR(PCINT2_vect, ISR_ALIASOF(PCINT0_vect));
#endif

#if defined(PCINT3_vect)
ISR(PCINT3_vect, ISR_ALIASOF(PCINT0_vect));
#endif

