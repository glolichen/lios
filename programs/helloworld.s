global _start

section .data
	text: db "Hello world", 10
	length equ $-text

; NOTE: this will not work on Linux
; In 64 bit, Linux wants system calls to use Intel's syscall instruction
; instead of triggering interrupt 0x80, which is used on 32 bit
; syscall is faster, but I can't find much documentation on what it actually does
; so, I will use int 0x80 and program LiOS system calls to be triggered this way

section .text
_start:
	mov rax, 1
	mov rdi, 1
	mov rsi, text
	mov rdx, length
	; syscall
	int 0x80

	mov rax, 60
	mov rdi, 0
	; syscall
	int 0x80
