# Objects
OBJ_KERN := kern/callback.o
OBJ_KERN += kern/cmdline.o
OBJ_KERN += kern/cookie.o
OBJ_KERN += kern/die.o
OBJ_KERN += kern/e820.o
OBJ_KERN += kern/efi.o
OBJ_KERN += kern/efiblock.o
OBJ_KERN += kern/efiboot.o
OBJ_KERN += kern/efifile.o
OBJ_KERN += kern/efimain.o
OBJ_KERN += kern/int13.o
OBJ_KERN += kern/main.o
OBJ_KERN += kern/paging.o
OBJ_KERN += kern/payload.o
OBJ_KERN += kern/startup.o

OBJECTS += $(OBJ_KERN)

OBJECTS_i386 += $(patsubst %.o,%.i386.o,$(OBJ_KERN))
OBJECTS_x86_64 += $(patsubst %.o,%.x86_64.o,$(OBJ_KERN))
OBJECTS_i386_x86_64 += $(patsubst %.o,%.i386.x86_64.o,$(OBJ_KERN))
OBJECTS_arm64 += $(patsubst %.o,%.arm64.o,$(OBJ_KERN))

RM_FILES += kern/*.s kern/*.o

