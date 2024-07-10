global _start
global page_directory

extern kmain

MULTIBOOT_MAGIC equ 0x1BADB002
MULTIBOOT_FLAGS equ 0x2

; 3.1.2 at https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
section .multiboot
	; 32 bit = 4 byte = dd
	dd MULTIBOOT_MAGIC
	dd MULTIBOOT_FLAGS
	dd -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS) ; checksum

section .bss
	align 16
stack_bottom:
	resb 16384 ; reserver 16KB stack for kernel
stack_top:

section .text
_start:
	; set cr3 page directory pointer
	mov ecx, (page_directory - 0xC0000000)
	mov cr3, ecx

	; enable PSE for identity mapping
	mov ecx, cr4
	or ecx, 0x10
	mov cr4, ecx

	; enable paging
	mov ecx, cr0
	or ecx, 0x80000000
	mov cr0, ecx

	mov esp, stack_top ; move stack pointer on top of kernel reserved stack
	push ebx ; push multiboot pointer
	push eax ; should contain magic value 0x2BADB002
	call kmain

; infinite loop
halt:
	jmp halt

section .data
align 4096
page_directory:
	; identity map first 4MB (use to PSE)
	dd 0x83
	times 768-1 dd 0
	dd 0x83
	times 256-1 dd 0