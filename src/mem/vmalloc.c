#include <stdbool.h>
#include "vmalloc.h"
#include "vmm.h"
#include "../io/output.h"
#include "../util/panic.h"
#include "../util/const.h"
#include "../util/kmath.h"
#include "../util/misc.h"

#define SET_BIT(num, pos) ((num) |= ((u64) 1) << (pos))
#define UNSET_BIT(num, pos) ((num) &= ~(((u64) 1) << (pos)))
#define QUERY_BIT(num, pos) (((num) >> (pos)) & ((u64) 1))

// sort of based on https://wiki.osdev.org/User:Pancakes/BitmapHeapImplementation
// I thought of this system after viewing their interactive website for a bit,
// it may or may not be the same as the one in that web page

#define SECTION_SIZE 8

struct HeapBitmapNode {
	struct HeapBitmapNode *next;
	
	// in number of bits, each bit represents whether a section is used
	u64 bitmap_size;

	// in number of entries, which is the same as # of bytes
	u64 total_size;

	// split into bitmap and data section
	u8 mem[];
};
struct HeapBlockHeader {
	u64 size;
};

struct HeapBitmapNode *heap_head = 0, *heap_tail = 0;

void add_block(u64 wanted_sections) {
	u64 wanted_bytes = wanted_sections * SECTION_SIZE;
	u64 block_total = wanted_bytes + wanted_sections;

	u64 pages = ceil_u64_div(block_total, PAGE_SIZE);

	serial_info("vmalloc: request extra block of size %u bytes:", wanted_bytes);
	serial_info("    bitmap %u bits, total size %u, %u pages", wanted_sections, block_total, pages);

	struct HeapBitmapNode *addr = (struct HeapBitmapNode *) vmm_alloc(pages);
	addr->next = 0;
	addr->total_size = block_total;
	addr->bitmap_size = wanted_sections;

	// initialize bitmap values to 0 (indicating free)
	for (u64 i = 0; i < wanted_sections; i++)
		addr->mem[i] = 0;

	if (!heap_head && !heap_tail) {
		heap_head = addr;
		heap_tail = addr;
	}
	else {
		heap_tail->next = addr;
		heap_tail = addr;
	}

	serial_info("vmalloc: added block at 0x%x with size 0x%x = %u pages",
			 addr, block_total, pages);
}

void vmalloc_init(void) {
	add_block(32 / SECTION_SIZE);
}

void mark_bitmap(struct HeapBitmapNode *node, u64 start, u64 bits) {
	u64 start_index = start / 8, start_bit = start % 8;
	u64 end_index = (start + bits - 1) / 8, end_bit = (start + bits - 1) % 8;

	serial_info("vmalloc: marking bitmap with 1 (used) (index, bit): (%u, %u) -> (%u, %u)",
			 start_index, start_bit, end_index, end_bit);

	if (start_index == end_index) {
		for (u32 i = start_bit; i <= end_bit; i++)
			SET_BIT(node->mem[start_index], i);
		return;
	}

	for (u32 i = start_bit; i < 8; i++)
		SET_BIT(node->mem[start_index], i);
	for (u32 i = start_index + 1; i < end_index; i++)
		node->mem[i] = 0xFF;
	for (u32 i = 0; i <= end_bit; i++)
		SET_BIT(node->mem[end_index], i);
}

void unmark_bitmap(struct HeapBitmapNode *node, u64 start, u64 bits) {
	u64 start_index = start / 8, start_bit = start % 8;
	u64 end_index = (start + bits - 1) / 8, end_bit = (start + bits - 1) % 8;

	serial_info("vmalloc: marking bitmap with 0 (free) (index, bit): (%u, %u) -> (%u, %u)",
			 start_index, start_bit, end_index, end_bit);

	if (start_index == end_index) {
		for (u32 i = start_bit; i <= end_bit; i++)
			UNSET_BIT(node->mem[start_index], i);
		return;
	}

	for (u32 i = start_bit; i < 8; i++)
		UNSET_BIT(node->mem[start_index], i);
	for (u32 i = start_index + 1; i < end_index; i++)
		node->mem[i] = 0;
	for (u32 i = 0; i <= end_bit; i++)
		UNSET_BIT(node->mem[end_index], i);
}

