// file F7msg.h - F7 message info

#pragma once

// F7 bit macros to simplify code
#define SET_BIT(b,v,m)       (((b) & ~(m)) | ((v) ? (m) : (0x00)))
#define SET_READY(b,v)       SET_BIT(b,v,0x10)  // BYTE2
#define SET_ARMED_STAY(b,v)  SET_BIT(b,v,0x80)  // BYTE2
#define SET_ARMED_AWAY(b,v)  SET_BIT(b,v,0x04)  // BYTE3
#define SET_CHIME(b,v)       SET_BIT(b,v,0x20)  // BYTE3
#define SET_POWER(b,v)       SET_BIT(b,v,0x08)  // BYTE3

#define GET_BIT(b,m)         ((b) & (m))
#define GET_READY(b)         GET_BIT(b,0x10)    // BYTE2
#define GET_ARMED_STAY(b)    GET_BIT(b,0x80)    // BYTE2
#define GET_ARMED_AWAY(b)    GET_BIT(b,0x04)    // BYTE3
#define GET_CHIME(b)         GET_BIT(b,0x20)    // BYTE3
#define GET_POWER(b)         GET_BIT(b,0x08)    // BYTE3

#define H2B(a)               ((uint8_t)(((a) >= '0' && (a) <= '9') ? (a) - '0' : (a) - 'A' + 10))
#define GET_BYTE(c1,c2)      ((uint8_t)((H2B(c1) << 4) | H2B(c2)))
#define GET_NIBBLE(c)        ((uint8_t)(H2B(c)))
#define GET_BOOL(b)          ((b) == '1' ? 1 : 0)

#define F7_MSG_SIZE          (48)

#pragma pack(push,1)  // mesg struct needs 1 byte packing

typedef struct {
    uint8_t type;
    uint8_t addr1;
    uint8_t addr2;
    uint8_t keypads;
    uint8_t addr4;
    uint8_t zone;
    uint8_t byte1;
    uint8_t byte2;
    uint8_t byte3;
    uint8_t prog;
    uint8_t prompt;
    uint8_t pad1;
    char    line1[16];
    char    line2[16];
    uint8_t chksum;
    uint8_t unused[3];
} t_MesgF7;               // F7 mesg is 48 bytes

#pragma pack(pop)  // restore default packing

