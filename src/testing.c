#include "testing.h"
#include "io/output.h"
#include "mem/kmalloc.h"
#include "mem/vmalloc.h"
#include "mem/vmm.h"
#include "mem/page.h"
#include "mem/pmm.h"
#include "file/nvme.h"
#include "file/fat32.h"
#include "util/const.h"
#include "util/hexdump.h"

void test_div0(void) {
	asm volatile("mov rax, 0; mov rbx, 0; div rbx");
}

void fat32_test(void) {
	struct FAT32_OpenResult file_data = fat32_open("HLWORLD", "OUT");
	if (file_data.cluster == 0)
		vga_printf("file read error: %s\n", FAT32_OPEN_ERRORS[file_data.size_or_error.error]);
	else {
		vga_printf("file read size: %u\n", file_data.size_or_error.size);
		void *buffer = vcalloc(file_data.size_or_error.size * 512);
		fat32_read(file_data.cluster, file_data.size_or_error.size, buffer);
		hexdump(buffer, file_data.size_or_error.size * 512, true);
		vfree(buffer);
	}

	struct FAT32_NewFileResult info = fat32_new_file("peddie", "die");
	if (info.fd != 0)
		vga_printf("new file creation successful at %u\n", info.fd);
	else
		vga_printf("new file creation error: %s\n", FAT32_NEW_FILE_ERRORS[info.error]);
}

void test_run_tests(void) {
	vga_printf("starting tests\n");

	fat32_test();

	return;

	vmm_log_status();
	vmalloc_log_status();

	void *thing = vmalloc(5000);
	vfree(thing);

	vga_printf("ok\n");
	
	u64 addrs[5];
	for (int i = 0; i < 5; i++) {
		vga_printf("%u\n", i);
		serial_info("COMEHERE %u", i);
		addrs[i] = (u64) vmalloc(5000);
	}

	vga_printf("ok\n");

	// for (int i = 0; i < 5; i++) {
	// 	vga_printf("%u\n", i);
	// 	vfree((void *) addrs[i]);
	// }
	for (int i = 0; i < 5; i += 2) {
		vga_printf("%u\n", i);
		vfree((void *) addrs[i]);
	}
	// vmm_log_status();
	// vmalloc_log_status();
	for (int i = 1; i < 5; i += 2) {
		vga_printf("%u\n", i);
		vfree((void *) addrs[i]);
	}

	vga_printf("tests complete\n");

	// vmm_log_status();
	// vmalloc_log_status();


	// serial_info("===== TESTING BELOW =====");
	//
	// u64 *write = (u64 *) vcalloc(NVME_LBA_SIZE);
	// write[0] = 0x4242424241414141;
	// write[1] = 0x4242424241414141;
	// nvme_write(36, 1, write);

	// volatile u64 *read = (u64 *) vcalloc(NVME_LBA_SIZE);
	// nvme_read(nvme, 1, 1, read);
	// for (u32 i = 0; i < (NVME_LBA_SIZE / 8); i++)
	// 	serial_info("%u: 0x%x", i, read[i]);

	return;

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
	void *mem1 = vmalloc(44);
	vmalloc_log_status();
	serial_info("address: 0x%x", mem1);

	void *mem2 = vmalloc(123);
	vmalloc_log_status();
	serial_info("address: 0x%x", mem2);

	void *mem3 = vmalloc(252);
	vmalloc_log_status();
	serial_info("address: 0x%x", mem3);

	vfree(mem1);
	vfree(mem2);
	vfree(mem3);
	vmalloc_log_status();

	void *mem4 = vmalloc(75 + 64 * 8);
	vmalloc_log_status();
	serial_info("address: 0x%x", mem4);

	void *mem5 = vmalloc(12000);
	vmalloc_log_status();
	serial_info("address: 0x%x", mem5);

	vmm_log_status();
	vmalloc_log_status();
	vfree(mem4);
	vmalloc_log_status();
	vfree(mem5);
	vmalloc_log_status();
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
