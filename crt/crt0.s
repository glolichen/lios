; https://en.wikipedia.org/wiki/Crt0
; https://wiki.osdev.org/Creating_a_C_Library

; - kernel places argv on the stack
; - kernel places argc in rdi
;
; - calling convention: argc = rdi, argv = rsi

; NOTE: assume no argc/argv for now

global _start

extern main
section .text

_start:
	; TODO: argv/argv

	call main

	; return code placed in rax by calling convention
	; move into rdi for syscall
	mov rdi, rax

	; rax is syscall number
	; mov rax, EXIT SYSCALL NUMBER
	; int 128

