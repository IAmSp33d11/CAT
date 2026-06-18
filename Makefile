CC := x86_64-elf-gcc
LD := x86_64-elf-ld
NASM := nasm

OUTPUT      := bin/cat_kernel.elf
USER_OUTPUT := bin/init.bin
ISO_IMG     := out/cat.iso
BIOS        := /usr/share/edk2/x64/OVMF.4m.fd

CFLAGS  := -Wall -Wextra -O2 -std=c11 -ffreestanding \
           -fno-stack-protector -fno-stack-check -fno-lto -fno-pic -fno-pie \
           -m64 -march=x86-64 -mabi=sysv \
           -mno-red-zone \
           -mcmodel=kernel -Isrc/kernel/include

LDFLAGS := -m elf_x86_64 -nostdlib -static -z max-page-size=0x1000 -T linker.lds

USER_CFLAGS := -Wall -Wextra -O2 -std=c11 -ffreestanding \
               -fno-stack-protector -fno-stack-check -fno-lto -fno-pic -fno-pie \
               -m64 -march=x86-64 -mabi=sysv -mno-red-zone

USER_ELF_LDFLAGS := -m elf_x86_64 -nostdlib -static -T user.lds
INIT_BIN_LDFLAGS := $(USER_ELF_LDFLAGS) --oformat=binary 

NASMFLAGS := -f elf64

SRCS_KERNEL_C    := $(shell find src/kernel -name "*.c")
SRCS_KERNEL_ASM  := $(shell find src/kernel -name "*.s" -o -name "*.S")
SRCS_KERNEL_NASM := $(shell find src/kernel -name "*.asm")

SRCS_USER_C := $(filter-out src/user/init.c, $(shell find src/user -name "*.c" 2>/dev/null))
USER_ELFS   := $(patsubst src/user/%.c,bin/%.elf,$(SRCS_USER_C))

OBJS     := $(patsubst src/kernel/%.c,obj/%.o,$(SRCS_KERNEL_C)) \
            $(patsubst src/kernel/%.s,obj/%.o,$(SRCS_KERNEL_ASM:.S=.o)) \
            $(patsubst src/kernel/%.asm,obj/%.o,$(SRCS_KERNEL_NASM))

all: clean $(OUTPUT) $(USER_OUTPUT) $(USER_ELFS) $(ISO_IMG)

$(OUTPUT): $(OBJS)
	@mkdir -p bin
	@mkdir -p out
	@echo "[LD] Linking kernel binary..."
	$(LD) $(LDFLAGS) $(OBJS) -o $@

$(USER_OUTPUT): src/user/init.c
	@mkdir -p bin
	@mkdir -p obj/user
	@echo "[CC-USER] Compiling user space init..."
	$(CC) $(USER_CFLAGS) -c src/user/init.c -o obj/user/init.o
	@echo "[LD-USER] Linking flat binary init.bin..."
	$(LD) $(INIT_BIN_LDFLAGS) obj/user/init.o -o $@

bin/%.elf: src/user/%.c
	@mkdir -p bin
	@mkdir -p obj/user
	@echo "[CC-USER] Compiling userspace ELF: $<"
	$(CC) $(USER_CFLAGS) -c $< -o obj/user/$*.o
	@echo "[LD-USER] Linking userspace ELF binary at 0x400000..."
	$(LD) $(USER_ELF_LDFLAGS) obj/user/$*.o -o $@

obj/%.o: src/kernel/%.c
	@mkdir -p $(@D)
	@echo "[CC] $<"
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/kernel/%.s
	@mkdir -p $(@D)
	@echo "[AS] $<"
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/kernel/%.S
	@mkdir -p $(@D)
	@echo "[AS] $<"
	$(CC) $(CFLAGS) -c $< -o $@

obj/%.o: src/kernel/%.asm
	@mkdir -p $(@D)
	@echo "[AS-NAS] $<"
	$(NASM) $(NASMFLAGS) $< -o $@

$(ISO_IMG): $(OUTPUT) $(USER_OUTPUT) limine
	@mkdir -p iso_root/boot/limine
	@mkdir -p iso_root/EFI/BOOT
	@cp $(OUTPUT) iso_root/boot/
	@cp $(USER_OUTPUT) iso_root/boot/
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
	@if [ ! -d "limine" ]; then \
		echo "[FETCH] Pulling Limine bootloader binaries..."; \
		curl -L https://github.com/Limine-Bootloader/Limine/releases/latest/download/limine-binary.tar.gz | gunzip | tar -xf -; \
		mv limine-binary/ limine; \
		$(MAKE) -C limine; \
	fi

.PHONY: run-kvm
run-kvm: $(ISO_IMG)
	@echo "[LAUNCH] Initializing CAT microkernel inside QEMU..."
	qemu-system-x86_64 -m 2G -cdrom $(ISO_IMG) -bios $(BIOS) -serial stdio -cpu host,host-phys-bits=on -enable-kvm

.PHONY: run
run: $(ISO_IMG)
	@echo "[LAUNCH] Initializing CAT microkernel inside QEMU..."
	qemu-system-x86_64 -m 2G -cdrom $(ISO_IMG) -bios $(BIOS) -serial stdio -cpu max

.PHONY: debug
debug: $(ISO_IMG)
	@echo "[DEBUG] Initializing CAT microkernel inside bochs..."
	qemu-system-x86_64 -m 2G -cdrom $(ISO_IMG) -bios $(BIOS) -serial stdio -s -S -cpu max


.PHONY: gdb
gdb: $(ISO_IMG)
	@echo "[DEBUG] Starting up GDB..."
	gdb bin/cat_kernel.elf -ex "target remote :1234" -ex "layout asm" -ex "layout regs" 

.PHONY: clean
clean:
	rm -rf bin obj iso_root out