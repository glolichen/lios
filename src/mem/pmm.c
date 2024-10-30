#include <stdbool.h>
#include "pmm.h"
#include "../io/output.h"
#include "../const.h"
#include "../panic.h"
#include "../kmath.h"

#define TWO_GB 2147483648

struct PhysFreeListNode {
	struct PhysFreeListNode *next;
	u64 addr;
};

struct PhysFreeListNode *pmm_low = 0, *pmm_high = 0, *pmm_init_temp = 0, *pmm_free_list = 0;
u64 total_size, nodes_needed;

void pmm_set_total(u64 size) {
	total_size = size;
	// # nodes * page size = total mem - # nodes * sizeof(node)
	// # nodes * (page size + sizeof(node)) = total mem
	// # nodes = total mem / (page size + sizeof(node))
	nodes_needed = floor_u64_div(total_size, PAGE_SIZE + sizeof(struct PhysFreeListNode)) - 1;
	serial_info("pmm: total available memory: 0x%x bytes (%u nodes)", total_size, nodes_needed);
}

void linked_list_add_node(u64 start) {
	if (!pmm_low) {
		pmm_low = (struct PhysFreeListNode *) (start + KERNEL_OFFSET);
		pmm_low->addr = 0;
		pmm_init_temp = pmm_low;
		nodes_needed--;
		return;
	}
	pmm_init_temp->next = (struct PhysFreeListNode *) (start + KERNEL_OFFSET);
	pmm_init_temp->next->addr = 0;
	pmm_init_temp = (struct PhysFreeListNode *) (start + KERNEL_OFFSET);
	nodes_needed--;
}

void pmm_add_block(u64 start, u64 end) {
	serial_info("pmm: add block: start 0x%x end 0x%x", start, end);
	if (nodes_needed == 0) {
		start = (start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
		for (u64 i = start; i < end && pmm_init_temp; i += PAGE_SIZE) {
			pmm_init_temp->addr = i;
			pmm_init_temp = pmm_init_temp->next;
		}
		return;
	}

	u64 cur = start;
	for (; cur < end; cur += sizeof(struct PhysFreeListNode)) {
		linked_list_add_node(cur);
		if (nodes_needed == 0)
			break;
	}
	if (nodes_needed != 0)
		return;

	pmm_init_temp = pmm_low;
	if (cur < end)
		pmm_add_block(cur, end);
}

void pmm_init_final(void) {
	struct PhysFreeListNode *cur = pmm_low;
	while (cur != 0) {
		if (cur->next->addr >= TWO_GB) {
			pmm_high = cur->next;
			cur->next = 0;
			break;
		}
		cur = cur->next;
	}
}

PhysicalAddress pmm_alloc_low(void) {
	if (pmm_low == 0)
		panic("pmm: out of memory!");

	u64 next = (u64) pmm_low->addr;

	struct PhysFreeListNode *node = pmm_low;

	serial_info("pmm: allocate node 0x%x, addr 0x%x (kernel)", pmm_low, pmm_low->addr);
	pmm_low = pmm_low->next;

	node->next = pmm_free_list;
	pmm_free_list = node;

	return next;
}

PhysicalAddress pmm_alloc_high(void) {
	// if there is no user memory left then allocate something <2GiB
	if (pmm_high == 0) {
		serial_info("pmm: kernel (<2GiB) mem allocate for user below");
		panic("pmm: not yet implented!");
		// return pmm_alloc_kernel();
	}

	u64 next = (u64) pmm_high->addr;

	struct PhysFreeListNode *node = pmm_high;

	serial_info("pmm: allocate node 0x%x, addr 0x%x (user)", pmm_high, pmm_high->addr);
	pmm_high = pmm_high->next;

	node->next = pmm_free_list;
	pmm_free_list = node;

	return next;
}

void pmm_free(PhysicalAddress mem) {
	struct PhysFreeListNode *cur = pmm_free_list;
	while (cur != 0) {
		if (cur->addr == mem)
			break;
		cur = cur->next;
	}

	if (cur == 0)
		panic("pmm: free: cannot find node!");

	if (mem >= TWO_GB) {
		// freeing user memory
		serial_info("pmm: free node 0x%x, addr 0x%x (user)", cur, mem);
		struct PhysFreeListNode *old_user = pmm_high;
		pmm_high = (struct PhysFreeListNode *) cur;
		pmm_high->next = old_user;
		pmm_high->addr = mem;
		serial_info("pmm ok");
		return;
	}
	// freeing kernel memory
	serial_info("pmm: free node 0x%x, addr 0x%x (kernel)", cur, mem);
	struct PhysFreeListNode *old_kernel = pmm_low;
	pmm_low = (struct PhysFreeListNode *) cur;
	pmm_low->next = old_kernel;
	pmm_low->addr = mem;
}

// reserve parts of physical memory for something (just the frame buffer)
// only works before anything is allocated
void pmm_clear_blocks(u64 start, u64 end) {
	// pretend it has to be above 2GiB, I guess
	// find first and last node that need to be removed
	struct PhysFreeListNode *cur = pmm_high, *start_node = 0, *end_node = 0;
	while (cur != 0) {
		if (cur->addr <= start - PAGE_SIZE)
			start_node = cur;
		if (cur->addr >= end) {
			end_node = cur;
			break;
		}
		cur = cur->next;
	}

	if (!start_node || !end_node) {
		serial_info("pmm: nothing to reserve");
		return;
	}

	// skip everything from the start node to the end node
	serial_info("pmm: 0x%x points to 0x%x (reserve 0x%x -- 0x%x)",
			  start_node, end_node, start_node->next->addr, end_node->addr);
	start_node->next = end_node->next;
}

void pmm_log_status(void) {
	struct PhysFreeListNode *cur;
	serial_debug("pmm: printing status");

	u64 count = 0;
	cur = pmm_low;
	serial_debug("    kernel: first: 0x%x, node at 0x%x (0x%x phys)",
			  cur->addr, cur, (u64) cur - KERNEL_OFFSET);
	while (cur != 0) {
		count++;
		if (cur->next == 0) {
			serial_debug("    kernel: last 0x%x, node at 0x%x (0x%x phys)",
				cur->addr, cur, (u64) cur - KERNEL_OFFSET);
		}
		cur = cur->next;
	}
	serial_debug("    kernel: count: 0x%x", count);

	count = 0;
	cur = pmm_high;
	serial_debug("    user: first: 0x%x, node at 0x%x (0x%x phys)",
			  cur->addr, cur, (u64) cur - KERNEL_OFFSET);
	while (cur != 0) {
		count++;
		if (cur->next == 0) {
			serial_debug("    user: last 0x%x, node at 0x%x (0x%x phys)",
				cur->addr, cur, (u64) cur - KERNEL_OFFSET);
		}
		cur = cur->next;
	}
	serial_debug("    user: count: 0x%x", count);
}
