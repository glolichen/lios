extern handle_exception
extern handle_interrupt

%macro pushregs 0
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
%endmacro
%macro popregs 0
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
%endmacro

%macro exception_no_code_stub 1
global exception%1
exception%1:
	push 0
	push %1
	jmp exception_handler
%endmacro

%macro exception_code_stub 1
global exception%1
exception%1:
	push %1
	jmp exception_handler
%endmacro

%macro irq_stub 1
global irq%1
irq%1:
	push 0
	push 32+%1
	jmp irq_handler
%endmacro

exception_no_code_stub 0
exception_no_code_stub 1
exception_no_code_stub 2
exception_no_code_stub 3
exception_no_code_stub 4
exception_no_code_stub 5
exception_no_code_stub 6
exception_no_code_stub 7
exception_code_stub 8
exception_no_code_stub 9
exception_code_stub 10
exception_code_stub 11
exception_code_stub 12
exception_code_stub 13
exception_code_stub 14
exception_no_code_stub 15
exception_no_code_stub 16
exception_code_stub 17
exception_no_code_stub 18
exception_no_code_stub 19
exception_no_code_stub 20
exception_code_stub 21
exception_no_code_stub 22
exception_no_code_stub 23
exception_no_code_stub 24
exception_no_code_stub 25
exception_no_code_stub 26
exception_no_code_stub 27
exception_no_code_stub 28
exception_code_stub 29
exception_code_stub 30
exception_no_code_stub 31

irq_stub 0
irq_stub 1
irq_stub 2
irq_stub 3
irq_stub 4
irq_stub 5
irq_stub 6
irq_stub 7
irq_stub 8
irq_stub 9
irq_stub 10
irq_stub 11
irq_stub 12
irq_stub 13
irq_stub 14
irq_stub 15

exception_handler:
	pushregs
	mov rdi, rsp
	call handle_exception
	popregs
	add rsp, 16
	iretq

; https://forum.osdev.org/viewtopic.php?f=1&t=56532
irq_handler:
	pushregs
	mov rdi, rsp
	call handle_interrupt
	popregs
	add rsp, 16
	iretq
