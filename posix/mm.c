/* mm.c - functions for memory manager */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2005,2007,2008,2009  Free Software Foundation, Inc.
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

/*
 *  The design of this memory manager.
 *
 *  This is a simple implementation of malloc with a few extensions. These are
 *  the extensions:
 *
 *  - memalign is implemented efficiently.
 *
 *  - multiple regions may be used as free space. They may not be
 *  contiguous.
 *
 *  - if existing regions are insufficient to satisfy an allocation, a new
 *  region can be requested from firmware.
 *
 *  Regions are managed by a singly linked list, and the meta information is
 *  stored in the beginning of each region. Space after the meta information
 *  is used to allocate memory.
 *
 *  The memory space is used as cells instead of bytes for simplicity. This
 *  is important for some CPUs which may not access multiple bytes at a time
 *  when the first byte is not aligned at a certain boundary (typically,
 *  4-byte or 8-byte). The size of each cell is equal to the size of struct
 *  mm_header, so the header of each allocated/free block fits into one
 *  cell precisely. One cell is 16 bytes on 32-bit platforms and 32 bytes
 *  on 64-bit platforms.
 *
 *  There are two types of blocks: allocated blocks and free blocks.
 *
 *  In allocated blocks, the header of each block has only its size. Note that
 *  this size is based on cells but not on bytes. The header is located right
 *  before the returned pointer, that is, the header resides at the previous
 *  cell.
 *
 *  Free blocks constitutes a ring, using a singly linked list. The first free
 *  block is pointed to by the meta information of a region. The allocator
 *  attempts to pick up the second block instead of the first one. This is
 *  a typical optimization against defragmentation, and makes the
 *  implementation a bit easier.
 *
 *  For safety, both allocated blocks and free ones are marked by magic
 *  numbers. Whenever anything unexpected is detected, GRUB aborts the
 *  operation.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ntloader.h"
#include "mm.h"

/*
 * MM_MGMT_OVERHEAD is an upper bound of management overhead of
 * each region, with any possible padding taken into account.
 *
 * The value must be large enough to make sure memalign(align, size)
 * always succeeds after a successful call to
 * mm_init_region(addr, size + align + MM_MGMT_OVERHEAD),
 * for any given addr, align and size (assuming no interger overflow).
 *
 * The worst case which has maximum overhead is shown in the figure below:
 *
 * +-- addr
 * v                                           |<- size + align ->|
 * +---------+----------------+----------------+------------------+---------+
 * | padding | mm_region | mm_header |   usable bytes   | padding |
 * +---------+----------------+----------------+------------------+---------+
 * |<-  a  ->|<-   b   ->|<-   c   ->|<-      d       ->|<-  e  ->|
 *                                             ^
 *    b == sizeof (struct mm_region)           | / Assuming no other suitable
 *    c == sizeof (struct mm_header)           | | block is available, then:
 *    d == size + align                        +-| If align == 0, this will be
 *                                               | the pointer returned by next
 * Assuming addr % MM_ALIGN == 1, then:          | memalign(align, size).
 *    a == MM_ALIGN - 1                          | If align > 0, this chunk may
 *                                               | need to be split to fulfill
 * Assuming d % MM_ALIGN == 1, then:             | alignment requirements, and
 *    e == MM_ALIGN - 1                          | the returned pointer may be
 *                                               \ inside these usable bytes.
 * Therefore, the maximum overhead is:
 *    a + b + c + e == (MM_ALIGN - 1) + sizeof (struct mm_region)
 *                     + sizeof (struct mm_header) + (MM_ALIGN - 1)
 */
#define MM_MGMT_OVERHEAD \
    ((MM_ALIGN - 1) \
     + sizeof (struct mm_region) \
     + sizeof (struct mm_header) \
     + (MM_ALIGN - 1))

/* The size passed to mm_add_region_fn() is aligned up by this value. */
#define MM_HEAP_GROW_ALIGN	4096

/* Minimal heap growth granularity when existing heap space is exhausted. */
#define MM_HEAP_GROW_EXTRA	0x100000

mm_region_t mm_base;
mm_add_region_func_t mm_add_region_fn;

