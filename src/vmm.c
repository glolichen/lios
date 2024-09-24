#include "vmm.h"
#include "output.h"
#include "page.h"
#include "pmm.h"
#include "const.h"
#include "panic.h"
#include <stdbool.h>

// NOTE: yeah it's really bad
// eventually (i.e. in 3 years) I will switch to AVL trees
// but now we just have a first fit flat list structure

// node in linked list that stores free blocks
struct VMMFreeNode {
	struct VMMFreeNode *prev, *next; // prev and next node
	u64 start; // start of current free virtual address block
	u64 size; // size of current block
};
struct VMMMemHeader {
	u32 size; // number of (4KiB) pages
	u32 magic; // magic number for checking
};

struct VMMFreeNode *head, *tail;

void vmm_init(u64 free_virt_start) {
	u64 page_frame = pmm_alloc_kernel();
	page_unmap(free_virt_start);
	page_map(free_virt_start, page_frame);
	head = (struct VMMFreeNode *) free_virt_start;
	head->prev = 0;
	head->start = free_virt_start + PAGE_SIZE;
	head->size = 0xFFFFFFFFFFFFFFFF - head->start;

	// sizeof node is 32 bytes
	// 4096/32 = 128
	// in total will have 128 nodes, already created 1
	// need to move forward 127 times
	struct VMMFreeNode *cur = head;
	for (u32 i = 1; i < 128; i++) {
		// pointer arithemetic -- will increment by 32 bytes
		cur->next = cur + 1;
		cur = cur->next;
		cur->prev = cur - 1;
		cur->start = 0;
		cur->size = 0;
	}
	cur->next = 0;

	serial_info("vmm: created vmm list nodes, starting 0x%x, ending 0x%x", head, cur);

	// check linked list
	// struct VMMFreeNode *test_cur = head;
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

// TODO: allocate and map page frames!!!
void *vmm_alloc(u32 pages) {
	// make space for header here
	u64 request_size = pages * PAGE_SIZE + sizeof(struct VMMMemHeader);
	struct VMMFreeNode *cur = head;
	// after the last node cur moves to 0
	while (cur != 0) {
		u64 start = cur->start, size = cur->size;
		if (size >= request_size) {
			// exact fit -- delete node by adding to the end
			if (size == request_size) {
				cur->start = 0;
				cur->size = 0;
				cur->next->prev = cur->prev;
				cur->prev->next = cur->next;
				cur->next = 0;
				cur->prev = tail;
				tail->next = cur;
				tail = cur;
			}
			// inexact fit -- shrink node
			else {
				cur->start += request_size;
				cur->size -= request_size;
			}

			// unmap virtual address, allocate page frame, map to new page frame
			serial_info("vmm: request physical page frames");
			for (u32 i = 0; i < pages; i++) {
				u64 virt = start + sizeof(struct VMMMemHeader) + i * PAGE_SIZE;
				PhysicalAddress page_frame = pmm_alloc_kernel();
				page_unmap(virt);
				page_map(virt, page_frame);
			}

			((struct VMMMemHeader *) start)->magic = VMM_MAGIC;
			((struct VMMMemHeader *) start)->size = pages;

			serial_info("vmm: allocate %u pages with header at 0x%x", pages, start);
			return (void *) (start + sizeof(struct VMMMemHeader));
		}
		
		// serial_info("VMM NODE start 0x%x size 0x%x", start, size);
		cur = cur->next;
	}

	panic("Out of memory! (VMM)");
	return 0;
}

void vmm_free(void *mem) {
	struct VMMMemHeader *header = ((struct VMMMemHeader *) mem - 1);
	serial_info("vmm: free virt block at 0x%x, size 0x%x", mem, header->size);

	if (header->magic != VMM_MAGIC)
		panic("Incorrect magic number! (VMM)");

	// remap virtual address to normal position, free page frame
	for (u32 i = 0; i < header->size; i++) {
		u64 virt = (u64) mem + i * PAGE_SIZE;
		PhysicalAddress page_frame = page_virt_to_phys_addr(virt);
		pmm_free(page_frame);
		page_unmap(virt);
		page_map(virt, virt - KERNEL_OFFSET);
	}

	// add virtual addresses back to linked list for future use
	
	// find node to merge current block with
	struct VMMFreeNode *cur = head;
	bool found_merge = false;
	while (cur != 0) {
		u64 start = cur->start, size = cur->size;
		cur = cur->next;
	}

	// not found then add node
	if (!found_merge) {
		
		return;
	}	

	// if found then check again to see if can merge again
}

