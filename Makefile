OBJECTS = src/loader.o src/kmain.o src/const.o src/interrupt.o src/interrupts.o src/panic.o src/io/io.o src/io/keyboard.o src/io/output.o src/io/vga.o src/io/serial.o src/io/vgafont.o src/mem/vmalloc.o src/mem/kmalloc.o src/mem/page.o src/mem/vmm.o src/mem/pmm.o src/testing.o src/kmath.o src/pcie/acpi.o

ASM = nasm
ASM_FLAGS = -f elf64

CC = x86_64-elf-gcc
COMPILE_FLAGS = -ffreestanding -mno-red-zone -Wall -Wextra -Wpedantic -c -z max-page-size=0x1000 -mcmodel=kernel -masm=intel -mgeneral-regs-only -O2 -lgcc --debug -g -fno-pie -I gnu-efi/inc/
LINK_FLAGS = -T link.ld -o iso/boot/os.bin -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -O2 -nostdlib -lgcc -z max-page-size=0x1000 -no-pie

QEMU = qemu-system-x86_64
QEMU_FLAGS = -boot d \
			 -cdrom iso/os.iso \
			 -d int,guest_errors,unimp \
			 -D qemulog.txt \
			 -no-reboot \
			 -serial file:serial.out \
			 -machine q35 \
			 -m 4096M \
			 -cpu qemu64 \
			 -bios /usr/share/edk2-ovmf/x64/OVMF.fd
			 # -drive if=pflash,format=raw,unit=0,file=/usr/share/edk2-ovmf/x64/OVMF_CODE.fd,readonly=on \
			 # -drive if=pflash,format=raw,unit=1,file=/usr/share/edk2-ovmf/x64/OVMF_VARS.fd \
			 # -net none \
			 # -drive file=nvm.img,if=none,id=NVME1 \
			 # -device nvme,serial=1234,drive=NVME1 \
			 # -monitor stdio

# qemu-system-x86_64 -m 2048 \
	# -hda ./vdisk/16GB.img \
	# -drive file=./vdisk/nvme_dut.img,if=none,id=drv0 \
	# -device nvme,drive=drv0,serial=foo \
	# --enable-kvm \
	# -smp 2

build: clean os.iso

all: kernel.elf

kernel: $(OBJECTS)
	$(CC) $(LINK_FLAGS) $(OBJECTS)

os.iso: kernel
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o iso/os.iso iso/

debug: clean os.iso
	bochs -f bochsrc.txt -q

run: clean os.iso
	$(QEMU) $(QEMU_FLAGS)

%.o: %.c
	$(CC) $(COMPILE_FLAGS) $< -o $@

%.o: %.s
	$(ASM) $(ASM_FLAGS) $< -o $@

clean:
	rm -rf iso/os.iso qemulog.txt serial.txt serial.out
	# https://unix.stackexchange.com/a/116391
	find src/ -name '*.o' -delete

gen_bear:
	bear -- make

