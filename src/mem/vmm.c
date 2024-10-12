#include <stdbool.h>
#include "vmm.h"
#include "page.h"
#include "pmm.h"
#include "../const.h"
#include "../panic.h"
#include "../io/output.h"

// NOTE: yeah it's really bad
// eventually (i.e. in 3 years) I will switch to AVL trees
// but now we just have a first fit flat list structure

// node in linked list that stores free blocks
struct VirtFreeListNode {
	struct VirtFreeListNode *prev, *next; // prev and next node
	u64 start; // start of current free virtual address block
	u64 size; // size of current block
};
struct MemBlockHeader {
	u32 size; // number of (4KiB) pages
	u32 magic; // magic number for checking
};

struct VirtFreeListNode *vmm_head = 0, *vmm_tail = 0;

void vmm_init(u64 free_virt_start) {
	u64 page_frame = pmm_alloc_kernel();
	page_unmap(free_virt_start);
	page_map(free_virt_start, page_frame);
	vmm_head = (struct VirtFreeListNode *) free_virt_start;
	vmm_head->prev = 0;
	vmm_head->start = free_virt_start + PAGE_SIZE;
	vmm_head->size = 0xFFFFFFFFFFFFFFFF - vmm_head->start;

	// sizeof node is 32 bytes
	// 4096/32 = 128
	// in total will have 128 nodes, already created 1
	// need to move forward 127 times
	struct VirtFreeListNode *cur = vmm_head;
	for (u32 i = 1; i < 128; i++) {
		// FIXME: the PMM user stack gets overwritten here???
		pmm_log_status();
		// pointer arithemetic -- will increment by 32 bytes
		cur->next = cur + 1;
		cur = cur->next;
		cur->prev = cur - 1;
		cur->start = 0;
		cur->size = 0;
	}
	vmm_tail = cur;
	vmm_tail->next = 0;

	serial_info("vmm: created vmm list nodes, starting 0x%x, ending 0x%x", vmm_head, cur);

	// check linked list
	// struct FreeListNode *test_cur = head;
	// while (true) {
	// 	serial_debug("FREE NODE");
	// 	serial_debug("    addr:  0x%x", test_cur);
	// 	serial_debug("    start: 0x%x", test_cur->start);
	// 	serial_debug("    size:  0x%x", test_cur->size);
	// 	serial_debug("    next:  0x%x", test_cur->next);
	// 	serial_debug("    prev:  0x%x", test_cur->prev);
	// 	if (test_cur->next == 0)
	// 		break;
	// 	test_cur = test_cur->next;
	// }
}

void remove_node(struct VirtFreeListNode *node) {
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

// TODO: allocate and map page frames!!!
void *vmm_alloc(u32 pages) {
	// make space for header here
	u64 request_size = pages * PAGE_SIZE + sizeof(struct MemBlockHeader);
	struct VirtFreeListNode *cur = vmm_head;
	// after the last node cur moves to 0
	while (cur != 0) {
		u64 start = cur->start, size = cur->size;
		if (size >= request_size) {
			// exact fit -- delete node by adding to the end
			if (size == request_size)
				remove_node(cur);
			// inexact fit -- shrink node
			else {
				cur->start += request_size;
				cur->size -= request_size;
			}

			// unmap virtual address, allocate page frame, map to new page frame
			serial_info("vmm: request physical page frames");
			for (u32 i = 0; i < pages; i++) {
				u64 virt = start + sizeof(struct MemBlockHeader) + i * PAGE_SIZE;
				PhysicalAddress page_frame = pmm_alloc_kernel();
				page_unmap(virt);
				page_map(virt, page_frame);
			}

			((struct MemBlockHeader *) start)->magic = VMM_MAGIC;
			((struct MemBlockHeader *) start)->size = pages;

			serial_info("vmm: allocate %u pages with header at 0x%x", pages, start);
			return (void *) (start + sizeof(struct MemBlockHeader));
		}
		
		// serial_info("VMM NODE start 0x%x size 0x%x", start, size);
		cur = cur->next;
	}

	panic("Out of memory! (VMM)");
	return 0;
}

void vmm_free(void *mem) {
	struct MemBlockHeader *header = ((struct MemBlockHeader *) mem - 1);
	// save the # of pages before page is freed
	u32 pages = header->size;
	u64 block_start = (u64) header, block_end = (u64) mem + pages * PAGE_SIZE;
	serial_info("vmm: free virt block at 0x%x, %u pages", mem, pages);

	if (header->magic != VMM_MAGIC)
		panic("Incorrect magic number! (VMM)");

	// remap virtual address to normal position, free page frame
	for (u32 i = 0; i < pages; i++) {
		u64 virt = (u64) mem + i * PAGE_SIZE;
		PhysicalAddress page_frame = page_virt_to_phys_addr(virt);
		pmm_free(page_frame);
		page_unmap(virt);
		page_map(virt, virt - KERNEL_OFFSET);
	}

	// add virtual addresses back to linked list for future use
	
	// find node to merge current block with
	struct VirtFreeListNode *cur = vmm_head;
	bool found_merge = false;
	while (cur != 0) {
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
		cur = cur->next;
	}

	// not found then add node by moving (hopefully empty) tail node to head
	if (!found_merge) {
		if (vmm_tail->start && vmm_tail->size)
			panic("Not yet implemented D: (VMM)");

		// make it no longer tail, make previous node tail
		cur = vmm_tail;
		vmm_tail = vmm_tail->prev;
		vmm_tail->next = 0;

		// previous = 0 means head, make it point to current head
		cur->prev = 0;
		cur->next = vmm_head;
		// head is memory but subtract header
		cur->start = (u64) mem - sizeof(struct MemBlockHeader);
		// size is based on number of blocks but add header size
		cur->size = pages * PAGE_SIZE + sizeof(struct MemBlockHeader);
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
			remove_node(merged);
			break;
		}
		if (block_end == start) {
			cur->start -= (block_end - block_start);
			cur->size += (block_end - block_start);
			remove_node(merged);
			break;
		}
		cur = cur->next;
	}
}

void vmm_log_status() {
	serial_debug("vmm: printing status");
	u32 empty_node_count = 0, node_count = 0;
	struct VirtFreeListNode *cur = vmm_head;
	while (cur != 0) {
		u64 start = cur->start, size = cur->size;
		if (start && size)
			serial_debug("vmm: node 0x%x, addr 0x%x, size 0x%x", (u64) cur, start, size);
		else
			empty_node_count++;
		node_count++;
		cur = cur->next;
	}

	serial_debug("vmm: %u nodes, %u empty", node_count, empty_node_count);
}

