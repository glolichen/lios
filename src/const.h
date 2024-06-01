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

extern const u32 FB_ROWS, FB_COLS;
extern const u32 FB_COMMAND_PORT, FB_DATA_PORT;
extern const u32 FB_COMMAND_LOW_BYTE_COMMAND, FB_COMMAND_HIGH_BYTE_COMMAND;
extern const u32 SHORT_LOW_BYTE, SHORT_HIGH_BYTE;

extern const u32 SERIAL_COM1_BASE;
extern const u32 SERIAL_LINE_ENABLE_DLAB;

extern const u32 FB_ADDRESS;

extern const char *EXCEPTIONS[];

extern const u8 PICM;
extern const u8 PICS;
extern const u8 PICM_COMMAND;
extern const u8 PICS_COMMAND;
extern const u8 PICM_DATA;
extern const u8 PICS_DATA;
extern const u8 PIC_EOI;

#endif