global _start

section .data
message: db "Type here: "
length equ $-message

message2: db "You typed: "
length2 equ $-message2

entered_message: dq 0, 0, 0, 0
entered_length equ $-entered_message

section .text
_start:
	mov rax, 1
	mov rdi, 1
	mov rsi, message
	mov rdx, length
	int 128

	mov rax, 0
	mov rdi, 0
	mov rsi, entered_message
	mov rdx, entered_length
	int 128
	
	mov rax, 1
	mov rdi, 1
	mov rsi, message2
	mov rdx, length2
	int 128

	mov rsi, entered_message
	mov rdx, entered_length
	int 128

	jmp $

