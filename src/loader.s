global loader
global div_zero_test

extern kmain

MAGIC equ 0x1BADB002
FLAGS equ 0x0
CHECKSUM equ -MAGIC
STACK_SIZE equ 4096

section .bss
	align 4
	kernal_stack:
		resb STACK_SIZE

section .text
	align 4
	dd MAGIC
	dd FLAGS
	dd CHECKSUM

loader:
	mov esp, STACK_SIZE + kernal_stack
	push eax
	push ebx
	call kmain
.loop:
	jmp .loop