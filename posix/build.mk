# Objects
OBJ_POSIX := posix/stdio.o
OBJ_POSIX += posix/string.o
OBJ_POSIX += posix/vsprintf.o

OBJECTS += $(OBJ_POSIX)

OBJECTS_i386 += $(patsubst %.o,%.i386.o,$(OBJ_POSIX))
OBJECTS_x86_64 += $(patsubst %.o,%.x86_64.o,$(OBJ_POSIX))
OBJECTS_i386_x86_64 += $(patsubst %.o,%.i386.x86_64.o,$(OBJ_POSIX))
OBJECTS_arm64 += $(patsubst %.o,%.arm64.o,$(OBJ_POSIX))

RM_FILES += posix/*.s posix/*.o
