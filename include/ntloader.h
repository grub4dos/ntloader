#ifndef _NTLOADER_H
#define _NTLOADER_H

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
 * NT loader
 *
 */

/** Debug switch */
#ifndef DEBUG
#define DEBUG 1
#endif

/** Base segment address
 *
 * We place everything at 2000:0000, since this region is used by the
 * Microsoft first-stage loaders (e.g. pxeboot.n12, etfsboot.com).
 */
#define BASE_SEG 0x2000

/** Base linear address */
#define BASE_ADDRESS (BASE_SEG << 4)

/** 64 bit long mode code segment */
#define LM_CS 0x10

/** 32 bit protected mode flat code segment */
#define FLAT_CS 0x20

/** 32 bit protected mode flat data segment */
#define FLAT_DS 0x30

/** 16 bit real mode code segment */
#define REAL_CS 0x50

/** 16 bit real mode data segment */
#define REAL_DS 0x60

#ifndef ASSEMBLY

#include <stdint.h>
#include <bootapp.h>

/** Construct wide-character version of a string constant */
#define L(x) _L (x)
#define _L(x) L ## x

/** Page size */
#define PAGE_SIZE 4096

#define BYTES_TO_PAGES(bytes)   (((bytes) + 0xfff) >> 12)
#define PAGES_TO_BYTES(pages)   ((pages) << 12)

/** Calculating log base 2 of 64-bit integers */
#define SECTOR_LOG2ULL(n, s) \
    for (n = 0; (1U << n) < s; n++);

/**
 * Calculate start page number
 *
 * @v address		Address
 * @ret page		Start page number
 */
static inline unsigned int page_start (const void *address)
{
    return (((intptr_t) address) / PAGE_SIZE);
}

/**
 * Calculate end page number
 *
 * @v address		Address
 * @ret page		End page number
 */
static inline unsigned int page_end (const void *address)
{
    return ((((intptr_t) address) + PAGE_SIZE - 1) / PAGE_SIZE);
}

/**
 * Calculate page length
 *
 * @v start		Start address
 * @v end		End address
 * @ret num_pages	Number of pages
 */
static inline unsigned int page_len (const void *start, const void *end)
{
    return (page_end (end) - page_start (start));
}

/**
 * Bochs magic breakpoint
 *
 */
static inline void bochsbp (void)
{
    __asm__ __volatile__ ("xchgw %bx, %bx");
}

/** Debugging output */
#define DBG(...) \
do \
{ \
    if (DEBUG & 1) \
    { \
        printf (__VA_ARGS__); \
    } \
} while (0)

/** Verbose debugging output */
#define DBG2(...) \
do \
{ \
    if (DEBUG & 2) \
    { \
        printf (__VA_ARGS__); \
    } \
} while (0)

/* Branch prediction macros */
#define likely(x) __builtin_expect (!! (x), 1)
#define unlikely(x) __builtin_expect ((x), 0)

#ifdef __i386__
extern void call_real (struct bootapp_callback_params *params);
extern void call_interrupt (struct bootapp_callback_params *params);
extern void __attribute__ ((noreturn)) reboot (void);
#else
static inline void call_real (struct bootapp_callback_params *params)
{
    (void) params;
}
static inline void call_interrupt (struct bootapp_callback_params *params)
{
    (void) params;
}
static inline void reboot (void)
{
}
#endif

extern void __attribute__ ((noreturn, format (printf, 1, 2)))
die (const char *fmt, ...);

extern unsigned long __stack_chk_guard;
extern void init_cookie (void);

#endif /* ASSEMBLY */

#endif /* _NTLOADER_H */
