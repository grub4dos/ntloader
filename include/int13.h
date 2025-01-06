#ifndef _INT13_H
#define _INT13_H

/*
 * Copyright (C) 2012 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/**
 * @file
 *
 * INT 13 emulation
 *
 */

/** Absolute Pointer
 * The ABS_PTR() macro borrows the idea from Linux kernel of using
 * RELOC_HIDE() macro to stop GCC from checking the result of pointer arithmetic
 * and also it's conversion to be inside the symbol's boundary [1]. The check
 * is sometimes false positive, especially it is controversial to emit the array
 * bounds [-Warray-bounds] warning on all hardwired literal pointers since GCC
 * 11/12 [2]. Unless a good solution can be settled, for the time being we
 * would be in favor of the macro instead of GCC pragmas which cannot match the
 * places the warning needs to be ignored in an exact way.
 *
 * [1] https://lists.linuxcoding.com/kernel/2006-q3/msg17979.html
 * [2] https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99578
 */
#if defined(__GNUC__)
# define ABS_PTR( val ) \
({ \
	intptr_t __ptr; \
	asm ( "" : "=r" ( __ptr ) : "0" ( ( void * ) ( intptr_t ) ( val ) ) ); \
	( void * ) ( __ptr ); \
})
#else
# define ABS_PTR(val) ( ( void * ) ( intptr_t ) ( val ) )
#endif

/** Construct a pointer from a real-mode segment:offset address */
#define REAL_PTR( segment, offset ) \
	( ABS_PTR ( ( (segment) << 4 ) + offset ) )

/** Get drive parameters */
#define INT13_GET_PARAMETERS		0x08
/** Get disk type */
#define INT13_GET_DISK_TYPE		0x15
/** Extensions installation check */
#define INT13_EXTENSION_CHECK		0x41
/** Extended read */
#define INT13_EXTENDED_READ		0x42
/** Get extended drive parameters */
#define INT13_GET_EXTENDED_PARAMETERS	0x48

/** Operation completed successfully */
#define INT13_STATUS_SUCCESS		0x00
/** Invalid function or parameter */
#define INT13_STATUS_INVALID		0x01
/** Read error */
#define INT13_STATUS_READ_ERROR		0x04
/** Reset failed */
#define INT13_STATUS_RESET_FAILED	0x05
/** Write error */
#define INT13_STATUS_WRITE_ERROR	0xcc

/** No such drive */
#define INT13_DISK_TYPE_NONE		0x00
/** Floppy without change-line support */
#define INT13_DISK_TYPE_FDD		0x01
/** Floppy with change-line support */
#define INT13_DISK_TYPE_FDD_CL		0x02
/** Hard disk */
#define INT13_DISK_TYPE_HDD		0x03

/** Extended disk access functions supported */
#define INT13_EXTENSION_LINEAR		0x01

/** INT13 extensions version 1.x */
#define INT13_EXTENSION_VER_1_X		0x01

/** DMA boundary errors handled transparently */
#define INT13_FL_DMA_TRANSPARENT 	0x01

/** BIOS drive counter */
#define INT13_DRIVE_COUNT ( *( ( ( uint8_t * ) REAL_PTR ( 0x40, 0x75 ) ) ) )

/** An INT 13 disk address packet */
struct int13_disk_address {
	/** Size of the packet, in bytes */
	uint8_t bufsize;
	/** Reserved */
	uint8_t reserved_a;
	/** Block count */
	uint8_t count;
	/** Reserved */
	uint8_t reserved_b;
	/** Data buffer */
	struct segoff buffer;
	/** Starting block number */
	uint64_t lba;
	/** Data buffer (EDD 3.0+ only) */
	uint64_t buffer_phys;
	/** Block count (EDD 4.0+ only) */
	uint32_t long_count;
	/** Reserved */
	uint32_t reserved_c;
} __attribute__ (( packed ));

/** INT 13 disk parameters */
struct int13_disk_parameters {
	/** Size of this structure */
	uint16_t bufsize;
	/** Flags */
	uint16_t flags;
	/** Number of cylinders */
	uint32_t cylinders;
	/** Number of heads */
	uint32_t heads;
	/** Number of sectors per track */
	uint32_t sectors_per_track;
	/** Total number of sectors on drive */
	uint64_t sectors;
	/** Bytes per sector */
	uint16_t sector_size;
} __attribute__ (( packed ));

extern int initialise_int13 ( void );
extern void emulate_int13 ( struct bootapp_callback_params *params );

#endif /* _INT13_H */
