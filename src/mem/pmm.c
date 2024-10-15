#include <stdbool.h>
#include "pmm.h"
#include "../io/output.h"
#include "../const.h"
#include "../panic.h"

#define TWO_GB 2147483648

struct PhysFreeListNode {
	struct PhysFreeListNode *next;
	u64 addr;
};

u64 stack_start, free_start;
struct PhysFreeListNode *kernel = 0, *user = 0;

// returns start of free virtual memory
u64 pmm_init(u64 start, u64 end) {
	// upper bound for stack space needed
	u64 stack_space;
	struct PhysFreeListNode *cur;

	bool more_than_2;
	if ((more_than_2 = (end > TWO_GB)))
		stack_space = 16 * ((end - TWO_GB) / PAGE_SIZE + (TWO_GB - start) / PAGE_SIZE);
	else
		stack_space = 16 * (end - start) / PAGE_SIZE;

	// move start forward then 4KiB align
	free_start = (start + stack_space + PAGE_SIZE) & ~(PAGE_SIZE - 1);
	stack_start = start;

	serial_info("pmm: stack start: 0x%x", start);
	serial_info("pmm: mem end: 0x%x", end);
	serial_info("pmm: free start: 0x%x", free_start);
	serial_info("pmm: stack space: 0x%x", stack_space);

	u64 addr;
	kernel = (struct PhysFreeListNode *) (start + KERNEL_OFFSET);
	cur = kernel;
	for (addr = free_start;; addr += PAGE_SIZE) {
		cur->addr = addr;
		if ((u64) cur->addr + PAGE_SIZE >= (more_than_2 ? TWO_GB : end)) {
			cur->next = 0;
			break;
		}
		cur->next = cur + 1;
		cur = cur->next;
	}

	if (!more_than_2)
		return (u64) (cur + 1 + PAGE_SIZE) & ~(PAGE_SIZE - 1);

	// 4KiB align downwards
	end &= ~(PAGE_SIZE - 1);
	addr += PAGE_SIZE;
	cur += 1;
	user = cur;
	for (;; addr += PAGE_SIZE) {
		cur->addr = addr;
		if ((u64) cur->addr + PAGE_SIZE >= end) {
			cur->next = 0;
			break;
		}
		cur->next = cur + 1;
		cur = cur->next;
	}

	return (u64) (cur + 1 + PAGE_SIZE) & ~(PAGE_SIZE - 1);
}

PhysicalAddress pmm_alloc_kernel() {
	if (kernel == 0)
		panic("pmm: out of memory!");
	u64 next = (u64) kernel->addr;
	serial_info("pmm: allocate node 0x%x, addr 0x%x (kernel)", kernel, kernel->addr);
	kernel = kernel->next;
	return next;
}

PhysicalAddress pmm_alloc_user() {
	// if there is no user memory left then allocate something <2GiB
	if (user == 0) {
		serial_info("pmm: kernel (<2GiB) mem allocate for user below");
		return pmm_alloc_kernel();
	}
	u64 next = (u64) user->addr;
	serial_info("pmm: allocate node 0x%x, addr 0x%x (user)", user, user->addr);
	user = user->next;
	return next;
}

void pmm_free(PhysicalAddress mem) {
	u64 node_addr = stack_start + sizeof(struct PhysFreeListNode) * (mem - free_start) / PAGE_SIZE + KERNEL_OFFSET;
	if (mem >= TWO_GB) {
		// freeing user memory
		serial_info("pmm: free node 0x%x, addr 0x%x (user)", node_addr, mem);
		struct PhysFreeListNode *old_user = user;
		user = (struct PhysFreeListNode *) node_addr;
		user->next = old_user;
		user->addr = mem;
		return;
	}
	// freeing kernel memory
	serial_info("pmm: free node 0x%x, addr 0x%x (kernel)", node_addr, mem);
	struct PhysFreeListNode *old_kernel = kernel;
	kernel = (struct PhysFreeListNode *) node_addr;
	kernel->next = old_kernel;
	kernel->addr = mem;
}

// reserve parts of physical memory for something (just the frame buffer)
// only works before anything is allocated
void pmm_clear_blocks(u64 start, u64 end) {
	// pretend it has to be above 2GiB, I guess
	// find first and last node that need to be removed
	struct PhysFreeListNode *cur = user, *start_node = 0, *end_node = 0;
	u64 count = 0;
	while (cur != 0) {
		count++;
		if (cur->addr <= start - PAGE_SIZE)
			start_node = cur;
		if (cur->addr >= end) {
			end_node = cur;
			break;
		}
		cur = cur->next;
	}

	// skip everything from the start node to the end node
	serial_info("pmm: 0x%x points to 0x%x (reserve 0x%x -- 0x%x)",
			  start_node, end_node, start_node->next->addr, end_node->addr);
	start_node->next = end_node->next;
}

void pmm_log_status() {
	struct PhysFreeListNode *cur;
	serial_debug("pmm: printing status");

	u64 count = 0;
	cur = kernel;
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
	cur = user;
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
