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
	mov rdi, 0
	mov rsi, 0
	jmp isr_handler

isr1:
	mov rdi, 1
	mov rsi, 0
	jmp isr_handler

isr2:
	mov rdi, 2
	mov rsi, 0
	jmp isr_handler

isr3:
	mov rdi, 3
	mov rsi, 0
	jmp isr_handler

isr4:
	mov rdi, 4
	mov rsi, 0
	jmp isr_handler

isr5:
	mov rdi, 5
	mov rsi, 0
	jmp isr_handler

isr6:
	mov rdi, 6
	mov rsi, 0
	jmp isr_handler

isr7:
	mov rdi, 7
	mov rsi, 0
	jmp isr_handler

isr8:
	mov rdi, 8
	pop rsi
	jmp isr_handler

isr9:
	mov rdi, 9
	mov rsi, 0
	jmp isr_handler

isr10:
	mov rdi, 10
	pop rsi
	jmp isr_handler

isr11:
	mov rdi, 11
	pop rsi
	jmp isr_handler

isr12:
	mov rdi, 12
	pop rsi
	jmp isr_handler

isr13:
	mov rdi, 13
	pop rsi
	jmp isr_handler

isr14:
	mov rdi, 14
	pop rsi
	jmp isr_handler

isr15:
	mov rdi, 15
	mov rsi, 0
	jmp isr_handler

isr16:
	mov rdi, 16
	mov rsi, 0
	jmp isr_handler

isr17:
	mov rdi, 17
	pop rsi
	jmp isr_handler

isr18:
	mov rdi, 18
	mov rsi, 0
	jmp isr_handler

isr19:
	mov rdi, 19
	mov rsi, 0
	jmp isr_handler

isr20:
	mov rdi, 20
	mov rsi, 0
	jmp isr_handler

isr21:
	mov rdi, 21
	mov rsi, 0
	jmp isr_handler

isr22:
	mov rdi, 22
	mov rsi, 0
	jmp isr_handler

isr23:
	mov rdi, 23
	mov rsi, 0
	jmp isr_handler

isr24:
	mov rdi, 24
	mov rsi, 0
	jmp isr_handler

isr25:
	mov rdi, 25
	mov rsi, 0
	jmp isr_handler

isr26:
	mov rdi, 26
	mov rsi, 0
	jmp isr_handler

isr27:
	mov rdi, 27
	mov rsi, 0
	jmp isr_handler

isr28:
	mov rdi, 28
	mov rsi, 0
	jmp isr_handler

isr29:
	mov rdi, 29
	mov rsi, 0
	jmp isr_handler

isr30:
	mov rdi, 30
	pop rsi
	jmp isr_handler

isr31:
	mov rdi, 31
	mov rsi, 0
	jmp isr_handler

isr_handler:
	call isr_handle_exception
	iret
