#ifndef _COMPILER_H
#define _COMPILER_H

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
 * Global compiler definitions
 *
 */

/* Force visibility of all symbols to "hidden", i.e. inform gcc that
 * all symbol references resolve strictly within our final binary.
 * This avoids unnecessary PLT/GOT entries on x86_64.
 *
 * This is a stronger claim than specifying "-fvisibility=hidden",
 * since it also affects symbols marked with "extern".
 */
#ifndef ASSEMBLY
#if __GNUC__ >= 4
#pragma GCC visibility push(hidden)
#endif
#endif /* ASSEMBLY */

#define FILE_LICENCE(x)

/* Mark parameter as unused */
#define __unused __attribute__ ((unused))

/* GCC version checking borrowed from glibc. */
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define GNUC_PREREQ(maj,min) \
((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#  define GNUC_PREREQ(maj,min) 0
#endif

/* Does this compiler support compile-time error attributes? */
#if GNUC_PREREQ(4,3)
#  define GRUB_ATTRIBUTE_ERROR(msg) \
__attribute__ ((__error__ (msg)))
#else
#  define GRUB_ATTRIBUTE_ERROR(msg) __attribute__ ((noreturn))
#endif

#if defined(__clang__) && defined(__clang_major__) && defined(__clang_minor__)
#  define CLANG_PREREQ(maj,min) \
((__clang_major__ > (maj)) || \
(__clang_major__ == (maj) && __clang_minor__ >= (min)))
#else
#  define CLANG_PREREQ(maj,min) 0
#endif

/* Safe math */
#if GNUC_PREREQ(5, 1) || CLANG_PREREQ(8, 0)

#define safe_add(a, b, res) __builtin_add_overflow(a, b, res)
#define safe_sub(a, b, res) __builtin_sub_overflow(a, b, res)
#define safe_mul(a, b, res) __builtin_mul_overflow(a, b, res)

#define safe_cast(a, res)   grub_add ((a), 0, (res))

/*
 * It's caller's responsibility to check "align" does not equal 0 and
 * is power of 2.
 */
#define ALIGN_UP_OVF(v, align, res) \
({ \
    bool __failed; \
    typeof(v) __a = ((typeof(v))(align) - 1); \
    \
    __failed = safe_add (v, __a, res); \
    if (__failed == false) \
        *(res) &= ~__a; \
        __failed; \
})

#else
#error gcc 5.1 or newer or clang 8.0 or newer is required
#endif

#endif /* _COMPILER_H */
