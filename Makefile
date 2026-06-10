CC := x86_64-elf-gcc
LD := x86_64-elf-ld
NASM := nasm
OUTPUT := bin/cat_kernel.elf
ISO_IMG := out/cat.iso

CFLAGS  := -Wall -Wextra -O2 -std=c11 -ffreestanding \
           -fno-stack-protector -fno-stack-check -fno-lto -fno-pic -fno-pie \
           -m64 -march=x86-64 -mabi=sysv \
           -mno-80387 -mno-mmx -mno-sse -mno-sse2 -mno-red-zone \
           -mcmodel=kernel -Isrc/include

NASMFLAGS := -f elf64

LDFLAGS := -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -T linker.lds


SRCS_C   := $(shell find src -name "*.c")
SRCS_ASM := $(shell find src -name "*.s" -o -name "*.S")
SRCS_NASM := $(shell find src -name "*.asm")

OBJS     := $(patsubst src/%.c,obj/%.o,$(SRCS_C)) \
            $(patsubst src/%.s,obj/%.o,$(patsubst src/%.S,obj/%.o,$(SRCS_ASM))) \
            $(patsubst src/%.asm,obj/%.o,$(SRCS_NASM))

all: $(OUTPUT)

$(OUTPUT): $(OBJS)
	@mkdir -p bin
	@mkdir -p out
	@echo "[LD] Linking kernel binary..."
	$(LD) $(LDFLAGS) $(OBJS) -o $@

obj/%.o: src/%.c
	@mkdir -p $(@D)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.s
	@mkdir -p $(@D)
	@echo "[AS] $<"
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.S
	@mkdir -p $(@D)
	@echo "[AS] $<"
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/%.asm
	@mkdir -p $(@D)
	@echo "[AS-NAS] $<"
	$(NASM) $(NASMFLAGS) $< -o $@

$(ISO_IMG): $(OUTPUT) limine
	@echo "[ISO] Staging bootable directory tree..."
	@mkdir -p iso_root/boot/limine
	@mkdir -p iso_root/EFI/BOOT
	
	@cp $(OUTPUT) iso_root/boot/
	@cp limine.cfg iso_root/boot/limine/ 2>/dev/null || cp limine.cfg iso_root/ 2>/dev/null || true
	
	@cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/boot/limine/
	@cp limine/BOOTX64.EFI iso_root/EFI/BOOT/
	@cp limine/BOOTIA32.EFI iso_root/EFI/BOOT/
	@cp limine.conf iso_root/boot/limine/
	@cp -r copy_to_iso/* iso_root/ 2>/dev/null || true
	
	@echo "[ISO] Mastering image with xorriso..."
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
		-apm-block-size 2048 --efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO_IMG)
	
	@echo "[ISO] Flashing MBR boot sectors..."
	@./limine/limine bios-install $(ISO_IMG) 2>/dev/null

limine:
	@rm -rf limine limine-binary    
	@curl -L https://github.com/Limine-Bootloader/Limine/releases/latest/download/limine-binary.tar.gz | gunzip | tar -xf -
	@mv limine-binary/ limine
	$(MAKE) -C limine

.PHONY: run
run: $(ISO_IMG)
	@echo "[LAUNCH] Initializing CAT core inside QEMU..."
	qemu-system-x86_64 -m 2G -cdrom $(ISO_IMG) -serial stdio

.PHONY: clean
clean:
	@echo "[CLEAN] Destroying build artifacts..."
	rm -rf bin obj iso_root $(ISO_IMG)