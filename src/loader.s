global _start
global mboot

bits 32

section .text
	%define no_offset(addr) addr - 0xFFFFFFFF80000000

; TODO consider multiboot 2
	MULTIBOOT_MAGIC equ 0x1BADB002
	MULTIBOOT_FLAGS equ 0x0
	MULTIBOOT_CHECK equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

	NOT_MULTIBOOT db "Not multiboot!"
	NOT_MULTIBOOT_SIZE equ $-NOT_MULTIBOOT

	NO_CPUID db "CPUID not available!"
	NO_CPUID_SIZE equ $-NO_CPUID

	NO_EXT_CPUID db "Extended CPUID not available!"
	NO_EXT_CPUID_SIZE equ $-NO_EXT_CPUID

	NO_LONG_MODE db "No long mode!"
	NO_LONG_MODE_SIZE equ $-NO_LONG_MODE

	FRAME_BUFFER equ 0xB8000

mboot:
	align 32
	dd MULTIBOOT_MAGIC
	dd MULTIBOOT_FLAGS
	dd MULTIBOOT_CHECK

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
	je error_end
	inc edx
	inc edx
	inc eax
	jmp error_loop
error_end:
	jmp error_end


_start:
	call clear_message

	cli
	mov esp, no_offset(stack)
	mov ebp, no_offset(stack)

	cmp eax, 0x2BADB002
	je is_multiboot
	mov eax, NOT_MULTIBOOT
	mov ebx, NOT_MULTIBOOT_SIZE
	call error

is_multiboot:
	; pushad
	; https://wiki.osdev.org/CPUID#Checking_CPUID_availability
	; https://wiki.osdev.org/Setting_Up_Long_Mode
	; check if cpu has cpuid
	pushfd
	pushfd
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
	; enable PAE
	mov eax, cr4
	or eax, 0x20
	mov cr4, eax

	; disable paging (for now)
	mov eax, cr0
	and eax, 0x7FFFFFFF
	mov cr0, eax

	; PML4 points to PDPT
	mov eax, no_offset(pdpt_low)
	or eax, 0x03
	mov [no_offset(pml4)], eax
	
	mov eax, no_offset(pdpt_high)
	or eax, 0x03
	mov [no_offset(pml4) + 511 * 8], eax

	; move PML4 to CR3
	mov eax, no_offset(pml4)
	mov cr3, eax

	; last 2 PDPTEs point to high PDTs
	; 510 * 8 bytes = second to last PDPTE
	mov eax, no_offset(pdt_high)
	or eax, 0x03
	mov [no_offset(pdpt_high) + 510 * 8], eax

	mov eax, no_offset(pdt_high) + 512 * 8
	or eax, 0x03
	mov [no_offset(pdpt_high) + 511 * 8], eax

	; first 2 PDPTEs point to low PDTs
	mov eax, no_offset(pdt_low)
	or eax, 0x03
	mov [no_offset(pdpt_low)], eax

	mov eax, no_offset(pdt_low) + 512 * 8
	or eax, 0x03
	mov [no_offset(pdpt_low) + 8], eax

	mov eax, no_offset(pt_high)
	or eax, 0x03
	mov ebx, no_offset(pdt_high)
	; 2 PDTs, each with 512 entries
	mov ecx, 2 * 512
populate_pdt_high_loop:
	mov [ebx], eax
	; eax -> PT, size of PT is 0x1000
	add eax, 0x1000
	; ebx -> PDE, size of PDE is 8
	add ebx, 8
	dec ecx
	jnz populate_pdt_high_loop

	mov eax, 0
	or eax, 0x03
	mov ebx, no_offset(pt_high)
	; 512 PDEs = 512 PTs = 1024 * 512 PTEs
	mov ecx, 1024 * 512
populate_pt_high_loop:
	mov [ebx], eax
	; eax -> page frame of size 0x1000
	add eax, 0x1000
	; ebx -> PTE, size of PTE is 8
	add ebx, 8
	dec ecx
	jnz populate_pt_high_loop

	mov eax, no_offset(pt_low)
	or eax, 0x03
	mov ebx, no_offset(pdt_low)
	; 2 PDTs, each with 512 entries
	mov ecx, 2 * 512
populate_pdt_low_loop:
	mov [ebx], eax
	; eax -> PT, size of PT is 0x1000
	add eax, 0x1000
	; ebx -> PDE, size of PDE is 8
	add ebx, 8
	dec ecx
	jnz populate_pdt_low_loop

	mov eax, 0
	or eax, 0x03
	mov ebx, no_offset(pt_low)
	; 512 PDEs = 512 PTs = 1024 * 512 PTEs
	mov ecx, 1024 * 512
populate_pt_low_loop:
	mov [ebx], eax
	; eax -> page frame of size 0x1000
	add eax, 0x1000
	; ebx -> PTE, size of PTE is 8
	add ebx, 8
	dec ecx
	jnz populate_pt_low_loop

	; read EFER MSR, then enable long mode
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	; enable paging
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	lgdt [no_offset(gdtr)]
	jmp 0x08:long_mode_start
	; popad

bits 64
long_mode_start:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp $

section .bss
	align 0x1000
pml4:
	resb 0x1000
pdpt_low:
	resb 0x1000
pdpt_high:
	resb 0x1000
pdt_low:
	resb 0x1000 * 2
pdt_high:
	resb 0x1000 * 2
pt_low:
	resb 0x1000 * 1024
pt_high:
	resb 0x1000 * 1024

	stack resb 0x4000
	

section .rodata

section .data
gdtr:
	dw no_offset(gdt_end) - no_offset(gdt) - 1
	dq no_offset(gdt)
gdt:
gdt_null:
	dd 0x0
	dd 0x0
gdt_code:
	dw 0xFFFF
	dw 0x0
	db 0x0
	db 0x9A
	db 0xAF
	db 0x0
gdt_data:
	dw 0xFFFF
	dw 0x0
	db 0x0
	db 0x92
	db 0xCF
	db 0x0
gdt_end: