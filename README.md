# LiOS

An experimental x86_64 operating system written from scratch in C.

## Currently Implemented
 * Bootstrapping to long mode
 * Interrupts (only keyboard implemented)
 * Printing to VGA (text mode only) and serial
 * Some basic memory management
   * Uses paging (obviously)
   * Linked list based page frame and virtual address allocator
   * Kernel heap allocator (using linked lists and bitmap)

## Memory Layout

Sort of based on Linux.
 * 0xFFFFFFFF80000000-0xFFFFFFFFFFFFFFFF (highest 2GiB) is directly mapped to the first 2GiB of physical memory. Paging structures, and also DMA structures sometime in the future, are stored here.
 * 0xFFFFF00000000000-0xFFFFFFFF80000000 (the rest of higher-half virtual memory) are used for other pieces of kernel memory. Page frames come from the rest of physical memory, outside of the first 2GiB. User page frames also come from here.
 * Lower-half virtual memory for user processes.

## To Do
 * Support UEFI. Move away from VGA text mode. Instead draw each pixel manually... how amazing
 * Virtual address allocator can only use 1 4KiB page, which is only 128 linked list nodes. So if the memory gets too fragmented the allocator will completely break.
 * Filesystem and hard disk/solid state drive driver
 * Processes
 * ELF program loading and userspace programs
 * Userspace shell program

## Sources
I didn't really keep track but here are a couple that I remember.
 * Intel and AMD architecture manuals
 * OSDev wiki
 * Some parts of Operating Systems: Three Easy Pieces (Arpaci-Dusseau)
