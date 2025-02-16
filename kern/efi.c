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

#include "ntloader.h"
#include "efi.h"
#include "efi/Protocol/BlockIo.h"
#include "efi/Protocol/DevicePath.h"
#include "efi/Protocol/SimpleFileSystem.h"
#include "efi/Protocol/LoadFile2.h"
#include "efi/Protocol/LoadedImage.h"

/** EFI system table */
EFI_SYSTEM_TABLE *efi_systab;

/** EFI image handle */
EFI_HANDLE efi_image_handle;

/** Block I/O protocol GUID */
EFI_GUID efi_block_io_protocol_guid
= EFI_BLOCK_IO_PROTOCOL_GUID;

/** Device path protocol GUID */
EFI_GUID efi_device_path_protocol_guid
= EFI_DEVICE_PATH_PROTOCOL_GUID;

/** Loaded image protocol GUID */
EFI_GUID efi_loaded_image_protocol_guid
= EFI_LOADED_IMAGE_PROTOCOL_GUID;

/** Simple file system protocol GUID */
EFI_GUID efi_simple_file_system_protocol_guid
= EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

/** Load File 2 protocol GUID */
EFI_GUID efi_load_file2_protocol_guid
= EFI_LOAD_FILE2_PROTOCOL_GUID;

void *efi_malloc (size_t size)
{
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    EFI_STATUS efirc;
    void *ptr = NULL;
    efirc = bs->AllocatePool (EfiLoaderData, size, &ptr);
    if (efirc != EFI_SUCCESS || ! ptr)
        die ("Could not allocate memory.\n");
    return ptr;
}

void efi_free (void *ptr)
{
    efi_systab->BootServices->FreePool (ptr);
    ptr = 0;
}

void efi_free_pages (void *ptr, UINTN pages)
{
    EFI_PHYSICAL_ADDRESS addr = (intptr_t) ptr;
    efi_systab->BootServices->FreePages (addr, pages);
}

void *efi_allocate_pages (UINTN pages)
{
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    EFI_STATUS efirc;
    EFI_PHYSICAL_ADDRESS addr = 0;
    efirc = bs->AllocatePages (AllocateAnyPages,
                               EfiLoaderData, pages, &addr);
    if (efirc != EFI_SUCCESS)
        die ("Could not allocate memory.\n");
    return (void *) (intptr_t) addr;
}
