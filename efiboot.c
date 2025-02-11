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
 * EFI boot manager invocation
 *
 */

#include <stdio.h>
#include <string.h>
#include "ntloader.h"
#include "cmdline.h"
#include "vdisk.h"
#include "efi.h"
#include "efiboot.h"

/**
 * Boot from EFI device
 *
 * @v path		Device path
 * @v device		Device handle
 */
void efi_boot (EFI_DEVICE_PATH_PROTOCOL *path,
               EFI_HANDLE device)
{
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    union
    {
        EFI_LOADED_IMAGE_PROTOCOL *image;
        void *intf;
    } loaded;
    EFI_PHYSICAL_ADDRESS phys;
    void *data;
    unsigned int pages;
    EFI_HANDLE handle;
    EFI_STATUS efirc;

    /* Allocate memory */
    pages = ((nt_cmdline->bootmgr_length + PAGE_SIZE - 1) / PAGE_SIZE);
    efirc = bs->AllocatePages (AllocateAnyPages,
                               EfiBootServicesData, pages,
                               &phys);
    if (efirc != 0)
    {
        die ("Could not allocate %d pages: %#lx\n",
             pages, ((unsigned long) efirc));
    }
    data = ((void *) (intptr_t) phys);


    /* Copy image */
    memcpy (data, nt_cmdline->bootmgr, nt_cmdline->bootmgr_length);

    /* Load image */
    efirc = bs->LoadImage (FALSE, efi_image_handle, path, data,
                           nt_cmdline->bootmgr_length, &handle);
    if (efirc != 0)
    {
        die ("Could not load bootmgfw.efi: %#lx\n",
             ((unsigned long) efirc));
    }
    DBG ("Loaded bootmgfw.efi\n");

    /* Get loaded image protocol */
    efirc = bs->OpenProtocol (handle,
                              &efi_loaded_image_protocol_guid,
                              &loaded.intf, efi_image_handle, NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (efirc != 0)
    {
        die ("Could not get loaded image protocol: %#lx\n",
             ((unsigned long) efirc));
    }

    /* Force correct device handle */
    if (loaded.image->DeviceHandle != device)
    {
        DBG ("Forcing correct DeviceHandle (%p->%p)\n",
             loaded.image->DeviceHandle, device);
        loaded.image->DeviceHandle = device;
    }

    /* Start image */
    if ((efirc = bs->StartImage (handle, NULL, NULL)) != 0)
    {
        die ("Could not start bootmgfw.efi: %#lx\n",
             ((unsigned long) efirc));
    }

    die ("bootmgfw.efi returned\n");
}
