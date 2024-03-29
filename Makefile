VERSION := v2.0.1
WIMBOOT_VERSION := v2.7.5

OBJECTS := prefix.o startup.o callback.o vsprintf.o string.o charset.o
OBJECTS += int13.o vdisk.o cpio.o stdio.o misc.o memmap.o
OBJECTS += efiblock.o cmdline.o peloader.o main.o efimain.o efidisk.o
OBJECTS += msdos.o gpt.o fsuuid.o bcd.o biosdisk.o reg.o

OBJECTS_i386 := $(patsubst %.o,%.i386.o,$(OBJECTS))
OBJECTS_x86_64 := $(patsubst %.o,%.x86_64.o,$(OBJECTS))
OBJECTS_i386_x86_64 := $(patsubst %.o,%.i386.x86_64.o,$(OBJECTS))

HOST_CC		:= $(CC)
AS		:= $(AS)
ECHO		:= echo
OBJCOPY 	:= objcopy
AR		:= ar
RANLIB		:= ranlib
CP		:= cp
RM		:= rm
BINUTILS_DIR	:= /usr
BFD_DIR		:= $(BINUTILS_DIR)
ZLIB_DIR	:= /usr

HOST_CFLAGS += -Wall -W -Werror

CFLAGS += -Os -ffreestanding -Wall -W -Werror -nostdinc -Iinclude/ -fshort-wchar
CFLAGS += -DVERSION="\"$(VERSION)\""
CFLAGS += -DWIMBOOT_VERSION="\"$(WIMBOOT_VERSION)\""

CFLAGS_i386 += -m32 -march=i386 -malign-double -fno-pic
CFLAGS_x86_64 += -m64 -mno-red-zone -fpie

# Enable stack protection if available
#
SPG_TEST = $(CC) -fstack-protector-strong -mstack-protector-guard=global \
		 -x c -c /dev/null -o /dev/null >/dev/null 2>&1
SPG_FLAGS := $(shell $(SPG_TEST) && $(ECHO) '-fstack-protector-strong ' \
					    '-mstack-protector-guard=global')
CFLAGS += $(SPG_FLAGS)

# Inhibit unwanted debugging information
CFI_TEST = $(CC) -fno-dwarf2-cfi-asm -fno-exceptions -fno-unwind-tables \
		 -fno-asynchronous-unwind-tables -x c -c /dev/null \
		 -o /dev/null >/dev/null 2>&1
CFI_FLAGS := $(shell $(CFI_TEST) && \
	       $(ECHO) '-fno-dwarf2-cfi-asm -fno-exceptions ' \
		    '-fno-unwind-tables -fno-asynchronous-unwind-tables')
WORKAROUND_CFLAGS += $(CFI_FLAGS)

# Add -maccumulate-outgoing-args if required by this version of gcc
MS_ABI_TEST_CODE := extern void __attribute__ (( ms_abi )) ms_abi(); \
		    void sysv_abi ( void ) { ms_abi(); }
MS_ABI_TEST = $(ECHO) '$(MS_ABI_TEST_CODE)' | \
	      $(CC) -m64 -mno-accumulate-outgoing-args -x c -c - -o /dev/null \
		    >/dev/null 2>&1
MS_ABI_FLAGS := $(shell $(MS_ABI_TEST) || $(ECHO) '-maccumulate-outgoing-args')
WORKAROUND_CFLAGS += $(MS_ABI_FLAGS)

# Inhibit LTO
LTO_TEST = $(CC) -fno-lto -x c -c /dev/null -o /dev/null >/dev/null 2>&1
LTO_FLAGS := $(shell $(LTO_TEST) && $(ECHO) '-fno-lto')
WORKAROUND_CFLAGS += $(LTO_FLAGS)

CFLAGS += $(WORKAROUND_CFLAGS)
CFLAGS += $(EXTRA_CFLAGS)

ifneq ($(DEBUG),)
CFLAGS += -DDEBUG=$(DEBUG)
endif

CFLAGS += -include include/compiler.h

###############################################################################
#
# Final targets

all : ntloader ntloader.i386

ntloader : ntloader.x86_64.efi
	$(CP) $< $@

ntloader.i386 : ntloader.i386.efi
	$(CP) $< $@

ntloader.%.elf : prefix.%.o lib.%.a
	$(LD) -m elf_$* -T script.lds -o $@ -q -Map ntloader.$*.map \
		prefix.$*.o lib.$*.a

ntloader.%.efi : ntloader.%.elf efireloc
	$(OBJCOPY) -Obinary $< $@
	./efireloc $< $@

###############################################################################
#
# i386 objects

%.i386.s : %.S
	$(CC) $(CFLAGS) $(CFLAGS_i386) -DASSEMBLY -Ui386 -E $< -o $@

%.i386.s : %.c
	$(CC) $(CFLAGS) $(CFLAGS_i386) -S $< -o $@

%.i386.o : %.i386.s
	$(AS) --32 i386.i $< -o $@

lib.i386.a : $(OBJECTS_i386)
	$(RM) -f $@
	$(AR) r $@ $(OBJECTS_i386)
	$(RANLIB) $@

###############################################################################
#
# i386 objects to be linked into an x86_64 binary

%.i386.x86_64.raw.o : %.i386.s
	$(AS) --64 i386.i $< -o $@

%.i386.x86_64.o : %.i386.x86_64.raw.o
	$(OBJCOPY) --prefix-symbols=__i386_ $< $@

###############################################################################
#
# x86_64 objects

%.x86_64.s : %.S
	$(CC) $(CFLAGS) $(CFLAGS_x86_64) -DASSEMBLY -Ui386 -E $< -o $@

%.x86_64.s : %.c
	$(CC) $(CFLAGS) $(CFLAGS_x86_64) -S $< -o $@

%.x86_64.o : %.x86_64.s
	$(AS) --64 x86_64.i $< -o $@

lib.x86_64.a : $(OBJECTS_x86_64) $(OBJECTS_i386_x86_64)
	$(RM) -f $@
	$(AR) r $@ $(OBJECTS_x86_64) $(OBJECTS_i386_x86_64)
	$(RANLIB) $@

###############################################################################
#
# EFI relocator

EFIRELOC_CFLAGS := -I$(BINUTILS_DIR)/include -I$(BFD_DIR)/include \
		   -I$(ZLIB_DIR)/include -idirafter include/
EFIRELOC_LDFLAGS := -L$(BINUTILS_DIR)/lib -L$(BFD_DIR)/lib -L$(ZLIB_DIR)/lib \
		    -lbfd -ldl -liberty -lz -Wl,--no-warn-search-mismatch

efireloc : utils/efireloc.c
	$(CC) $(HOST_CFLAGS) $(EFIRELOC_CFLAGS) $< $(EFIRELOC_LDFLAGS) -o $@

###############################################################################
#
# fsuuid

FSUUID_CFLAGS := -Wall -Werror -idirafter include/

fsuuid.exe : utils/fsuuid.c
	i686-w64-mingw32-gcc $(FSUUID_CFLAGS) $< -o $@

###############################################################################
#
# Cleanup

clean :
	$(RM) -f *.s *.o *.a *.elf *.map
	$(RM) -f efireloc
	$(RM) -f fsuuid.exe
	$(RM) -f ntloader ntloader.i386
	$(RM) -f ntloader.i386.efi ntloader.x86_64.efi