void *vmalloc(u64 size) {
	u64 real_size = size;
	size += sizeof(struct HeapBlockHeader);

	u64 sections_needed = ceil_u64_div(size, SECTION_SIZE);
	serial_info("vmalloc: requested %u bytes = %u sections (%u bytes w/o header)",
			 size, sections_needed, real_size);
	
	struct HeapBitmapNode *cur = heap_head;
	while (cur != 0) {
		u64 sections_found = 0, block_start = 0;

		for (u64 i = 0; i < cur->bitmap_size; i++) {
			// 1 = used, set to zero
			if (QUERY_BIT(cur->mem[i / 8], i % 8))
				sections_found = 0;
			else {
				if (sections_found == 0)
					block_start = i;

				// can skip over more than a bit (this is a SIGNIFICANT optimization)
				if (i % 8 == 0) {
					if (i < cur->bitmap_size - 64 && sections_found + 64 < sections_needed) {
						if (*((u64 *) ((u64) cur->mem + i / 8)) == 0) {
							sections_found += 64 / SECTION_SIZE;
							continue;
						}
					}
					if (i < cur->bitmap_size - 32 && sections_found + 32 < sections_needed) {
						if (*((u32 *) ((u64) cur->mem + i / 8)) == 0) {
							sections_found += 32 / SECTION_SIZE;
							continue;
						}
					}
					if (i < cur->bitmap_size - 16 && sections_found + 16 < sections_needed) {
						if (*((u16 *) ((u64) cur->mem + i / 8)) == 0) {
							sections_found += 16 / SECTION_SIZE;
							continue;
						}
					}
				}

				sections_found++;
			}

			if (sections_found == sections_needed) {
				mark_bitmap(cur, block_start, sections_needed);

				u64 addr = (u64) cur->mem + ceil_u64_div(cur->bitmap_size, 8) + block_start * SECTION_SIZE;
				((struct HeapBlockHeader *) addr)->size = sections_needed;
				addr += sizeof(struct HeapBlockHeader);

				serial_info("vmalloc: return address 0x%x in node 0x%x at bitmap offset %u",
						addr, cur, block_start);

				return (void *) addr;
			}
		}
		cur = cur->next;
	}

	// no memory left, allocate more blocks
	// u64 pages64 = ceil_u64_div(size, PAGE_SIZE);
	// if (pages64 & 0xFFFFFFFF00000000)
	// 	panic("vmalloc: you asked for too much memory"); // lol yeah

	add_block(ceil_u64_div(size, SECTION_SIZE));
	
	// this is very wasteful, but I'm lazy
	return vmalloc(size);
}

void *vcalloc(u64 size) {
	u8 *mem = (u8 *) vmalloc(size);
	memset(mem, 0, size);
	return (void *) mem;
}

void release_if_unused(struct HeapBitmapNode *prev, struct HeapBitmapNode *node) {
	// then the node is head. do not release the head
	if (prev == node)
		return;

	if (prev->next != node)
		panic("vmalloc: assertion failed!");

	for (u64 i = 0; i < ceil_u64_div(node->bitmap_size, 8); i++) {
		// something is being used
		if (node->mem[i])
			return;
	}

	// release the memory because nothing is used
	prev->next = node->next;
	if (heap_tail == node)
		heap_tail = prev;
	vmm_free(node);

	serial_info("vmalloc: released heap block at 0x%x", node);
}

void vfree(const void *mem) {
	// I love C.
	u64 size = ((struct HeapBlockHeader *) ((u64) mem - sizeof(struct HeapBlockHeader)))->size;
	// move it back to get the "real" location
	mem = (void *) ((u64) mem - sizeof(struct HeapBlockHeader));

	serial_info("vmalloc: freeing memory at 0x%x with detected size %u", mem, size);

	struct HeapBitmapNode *cur = heap_head, *prev = heap_head;
	while (cur != 0) {
		u64 total_size = cur->total_size, bitmap_size = ceil_u64_div(cur->bitmap_size, 8);
		u64 cur_addr = (u64) cur;

		if (cur_addr + bitmap_size <= (u64) mem && (u64) mem <= cur_addr + total_size) {
			u64 bitmap_offset = ((u64) mem - (cur_addr + bitmap_size)) / SECTION_SIZE;
			unmark_bitmap(cur, bitmap_offset, size);
			release_if_unused(prev, cur);
			return;
		}

		if (cur != heap_head)
			prev = prev->next;
		cur = cur->next;
	}

	panic("vfree: something is very wrong");
}

void vmalloc_log_status(void) {
	serial_debug("vmalloc: printing status");
	struct HeapBitmapNode *cur = heap_head;
	while (cur != 0) {
		u32 total_size = cur->total_size, bitmap_size = ceil_u64_div(cur->bitmap_size, 8);
		serial_debug("vmalloc: node addr 0x%x, total size 0x%x, bitmap %u entries (size 0x%x)",
			   (u64) cur, total_size, bitmap_size / 8, bitmap_size);
		serial_debug("vmalloc: bitmap contents:");
		for (u32 i = 0; i < bitmap_size / 8; i++)
			serial_debug("  %u (offset 0x%x): 0x%x", i, i * 8, cur->mem[i]);
		cur = cur->next;
	}
}

