#ifndef CONST_H
#define CONST_H

#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define GET_SERIAL_DATA_PORT(base) (base)
#define GET_SERIAL_FIFO_COMMAND_PORT(base) (base + 2)
#define GET_SERIAL_LINE_COMMAND_PORT(base) (base + 3)
#define GET_SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define GET_SERIAL_LINE_STATUS_PORT(base) (base + 5)

#define MAKE_COLOR(fg, bg) (((bg & 0xF) << 4) | (fg & 0xF))

typedef enum {
	BLACK = 0,
	BLUE = 1,
	GREEN = 2,
	CYAN = 3,
	RED = 4,
	MAGENTA = 5,
	BROWN = 6,
	LIGHT_GRAY = 7,
	DARK_GRAY = 8,
	LIGHT_BLUE = 9,
	LIGHT_GREEN = 10,
	LIGHT_CYAN = 11,
	LIGHT_RED = 12,
	LIGHT_MAGENTA = 13,
	LIGHT_BROWN = 14,
	WHITE = 15
} FBColor;

#define FB_ROWS 25
#define FB_COLS 80
#define FB_COMMAND_PORT 0x3D4
#define FB_DATA_PORT 0x3D5
#define FB_COMMAND_LOW_BYTE_COMMAND 14
#define FB_COMMAND_HIGH_BYTE_COMMAND 15

#define U16_LOW_BYTE 0x00FF
#define U16_HIGH_BYTE 0xFF00

#define SERIAL_COM1_BASE 0x3F8
#define SERIAL_LINE_ENABLE_DLAB 0x80

#define FB_ADDRESS 0xB8000

#define PICM 0x20
#define PICS 0xA0
#define PICM_COMMAND PICM
#define PICS_COMMAND PICS
#define PICM_DATA (PICM + 1)
#define PICS_DATA (PICS + 1)
#define PIC_EOI 0x20

extern const char *EXCEPTIONS[];

#define PMM_BLOCK_SIZE 4096


#endif