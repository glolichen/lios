OBJECTS = src/loader.o src/const.o src/gdts.o src/gdt.o src/pic.o src/idts.o src/idt.o src/isrs.o src/isr.o src/irqs.o src/irq.o src/io.o src/fb.o 	src/serial.o src/printf.o src/kmain.o src/keyboard.o
CC = i686-elf-gcc
CFLAGS = -ffreestanding -mno-red-zone -Wall -Wextra -Wpedantic -c -g --debug
LD = i686-elf-ld
LDFLAGS = -T link.ld
AS = nasm
ASFLAGS = -f elf

all: kernel.elf

kernel.elf: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o kernel.elf

os.iso: kernel.elf
	cp kernel.elf iso/boot/kernel.elf
	genisoimage -R                              \
				-b boot/grub/stage2_eltorito    \
				-no-emul-boot                   \
				-boot-load-size 4               \
				-A os                           \
				-input-charset utf8             \
				-quiet                          \
				-boot-info-table                \
				-o os.iso                       \
				iso

build: clean kernel.elf

run: clean os.iso
	qemu-system-x86_64 -boot d -cdrom os.iso -m 512 -d int -D qemulog.txt -serial file:serial.txt \
	# -s -S

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

clean:
	rm -rf src/*.o kernel.elf os.iso qemulog.txt serial.txt