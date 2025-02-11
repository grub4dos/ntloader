/*
 * ntloader  --  Microsoft Windows NT6+ loader
 * Copyright (C) 2025  A1ive.
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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>
#include "vdisk.h"
#include "cpio.h"
#include "ntloader.h"
#include "bcd.h"
#include "cmdline.h"
#include "efi.h"

/**
 * Read virtual file from memory
 *
 * @v file		Virtual file
 * @v data		Data buffer
 * @v offset		Offset
 * @v len		Length
 */
static void read_mem_file (struct vdisk_file *file, void *data,
                           size_t offset, size_t len)
{
    memcpy (data, (file->opaque + offset), len);
}

/**
 * Get architecture-specific boot filename
 *
 * @ret bootarch	Architecture-specific boot filename
 */
static const CHAR16 *efi_bootarch (void)
{
    static const CHAR16 bootarch_full[] = EFI_REMOVABLE_MEDIA_FILE_NAME;
    const CHAR16 *tmp;
    const CHAR16 *bootarch = bootarch_full;

    for (tmp = bootarch_full ; *tmp ; tmp++)
    {
        if (*tmp == L'\\')
            bootarch = (tmp + 1);
    }
    return bootarch;
}

/**
 * File handler
 *
 * @v name		File name
 * @v data		File data
 * @v len		Length
 * @ret rc		Return status code
 */
static int add_file (const char *name, void *data, size_t len)
{
    char bootarch[32];

    snprintf (bootarch, sizeof (bootarch), "%ls", efi_bootarch());

    vdisk_add_file (name, data, len, read_mem_file);

    /* Check for special-case files */
    if ((efi_systab && strcasecmp (name, bootarch) == 0) ||
        (!efi_systab && strcasecmp (name, "bootmgr.exe") == 0))
    {
        DBG ("...found bootmgr file %s\n", name);
        nt_cmdline->bootmgr_length = len;
        nt_cmdline->bootmgr = data;
    }
    else if (strcasecmp (name, "BCD") == 0)
    {
        DBG ("...found BCD\n");
        nt_cmdline->bcd_length = len;
        nt_cmdline->bcd = data;
    }

    return 0;
}

/**
 * Extract cpio initrd
 *
 * @v ptr		Initrd data
 * @v len		Initrd length
 */
void extract_initrd (void *ptr, size_t len)
{
    if (cpio_extract (ptr, len, add_file) != 0)
        die ("FATAL: could not extract initrd files\n");

    if (!nt_cmdline->bootmgr)
        die ("FATAL: no bootmgr\n");

    bcd_patch_data ();
}
