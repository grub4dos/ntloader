# Objects
OBJECTS += kern/cmdline.o
OBJECTS += kern/cookie.o
OBJECTS += kern/die.o
OBJECTS += kern/efi.o
OBJECTS += kern/efiblock.o
OBJECTS += kern/efiboot.o
OBJECTS += kern/efifile.o
OBJECTS += kern/efimain.o
OBJECTS += kern/efiterm.o
OBJECTS += kern/payload.o

OBJECTS += kern/callback.o
OBJECTS += kern/e820.o
OBJECTS += kern/int13.o
OBJECTS += kern/main.o
OBJECTS += kern/paging.o
OBJECTS += kern/startup.o

RM_FILES += kern/*.s kern/*.o

