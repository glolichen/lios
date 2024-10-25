# LiOS Journal

## NVME

Storage and filesystem! Unfortunately there's a list of very confusing and hard to understand steps needed...
1. enumerate PCIe devices
  * to do this we need to find the MCFG ACPI table
  * to find that we need to find the RSDT/XSDT
  * to find that we need to find the Root System Description Pointer (RSDP)
  * to find that it depends on whether we're booted in BIOS of UEFI
     * modern hardware uses UEFI so we probably should make the emulator use UEFI too
     * but QEMU uses BIOS... need to somehow include OVMF so we can boot in UEFI
  * if we're in UEFI, which we should be, GRUB2 will provide us with the EFI system table
  * basically: **make QEMU use UEFI --> find EFI system table --> find RSDP --> find RSDT or XSDT --> find MCFG ACPI --> read memory mapped io base address --> read PCIe devices**
2. write an NVMe driver...
3. (write a layer of abstraction, the virtual filesystem (VFS), but I probably won't)
4. filesystem (such as FAT or ext2)

### Nasty bug

There is a nasty physical memory allocation bug. In the past we assumed that the available physical memory is contiguous and starts at the end of the kernel. This is of course not the case as there are a bunch of random UEFI, architecture or other sections fragmenting our available memory. Fixed by rewriting how the PMM is initialized (by adding "blocks" of available memory instead of all at once).

Implemented in `b0f7b63`.

## VGA graphical mode

In the past, we used VGA text mode. This was a nice system. We could just write an ASCII code to a certain memory offset and that letter is displayed automatically.

VGA text mode is no longer supported when booting with UEFI and only works with legacy BIOS. Unfortunately, modern hardware manufacturers do not want to support legacy BIOS and only support UEFI (our school computers are one such case). So if LiOS has any hope of working on real hardware we have to switch to VGA graphical mode.

In VGA graphical mode you draw pixels instead of letters, and I have no interest in creating a graphical user interface, so the interface will still be terminal based. We will have to download a PC Screen Font (PSF), decode it to figure out which pixels need to be drawn for each characters, and draw them.

We havev no filesystem currently, so we can't just read a PSF file. We will instead hard code the font directly in the C code. Other methods described [here](https://wiki.osdev.org/Drawing_In_a_Linear_Framebuffer) involve quering the BIOS in real mode, fetching them from VGA registers, or linker trickery. This sounds really complicated and hard coding sounds simple enough.

Steps:

1. download 8x16 VGA font [here](https://www.zap.org.au/projects/console-fonts-zap/)
2. use [this](https://github.com/talamus/rw-psf) tool to convert PSF to plain text
3. use script (util/psftxt2ints.cpp) to convert plain text to integers

Implemented in `1b32127`.

## Linux-esque memory allocation scheme

Right now there the kernel memory system is like this:

1. kernel asks heap allocator for some space
2. heap allocator checks if it has enough free memory. if yes, return the memory. if not, ask the virtual memory manager for more
3. *virtual memory manager looks through its linked list to find a free block (blocks are measured in 4KiB pages)*
4. *once it finds one, ask the physical memory manager for that number of pages*
5. *map the virtual memory to the physical memory*
6. go back to step 2

That is a problem. To map pages you need to create page structures (pml4, page directory pointer table, page directory table, page tables) which you also need to allocate. Here is the problem: we obviously cannot use paging to map the page structures to a physical page frame, because then there is a horrible recursion which will not end. So instead we use the fact that the first 2GiB of memory is mapped to the highest 2GiB of the virtual address space. But steps 3-5 will mess this up: what if `page_frame_phys_addr + KERNEL_OFFSET` (which should be the virtual address of the page frame) is already given away by the virtual memory manager?

But what we have before is fine for "normal" kernel structures where physical address does not matter. In Linux this is called `vmalloc`. (In LiOS it is currently called `kmalloc`, confusingly.)

Linux also has `kmalloc`, which allocates *physically* contiguous memory. We will implement a simple version of this, without a heap manager because it's only used for making paging structures, each of which is 4KiB = 1 page.

Implemented in `c52fc2f`.

## Everything before

I wrote most of this stuff during exam week last year and over the summer. This includes
 * A physical memory manager (allocates 4096 byte pages of physical memory)
 * A virtual memory manager (allocates sections of virtual memory in blocks)
 * Heap allocator (manages free virtual memory (which has to correspond to blocks of physical memory) and gives them away in any size (specifically, multiples of 4 bytes))
 * *VGA text mode* frame buffer, which cannot be used on UEFI hardware, which has to be supported to boot on modern hardware
 * Keyboard interrupts, obviously

