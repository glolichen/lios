extern lmain
global _start

; TODO consider multiboot 2
MULTIBOOT_MAGIC equ 0x1BADB002
MULTIBOOT_FLAGS equ 0x0
MULTIBOOT_CHECK equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

NO_CPUID db "CPUID not available!"
NO_CPUID_SIZE equ $-NO_CPUID

NO_EXT_CPUID db "Extended CPUID not available!"
NO_EXT_CPUID_SIZE equ $-NO_EXT_CPUID

NO_LONG_MODE db "No long mode!"
NO_LONG_MODE_SIZE equ $-NO_LONG_MODE

CHECKS_SUCCESS db "Checks successful"
CHECKS_SUCCESS_SIZE equ $-CHECKS_SUCCESS

FRAME_BUFFER equ 0xB8000

section .multiboot
	align 32
	dd MULTIBOOT_MAGIC
	dd MULTIBOOT_FLAGS
	dd MULTIBOOT_CHECK

section .boot_bss
	align 16
stack_top:
	resb 16384 ; reserve 16KB stack for kernel
stack_bottom:

section .boot_rodata

section .boot_text
	bits 32

; clear the "booting os" message
clear_message:
	; this is shit but it works
	mov byte [FRAME_BUFFER], 32
	mov byte [FRAME_BUFFER + 1], 7
	mov byte [FRAME_BUFFER + 2], 32
	mov byte [FRAME_BUFFER + 3], 7
	mov byte [FRAME_BUFFER + 4], 32
	mov byte [FRAME_BUFFER + 5], 7
	mov byte [FRAME_BUFFER + 6], 32
	mov byte [FRAME_BUFFER + 7], 7
	mov byte [FRAME_BUFFER + 8], 32
	mov byte [FRAME_BUFFER + 9], 7
	mov byte [FRAME_BUFFER + 10], 32
	mov byte [FRAME_BUFFER + 11], 7
	mov byte [FRAME_BUFFER + 12], 32
	mov byte [FRAME_BUFFER + 13], 7
	mov byte [FRAME_BUFFER + 14], 32
	mov byte [FRAME_BUFFER + 15], 7
	mov byte [FRAME_BUFFER + 16], 32
	mov byte [FRAME_BUFFER + 17], 7
	mov byte [FRAME_BUFFER + 18], 32
	mov byte [FRAME_BUFFER + 19], 7
	mov byte [FRAME_BUFFER + 20], 32
	mov byte [FRAME_BUFFER + 21], 7
	mov byte [FRAME_BUFFER + 22], 32
	mov byte [FRAME_BUFFER + 23], 7
	mov byte [FRAME_BUFFER + 24], 32
	mov byte [FRAME_BUFFER + 25], 7
	mov byte [FRAME_BUFFER + 26], 32
	mov byte [FRAME_BUFFER + 27], 7
	ret

; uses eax, ebx, ecx, edx; eax: string, ebx: length
error:
	dec ebx
	add ebx, ebx
	add ebx, FRAME_BUFFER
	mov edx, FRAME_BUFFER
error_loop:
	mov cl, [eax]
	mov byte [edx], cl
	mov byte [edx + 1], 7
	cmp edx, ebx
	je halt
	inc edx
	inc edx
	inc eax
	jmp error_loop

_start:
	call clear_message

	cli
	mov esp, stack_bottom
	mov ebp, stack_bottom

	pushad
	
	; xchg bx, bx
	; test for cpuid and long mode
	; https://wiki.osdev.org/CPUID#Checking_CPUID_availability
	; https://wiki.osdev.org/Setting_Up_Long_Mode
	pushfd
	pushfd

	; check if cpu has cpuid
	xor dword [esp], 0x200000
	popfd
	pushfd
	pop eax
	xor eax, [esp]
	popfd
	and eax, 0x200000
	cmp eax, 0
	jne ext_cpuid_check
	mov eax, NO_CPUID
	mov ebx, NO_CPUID_SIZE
	call error

	; check for extended function cpuid
ext_cpuid_check:
	mov eax, 0x80000000
	cpuid
	; max supported less than 0x80000001 = bad
	cmp eax, 0x80000001
	jnb check_long_mode
	mov eax, NO_EXT_CPUID
	mov ebx, NO_EXT_CPUID_SIZE
	call error

	; check for long mode
check_long_mode:
	mov eax, 0x80000001
	cpuid
	test edx, 1 << 29
	jnz checks_complete
	mov eax, NO_LONG_MODE
	mov ebx, NO_LONG_MODE_SIZE
	call error

checks_complete:
	mov eax, cr4
	; enable PAE
	or eax, 0x20
	mov cr4, eax

	popad

	xchg bx, bx
	; push eax
	; push ebx

	; xchg bx, bx

	; mov ecx, 99
	; mov edx, 0xB8000
	; mov [edx], ecx

	; xchg bx, bx

halt:
	jmp halt

section .boot_data
