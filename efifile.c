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
 * EFI file system access
 *
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>
#include "ntloader.h"
#include "vdisk.h"
#include "cmdline.h"
#include "payload.h"
#include "efi.h"
#include "efifile.h"
#include "efi/Protocol/LoadFile2.h"
#include "efi/Guid/LinuxEfiInitrdMedia.h"


/** Linux initrd media device path */
static struct
{
    VENDOR_DEVICE_PATH vendor;
    EFI_DEVICE_PATH_PROTOCOL end;
} __attribute__ ((packed)) efi_initrd_path =
{
    .vendor = {
        .Header = EFI_DEVPATH_INIT (efi_initrd_path.vendor,
                                    MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP),
        .Guid = LINUX_EFI_INITRD_MEDIA_GUID,
    },
    .end = EFI_DEVPATH_END_INIT (efi_initrd_path.end),
};

/**
 * Extract files from Linux initrd media
 *
 * @ret rc		Return status code
 */
static int
efi_load_lf2_initrd (void)
{
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    EFI_HANDLE lf2_handle;
    EFI_LOAD_FILE2_PROTOCOL *lf2;
    EFI_DEVICE_PATH_PROTOCOL *dp = (EFI_DEVICE_PATH_PROTOCOL *) &efi_initrd_path;
    UINTN initrd_len = 0;
    UINTN pages;
    void *initrd;
    EFI_PHYSICAL_ADDRESS phys;
    EFI_STATUS efirc;

    /* Locate initrd media device */
    efirc = bs->LocateDevicePath (&efi_load_file2_protocol_guid,
                                   &dp, &lf2_handle);
    if (efirc != EFI_SUCCESS)
        return -1;
    DBG ("...found initrd media device\n");

    /* Get LoadFile2 protocol */
    efirc = bs->HandleProtocol (lf2_handle, &efi_load_file2_protocol_guid,
                                 (void **) &lf2);
    if (efirc != EFI_SUCCESS)
        die ("Could not get LoadFile2 protocol.\n");

    /* Get initrd size */
    efirc = lf2->LoadFile (lf2, dp, FALSE, &initrd_len, NULL);
    if (initrd_len == 0)
        die ("Could not get initrd size\n");

    /* Allocate memory */
    pages = ((initrd_len + PAGE_SIZE - 1) / PAGE_SIZE);
    if ((efirc = bs->AllocatePages (AllocateAnyPages,
                                       EfiLoaderData, pages,
                                       &phys)) != 0)
    {
        die ("Could not allocate %ld pages: %#lx\n",
              ((unsigned long) pages), ((unsigned long) efirc));
    }
    initrd = ((void *) (intptr_t) phys);

    /* Read initrd */
    efirc = lf2->LoadFile (lf2, dp, FALSE, &initrd_len, initrd);
    if (efirc != EFI_SUCCESS)
        die ("Could not read initrd.\n");

    extract_initrd (initrd, initrd_len);

    return 0;
}

/**
 * Extract files from EFI file system
 *
 * @v handle		Device handle
 */
void efi_extract (EFI_HANDLE handle __unused)
{
    /* Extract files from initrd media */
    if (efi_load_lf2_initrd () != 0)
        die ("Could not load initrd\n");
}
