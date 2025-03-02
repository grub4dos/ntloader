/* mm.h - prototypes and declarations for memory manager */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2007  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _MM_H
#define _MM_H 1

#include <stddef.h>
#include <stdint.h>

/* Magic words.  */
#define MM_FREE_MAGIC	0x2d3c2808
#define MM_ALLOC_MAGIC	0x6db08fa4

/* A header describing a block of memory - either allocated or free */
typedef struct mm_header
{
    /*
     * The 'next' free block in this region's circular free list.
     * Only meaningful if the block is free.
     */
    struct mm_header *next;
    /* The block size, not in bytes but the number of cells of
     * GRUB_MM_ALIGN bytes. Includes the header cell.
     */
    size_t size;
    /* either free or alloc magic, depending on the block type. */
    size_t magic;
    /* pad to cell size: see the top of kern/mm.c. */
#if SIZEOF_VOID_P == 4
    char padding[4];
#elif SIZEOF_VOID_P == 8
    char padding[8];
#else
# error "unknown word size"
#endif
}
*mm_header_t;

#if SIZEOF_VOID_P == 4
# define MM_ALIGN_LOG2	4
#elif SIZEOF_VOID_P == 8
# define MM_ALIGN_LOG2	5
#endif

#define MM_ALIGN	(1 << MM_ALIGN_LOG2)

/* A region from which we can make allocations. */
typedef struct mm_region
{
    /* The first free block in this region. */
    struct mm_header *first;

    /*
     * The next region in the linked list of regions. Regions are initially
     * sorted in order of increasing size, but can grow, in which case the
     * ordering may not be preserved.
     */
    struct mm_region *next;

    /*
     * A mm_region will always be aligned to cell size. The pre-size is
     * the number of bytes we were given but had to skip in order to get that
     * alignment.
     */
    size_t pre_size;

    /*
     * Likewise, the post-size is the number of bytes we wasted at the end
     * of the allocation because it wasn't a multiple of MM_ALIGN
     */
    size_t post_size;

    /* How many bytes are in this region? (free and allocated) */
    size_t size;

    /* pad to a multiple of cell size */
    char padding[3 * SIZEOF_VOID_P];
}
*mm_region_t;

static inline void
mm_size_sanity_check (void)
{
    /* Ensure we preserve alignment when doing h = (mm_header_t) (r + 1). */
    COMPILE_TIME_ASSERT ((sizeof (struct mm_region) %
                          sizeof (struct mm_header)) == 0);

    /*
     * MM_ALIGN is supposed to represent cell size, and a mm_header is
     * supposed to be 1 cell.
     */
    COMPILE_TIME_ASSERT (sizeof (struct mm_header) == MM_ALIGN);
}

#define MM_ADD_REGION_NONE        0
#define MM_ADD_REGION_CONSECUTIVE (1 << 0)

/*
 * Function used to request memory regions of `size_t` bytes. The second
 * parameter is a bitfield of `MM_ADD_REGION` flags.
 */
typedef int (*mm_add_region_func_t) (size_t, unsigned int);

extern void bios_mm_init_region (void *addr, size_t size);

extern void *bios_memalign (size_t align, size_t size);

#endif /* ! _MM_H */
