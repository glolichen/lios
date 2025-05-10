global _start

section .bss
	text: resb 10

section .data
	length equ 10

; NOTE: this will not work on Linux
; In 64 bit, Linux wants system calls to use Intel's syscall instruction
; instead of triggering interrupt 0x80, which is used on 32 bit
; syscall is faster, but I can't find much documentation on what it actually does
; so, I will use int 0x80 and program LiOS system calls to be triggered this way

section .text
_start:
	mov rax, 0
	mov rdi, 0
	mov rsi, text
	mov rdx, length
	syscall

	mov rax, 1
	mov rdi, 1
	mov rsi, text
	mov rdx, length
	syscall

	; int 0x80

	mov rax, 60
	mov rdi, 0
	syscall
	; int 0x80
