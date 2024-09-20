#include <stdbool.h>
#include "pmm.h"
#include "output.h"
#include "const.h"
#include "panic.h"

#define TWO_GB 2147483648

struct StackNode {
	struct StackNode *next;
	u64 addr;
};

u64 stack_start, free_start;
struct StackNode *kernel = 0, *user = 0;

void pmm_init(u64 start, u64 end) {
	// upper bound for stack space needed
	u64 stack_space;
	struct StackNode *current;

	bool more_than_2;
	if ((more_than_2 = (end > TWO_GB)))
		stack_space = 16 * ((end - TWO_GB) / 4096 + (TWO_GB - start) / 4096);
	else
		stack_space = 16 * (end - start) / 4096;

	// move start forward then 4KiB align
	free_start = (start + stack_space + 0x1000) & ~(0x1000 - 1);
	stack_start = start;

	u64 addr;
	kernel = (struct StackNode *) (start + KERNEL_OFFSET);
	current = kernel;
	for (addr = free_start;; addr += 0x1000) {
		current->addr = addr;
		if (current->addr + 0x1000 >= (more_than_2 ? TWO_GB : end)) {
			current->next = 0;
			break;
		}
		current->next = current + 1;
		current = current->next;
	}

	serial_info("pmm: stack start: 0x%x", start);
	serial_info("pmm: mem end: 0x%x", end);
	serial_info("pmm: free start: 0x%x", free_start);
	serial_info("pmm: stack space: 0x%x", stack_space);
	
	if (!more_than_2)
		return;

	// 4KiB align downwards
	end &= ~(0x1000 - 1);
	addr += 0x1000;
	current += sizeof(struct StackNode *);
	user = current;
	for (;; addr += 0x1000) {
		current->addr = addr;
		if (current->addr + 0x1000 >= end) {
			current->next = 0;
			break;
		}
		current->next = current + sizeof(struct StackNode);
		current = current->next;
	}
}

u64 pmm_alloc_kernel() {
	if (kernel == 0)
		panic("Out of memory!");
	u64 next = (u64) kernel->addr;
	serial_info("pmm: allocating node 0x%x, addr 0x%x (kernel)", kernel, kernel->addr);
	kernel = kernel->next;
	return next;
}

u64 pmm_alloc_user() {
	// if there is no user memory left then allocate something <2GiB
	if (user == 0) {
		serial_info("pmm: kernel (<2GiB) mem allocated for user below");
		return pmm_alloc_kernel();
	}
	u64 next = (u64) user->addr;
	serial_info("pmm: allocating node 0x%x, addr 0x%x (user)", user, user->addr);
	user = user->next;
	return next;
}

void pmm_free(u64 mem) {
	u64 node_addr = stack_start + sizeof(struct StackNode) * (mem - free_start) / 4096 + KERNEL_OFFSET;
	if (mem >= TWO_GB) {
		// freeing user memory
		serial_info("pmm: freeing node 0x%x, addr 0x%x (user)", node_addr, mem);
		struct StackNode *old_user = user;
		user = (struct StackNode *) node_addr;
		user->next = old_user;
		user->addr = mem;
		return;
	}
	// freeing kernel memory
	serial_info("pmm: freeing node 0x%x, addr 0x%x (kernel)", node_addr, mem);
	struct StackNode *old_kernel = kernel;
	kernel = (struct StackNode *) node_addr;
	kernel->next = old_kernel;
	kernel->addr = mem;
}
