# LiOS Journal

## File system

I have decided to use FAT32. It is quite simple (compared to ext2 and others) while also being quite capable (unlike FAT12/16, except for the 4GB file size limit) and widely supported on existing operating systems.

The LiOS implementation of FAT32 will NOT support:
- directories (this means we are capped at [65536](https://serverfault.com/a/1013729) files but I will probably never get to that point)
- long file names
- more than 1 sector per cluster

### Creating the disk image

Ideally I would like the disk image to have partitions which are formatted in our chosen file system, and for us to be able to mount that file on our local computer for reading and writing.

1. Create the disk `qemu-img create -f raw disk.img [size]`
2. Add a GUID Partition Table (GPT) `parted disk.img mklabel gpt`
3. Create a FAT32 partition `parted disk.img mkpart primary fat32 0% 100%`
4. Set up a loop device `sudo losetup -Pf --show disk.img`
5. Format the partition (`parted` only creates) `sudo mkfs.fat -F 32 /dev/loopNp1`

### Reading and writing

To read and write for testing (i.e. from my real computer) simply set up the loop device (`sudo losetup -Pf --show disk.img`) and udiskie will automatically mount it.

### GPT

Before we can even get to implementing/programming the file system, we need to first read the GUID Partition Table (GPT) to find a list of partitions, which is in Logical Block Address (LBA) 1, which by default is 512 bytes. The 32 LBAs from 2 to 33 are used to store partition entries. Each partition entry is 128 bytes so each LBA stores 4 partitions, so with 32 LBAs a maximum of 128 partitions is possible.

LiOS has two very major restrictions on partitioning. Only the Microsoft basic data partition (which uses FAT) can be used, and the disk must only have one such Microsoft partition, which will be used by the OS for storage. This makes it a lot easier for me.

## Programming the NVMe Device

After locating the NVMe base address, we need to create the I/O Submission and Completion Queues, which involves sending commands to the device using the Admin Submission Queue. This entails configuring the Controller Capabilities field (CC), restarting the controller, and sending two commands. Base spec P138 and onwards is quite useful for decoding the posted completion queue entry.

This step was quite confusing as NVMe is quite a complex standard, but after asking around the internet and reading the manual I have figured out how to do this.

I have also figured out how to send the "read" and "write" commands through the created IO submission queue. This allows us to read and write to a storage device, which is pretty huge. That by itself is not very useful, as we need a few more abstractions to have a properly functioning filesystem.

Something quite neat here is because our `vmalloc` heap allocator works in sections of 16 bytes, we do not need to worry about NVMe's requirement of dword (4 byte) alignment for physical region page (PRP).

Implemented in `309d017`.

## Enumerate Devices

This works. We are able to make QEMU attach an NVMe drive to the emulated system, the OS can find the PCI configuration spaces using memory mapped IO, list the devices and are able to find this attached drive. It shows up as class code `0x1`, subclass `0x8`, prog IF `0x2`, which is an NVM Express Mass Storage Controller according to [this table](https://wiki.osdev.org/PCI#Class_Codes).

![A picture of the emulator](./media/pci_nvme.png)

## File System -- Big Picture

Storage and filesystem! Unfortunately there's a list of very confusing and hard to understand steps needed...

1. enumerate PCIe devices
  - to do this we need to find the MCFG ACPI table
  - to find that we need to find the RSDT/XSDT
  - to find that we need to find the Root System Description Pointer (RSDP)
  - to find that it depends on whether we're booted in BIOS of UEFI
     - modern hardware uses UEFI so we probably should make the emulator use UEFI too
     - but QEMU uses BIOS... need to somehow include OVMF so we can boot in UEFI
  - if we're in UEFI, which we should be, GRUB2 will provide us with the EFI system table
  - basically: **make QEMU use UEFI --> find EFI system table --> find RSDP --> find RSDT or XSDT --> find MCFG ACPI --> read memory mapped io base address --> read PCIe devices**
2. write an NVMe driver...
3. filesystem (such as FAT or ext2)
4. (write a layer of abstraction, the virtual filesystem (VFS), but I probably won't)

## Problems with real harwdare

I had some problems booting on real hardware which I asked about on the osdev forums [here](https://forum.osdev.org/viewtopic.php?p=349986). There were a number of serious problems...

- Stack pointer (`esp`/`rsp`) should point to the top of stack not the bottom.
- Linked list created in `pmm.c` is not terminated with a zero, and in case the amount of physical memory is less than 2GB it will attempt to dereference some uninitialized garbage which will cause either a page fault or general protection fault.
- Virtual memory manager's `alloc_list` has the same problem.
- `[page structure]_set_addr` in `page.c` does not clear/zero the address before setting the new one.
- When allocating a new page structure, that memory/page is not initialized, which may lead to a nonpresent structure being read as present. I made a function called `kcalloc_page` which initializes it to zero and use this for creating new page structures instead.
- Memory used to store printed chars in `vga.c` is also uninitialized, use a new `vcalloc` function instead.

As a result, LiOS can now run on real hardware!

Implemented in `dbbddc0`.

## Redesigned page frame allocation

There is a nasty physical memory allocation bug. In the past we assumed that the available physical memory is contiguous and starts at the end of the kernel. This is of course not the case as there are a bunch of random UEFI, architecture or other sections fragmenting our available memory. Fixed by rewriting how the PMM is initialized (by adding "blocks" of available memory instead of all at once).

Implemented in `9a0aebe`.

## VGA graphical mode

In the past, we used VGA text mode. This was a nice system. We could just write an ASCII code to a certain memory offset and that letter is displayed automatically.

VGA text mode is no longer supported when booting with UEFI and only works with legacy BIOS. Unfortunately, modern hardware manufacturers do not want to support legacy BIOS and only support UEFI (our school computers are one such case). So if LiOS has any hope of working on real hardware we have to switch to VGA graphical mode.

In VGA graphical mode you draw pixels instead of letters, and I have no interest in creating a graphical user interface, so the interface will still be terminal based. We will have to download a PC Screen Font (PSF), decode it to figure out which pixels need to be drawn for each characters, and draw them.

We have no filesystem currently, so we can't just read a PSF file. We will instead hard code the font directly in the C code. Other methods described [here](https://wiki.osdev.org/Drawing_In_a_Linear_Framebuffer) involve quering the BIOS in real mode, fetching them from VGA registers, or linker trickery. This sounds really complicated and hard coding sounds simple enough.

Steps:

1. download 8x16 VGA font [here](https://www.zap.org.au/projects/console-fonts-zap/)
2. use [this](https://github.com/talamus/rw-psf) tool to convert PSF to plain text
3. write a script (util/psftxt2ints.cpp) to convert plain text to integers

Implemented in `1b32127` and `45ee8c1`.

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

- A physical memory manager (allocates 4096 byte pages of physical memory)
- A virtual memory manager (allocates sections of virtual memory in blocks)
- Heap allocator (manages free virtual memory (which has to correspond to blocks of physical memory) and gives them away in any size (specifically, multiples of 4 bytes))
- *VGA text mode* frame buffer, which cannot be used on UEFI hardware, which has to be supported to boot on modern hardware
- Keyboard interrupts, obviously

