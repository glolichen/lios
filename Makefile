# OBJECTS = src/loader.o src/const.o src/gdts.o src/gdt.o src/pic.o src/idts.o src/idt.o src/isrs.o src/isr.o src/irqs.o src/irq.o src/io.o src/serial.o src/output.o src/kmain.o src/keyboard.o src/pmm.o src/panic.o src/page.o
OBJECTS = src/loader.o src/kmain.o src/const.o src/output.o src/io.o src/serial.o src/interrupt.o src/interrupts.o src/keyboard.o

ASM = nasm
ASM_FLAGS = -f elf64

CC = x86_64-elf-gcc
COMPILE_FLAGS = -ffreestanding -mno-red-zone -Wall -Wextra -Wpedantic -c -z max-page-size=0x1000 -mcmodel=large -masm=intel
LINK_FLAGS = -T link.ld -o iso/boot/os.bin -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -O2 -nostdlib -lgcc -z max-page-size=0x1000 -fPIC

QEMU_FLAGS = -boot d -cdrom iso/os.iso -d int -D qemulog.txt -no-reboot -serial file:serial.out -M q35,accel=tcg

all: kernel.elf

kernel: $(OBJECTS)
	$(CC) $(LINK_FLAGS) $(OBJECTS)

os.iso: kernel
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o iso/os.iso iso/

run: clean os.iso
	bochs -f bochsrc.txt -q

run_qemu: clean os.iso
	qemu-system-x86_64 $(QEMU_FLAGS)

build: clean os.iso

%.o: %.c
	$(CC) $(COMPILE_FLAGS) $< -o $@

%.o: %.s
	$(ASM) $(ASM_FLAGS) $< -o $@

clean:
	rm -rf src/*.o iso/os.iso qemulog.txt serial.txt serial.out