extern enter_user_mode

bits 64
; first parameter = RDI = stack base
; this will become RSP/current stack and RBP/stack top
; this means we start with empty stack and grow down

; second parameter = RSI = entry point
enter_user_mode:
	; NOTE: ORing segment descriptor with 3 to set requestor privilege level (RPL)
	; this is needed for IRETQ to not #GP -- RPL has to equal privilege of the segment
	
	mov ax, 32 | 3
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax

	; push user iata segment
	push 32 | 3

	; push first parameter, which is the stack pointer
	; this will tell CPU to set what is in RDI as RSP after going to ring 3
	push rdi

	; push rflags
	pushfq

	; push user code segment
	push 24 | 3

	; push return address/rip
	cmp rsi, 0
	jnz if_end
	mov rsi, user_mode_start
if_end:
	push rsi

	; set stack base address to the first parameter
	mov rbp, rdi

	iretq

user_mode_start:
	push 0xDEAD
	pop rax

	nop
	nop
	nop
	nop
	nop
	; sti
	; mov rax, 0
	; mov rbx, 0
	; div rbx
	; hlt
	jmp $
