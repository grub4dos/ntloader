# Build tools for host binaries
#
HOST_CC		?= gcc
MINGW_CC	?= i686-w64-mingw32-gcc

# Build flags for host binaries
#
HOST_CFLAGS	+= -Wall -W -Werror -fshort-wchar -DNTLOADER_UTIL

# EFI relocator
#
elf2efi32 : utils/elf2efi.c
	$(HOST_CC) $(HOST_CFLAGS) -idirafter include/ -DEFI_TARGET32 $< -o $@

elf2efi64 : utils/elf2efi.c
	$(HOST_CC) $(HOST_CFLAGS) -idirafter include/ -DEFI_TARGET64 $< -o $@

RM_FILES += elf2efi32 elf2efi64

# Initrd rootfs
#
mkinitrd.exe : utils/mkinitrd.c
	$(MINGW_CC) $(HOST_CFLAGS) -iquote include/ $< -o $@

mkinitrd : utils/mkinitrd.c
	$(HOST_CC) $(HOST_CFLAGS) -iquote include/ $< -o $@

initrd.cpio : mkinitrd
	./mkinitrd utils/rootfs $@

RM_FILES += mkinitrd mkinitrd.exe initrd.cpio

# fsuuid
#
fsuuid.exe : utils/fsuuid.c
	$(MINGW_CC) $(HOST_CFLAGS) -iquote include/ $< -o $@

fsuuid : utils/fsuuid.c
	$(HOST_CC) $(HOST_CFLAGS) -iquote include/ $< -o $@

RM_FILES += fsuuid fsuuid.exe

# regview
#
REG_FILES := libnt/reg.c libnt/charset.c utils/regview.c

regview.exe : $(REG_FILES)
	$(MINGW_CC) $(HOST_CFLAGS) -iquote include/ $(REG_FILES) -o $@

regview : $(REG_FILES)
	$(HOST_CC) $(HOST_CFLAGS) -iquote include/ $(REG_FILES) -o $@

RM_FILES += regview regview.exe

# bmtool
#
BMTOOL_FILES := libnt/huffman.c libnt/lznt1.c libnt/xca.c
BMTOOL_FILES += utils/bmtool.c

bmtool.exe : $(BMTOOL_FILES)
	$(MINGW_CC) $(HOST_CFLAGS) -iquote include/ $(BMTOOL_FILES) -o $@

bmtool : $(BMTOOL_FILES)
	$(HOST_CC) $(HOST_CFLAGS) -iquote include/ $(BMTOOL_FILES) -o $@

RM_FILES += bmtool bmtool.exe
