#include "testing.h"
#include "io/output.h"
#include "mem/kmalloc.h"
#include "mem/vmalloc.h"
#include "mem/vmm.h"
#include "mem/page.h"
#include "mem/pmm.h"

void run_tests() {
	// serial_info("===== TESTING BELOW =====");
	// u64 *kmem1 = (u64 *) kmalloc_page();
	// u64 *kmem2 = (u64 *) kmalloc_page();
	//
	// kmem1[5] = 123;
	// kmem1[9] = 932;
	// for (u32 i = 1; i < 5; i++)
	// 	kmem1[i] = 1 << i, kmem2[i + 4] = i * 9;
	//
	// for (u32 i = 0; i < 20; i++)
	// 	serial_info("i: %u %u", kmem1[i], kmem2[i]);
	//
	// kfree_page((u64) kmem2);
	//
	// for (u32 i = 0; i < 20; i++)
	// 	serial_info("i: %u %u", kmem1[i], 0);
	//
	// kfree_page((u64) kmem1);
	//
	// u64 *kmem3 = (u64 *) kmalloc_page();
	// kmem3[2] = 69420;
	// for (u32 i = 0; i < 20; i++)
	// 	serial_info("i: %u", kmem1[i]);
	// kfree_page((u64) kmem3);

	// heap allocation testing code
	// void *mem1 = vmalloc(44);
	// vmalloc_log_status();
	// serial_info("address: 0x%x", mem1);
	//
	// void *mem2 = vmalloc(123);
	// vmalloc_log_status();
	// serial_info("address: 0x%x", mem2);
	//
	// void *mem3 = vmalloc(2302);
	// vmalloc_log_status();
	// serial_info("address: 0x%x", mem3);
	//
	// vfree(mem1);
	// vfree(mem2);
	// vfree(mem3);
	// vmalloc_log_status();
	//
	// void *mem4 = vmalloc(75 + 64 * 8);
	// vmalloc_log_status();
	// serial_info("address: 0x%x", mem4);
	//
	// void *mem5 = vmalloc(12000);
	// vmalloc_log_status();
	// serial_info("address: 0x%x", mem5);
	//
	// vmm_log_status();
	// vmalloc_log_status();
	// vfree(mem4);
	// vmalloc_log_status();
	// vfree(mem5);
	// vmalloc_log_status();
	// vmm_log_status();

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
	// // 	vga_printf("%u ", thing1[i]);
	// // vga_printf("\n");
	// // for (u32 i = 0; i < 10; i++)
	// // 	vga_printf("%u ", thing2[i]);
	// // vga_printf("\n");
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
	// vga_printf("ok\n");

	// page_map testing code
	// u64 page_frame = pmm_alloc_kernel();
	// asm("xchg bx, bx");
	//
	// page_map(page_frame, page_frame);
	// asm("xchg bx, bx");
	// vga_printf("0x%x 0x%x\n", page_frame, *((u64 *) page_frame));
	// *((u64 *) page_frame) = 0xDEADBEEF;
	//
	// asm("xchg bx, bx");
	// vga_printf("0x%x 0x%x\n", page_frame, *((u64 *) page_frame));
	//
	// page_unmap(page_frame);
	// asm("xchg bx, bx");
	// vga_printf("0x%x 0x%x\n", page_frame, *((u64 *) page_frame));

	// physical allocation testing code
	// u64 mem1 = pmm_alloc_low();
	// pmm_free(mem1);
	// u64 mem2 = pmm_alloc_low();
	// u64 mem3 = pmm_alloc_low();
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
