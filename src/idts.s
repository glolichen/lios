global idt_load

idtr
	dw 0
	dd 0

idt_load:
	mov ax, [esp + 4]
	mov [idtr], ax
	mov eax, [esp + 8]
	mov [idtr + 2], eax
	lidt [idtr]
	ret