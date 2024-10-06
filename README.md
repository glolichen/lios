# LiOS

An experimental x86_64 operating system written from scratch in C.

## Currently Implemented
 * Bootstrapping to long mode
 * Interrupts (only keyboard implemented)
 * Basic `printf` to VGA and serial
 * Some basic memory management
   * Uses paging (obviously)
   * Linked list based page frame and virtual address allocator

## Planned Features
 * Kernel heap allocator (in progress, more linked lists + bitmap)
 * Virtual address allocator can only use 1 4KiB page, which is 128 linked list nodes! This is quite bad and needs to be fixed ASAP
 * Filesystem (likely `ext2`)
 * ELF program loading and userspace programs
 * Userspace shell program
 * More efficient memory allocation (buddy, slab/slub, something other than linked list)
 * Port some C library (though this is probably 2 years away)

## Sources
I didn't really keep track but here are a couple that I remember.
 * Intel and AMD architecture manuals
 * OSDev wiki
 * Some parts of Operating Systems: Three Easy Pieces (Arpaci-Dusseau)
