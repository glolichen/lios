#include <stdbool.h>
#include "vmm.h"
#include "kmalloc.h"
#include "page.h"
#include "pmm.h"
#include "../util/const.h"
#include "../util/panic.h"
#include "../util/kmath.h"
#include "../io/output.h"

// NOTE: yeah it's really bad
// also, the new pages allocated to store list nodes cannot be freed
// but that shouldn't use too much memory, right?

// node in linked list that stores free blocks
struct VirtFreeListNode {
	struct VirtFreeListNode *prev, *next; // prev and next node
	u64 start; // start of current free virtual address block
	u64 size; // size of current block
};
// node in a different linked list that stores allocated blocks
// look up the size of allocated block to free
struct VirtAllocatedNode {
	struct VirtAllocatedNode *next;
	u64 start;
	u64 size;
};

struct VirtFreeListNode *vmm_head = 0, *vmm_tail = 0;
struct VirtAllocatedNode *alloc_head;

void set_up_page_for_alloc_list(struct VirtAllocatedNode *node) {
	node->start = 0;
	node->size = 0;
	struct VirtAllocatedNode *alloc_cur = node;
	for (u32 i = 1; i < floor_u32_div(PAGE_SIZE, sizeof(struct VirtAllocatedNode)); i++) {
		alloc_cur->next = alloc_cur + 1;
		alloc_cur = alloc_cur->next;
		alloc_cur->start = 0;
		alloc_cur->size = 0;
	}
	alloc_cur->next = 0;
}
void set_up_page_for_free_list(struct VirtFreeListNode *node) {
	node->prev = 0;
	struct VirtFreeListNode *vmm_cur = node;
	for (u32 i = 1; i < floor_u32_div(PAGE_SIZE, sizeof(struct VirtFreeListNode)); i++) {
		vmm_cur->next = vmm_cur + 1;
		vmm_cur = vmm_cur->next;
		vmm_cur->prev = vmm_cur - 1;
		vmm_cur->start = 0;
		vmm_cur->size = 0;
	}
	vmm_tail = vmm_cur;
	vmm_tail->next = 0;
}

void vmm_init(void) {
	u64 free_virt_start = 0xFFFFF00000000000;

	vmm_head = kcalloc_page();
	vmm_head->start = free_virt_start;
	vmm_head->size = 0xFFFFFFFF80000000 - vmm_head->start;
	set_up_page_for_free_list(vmm_head);
	serial_info("vmm: created vmm list nodes, starting 0x%x", vmm_head);

	alloc_head = kcalloc_page();
	set_up_page_for_alloc_list(alloc_head);
	serial_info("vmm: created alloc list nodes, starting 0x%x", alloc_head);
}

void vmm_list_remove_node(struct VirtFreeListNode *node) {
	// set start and size to zero, indicating empty node
	node->start = 0;
	node->size = 0;

	// in the list, skip current node
	// next node points to previous node
	if (node->next)
		node->next->prev = node->prev;
	// node is tail
	// TODO: think of what to do here
	else {

	}
	
	// previous node points to next node
	if (node->prev)
		node->prev->next = node->next;
	// node is head
	else
		vmm_head = node->next;

	// move to tail
	node->next = 0;
	node->prev = vmm_tail;
	vmm_tail->next = node;
	vmm_tail = node;
}

void alloc_list_add_node(u64 start, u64 size) {
	struct VirtAllocatedNode *cur = alloc_head;
	while (true) {
		if (cur->start == 0 && cur->size == 0) {
			cur->start = start;
			cur->size = size;
			return;
		}
		if (cur->next == 0)
			break;
		cur = cur->next;
	}

	struct VirtAllocatedNode *next_page = kcalloc_page();
	set_up_page_for_alloc_list(next_page);
	cur->next = next_page;

	alloc_list_add_node(start, size);
}
u64 alloc_list_find_and_remove(u64 start) {
	struct VirtAllocatedNode *cur = alloc_head;
	while (cur != 0) {
		if (cur->start == start) {
			u64 size = cur->size;
			cur->start = 0;
			cur->size = 0;
			return size;
		}
		cur = cur->next;
	}
	panic("vmm: freeing block that is not in list!");
}

void *vmm_alloc(u64 pages) {
	// make space for header here
	u64 request_size = pages * PAGE_SIZE;
	struct VirtFreeListNode *cur = vmm_head;
	// after the last node cur moves to 0
	while (cur != 0) {
		u64 start = cur->start, size = cur->size;
		if (size >= request_size) {
			// exact fit -- delete node by adding to the end
			if (size == request_size)
				vmm_list_remove_node(cur);
			// inexact fit -- shrink node
			else {
				cur->start += request_size;
				cur->size -= request_size;
			}

			// unmap virtual address, allocate page frame, map to new page frame
			serial_info("vmm: request physical page frames");
			for (u64 i = 0; i < pages; i++) {
				u64 virt = start + i * PAGE_SIZE;
				PhysicalAddress page_frame = pmm_alloc_high();
				page_unmap(virt);
				page_map(virt, page_frame, true);
			}

			alloc_list_add_node(start, pages);
			serial_info("vmm: allocate %u pages at 0x%x", pages, start);
			return (void *) start;
		}
		
		cur = cur->next;
	}

	panic("vmm: out of memory!");
	return 0;
}

