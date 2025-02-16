# Objects
OBJ_DISK := disk/biosdisk.o
OBJ_DISK += disk/efidisk.o
OBJ_DISK += disk/fsuuid.o
OBJ_DISK += disk/gpt.o
OBJ_DISK += disk/msdos.o

OBJECTS += $(OBJ_DISK)

OBJECTS_i386 += $(patsubst %.o,%.i386.o,$(OBJ_DISK))
OBJECTS_x86_64 += $(patsubst %.o,%.x86_64.o,$(OBJ_DISK))
OBJECTS_i386_x86_64 += $(patsubst %.o,%.i386.x86_64.o,$(OBJ_DISK))
OBJECTS_arm64 += $(patsubst %.o,%.arm64.o,$(OBJ_DISK))

RM_FILES += disk/*.s disk/*.o

