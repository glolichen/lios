#include <stdbool.h>
#include "heap.h"
#include "vmm.h"
#include "../io/output.h"
#include "../panic.h"
#include "../const.h"

#define SET_BIT(num, pos) (num |= ((u64) 1) << (pos))
#define UNSET_BIT(num, pos) (num &= ~(((u64) 1) << (pos)))
#define QUERY_BIT(num, pos) ((num >> (pos)) & ((u64) 1))

// sort of based on https://wiki.osdev.org/User:Pancakes/BitmapHeapImplementation
// I thought of this system after viewing their interactive website for a bit,
// it may or may not be the same as the one in that web page

const u32 SECTION_SIZE = 8;

struct HeapBitmapNode {
	struct HeapBitmapNode *next;
	// in bytes, NOT number of entries! for that divide by 8
	// (bitmap_size guaranteed multiple of 8)
	u32 bitmap_size, total_size;
	// split into bitmap and data section
	u64 mem[];
};
struct HeapBlockHeader {
	u64 size;
};

// I literally cannot believe I have to do this
// floating point numbers are not allowed... so will need another way to divide
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

u64 ceil_u64_div(u64 dividend, u64 divisor) {
	u64 floor = dividend / divisor;
	return floor * divisor == dividend ? floor : floor + 1;
}

struct HeapBitmapNode *heap_head = 0, *heap_tail = 0;

void add_block(u32 pages) {
	/*
	 * everything below measured in bytes, all vars are integers (obviously)
	 * block_total = PAGE_SIZE * pages
	 * data + bitmap + sizeof(struct HeapBitmapNode) = block_total
	 *  => data + bitmap = block_total - 16
	 *  => data = block_total - 16 - bitmap
	 * bitmap * 16 * section_size = data
	 *  => bitmap * 8 * section_size = block_total - 16 - bitmap
	 *  => bitmap * (8 * section_size + 1) = block_total - 16
	 *  => bitmap = (block_total - 16) / (8 * section_size + 1)
	 * but data has to be integer, but we use integer division so it's already floored
	 */
	u32 block_total = PAGE_SIZE * pages;
	u32 bitmap = round_u32_div(block_total - 16, 8 * SECTION_SIZE + 1);
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

	serial_info("heap: added block at 0x%x with size 0x%x = %u pages",
			 addr, block_total, pages);
}

void heap_init() {
	add_block(1);
}

void mark_bitmap(struct HeapBitmapNode *node, u64 start, u64 bits) {
	u64 start_index = start / 64, start_bit = start % 64;
	u64 end_index = (start + bits - 1) / 64, end_bit = (start + bits - 1) % 64;

	serial_info("heap: marking bitmap with 1 (used) (index, bit): (%u, %u) -> (%u, %u)",
			 start_index, start_bit, end_index, end_bit);

	if (start_index == end_index) {
		for (u32 i = start_bit; i <= end_bit; i++)
			SET_BIT(node->mem[start_index], i);
		return;
	}

	for (u32 i = start_bit; i < 64; i++)
		SET_BIT(node->mem[start_index], i);
	for (u32 i = start_index + 1; i < end_index; i++)
		node->mem[i] = 0xFFFFFFFFFFFFFFFF;
	for (u32 i = 0; i <= end_bit; i++)
		SET_BIT(node->mem[end_index], i);
}

void unmark_bitmap(struct HeapBitmapNode *node, u64 start, u64 bits) {
	u64 start_index = start / 64, start_bit = start % 64;
	u64 end_index = (start + bits - 1) / 64, end_bit = (start + bits - 1) % 64;

	serial_info("heap: marking bitmap with 0 (free) (index, bit): (%u, %u) -> (%u, %u)",
			 start_index, start_bit, end_index, end_bit);

	if (start_index == end_index) {
		for (u32 i = start_bit; i <= end_bit; i++)
			UNSET_BIT(node->mem[start_index], i);
		return;
	}

	for (u32 i = start_bit; i < 64; i++)
		UNSET_BIT(node->mem[start_index], i);
	for (u32 i = start_index + 1; i < end_index; i++)
		node->mem[i] = 0;
	for (u32 i = 0; i <= end_bit; i++)
		UNSET_BIT(node->mem[end_index], i);
}

void *kmalloc(u64 size) {
	// need a header to store how much to free (I think malloc does this?)
	u64 real_size = size;
	size += sizeof(struct HeapBlockHeader);

	u64 sections_needed = ceil_u64_div(size, SECTION_SIZE);
	serial_info("heap: requested %u bytes = %u sections (%u bytes w/o header)",
			 size, sections_needed, real_size);
	
	struct HeapBitmapNode *cur = heap_head;
	while (cur != 0) {
		u32 bitmap_size = cur->bitmap_size;
		u64 sections_found = 0, block_start = 0;
		for (u32 i = 0; i < bitmap_size / 8; i++) {
			// TODO: speedup by adding 64 sections if the bitmap is 0
			for (u32 j = 0; j < 64; j++) {
				// 1 = used, set to zero
				if (QUERY_BIT(cur->mem[i], j))
					sections_found = 0;
				else {
					if (sections_found == 0)
						block_start = i * 64 + j;
					sections_found++;
				}

				if (sections_found == sections_needed) {
					mark_bitmap(cur, block_start, sections_needed);

					u64 addr = (u64) cur->mem + cur->bitmap_size + block_start * SECTION_SIZE;
					((struct HeapBlockHeader *) addr)->size = sections_needed;
					addr += sizeof(struct HeapBlockHeader);

					serial_info("heap: return address 0x%x in node 0x%x at bitmap offset %u",
							addr, cur, block_start);

					return (void *) addr;
				}
			}
		}
		cur = cur->next;
	}

	// no memory left, allocate more blocks
	u64 pages64 = ceil_u64_div(size, PAGE_SIZE);
	if (pages64 & 0xFFFFFFFF00000000)
		panic("kmalloc: you asked for too much memory"); // lol yeah
	add_block((u32) (pages64 & 0xFFFFFFFF));
	
	// this is very wasteful, but I'm lazy
	return kmalloc(size);
}

void release_if_unused(struct HeapBitmapNode *prev, struct HeapBitmapNode *node) {
	// then the node is head. do not release the head
	if (prev == node)
		return;

	if (prev->next != node)
		panic("heap: assertion failed!");

	u32 total_size = node->total_size, bitmap_size = node->bitmap_size;
	for (u32 i = 0; i < bitmap_size / 8; i++) {
		// something is being used
		if (node->mem[i])
			return;
	}

	// release the memory because nothing is used
	prev->next = 0;
	vmm_free(node);

	serial_info("heap: released heap block at 0x%x", node);
}

void kfree(void *mem) {
	// I love C.
	u64 size = ((struct HeapBlockHeader *) ((u64) mem - sizeof(struct HeapBlockHeader)))->size;
	// move it back to get the "real" location
	mem = (void *) ((u64) mem - sizeof(struct HeapBlockHeader));

	serial_info("heap: freeing memory at 0x%x with detected size %u", mem, size);

	struct HeapBitmapNode *cur = heap_head, *prev = heap_head;
	while (cur != 0) {
		u32 total_size = cur->total_size, bitmap_size = cur->bitmap_size;
		u64 node_mem = (u64) cur->mem;

		if (node_mem + bitmap_size <= (u64) mem && (u64) mem <= node_mem + total_size) {
			u64 bitmap_offset = ((u64) mem - (node_mem + bitmap_size)) / SECTION_SIZE;
			unmark_bitmap(cur, bitmap_offset, size);
			release_if_unused(prev, cur);
			return;
		}

		if (cur != heap_head)
			prev = prev->next;
		cur = cur->next;
	}

	panic("kfree: something is very wrong");
}

void heap_log_status() {
	serial_debug("heap: printing status");
	struct HeapBitmapNode *cur = heap_head;
	while (cur != 0) {
		u32 total_size = cur->total_size, bitmap_size = cur->bitmap_size;
		serial_debug("heap: node addr 0x%x, total size 0x%x, bitmap %u entries (size 0x%x)",
			   (u64) cur, total_size, bitmap_size / 8, bitmap_size);
		serial_debug("heap: bitmap contents:");
		for (u32 i = 0; i < bitmap_size / 8; i++)
			serial_debug("  %u (offset 0x%x): 0x%x", i, i * 8, cur->mem[i]);
		cur = cur->next;
	}
}

