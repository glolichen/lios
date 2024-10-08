#include "testing.h"
#include "io/output.h"
#include "mem/heap.h"
#include "mem/vmm.h"
#include "mem/page.h"
#include "mem/pmm.h"

void run_tests() {
	// heap allocation testing code
	void *mem1 = kmalloc(44);
	heap_log_status();
	fb_printf("address: 0x%x\n", mem1);

	void *mem2 = kmalloc(123);
	heap_log_status();
	fb_printf("address: 0x%x\n", mem2);

	void *mem3 = kmalloc(2302);
	heap_log_status();
	fb_printf("address: 0x%x\n", mem3);

	kfree(mem1);
	kfree(mem2);
	kfree(mem3);
	heap_log_status();

	void *mem4 = kmalloc(75 + 64 * 8);
	heap_log_status();
	fb_printf("address: 0x%x\n", mem4);

	void *mem5 = kmalloc(12000);
	heap_log_status();
	fb_printf("address: 0x%x\n", mem5);

	vmm_log_status();
	kfree(mem4);
	kfree(mem5);
	heap_log_status();
	vmm_log_status();

	// virtual memory manager testing
	// u64 *thing1 = (u64 *) vmm_alloc(3);
	// u64 *thing2 = (u64 *) vmm_alloc(2);
	// vmm_log_status();
	// // for (u32 i = 0; i < 10; i++)
	// // 	thing1[i] = 1 << i;
	// // page unmap test, should result in page fault
	// // page_unmap((u64) thing1);
	// // for (u32 i = 0; i < 8; i++)
	// // 	thing1[i] = 80 * i;
	// // for (u32 i = 0; i < 8; i++)
	// // 	thing2[i] = 10 * i;
	// //
	// // for (u32 i = 0; i < 10; i++)
	// // 	fb_printf("%u ", thing1[i]);
	// // fb_printf("\n");
	// // for (u32 i = 0; i < 10; i++)
	// // 	fb_printf("%u ", thing2[i]);
	// // fb_printf("\n");
	//
	// vmm_free(thing1);
	// vmm_log_status();
	// // destroy header test
	// // *((u64 *) ((u64) thing2 - 8)) = 2000;
	// vmm_free(thing2);
	//
	// vmm_log_status();
	//
	// // u64 *thing3 = (u64 *) vmm_alloc(8);
	// u64 *thing4 = (u64 *) vmm_alloc(1);
	// // vmm_free(thing3);
	// vmm_log_status();
	// vmm_free(thing4);
	//
	// vmm_log_status();
	//
	// fb_printf("ok\n");

	// page_map testing code
	// u64 page_frame = pmm_alloc_kernel();
	// asm("xchg bx, bx");
	//
	// page_map(page_frame, page_frame);
	// asm("xchg bx, bx");
	// fb_printf("0x%x 0x%x\n", page_frame, *((u64 *) page_frame));
	// *((u64 *) page_frame) = 0xDEADBEEF;
	//
	// asm("xchg bx, bx");
	// fb_printf("0x%x 0x%x\n", page_frame, *((u64 *) page_frame));
	//
	// page_unmap(page_frame);
	// asm("xchg bx, bx");
	// fb_printf("0x%x 0x%x\n", page_frame, *((u64 *) page_frame));

	// physical allocation testing code
	// u64 mem1 = pmm_alloc_kernel();
	// pmm_free(mem1);
	// u64 mem2 = pmm_alloc_kernel();
	// u64 mem3 = pmm_alloc_kernel();
	// pmm_free(mem2);
	// pmm_free(mem3);
    // 
	// u64 last_mem;
	// for (u64 i = 0;; i++) {
	// 	if (i % 5 == 4)
	// 		pmm_free(last_mem);
	// 	u64 mem = pmm_alloc_user();
	// 	if (i % 5 == 0)
	// 		last_mem = mem;
	// }
}
