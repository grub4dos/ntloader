# Objects
OBJ_LIBNT := libnt/bcd.o
OBJ_LIBNT += libnt/charset.o
OBJ_LIBNT += libnt/cpio.o
OBJ_LIBNT += libnt/peloader.o
OBJ_LIBNT += libnt/reg.o
OBJ_LIBNT += libnt/vdisk.o

OBJECTS += $(OBJ_LIBNT)

OBJECTS_i386 += $(patsubst %.o,%.i386.o,$(OBJ_LIBNT))
OBJECTS_x86_64 += $(patsubst %.o,%.x86_64.o,$(OBJ_LIBNT))
OBJECTS_i386_x86_64 += $(patsubst %.o,%.i386.x86_64.o,$(OBJ_LIBNT))
OBJECTS_arm64 += $(patsubst %.o,%.arm64.o,$(OBJ_LIBNT))

RM_FILES += libnt/*.s libnt/*.o

