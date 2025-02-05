#ifndef _CMDLINE_H
#define _CMDLINE_H

/*
 * Copyright (C) 2014 Michael Brown <mbrown@fensystems.co.uk>.
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
 * Command line
 *
 */

#include <stdint.h>

#define MAX_PATH 255

#define NTBOOT_WIM 0x00
#define NTBOOT_VHD 0x01
#define NTBOOT_WOS 0x02
#define NTBOOT_REC 0x03
#if 0
#define NTBOOT_APP 0x04
#define NTBOOT_MEM 0x05
#define NTBOOT_RAM 0x06
#define NTBOOT_ISO 0x07
#endif

#define NTARG_BOOL_TRUE  1
#define NTARG_BOOL_FALSE 0
#define NTARG_BOOL_UNSET 0xff

struct nt_args
{
    /* BCD options */
    uint8_t testmode;
    uint8_t hires;
    uint8_t hal;
    uint8_t minint;
    uint8_t novga;
    uint8_t novesa;
    uint8_t safemode;
    uint8_t altshell;

    uint64_t nx;
    uint64_t pae;
    uint64_t timeout;
    uint64_t safeboot;

    char loadopt[128];
    char winload[64];
    char sysroot[32];

    uint32_t boottype;
    char fsuuid[17];
    uint8_t partid[16];
    uint8_t diskid[16];
    uint32_t partmap;
    char filepath[MAX_PATH + 1];

    char initrd_path[MAX_PATH + 1];
    wchar_t initrd_pathw[MAX_PATH + 1];
    void *bcd;
    uint32_t bcd_length;
};

extern struct nt_args *nt_cmdline;

extern void process_cmdline (char *cmdline);

#endif /* _CMDLINE_H */
