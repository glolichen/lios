OBJECTS = src/loader.o src/const.o src/gdts.o src/gdt.o src/pic.o src/idts.o src/idt.o src/isrs.o src/isr.o src/irqs.o src/irq.o src/io.o src/fb.o 	src/serial.o src/printf.o src/kmain.o src/keyboard.o src/pmm.o src/panic.o src/page.o src/pages.o
CC = i686-elf-gcc
CFLAGS = -ffreestanding -mno-red-zone -Wall -Wextra -Wpedantic -c -g --debug
LD = i686-elf-ld
LDFLAGS = -T link.ld -nostdlib
AS = nasm
ASFLAGS = -f elf

all: kernel.elf

kernel.elf: $(OBJECTS)
	$(CC) -T link.ld -o os.bin -ffreestanding -O2 -nostdlib $(OBJECTS) -L. -lgcc

os.iso: kernel.elf
	mv os.bin iso/boot/os.bin
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o os.iso iso/

build: clean kernel.elf

run: clean os.iso
	qemu-system-x86_64 -boot d -cdrom os.iso -d int -D qemulog.txt -no-reboot -serial file:serial.txt -M q35,accel=tcg
	# -s -S

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -rf src/*.o kernel.elf os.iso qemulog.txt serial.txt