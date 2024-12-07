OBJECTS = src/loader.o src/kmain.o src/const.o src/int/interrupt.o src/int/interrupts.o src/panic.o src/io/io.o src/io/keyboard.o src/io/output.o src/io/vga.o src/io/serial.o src/io/vgafont.o src/mem/vmalloc.o src/mem/kmalloc.o src/mem/page.o src/mem/vmm.o src/mem/pmm.o src/testing.o src/kmath.o src/file/acpi.o src/file/nvme.o src/file/gpt.o src/file/fat32.o 

ASM = nasm
ASM_FLAGS = -f elf64

CC = x86_64-elf-gcc
COMPILE_FLAGS = -ffreestanding -mno-red-zone \
				-Wno-variadic-macros -W -Wpedantic -Wextra -Wall -Wcast-align -Wcast-qual -Wstrict-aliasing=2 -Wframe-larger-than=32768 -Wno-strict-overflow -Wsync-nand -Wtrampolines -Wsign-compare -Werror=float-equal -Werror=missing-braces -Werror=init-self -Werror=logical-op -Werror=write-strings -Werror=address -Werror=array-bounds -Werror=char-subscripts -Werror=enum-compare -Werror=implicit-int -Werror=empty-body -Werror=main -Werror=aggressive-loop-optimizations -Werror=nonnull -Werror=parentheses -Werror=pointer-sign -Werror=return-type -Werror=sequence-point -Werror=uninitialized -Werror=volatile-register-var -Werror=ignored-qualifiers -Werror=missing-parameter-type -Werror=old-style-declaration -Wno-error=maybe-uninitialized -Wno-unused-function -Wodr -Wformat-signedness -Wsuggest-final-types -Wsuggest-final-methods -Wno-ignored-attributes -Wno-missing-field-initializers -Wshift-overflow=2 -Wduplicated-cond -Wduplicated-branches -Werror=restrict -Wdouble-promotion -Wformat=2 \
				-c -z max-page-size=0x1000 -mcmodel=kernel -masm=intel -mgeneral-regs-only -O3 -lgcc --debug -g -fno-pie -I gnu-efi/inc/
LINK_FLAGS = -T link.ld -o iso/boot/os.bin -ffreestanding -mcmodel=large -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -O3 -nostdlib -lgcc -z max-page-size=0x1000 -no-pie

QEMU = qemu-system-x86_64
QEMU_FLAGS = -boot c \
			 -cdrom iso/os.iso \
			 -d int,guest_errors,unimp \
			 -D qemulog.txt \
			 -no-reboot \
			 -serial file:serial.out \
			 -M q35 \
			 -m 4G \
			 -cpu qemu64 \
			 -bios copy_OVMF.4m.fd \
			 -monitor stdio \
			 -drive file=disk.img,if=none,format=raw,id=nvm \
			 -device nvme,serial=deadbeef,drive=nvm

			 # -drive file=image.img,if=none,id=drive0 \
			 # -device nvme,drive=drive0,serial=1234 \

			 # -device ich9-ahci,id=ahci0 \
			 # -drive id=disk1,file=image.img,if=none \
			 # -device ide-hd,drive=disk1,bus=ahci0.0 \

			 # -device nvme,drive=mydrive,serial=1234 \
			 # -drive file=image.img,if=none,id=mydrive \
			 # -device piix3-ide,id=ide -drive id=disk,file=image.img,format=raw,if=none -device ide-hd,drive=disk,bus=ide.0

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

# https://unix.stackexchange.com/a/116391
clean:
	rm -rf iso/os.iso qemulog.txt serial.txt serial.out
	find src/ -name '*.o' -delete

gen_bear:
	bear -- make

