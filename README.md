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
