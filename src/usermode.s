extern enter_user_mode

bits 64
enter_user_mode:
	; mov rbp, rsi
	; mov rsp, rdi
	;
	; push 0xDEADBEEF
	; mov rcx, 0xDEAD
	;
	; mov rax, 0
	; mov rbx, 0
	; div rbx
	;
	; ret



	; NOTE: ORing segment descriptor with 3 to set requestor privilege level (RPL)
	; this is needed for IRET to not #GP -- RPL has to equal privilege of the segment

	; mov qword [rdi], 500
	; ret
	
	mov ax, 32 | 3
	mov ds, ax
	mov es, ax 
	mov fs, ax 
	mov gs, ax

	; TODO: what should the stack pointer be??
	; mov rax, rsp
	mov rax, rdi

	; read first parameter which is stored in rdi
	; mov rax, rdi

	; push user iata segment
	push 32 | 3

	; push stack pointer
	push rax

	; push rflags
	pushfq

	; push user code segment
	push 24 | 3

	; push return address/rip
	push user_mode_start

	mov rbp, rsi

	mov qword [rdi], 0xDEAD

	iretq

user_mode_start:
	push 0x6969
	push 0x6969
	push 0x6969
	push 0x6969
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	; cli
	mov rax, 0
	mov rbx, 0
	div rbx
	; hlt
	jmp $
