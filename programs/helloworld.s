global _start

section .data
message: db "Hello world!", 10
length equ $-message

section .text
_start:
	mov rax, 1
	mov rdi, 1
	mov rsi, message
	mov rdx, length
	int 128
	int 128
	int 128
	int 128
	int 128

	jmp $

