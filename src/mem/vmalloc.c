#include <stdbool.h>
#include "vmalloc.h"
#include "vmm.h"
#include "../io/output.h"
#include "../util/panic.h"
#include "../util/const.h"
#include "../util/kmath.h"
#include "../util/misc.h"

// little endian is annoying sometimes
#define SET_BIT(num, pos) ((num) |= ((u64) 1) << (7 - (pos)))
#define UNSET_BIT(num, pos) ((num) &= ~(((u64) 1) << (7 - (pos))))
#define QUERY_BIT(num, pos) (((num) >> (7 - (pos))) & ((u64) 1))

// sort of based on https://wiki.osdev.org/User:Pancakes/BitmapHeapImplementation
// I thought of this system after viewing their interactive website for a bit,
// it may or may not be the same as the one in that web page

#define SECTION_SIZE 8

struct HeapBitmapNode {
	struct HeapBitmapNode *next;
	
	// in bits, each bit represents whether a section is used
	u64 bitmap_size;

	// in number of entries, which is the same as # of bytes
	u64 total_size;

	// split into bitmap and data section
	// each bit in the bitmap controls a section
	u8 mem[];
};
struct HeapBlockHeader {
	// number of sections, not bytes
	u64 size;
};

struct HeapBitmapNode *heap_head = 0, *heap_tail = 0;

void add_block(u64 wanted_sections) {
	// each bit controls a section
	u64 bitmap_size_bits = wanted_sections;
	u64 bitmap_size_bytes = ceil_u64_div(bitmap_size_bits, 8);

	u64 data_size_bytes = wanted_sections * SECTION_SIZE;
	u64 block_total = data_size_bytes + bitmap_size_bytes;

	u64 pages = ceil_u64_div(block_total, PAGE_SIZE);

	serial_info("vmalloc: request extra block of size %u bytes:", data_size_bytes);
	serial_info("    bitmap %u bytes, total size %u, %u pages", bitmap_size_bytes, block_total, pages);

	struct HeapBitmapNode *addr = (struct HeapBitmapNode *) vmm_alloc(pages);
	addr->next = 0;
	addr->total_size = block_total;
	addr->bitmap_size = bitmap_size_bits;

	// initialize bitmap values to 0 (indicating free)
	for (u64 i = 0; i < bitmap_size_bytes; i++)
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
	// see vmalloc for explanation
	add_block(504);
}

// used = mark as 1, !used = mark as 0
void mark_bitmap(struct HeapBitmapNode *node, u64 start, u64 bits, bool used) {
	u64 start_index = start / 8, start_bit = start % 8;
	u64 end_index = (start + bits - 1) / 8, end_bit = (start + bits - 1) % 8;

	serial_info("vmalloc: marking bitmap with 1 (used) (index, bit): (%u, %u) -> (%u, %u)",
			 start_index, start_bit, end_index, end_bit);

	if (start_index == end_index) {
		for (u32 i = start_bit; i <= end_bit; i++) {
			if (used)
				SET_BIT(node->mem[start_index], i);
			else
				UNSET_BIT(node->mem[start_index], i);
		}
		return;
	}

	for (u32 i = start_bit; i < 8; i++) {
		if (used)
			SET_BIT(node->mem[start_index], i);
		else
			UNSET_BIT(node->mem[start_index], i);
	}
	for (u32 i = start_index + 1; i < end_index; i++)
		node->mem[i] = used ? 0xFF : 0;
	for (u32 i = 0; i <= end_bit; i++) {
		if (used)
			SET_BIT(node->mem[end_index], i);
		else
			UNSET_BIT(node->mem[end_index], i);
	}
}

u64 get_block_size(const void *mem) {
	return ((struct HeapBlockHeader *) ((u64) mem - sizeof(struct HeapBlockHeader)))->size;
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
							sections_found += 64;
							i += 64 - 1;
							continue;
						}
					}
				}

				sections_found++;
			}
			if (sections_found == sections_needed) {
				mark_bitmap(cur, block_start, sections_needed, 1);

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

	// NOTE: since we are going to request pages in discrete numbers
	// from the VMM anyway, it's better to use all of that space
	// below we use the finding that the maximum number of sections
	// in a page is the solution to: x / 8 + x * 8 = 4096
	// (x/8 is the bitmap size, x*8 is the data size)
	// the solution is ~504.123, rounded down to 504
	// in a block with 504 sections: 63 byte bitmap, 4032 byte data
	//
	// (yes, this is an approximation and there are slightly better
	// solutions for larger number of pages. But i'm lazy)

	u64 num_pages = ceil_u64_div(size, 4032);
	add_block(num_pages * 504);
	
	return vmalloc(size);
}

void *vcalloc(u64 size) {
	u8 *mem = (u8 *) vmalloc(size);
	memset(mem, 0, size);
	return (void *) mem;
}

void *vrealloc(void *ptr, u64 new_size) {
	u64 old_size = get_block_size(ptr) * SECTION_SIZE;
	void *new_ptr = vmalloc(new_size);
	memcpy(new_ptr, ptr, u64_min(old_size, new_size));
	vfree(ptr);
	return new_ptr;
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
	u64 size = get_block_size(mem);

	// move it back to get the "real" location
	mem = (void *) ((u64) mem - sizeof(struct HeapBlockHeader));

	serial_info("vmalloc: freeing memory at 0x%x with detected size %u", mem, size);

	struct HeapBitmapNode *cur = heap_head, *prev = heap_head;
	while (cur != 0) {
		// in bytes
		u64 total_size = cur->total_size, bitmap_size = ceil_u64_div(cur->bitmap_size, 8);
		u64 cur_addr = (u64) cur->mem;

		if (cur_addr + bitmap_size <= (u64) mem && (u64) mem <= cur_addr + total_size) {
			u64 bitmap_offset = ((u64) mem - (cur_addr + bitmap_size)) / SECTION_SIZE;
			mark_bitmap(cur, bitmap_offset, size, 0);
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
		// bytes
		u32 total_size = cur->total_size, bitmap_size = ceil_u64_div(cur->bitmap_size, 8);

		serial_debug("vmalloc: node addr 0x%x, total size 0x%x, bitmap size 0x%x bytes",
			   (u64) cur, total_size, bitmap_size);

		serial_debug("vmalloc: bitmap contents:");

		for (u32 i = 0; i < bitmap_size; i++)
			serial_debug("  index %u (offset 0x%x): 0x%x", i, i * 8, cur->mem[i]);
		cur = cur->next;
	}
}

