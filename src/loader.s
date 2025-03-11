global _start
global mboot

extern kmain

bits 32

section .text
	%define no_offset(addr) addr - 0xFFFFFFFF80000000

	MBOOT_MAGIC equ 0xE85250D6
	MBOOT_ARCH equ 0
	MBOOT_HEADER_LENGTH equ no_offset(mboot_end) - no_offset(mboot)
	MBOOT_CHECK equ 0x100000000 - (MBOOT_MAGIC + MBOOT_ARCH + MBOOT_HEADER_LENGTH)

mboot:
	align 8
	dd MBOOT_MAGIC
	dd MBOOT_ARCH
	dd MBOOT_HEADER_LENGTH
	dd MBOOT_CHECK

	align 8
	dw 5
	dw 0
	dd 20
	dd 1024
	dd 768
	dd 32

	align 8
	dw 0
	dw 0
	dd 8
mboot_end:

_start:
	cli

	mov esp, no_offset(stack_top)
	mov ebp, no_offset(stack_bottom)
	; save multiboot struct pointer before ebx gets used
	mov [no_offset(mboot_struct_ptr)], ebx

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

	; remap PIC (maybe move this later...)
	mov dx, 0x20
	mov al, 0x11
	out dx, al

	mov dx, 0x80
	mov al, 0
	out dx, al

	mov dx, 0xA0
	mov al, 0x11
	out dx, al

	mov dx, 0x80
	mov al, 0
	out dx, al

	mov dx, 0x21
	mov al, 0x20
	out dx, al
 
	mov dx, 0x80
	mov al, 0
	out dx, al

	mov dx, 0xA1
	mov al, 0x28
	out dx, al

	mov dx, 0x80
	mov al, 0
	out dx, al

	mov dx, 0x21
	mov al, 0x04
	out dx, al
	
	mov dx, 0x80
	mov al, 0
	out dx, al

	mov dx, 0xA1
	mov al, 0x02
	out dx, al

	mov dx, 0x80
	mov al, 0
	out dx, al

	mov dx, 0x21
	mov al, 0x01
	out dx, al

	mov dx, 0x80
	mov al, 0
	out dx, al

	mov dx, 0xA1
	mov al, 0x01
	out dx, al

	mov dx, 0x80
	mov al, 0
	out dx, al

	xchg bx, bx

	lgdt [no_offset(gdt_ptr)]
	jmp 0x08:long_mode_start

bits 64
long_mode_start:
	xchg bx, bx
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp higher_half_text
  
section .kernel_text
higher_half_text:
	mov rsp, stack_top
	mov rbp, stack_bottom
	; put the offset back into gdt_ptr before removing identity map
	mov qword [gdt_ptr + 2], gdt_start
	lgdt [gdt_ptr]

	xchg bx, bx

	; move to registers for use in kmain
	; https://en.wikipedia.org/wiki/X86_calling_conventions#System_V_AMD64_ABI
	mov rdi, gdt_tss
	mov rsi, tss_start
	mov rdx, tss_end
	mov rcx, [mboot_struct_ptr]
	mov r8, cr3
	mov r9, pdpt_low
	push pdt_low
	push pt_low

	xchg bx, bx

	call kmain
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
stack_bottom:
	resb 0x4000
stack_top:

; reserve 4 bytes = 32 bits for mboot struct ptr 
mboot_struct_ptr:
	resb 4
; reserve 8 bytes = 64 bits for interrupt stack table
ist1:
	resb 8
ist2:
	resb 8
ist3:
	resb 8
ist4:
	resb 8
ist5:
	resb 8
ist6:
	resb 8
ist7:
	resb 8
privilege0_stack:
	resb 0x1000
	
; section .rodata

section .data
tss_start:
	dd 0 ; reserved
	dq no_offset(privilege0_stack)
	dq 0
	dq 0
	dq 0 ; reserved
	dq no_offset(ist1)
	dq no_offset(ist2)
	dq no_offset(ist3)
	dq no_offset(ist4)
	dq no_offset(ist5)
	dq no_offset(ist6)
	dq no_offset(ist7)
	dq 0 ; reserved
	dw 0 ; reserved
	dw 0 ; no I/O for now
tss_end:

gdt_ptr:
	dw no_offset(gdt_end) - no_offset(gdt_start) - 1
	dq no_offset(gdt_start)

gdt_start:
gdt_null:
	dd 0x0
	dd 0x0

gdt_kernel_code:
	dw 0xFFFF ; limit
	dw 0x0 ; base
	db 0x0 ; base
	db 0x9A ; access
	db 0xAF ; flags and limit
	db 0x0 ; base
gdt_kernel_data:
	dw 0xFFFF
	dw 0x0
	db 0x0
	db 0x92
	db 0xAF
	db 0x0

gdt_user_code:
	dw 0xFFFF
	dw 0x0
	db 0x0
	db 0xFA
	db 0xAF
	db 0x0
gdt_user_data:
	dw 0xFFFF
	dw 0x0
	db 0x0
	db 0xF2
	db 0xAF
	db 0x0

; filled by C code in kmain
gdt_tss:
	dw 0 ; limit
	dw 0 ; base
	db 0 ; base
	db 0 ; access
	db 0 ; flags and limit
	db 0 ; base
	dd 0 ; base
	dd 0 ; reserved
gdt_end:

