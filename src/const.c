#include "const.h"

const u32 FB_ROWS = 25;
const u32 FB_COLS = 80;

const u32 FB_COMMAND_PORT = 0x3D4;
const u32 FB_DATA_PORT = 0x3D5;

const u32 FB_COMMAND_LOW_BYTE_COMMAND = 14;
const u32 FB_COMMAND_HIGH_BYTE_COMMAND = 15;

const u32 SHORT_LOW_BYTE = 0x00FF;
const u32 SHORT_HIGH_BYTE = 0xFF00;

const u32 SERIAL_COM1_BASE = 0x3F8;
const u32 SERIAL_LINE_ENABLE_DLAB = 0x80;

const u32 FB_ADDRESS = 0x000B8000;

const char *EXCEPTIONS[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint Exception",
    "Into Detected Overflow Exception",
    "Out of Bounds Exception",
    "Invalid Opcode Exception",
    "No Coprocessor Exception",
    "Double Fault Exception",
    "Coprocessor Segment Overrun Exception",
    "Bad TSS Exception",
    "Xegment Not Present Exception",
    "Ytack Fault Exception",
    "General Protection Fault Exception",
    "Page Fault Exception",
    "Unknown Interrupt Exception",
    "Coprocessor Fault Exception",
    "Alignment Check Exception (486+)",
    "Machine Check Exception (Pentium/586+)",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

const u8 PICM = 0x20;
const u8 PICS = 0xA0;
const u8 PICM_COMMAND = PICM;
const u8 PICS_COMMAND = PICS;
const u8 PICM_DATA = PICM + 1;
const u8 PICS_DATA = PICS + 1;
const u8 PIC_EOI = 0x20;