/* Get a header from the pointer PTR, and set *P and *R to a pointer
 *   to the header and a pointer to its region, respectively. PTR must
 *   be allocated.  */
static void
get_header_from_pointer (void *ptr, mm_header_t *p, mm_region_t *r)
{
    if ((intptr_t) ptr & (MM_ALIGN - 1))
        die ("unaligned pointer %p\n", ptr);

    for (*r = mm_base; *r; *r = (*r)->next)
        if ((intptr_t) ptr > (intptr_t) ((*r) + 1)
            && (intptr_t) ptr <= (intptr_t) ((*r) + 1) + (*r)->size)
            break;

    if (! *r)
        die ("out of range pointer %p\n", ptr);

    *p = (mm_header_t) ptr - 1;
    if ((*p)->magic == MM_FREE_MAGIC)
        die ("double free at %p\n", *p);
    if ((*p)->magic != MM_ALLOC_MAGIC)
        die ("alloc magic is broken at %p: %zx\n", *p, (*p)->magic);
}

/* Initialize a region starting from ADDR and whose size is SIZE,
 *   to use it as free space.  */
void
mm_init_region (void *addr, size_t size)
{
    mm_header_t h;
    mm_region_t r, *p, q;

    DBG ("Using memory for heap: start=%p, end=%p\n",
         addr, (char *) addr + (unsigned int) size);

    /* Exclude last 4K to avoid overflows. */
    /* If addr + 0x1000 overflows then whole region is in excluded zone.  */
    if ((intptr_t) addr > ~((intptr_t) 0x1000))
        return;

    /* If addr + 0x1000 + size overflows then decrease size.  */
    if (((intptr_t) addr + 0x1000) > ~(intptr_t) size)
        size = ((intptr_t) -0x1000) - (intptr_t) addr;

    /* Attempt to merge this region with every existing region */
    for (p = &mm_base, q = *p; q; p = &(q->next), q = *p)
    {
        /*
         * Is the new region immediately below an existing region? That
         * is, is the address of the memory we're adding now (addr) + size
         * of the memory we're adding (size) + the bytes we couldn't use
         * at the start of the region we're considering (q->pre_size)
         * equal to the address of q? In other words, does the memory
         * looks like this?
         *
         * addr                          q
         *   |----size-----|-q->pre_size-|<q region>|
         */
        if ((uint8_t *) addr + size + q->pre_size == (uint8_t *) q)
        {
            /*
             * Yes, we can merge the memory starting at addr into the
             * existing region from below. Align up addr to MM_ALIGN
             * so that our new region has proper alignment.
             */
            r = (mm_region_t) ALIGN_UP ((intptr_t) addr, MM_ALIGN);
            /* Copy the region data across */
            *r = *q;
            /* Consider all the new size as pre-size */
            r->pre_size += size;

            /*
             * If we have enough pre-size to create a block, create a
             * block with it. Mark it as allocated and pass it to
             * free (), which will sort out getting it into the free
             * list.
             */
            if (r->pre_size >> MM_ALIGN_LOG2)
            {
                h = (mm_header_t) (r + 1);
                /* block size is pre-size converted to cells */
                h->size = (r->pre_size >> MM_ALIGN_LOG2);
                h->magic = MM_ALLOC_MAGIC;
                /* region size grows by block size converted back to bytes */
                r->size += h->size << MM_ALIGN_LOG2;
                /* adjust pre_size to be accurate */
                r->pre_size &= (MM_ALIGN - 1);
                *p = r;
                free (h + 1);
            }
            /* Replace the old region with the new region */
            *p = r;
            return;
        }

        /*
         * Is the new region immediately above an existing region? That
         * is:
         *   q                       addr
         *   |<q region>|-q->post_size-|----size-----|
         */
        if ((uint8_t *) q + sizeof (*q) + q->size + q->post_size ==
            (uint8_t *) addr)
        {
            /*
             * Yes! Follow a similar pattern to above, but simpler.
             * Our header starts at address - post_size, which should align us
             * to a cell boundary.
             *
             * Cast to (void *) first to avoid the following build error:
             *   kern/mm.c: In function ‘mm_init_region’:
             *   kern/mm.c:211:15: error: cast increases required alignment of target type [-Werror=cast-align]
             *     211 |           h = (mm_header_t) ((uint8_t *) addr - q->post_size);
             *         |               ^
             * It is safe to do that because proper alignment is enforced in mm_size_sanity_check().
             */
            h = (mm_header_t)(void *) ((uint8_t *) addr - q->post_size);
            /* our size is the allocated size plus post_size, in cells */
            h->size = (size + q->post_size) >> MM_ALIGN_LOG2;
            h->magic = MM_ALLOC_MAGIC;
            /* region size grows by block size converted back to bytes */
            q->size += h->size << MM_ALIGN_LOG2;
            /* adjust new post_size to be accurate */
            q->post_size = (q->post_size + size) & (MM_ALIGN - 1);
            free (h + 1);
            return;
        }
    }

    /*
     * If you want to modify the code below, please also take a look at
     * MM_MGMT_OVERHEAD and make sure it is synchronized with the code.
     */

    /* Allocate a region from the head.  */
    r = (mm_region_t) ALIGN_UP ((intptr_t) addr, MM_ALIGN);

    /* If this region is too small, ignore it.  */
    if (size < MM_ALIGN + (char *) r - (char *) addr + sizeof (*r))
        return;

    size -= (char *) r - (char *) addr + sizeof (*r);

    h = (mm_header_t) (r + 1);
    h->next = h;
    h->magic = MM_FREE_MAGIC;
    h->size = (size >> MM_ALIGN_LOG2);

    r->first = h;
    r->pre_size = (intptr_t) r - (intptr_t) addr;
    r->size = (h->size << MM_ALIGN_LOG2);
    r->post_size = size - r->size;

    /* Find where to insert this region. Put a smaller one before bigger ones,
     *     to prevent fragmentation.  */
    for (p = &mm_base, q = *p; q; p = &(q->next), q = *p)
        if (q->size > r->size)
            break;

    *p = r;
    r->next = q;
}

