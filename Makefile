# OBJECTS = src/loader.o src/const.o src/gdts.o src/gdt.o src/pic.o src/idts.o src/idt.o src/isrs.o src/isr.o src/irqs.o src/irq.o src/io.o src/serial.o src/output.o src/kmain.o src/keyboard.o src/pmm.o src/panic.o src/page.o
OBJECTS = src/loader.o

all: kernel.elf

kernel: $(OBJECTS)
	x86_64-elf-gcc -T link.ld -o iso/boot/os.bin -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -O2 -nostdlib -lgcc -z max-page-size=0x1000 $(OBJECTS)

os.iso: kernel
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o iso/os.iso iso/

run: clean os.iso
	bochs -f bochsrc.txt -q
	# qemu-system-x86_64 -boot d -cdrom os.iso -d int -D qemulog.txt -no-reboot -serial file:serial.txt -M q35,accel=tcg

%.o: %.c
	# TODO fix
	x86_64-elf-gcc -ffreestanding -mno-red-zone -Wall -Wextra -Wpedantic -c -g --debug -z max-page-size=0x1000 -masm=intel $< -o $@

%.o: %.s
	nasm -f elf64 $< -o $@

clean:
	rm -rf src/*.o kernel.elf iso/os.iso qemulog.txt serial.txt