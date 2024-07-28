#include "pmm.h"
#include "const.h"
#include "output.h"

struct __attribute__((packed)) PageFrameNode {
	struct PageFrameNode *next; // 64 bits = 8 bytes
	u64 space[511]; // 511 * 64 bits = 4088 bytes
	// 4088 + 8 = 4096 bytes = 1 page frame
};

struct PageFrameNode *head;
struct PageFrameNode *memory;

void pmm_init(u64 start, u64 size) {
	asm("xchg bx, bx");
	memory = (struct PageFrameNode *) start;
	u64 frame_count = size / 4096;	
	head = &memory[0];
	for (u64 i = 0; i < frame_count - 1; i++) {
		// fb_printf("%u 0x%x\n", i, &memory[i]);
		memory[i].next = &memory[i + 1];
	}
}
void *pmm_alloc() {
	void *mem = (void *) head;
	head = head->next;
	return mem;
}
void pmm_free(void *mem) {
	struct PageFrameNode *new_head = (struct PageFrameNode *) mem;
	new_head->next = head;
	head = new_head;
}