/* Allocate the number of units N with the alignment ALIGN from the ring
 * buffer given in *FIRST.  ALIGN must be a power of two. Both N and
 * ALIGN are in units of MM_ALIGN.  Return a non-NULL if successful,
 * otherwise return NULL.
 *
 * Note: because in certain circumstances we need to adjust the ->next
 * pointer of the previous block, we iterate over the singly linked
 * list with the pair (prev, cur). *FIRST is our initial previous, and
 * *FIRST->next is our initial current pointer. So we will actually
 * allocate from *FIRST->next first and *FIRST itself last.
 */
static void *
real_malloc (mm_header_t *first, size_t n, size_t align)
{
    mm_header_t cur, prev;

    /* When everything is allocated side effect is that *first will have alloc
     *     magic marked, meaning that there is no room in this region.  */
    if ((*first)->magic == MM_ALLOC_MAGIC)
        return 0;

    /* Try to search free slot for allocation in this memory region.  */
    for (prev = *first, cur = prev->next; ; prev = cur, cur = cur->next)
    {
        uint64_t extra;

        extra = ((intptr_t) (cur + 1) >> MM_ALIGN_LOG2) & (align - 1);
        if (extra)
            extra = align - extra;

        if (! cur)
            die ("null in the ring\n");

        if (cur->magic != MM_FREE_MAGIC)
            die ("free magic is broken at %p: 0x%zx\n", cur, cur->magic);

        if (cur->size >= n + extra)
        {
            extra += (cur->size - extra - n) & (~(align - 1));
            if (extra == 0 && cur->size == n)
            {
                /* There is no special alignment requirement and memory block
                 *	         is complete match.
                 *
                 *	         1. Just mark memory block as allocated and remove it from
                 *	            free list.
                 *
                 *	         Result:
                 *	         +---------------+ previous block's next
                 *	         | alloc, size=n |          |
                 *	         +---------------+          v
                 */
                prev->next = cur->next;
            }
            else if (align == 1 || cur->size == n + extra)
            {
                /* There might be alignment requirement, when taking it into
                 *	         account memory block fits in.
                 *
                 *	         1. Allocate new area at end of memory block.
                 *	         2. Reduce size of available blocks from original node.
                 *	         3. Mark new area as allocated and "remove" it from free
                 *	            list.
                 *
                 *	         Result:
                 *	         +---------------+
                 *	         | free, size-=n | next --+
                 *	         +---------------+        |
                 *	         | alloc, size=n |        |
                 *	         +---------------+        v
                 */
                cur->size -= n;
                cur += cur->size;
            }
            else if (extra == 0)
            {
                mm_header_t r;

                r = cur + extra + n;
                r->magic = MM_FREE_MAGIC;
                r->size = cur->size - extra - n;
                r->next = cur->next;
                prev->next = r;

                if (prev == cur)
                {
                    prev = r;
                    r->next = r;
                }
            }
            else
            {
                /* There is alignment requirement and there is room in memory
                 *	         block.  Split memory block to three pieces.
                 *
                 *	         1. Create new memory block right after section being
                 *	            allocated.  Mark it as free.
                 *	         2. Add new memory block to free chain.
                 *	         3. Mark current memory block having only extra blocks.
                 *	         4. Advance to aligned block and mark that as allocated and
                 *	            "remove" it from free list.
                 *
                 *	         Result:
                 *	         +------------------------------+
                 *	         | free, size=extra             | next --+
                 *	         +------------------------------+        |
                 *	         | alloc, size=n                |        |
                 *	         +------------------------------+        |
                 *	         | free, size=orig.size-extra-n | <------+, next --+
                 *	         +------------------------------+                  v
                 */
                mm_header_t r;

                r = cur + extra + n;
                r->magic = MM_FREE_MAGIC;
                r->size = cur->size - extra - n;
                r->next = cur;

                cur->size = extra;
                prev->next = r;
                cur += extra;
            }

            cur->magic = MM_ALLOC_MAGIC;
            cur->size = n;

            /* Mark find as a start marker for next allocation to fasten it.
             *	     This will have side effect of fragmenting memory as small
             *	     pieces before this will be un-used.  */
            /* So do it only for chunks under 32K.  */
            if (n < (0x8000 >> MM_ALIGN_LOG2)
                || *first == cur)
                *first = prev;

            return cur + 1;
        }

        /* Search was completed without result.  */
        if (cur == *first)
            break;
    }

    return 0;
}

