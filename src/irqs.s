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
	push byte 0
	push byte 32
	jmp irq_handler

irq1:
	push byte 0
	push byte 33
	jmp irq_handler

irq2:
	push byte 0
	push byte 34
	jmp irq_handler

irq3:
	push byte 0
	push byte 35
	jmp irq_handler

irq4:
	push byte 0
	push byte 36
	jmp irq_handler

irq5:
	push byte 0
	push byte 37
	jmp irq_handler

irq6:
	push byte 0
	push byte 38
	jmp irq_handler

irq7:
	push byte 0
	push byte 39
	jmp irq_handler

irq8:
	push byte 0
	push byte 40
	jmp irq_handler

irq9:
	push byte 0
	push byte 41
	jmp irq_handler

irq10:
	push byte 0
	push byte 42
	jmp irq_handler

irq11:
	push byte 0
	push byte 43
	jmp irq_handler

irq12:
	push byte 0
	push byte 44
	jmp irq_handler

irq13:
	push byte 0
	push byte 45
	jmp irq_handler

irq14:
	push byte 0
	push byte 46
	jmp irq_handler

irq15:
	push byte 0
	push byte 47
	jmp irq_handler

; https://forum.osdev.org/viewtopic.php?f=1&t=56532
irq_handler:
	pusha
	call irq_handle_interrupt
	popa
	add esp, 8
	iret