/*
 *  ntloader  --  Microsoft Windows NT6+ loader
 *  Copyright (C) 2025  A1ive.
 *
 *  ntloader is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published
 *  by the Free Software Foundation, either version 3 of the License,
 *  or (at your option) any later version.
 *
 *  ntloader is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ntloader.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PMAPI_H
#define _PMAPI_H

#include <stdint.h>

#define MAX_PATH 255

#define NTBOOT_WIM 0x00
#define NTBOOT_VHD 0x01
#define NTBOOT_WOS 0x02
#define NTBOOT_REC 0x03
#define NTBOOT_RAM 0x04
#if 0
#define NTBOOT_APP 0x05
#define NTBOOT_MEM 0x06
#endif

#define NTARG_BOOL_TRUE  1
#define NTARG_BOOL_FALSE 0
#define NTARG_BOOL_UNSET 0xff
#define NTARG_BOOL_NA    0x0f

struct nt_args
{
    uint8_t textmode;
    uint8_t testmode;
    uint8_t hires;
    uint8_t hal;
    uint8_t minint;
    uint8_t novga;
    uint8_t novesa;
    uint8_t safemode;
    uint8_t altshell;
    uint8_t exportcd;
    uint8_t advmenu;
    uint8_t optedit;

    uint64_t nx;
    uint64_t pae;
    uint64_t timeout;
    uint64_t safeboot;
    uint64_t gfxmode;
    uint64_t imgofs;

    char loadopt[128];
    char winload[64];
    char sysroot[32];

    uint32_t boottype;
    char fsuuid[17];
    uint8_t partid[16];
    uint8_t diskid[16];
    uint8_t partmap;
    char filepath[MAX_PATH + 1];

    char initrd_path[MAX_PATH + 1];
    void *bcd;
    uint32_t bcd_length;
    void *bootmgr;
    uint32_t bootmgr_length;

    /* Functions */
    void __attribute__ ((noreturn)) (* _reboot) (void);
    void (* _putchar) (int c);
    int (* _getchar) (void);
    void *(* _malloc) (size_t size);
    void (* _free) (void *ptr);
};

extern struct nt_args *pm;

/** Debugging output */
#if DEBUG == 1 || DEBUG == 2
#define DBG(...) \
do \
{ \
    printf (__VA_ARGS__); \
} while (0)
#else
#define DBG(...)
#endif

/** Verbose debugging output */
#if DEBUG == 2
#define DBG2(...) \
do \
{ \
    printf (__VA_ARGS__); \
} while (0)
#else
#define DBG2(...)
#endif

extern void __attribute__ ((noreturn, format (printf, 1, 2)))
die (const char *fmt, ...);

extern void bios_init (void);
extern void efi_init (void);

#endif /* _PMAPI */