/* Allocate SIZE bytes with the alignment ALIGN and return the pointer.  */
void *
memalign (size_t align, size_t size)
{
    mm_region_t r;
    size_t n = ((size + MM_ALIGN - 1) >> MM_ALIGN_LOG2) + 1;
    size_t grow;
    int count = 0;

    if (!mm_base)
        goto fail;

    if (size > ~(size_t) align)
        goto fail;

    grow = size + align;

    /* We currently assume at least a 32-bit size_t,
     *     so limiting allocations to <adress space size> - 1MiB
     *     in name of sanity is beneficial. */
    if (grow > ~(size_t) 0x100000)
        goto fail;

    align = (align >> MM_ALIGN_LOG2);
    if (align == 0)
        align = 1;

again:

    for (r = mm_base; r; r = r->next)
    {
        void *p;

        p = real_malloc (&(r->first), n, align);
        if (p)
            return p;
    }

    /* If failed, increase free memory somehow.  */
    switch (count)
    {
        case 0:
            /* Request additional pages, contiguous */
            count++;

            /*
             * Calculate the necessary size of heap growth (if applicable),
             * with region management overhead taken into account.
             */
            if (safe_add (grow, MM_MGMT_OVERHEAD, &grow))
                goto fail;

            /* Preallocate some extra space if heap growth is small. */
            grow = max (grow, MM_HEAP_GROW_EXTRA);

            /* Align up heap growth to make it friendly to CPU/MMU. */
            if (grow > ~(size_t) (MM_HEAP_GROW_ALIGN - 1))
                goto fail;
            grow = ALIGN_UP (grow, MM_HEAP_GROW_ALIGN);

            /* Do the same sanity check again. */
            if (grow > ~(size_t) 0x100000)
                goto fail;

            if (mm_add_region_fn != NULL &&
                mm_add_region_fn (grow, MM_ADD_REGION_CONSECUTIVE) == 0)
                goto again;

        /* fallthrough  */

        case 1:
            /* Request additional pages, anything at all */
            count++;

            if (mm_add_region_fn != NULL)
            {
                /*
                 * Try again even if this fails, in case it was able to partially
                 * satisfy the request
                 */
                mm_add_region_fn (grow, MM_ADD_REGION_NONE);
                goto again;
            }

        /* fallthrough */

        case 2:
            ///* Invalidate disk caches.  */
            //disk_cache_invalidate_all ();
            count++;
            goto again;

        default:
            break;
    }

fail:
    die ("out of memory\n");
    return 0;
}

/*
 * Allocate NMEMB instances of SIZE bytes and return the pointer, or error on
 * integer overflow.
 */
void *
calloc (size_t nmemb, size_t size)
{
    void *ret;
    size_t sz = 0;

    if (safe_mul (nmemb, size, &sz))
        die ("overflow is detected\n");

    ret = memalign (0, sz);
    if (!ret)
        return NULL;

    memset (ret, 0, sz);
    return ret;
}

/* Allocate SIZE bytes and return the pointer.  */
void *
malloc (size_t size)
{
    return memalign (0, size);
}

/* Allocate SIZE bytes, clear them and return the pointer.  */
void *
zalloc (size_t size)
{
    void *ret;

    ret = memalign (0, size);
    if (ret)
        memset (ret, 0, size);

    return ret;
}

/* Deallocate the pointer PTR.  */
void
free (void *ptr)
{
    mm_header_t p;
    mm_region_t r;

    if (! ptr)
        return;

    get_header_from_pointer (ptr, &p, &r);

    if (r->first->magic == MM_ALLOC_MAGIC)
    {
        p->magic = MM_FREE_MAGIC;
        r->first = p->next = p;
    }
    else
    {
        mm_header_t cur, prev;

        /* Iterate over all blocks in the free ring.
         *
         * The free ring is arranged from high addresses to low
         * addresses, modulo wraparound.
         *
         * We are looking for a block with a higher address than p or
         * whose next address is lower than p.
         */
        for (prev = r->first, cur = prev->next; cur <= p || cur->next >= p;
             prev = cur, cur = prev->next)
        {
            if (cur->magic != MM_FREE_MAGIC)
                die ("free magic is broken at %p: 0x%zx\n", cur, cur->magic);

            /* Deal with wrap-around */
            if (cur <= cur->next && (cur > p || cur->next < p))
                break;
        }

        /* mark p as free and insert it between cur and cur->next */
        p->magic = MM_FREE_MAGIC;
        p->next = cur->next;
        cur->next = p;

        /*
         * If the block we are freeing can be merged with the next
         * free block, do that.
         */
        if (p->next + p->next->size == p)
        {
            p->magic = 0;

            p->next->size += p->size;
            cur->next = p->next;
            p = p->next;
        }

        r->first = cur;

        /* Likewise if can be merged with the preceeding free block */
        if (cur == p + p->size)
        {
            cur->magic = 0;
            p->size += cur->size;
            if (cur == prev)
                prev = p;
            prev->next = p;
            cur = prev;
        }

        /*
         * Set r->first such that the just free()d block is tried first.
         * (An allocation is tried from *first->next, and cur->next == p.)
         */
        r->first = cur;
    }
}

/* Reallocate SIZE bytes and return the pointer. The contents will be
 *   the same as that of PTR.  */
void *
realloc (void *ptr, size_t size)
{
    mm_header_t p;
    mm_region_t r;
    void *q;
    size_t n;

    if (! ptr)
        return malloc (size);

    if (! size)
    {
        free (ptr);
        return 0;
    }

    /* FIXME: Not optimal.  */
    n = ((size + MM_ALIGN - 1) >> MM_ALIGN_LOG2) + 1;
    get_header_from_pointer (ptr, &p, &r);

    if (p->size >= n)
        return ptr;

    q = malloc (size);
    if (! q)
        return q;

    /* We've already checked that p->size < n.  */
    memcpy (q, ptr, p->size << MM_ALIGN_LOG2);
    free (ptr);
    return q;
}
