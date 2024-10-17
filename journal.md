# LiOS Journal

## Everything before

I wrote most of this stuff during exam week last year and over the summer. This includes
 * A physical memory manager (allocates 4096 byte pages of physical memory)
 * A virtual memory manager (allocates sections of virtual memory in blocks)
 * Heap allocator (manages free virtual memory (which has to correspond to blocks of physical memory) and gives them away in any size (specifically, multiples of 4 bytes))
 * *VGA text mode* frame buffer, which cannot be used on UEFI hardware, which has to be supported to boot on modern hardware
 * Keyboard interrupts, obviously

## Overcomplicated kernel memory scheme

Write now there the kernel memory system is like this:

1. kernel asks heap allocator for some space
2. heap allocator checks if it has enough free memory. if yes, return the memory. if not, ask the virtual memory manager for more
3. *virtual memory manager looks through its linked list to find a free block (blocks are measured in 4KiB pages)*
4. *once it finds one, ask the physical memory manager for that number of pages*
5. *map the virtual memory to the physical memory*
6. go back to step 2

That is a problem. To map pages you need to create page structures (pml4, page directory pointer table, page directory table, page tables) which you also need to allocate. Here is the problem: we obviously cannot use paging to map the page structures to a physical page frame, because then there is a horrible recursion which will not end. So instead we use the fact that the first 2GiB of memory is mapped to the highest 2GiB of the virtual address space. But steps 3-5 will mess this up: what if `page_frame_phys_addr + KERNEL_OFFSET` (which should be the virtual address of the page frame) is already given away by the virtual memory manager?

But what we have before is fine for "normal" kernel structures where physical address does not matter. In Linux this is called `vmalloc`. (In LiOS it is currently called `kmalloc`, confusingly.)

Linux also has `kmalloc`, which allocates *physically* contiguous memory. We will implement a simple version of this, without a heap manager because it's only used for making paging structures, each of which is 4KiB = 1 page.
4. *tell physical memory manager to mark the previous address, subtracted by `KERNEL_OFFSET`, as used*


