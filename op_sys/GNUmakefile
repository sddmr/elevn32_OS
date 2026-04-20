# SdmrOS — Ana GNUmakefile
# Bu Makefile kernel'ı derler, Limine bootloader'ı indirir ve ISO oluşturur.

.PHONY: all clean run

GMAKE := gmake

# Limine branch
LIMINE_BRANCH := v8.x-binary

# Kernel derleme
kernel/kernel.elf:
	$(GMAKE) -C kernel all

# Limine'ı indir (eğer yoksa)
limine/limine:
	git clone https://github.com/limine-bootloader/limine.git --branch=$(LIMINE_BRANCH) --depth=1
	$(GMAKE) -C limine

# Limine header'ı indir (eğer yoksa)
kernel/src/limine.h:
	curl -Lo $@ https://raw.githubusercontent.com/limine-bootloader/limine/trunk/limine.h

# ISO oluştur
all: kernel/src/limine.h kernel/kernel.elf limine/limine
	# ISO dizin yapısını oluştur
	rm -rf iso_root
	mkdir -p iso_root/boot
	mkdir -p iso_root/boot/limine
	mkdir -p iso_root/EFI/BOOT

	# Dosyaları kopyala
	cp kernel/kernel.elf iso_root/boot/kernel
	cp limine.conf iso_root/boot/limine/limine.conf

	# Limine dosyalarını kopyala
	cp limine/limine-bios.sys iso_root/boot/limine/
	cp limine/limine-bios-cd.bin iso_root/boot/limine/
	cp limine/limine-uefi-cd.bin iso_root/boot/limine/
	cp limine/BOOTX64.EFI iso_root/EFI/BOOT/
	cp limine/BOOTIA32.EFI iso_root/EFI/BOOT/

	# ISO oluştur
	xorriso -as mkisofs -R -r -J -b boot/limine/limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o sdmros.iso

	# Limine BIOS boot sektörlerini kur
	./limine/limine bios-install sdmros.iso

	@echo ""
	@echo "═══════════════════════════════════════"
	@echo "  SdmrOS ISO olusturuldu: sdmros.iso"
	@echo "  Calistirmak icin: gmake run"
	@echo "═══════════════════════════════════════"

# QEMU ile çalıştır
run: all
	qemu-system-x86_64 \
		-M q35 \
		-m 128M \
		-cdrom sdmros.iso \
		-serial stdio \
		-no-reboot \
		-no-shutdown

# Temizle
clean:
	$(GMAKE) -C kernel clean
	rm -rf iso_root sdmros.iso
