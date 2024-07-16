extern isr_handle_exception

global isr0
global isr1
global isr2
global isr3
global isr4
global isr5
global isr6
global isr7
global isr8
global isr9
global isr10
global isr11
global isr12
global isr13
global isr14
global isr15
global isr16
global isr17
global isr18
global isr19
global isr20
global isr21
global isr22
global isr23
global isr24
global isr25
global isr26
global isr27
global isr28
global isr29
global isr30
global isr31

isr0:
	push byte 0
	push byte 0
	jmp isr_handler

isr1:
	push byte 0
	push byte 1
	jmp isr_handler

isr2:
	push byte 0
	push byte 2
	jmp isr_handler

isr3:
	push byte 0
	push byte 3
	jmp isr_handler

isr4:
	push byte 0
	push byte 4
	jmp isr_handler

isr5:
	push byte 0
	push byte 5
	jmp isr_handler

isr6:
	push byte 0
	push byte 6
	jmp isr_handler

isr7:
	push byte 0
	push byte 7
	jmp isr_handler

isr8:
	push byte 8
	jmp isr_handler

isr9:
	push byte 0
	push byte 9
	jmp isr_handler

isr10:
	push byte 10
	jmp isr_handler

isr11:
	push byte 11
	jmp isr_handler

isr12:
	push byte 12
	jmp isr_handler

isr13:
	push byte 13
	jmp isr_handler

isr14:
	push byte 14
	jmp isr_handler

isr15:
	push byte 0
	push byte 15
	jmp isr_handler

isr16:
	push byte 0
	push byte 16
	jmp isr_handler

isr17:
	push byte 17
	jmp isr_handler

isr18:
	push byte 0
	push byte 18
	jmp isr_handler

isr19:
	push byte 0
	push byte 19
	jmp isr_handler

isr20:
	push byte 0
	push byte 20
	jmp isr_handler

isr21:
	push byte 0
	push byte 21
	jmp isr_handler

isr22:
	push byte 0
	push byte 22
	jmp isr_handler

isr23:
	push byte 0
	push byte 23
	jmp isr_handler

isr24:
	push byte 0
	push byte 24
	jmp isr_handler

isr25:
	push byte 0
	push byte 25
	jmp isr_handler

isr26:
	push byte 0
	push byte 26
	jmp isr_handler

isr27:
	push byte 0
	push byte 27
	jmp isr_handler

isr28:
	push byte 0
	push byte 28
	jmp isr_handler

isr29:
	push byte 0
	push byte 29
	jmp isr_handler

isr30:
	push byte 30
	jmp isr_handler

isr31:
	push byte 0
	push byte 31
	jmp isr_handler

isr_handler:
	xchg bx, bx
	call isr_handle_exception
	iret