void vmm_free(void *mem) {
	u64 pages = alloc_list_find_and_remove((u64) mem);
	serial_info("vmm: freeing block 0x%x, detected size %u", (u64) mem, pages);
	u64 block_start = (u64) mem;
	u64 block_end = block_start + pages * PAGE_SIZE;

	// remap virtual address to normal position, free page frame
	for (u64 i = 0; i < pages; i++) {
		u64 virt = (u64) mem + i * PAGE_SIZE;
		PhysicalAddress page_frame = page_virt_to_phys_addr(virt);
		serial_info("vmm: detected page frame: 0x%x", page_frame);
		pmm_free(page_frame);
		page_unmap(virt);
		page_map(virt, virt - KERNEL_OFFSET, true);
	}

	// add virtual addresses back to linked list for future use
	
	// find node to merge current block with
	struct VirtFreeListNode *cur = vmm_head;
	bool found_merge = false;
	while (true) {
		u64 start = cur->start, size = cur->size;
		// expand existing block to the right
		if (block_start == start + size) {
			cur->size += (block_end - block_start);
			block_start = start;
			found_merge = true;
			break;
		}
		// expand existing block to the left
		if (block_end == start) {
			cur->start -= (block_end - block_start);
			cur->size += (block_end - block_start);
			block_end = cur->start + cur->size;
			found_merge = true;
			break;
		}
		if (cur->next == 0)
			break;
		cur = cur->next;
	}

	// not found then add node by moving (hopefully empty) tail node to head
	if (!found_merge) {
		// FIXME: fix
		if (vmm_tail->start && vmm_tail->size) {
			struct VirtFreeListNode *next_page = kcalloc_page();
			set_up_page_for_free_list(next_page);
			cur->next = next_page;
		}

		// make it no longer tail, make previous node tail
		cur = vmm_tail;
		vmm_tail = vmm_tail->prev;
		vmm_tail->next = 0;

		// previous = 0 means head, make it point to current head
		cur->prev = 0;
		cur->next = vmm_head;

		cur->start = (u64) mem;
		cur->size = pages * PAGE_SIZE;

		// make before of current head point to current node
		vmm_head->prev = cur;
		// move current node to head
		vmm_head = cur;
		return;
	}	

	struct VirtFreeListNode *merged = cur;

	// if found then check again to see if can merge again
	// need to re-iterate through list to find further merges
	cur = vmm_head;
	while (cur != 0) {
		u64 start = cur->start, size = cur->size;
		if (block_start == start + size) {
			cur->size += (block_end - block_start);
			vmm_list_remove_node(merged);
			break;
		}
		if (block_end == start) {
			cur->start -= (block_end - block_start);
			cur->size += (block_end - block_start);
			vmm_list_remove_node(merged);
			break;
		}
		cur = cur->next;
	}
}

void vmm_log_status(void) {
	serial_debug("vmm: printing status");
	u32 vmm_empty_node = 0, vmm_node_count = 0;
	struct VirtFreeListNode *vmm_cur = vmm_head;
	serial_debug("vmm: list nodes:");
	while (vmm_cur != 0) {
		u64 start = vmm_cur->start, size = vmm_cur->size;
		if (start && size)
			serial_debug("    list node 0x%x, addr 0x%x, size 0x%x", (u64) vmm_cur, start, size);
		else
			vmm_empty_node++;
		vmm_node_count++;
		vmm_cur = vmm_cur->next;
	}
	serial_debug("vmm: list has %u nodes, %u empty", vmm_node_count, vmm_empty_node);

	u32 alloc_empty_node = 0, alloc_node_count = 0;
	struct VirtAllocatedNode *alloc_cur = alloc_head;
	serial_debug("vmm: alloc nodes:");
	while (alloc_cur != 0) {
		u64 start = alloc_cur->start, size = alloc_cur->size;
		if (start && size)
			serial_debug("    allocated block 0x%x, addr 0x%x, size 0x%x", (u64) alloc_cur, start, size);
		else
			alloc_empty_node++;
		alloc_node_count++;
		alloc_cur = alloc_cur->next;
	}
	serial_debug("alloc: list has %u nodes, %u empty", alloc_node_count, alloc_empty_node);
}

