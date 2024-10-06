#include <stdbool.h>
#include "pmm.h"
#include "output.h"
#include "const.h"
#include "panic.h"

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
	struct PhysFreeListNode *current;

	bool more_than_2;
	if ((more_than_2 = (end > TWO_GB)))
		stack_space = 16 * ((end - TWO_GB) / PAGE_SIZE + (TWO_GB - start) / PAGE_SIZE);
	else
		stack_space = 16 * (end - start) / PAGE_SIZE;

	// move start forward then 4KiB align
	free_start = (start + stack_space + PAGE_SIZE) & ~(0x1000 - 1);
	stack_start = start;

	u64 addr;
	kernel = (struct PhysFreeListNode *) (start + KERNEL_OFFSET);
	current = kernel;
	for (addr = free_start;; addr += PAGE_SIZE) {
		current->addr = addr;
		if (current->addr + PAGE_SIZE >= (more_than_2 ? TWO_GB : end)) {
			current->next = 0;
			break;
		}
		current->next = current + 1;
		current = current->next;
	}

	u64 free_virt_start = free_start + KERNEL_OFFSET;

	serial_info("pmm: stack start: 0x%x", start);
	serial_info("pmm: mem end: 0x%x", end);
	serial_info("pmm: free start: 0x%x", free_start);
	serial_info("pmm: stack space: 0x%x", stack_space);
	serial_info("pmm: virt addr start: 0x%x", free_virt_start);
	
	if (!more_than_2)
		return free_virt_start;

	// 4KiB align downwards
	end &= ~(PAGE_SIZE - 1);
	addr += PAGE_SIZE;
	current += sizeof(struct PhysFreeListNode *);
	user = current;
	for (;; addr += PAGE_SIZE) {
		current->addr = addr;
		if (current->addr + PAGE_SIZE >= end) {
			current->next = 0;
			break;
		}
		current->next = current + sizeof(struct PhysFreeListNode);
		current = current->next;
	}

	return free_virt_start;
}

PhysicalAddress pmm_alloc_kernel() {
	if (kernel == 0)
		panic("Out of memory! (PMM kernel)");
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
