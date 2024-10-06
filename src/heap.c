#include "heap.h"
#include "const.h"
#include "vmm.h"
#include "output.h"

// sort of based on https://wiki.osdev.org/User:Pancakes/BitmapHeapImplementation
// I thought of this system after viewing their interactive website for a bit,
// it may or may not be the same as the one in that web page

const u32 DEFAULT_SECTION_SIZE = 8;

struct HeapBitmapNode {
	struct HeapBitmapNode *next;
	u32 bitmap_size, total_size;
	// https://en.wikipedia.org/wiki/Flexible_array_member
	// split into bitmap and data section
	u64 mem[];
};

// I literally cannot believe I have to do this
// floating point numbers are not allowed... so will need another way to found
u32 absolute(i32 num) {
	return num > 0 ? num : -num;
}
u32 round_u32_div(u32 dividend, u32 divisor) {
	u32 low = dividend / divisor;
	u32 high = low + 1;
	u32 low_error = absolute(low * divisor - dividend);
	u32 high_error = absolute(high * divisor - dividend);
	return low_error < high_error ? low : high;
}

struct HeapBitmapNode *heap_head = 0, *heap_tail = 0;

void add_block(u32 pages, u32 section_size) {
	/*
	 * everything below measured in bytes, all vars are integers (obviously)
	 * block_total = PAGE_SIZE * pages
	 * data + bitmap + sizeof(struct Block) = block_total
	 *  => data + bitmap = block_total - 16
	 *  => data = block_total - 16 - bitmap
	 * bitmap * 16 * section_size = data
	 *  => bitmap * 8 * section_size = block_total - 16 - bitmap
	 *  => bitmap * (8 * section_size + 1) = block_total - 16
	 *  => bitmap = (block_total - 16) / (8 * section_size + 1)
	 * but data has to be integer, but we use integer division so it's already floored
	 */
	u32 block_total = PAGE_SIZE * pages;
	u32 bitmap = round_u32_div(block_total - 16, 8 * section_size + 1);
	// round to nearest multiple of 8 (u64 is 8 bytes)
	bitmap = (bitmap + 7) & ~7;
	u32 data = block_total - 16 - bitmap;

	struct HeapBitmapNode *addr = (struct HeapBitmapNode *) vmm_alloc(pages);
	addr->next = 0;
	addr->total_size = block_total;
	addr->bitmap_size = bitmap;

	// initialize bitmap values to 0 (indicating free)
	for (u32 i = 0; i < bitmap / 8; i++)
		addr->mem[i] = 0;

	if (!heap_head && !heap_tail) {
		heap_head = addr;
		heap_tail = addr;
	}
	else {
		heap_tail->next = addr;
		heap_tail = addr;
	}
}

void heap_init() {
	add_block(2, DEFAULT_SECTION_SIZE);
}

void *kmalloc(u64 size) {
	return (void *) 0;
}

void kfree(void *mem) {

}

void heap_log_status() {
	serial_debug("heap: printing status");
	struct HeapBitmapNode *cur = heap_head;
	while (cur != 0) {
		u32 total_size = cur->total_size, bitmap_size = cur->bitmap_size;
		serial_debug("heap: node addr 0x%x, total size 0x%x, bitmap size 0x%x",
			   (u64) cur, total_size, bitmap_size);
		serial_debug("heap: bitmap contents:");
		for (u32 i = 0; i < bitmap_size / 8; i++)
			serial_debug("  offset 0x%x: 0x%x", i * 8, cur->mem[i]);
		cur = cur->next;
	}
}

