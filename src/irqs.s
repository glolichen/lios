extern irq_handle_interrupt

global irq0
global irq1
global irq2
global irq3
global irq4
global irq5
global irq6
global irq7
global irq8
global irq9
global irq10
global irq11
global irq12
global irq13
global irq14
global irq15

irq0:
	mov rdi, 32
	mov rsi, 0
	jmp irq_handler

irq1:
	mov rdi, 33
	mov rsi, 0
	jmp irq_handler

irq2:
	mov rdi, 34
	mov rsi, 0
	jmp irq_handler

irq3:
	mov rdi, 35
	mov rsi, 0
	jmp irq_handler

irq4:
	mov rdi, 36
	mov rsi, 0
	jmp irq_handler

irq5:
	mov rdi, 37
	mov rsi, 0
	jmp irq_handler

irq6:
	mov rdi, 38
	mov rsi, 0
	jmp irq_handler

irq7:
	mov rdi, 39
	mov rsi, 0
	jmp irq_handler

irq8:
	mov rdi, 40
	mov rsi, 0
	jmp irq_handler

irq9:
	mov rdi, 41
	mov rsi, 0
	jmp irq_handler

irq10:
	mov rdi, 42
	mov rsi, 0
	jmp irq_handler

irq11:
	mov rdi, 43
	mov rsi, 0
	jmp irq_handler

irq12:
	mov rdi, 44
	mov rsi, 0
	jmp irq_handler

irq13:
	mov rdi, 45
	mov rsi, 0
	jmp irq_handler

irq14:
	mov rdi, 46
	mov rsi, 0
	jmp irq_handler

irq15:
	mov rdi, 47
	mov rsi, 0
	jmp irq_handler

; https://forum.osdev.org/viewtopic.php?f=1&t=56532
irq_handler:
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rsp
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	call irq_handle_interrupt

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rsp
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax

	add esp, 8
	iret
