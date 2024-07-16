#ifndef CONST_H
#define CONST_H

#include <stdint.h>

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

#define GET_SERIAL_DATA_PORT(base) (base)
#define GET_SERIAL_FIFO_COMMAND_PORT(base) (base + 2)
#define GET_SERIAL_LINE_COMMAND_PORT(base) (base + 3)
#define GET_SERIAL_MODEM_COMMAND_PORT(base) (base + 4)
#define GET_SERIAL_LINE_STATUS_PORT(base) (base + 5)

#define MAKE_COLOR(fg, bg) (((bg & 0xF) << 4) | (fg & 0xF))

enum FBColor {
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
};

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

extern const char *MULTIBOOT_ENTRY_TYPES[];
#define PMM_BLOCK_SIZE 4096

#define PAGE_TABLES_PER_DIRECTORY 1024
#define PAGES_PER_TABLE 1024

// no PSE for now
#define PAGE_SIZE 4096

enum PageDirectoryFlags {
	PDF_PRESENT = 0x1,
	PDF_WRITABLE = 0x2,
	PDF_USER = 0x4,
	PDF_WRITE_THROUGH = 0x8,
	PDF_CACHE_DISABLE = 0x10,
	PDF_ACCESSED = 0x20,
	PDF_PAE_SET = 0x80,
};
enum PageTableFlags {
	PTF_PRESENT = 0x1,
	PTF_WRITABLE = 0x2,
	PTF_USER = 0x4,
	PTF_WRITE_THROUGH = 0x8,
	PTF_CACHE_DISABLE = 0x10,
	PTF_ACCESSED = 0x20,
	PTF_DIRTY = 0x40,
	PTF_PAT = 0x80,
	PTF_GLOBAL = 0x100,
};

// page directory table is an array of page directory entries
// page directory entries point to page tables
// page tables is an array of page table entries
// page table entries point to pages (physical address (u32))
typedef u32 PageDirectoryEntry;
typedef u32 PageTableEntry;

typedef struct {
	PageDirectoryEntry table[1024];
} PageDirectoryTable;
typedef struct {
	PageTableEntry table[1024];	
} PageTable;

typedef u32 PhysicalAddress;

#define KERNEL_START 0x00100000

#endif
