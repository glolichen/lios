# OBJECTS = src/loader.o src/const.o src/gdts.o src/gdt.o src/pic.o src/idts.o src/idt.o src/isrs.o src/isr.o src/irqs.o src/irq.o src/io.o src/serial.o src/output.o src/kmain.o src/keyboard.o src/pmm.o src/panic.o src/page.o
OBJECTS = src/loader.o src/kmain.o src/const.o src/output.o src/io.o src/serial.o src/interrupt.o src/interrupts.o src/keyboard.o src/pmm.o src/page.o src/panic.o src/kmalloc.o src/vmm.o

ASM = nasm
ASM_FLAGS = -f elf64

CC = x86_64-elf-gcc
COMPILE_FLAGS = -ffreestanding -mno-red-zone -Wall -Wextra -Wpedantic -c -z max-page-size=0x1000 -mcmodel=kernel -masm=intel -mgeneral-regs-only -O2 -lgcc --debug -g -fno-pie -Wno-unused-parameter -Wno-unused-variable
LINK_FLAGS = -T link.ld -o iso/boot/os.bin -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -O2 -nostdlib -lgcc -z max-page-size=0x1000 -no-pie

QEMU_FLAGS = -boot d -cdrom iso/os.iso -d int -D qemulog.txt -no-reboot -serial file:serial.out -M q35,accel=tcg -m 4096M
# QEMU_FLAGS = -boot d -cdrom iso/os.iso -d int -D qemulog.txt -no-reboot -serial file:serial.out -M q35,accel=tcg -m 4096M

all: kernel.elf

kernel: $(OBJECTS)
	$(CC) $(LINK_FLAGS) $(OBJECTS)

os.iso: kernel
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o iso/os.iso iso/

debug: clean os.iso
	bochs -f bochsrc.txt -q

run: clean os.iso
	qemu-system-x86_64 $(QEMU_FLAGS)

build: clean os.iso

%.o: %.c
	$(CC) $(COMPILE_FLAGS) $< -o $@

%.o: %.s
	$(ASM) $(ASM_FLAGS) $< -o $@

clean:
	rm -rf src/*.o iso/os.iso qemulog.txt serial.txt serial.out
