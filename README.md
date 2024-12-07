# LiOS

An experimental x86_64 operating system written from scratch in C.

## Currently Implemented

 * Bootstrapping to long mode
 * Interrupts (only keyboard implemented)
 * Printing to VGA graphical mode and serial
 * Some basic memory management
   * Uses paging (obviously)
   * Linked list based page frame and virtual address allocator
   * Kernel heap allocator (using linked lists and bitmap)
 * Supports UEFI

## To Do

 * CPUID stuff?
 * Virtual address allocator can only use 1 4KiB page, which is only 128 linked list nodes. So if the memory gets too fragmented the allocator will completely break.
 * Filesystem and hard disk/solid state drive driver
 * Processes
 * ELF program loading and userspace programs
 * Userspace shell program

## Memory Layout

Sort of based on Linux, with other self-imposed janks as well.
 * 0xFFFFFFFF80000000-0xFFFFFFFFFFFFFFFF (highest 2GiB) is directly mapped to the first 2GiB of physical memory. Paging structures, and also DMA structures sometime in the future, are stored here. Used for `kmalloc`
 * 0xFFFFF00000000000-0xFFFFFFFF80000000 are used for other pieces of kernel memory. Used for `vmalloc`. Page frames used by `vmalloc` come from physical memory above the 2GiB mark. User page frames also come from there.
 * 0xFFFF800000000000-??? reserved for VGA frame buffer virtual address.
 * 0xFFFF900000000000-??? are used to temporarily map PCI device configuration spaces when looking for NVMe drives, and is then used to map the NVMe device for future use.
 * Lower-half virtual memory for user processes.

## Sources

 * [OSTEP book](https://pages.cs.wisc.edu/~remzi/OSTEP/)
 * [Intel](https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html) and [AMD](https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/40332.pdf) manuals
 * [QEMU docs](https://www.qemu.org/docs/master/index.html)
 * [Multiboot2 specification](https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html)
 * [OSDev Wiki](https://wiki.osdev.org)
 * [ACPI specification](https://uefi.org/htmlspecs/ACPI_Spec_6_4_html/index.html)
 * [UEFI specification](https://uefi.org/specs/UEFI/2.10_A/index.html)
 * [NVMe specifications](https://nvmexpress.org/specifications/)
   * NVM Express Base, revision 2.1
   * NVM Command Set, revision 1.1
   * NVMe over PCIe, revision 1.1
 * [Microsoft FAT32 specification](https://academy.cba.mit.edu/classes/networking_communications/SD/FAT.pdf)

write bootable usb: `sudo dd if=iso/os.iso of=/dev/sda`
create qemu drive: `qemu-img create -f raw disk.img [size]` and create GPT with `cfdisk disk.img`

