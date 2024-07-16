global inb
global outb
global io_wait

inb:
	mov dx, [esp + 4]
	in al, dx
	ret

outb:
	mov al, [esp + 8]
	mov dx, [esp + 4]
	out dx, al
	ret

io_wait:
	mov al, 0
	mov dx, 0x80
	out dx, al
	ret
