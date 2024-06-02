global page_enable

page_enable:
	mov eax, page_directory
	mov cr3, eax

	mov eax, cr0
	or eax, 0x80000001
	mov cr0, eax

	mov eax, cr4
	or eax, 0x10
	mov cr0, eax

	ret