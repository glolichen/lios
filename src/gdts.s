global gdt_load

gdtr
	dw 0
	dd 0

gdt_load:
	cli
	mov ax, [esp + 4]
	mov [gdtr], ax
	mov eax, [esp + 8]
	mov [gdtr + 2], eax
	lgdt [gdtr]
	mov eax, cr0
	or eax, 1
	mov cr0, eax
	jmp 0x08:reload_code_segment
reload_code_segment:
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	ret