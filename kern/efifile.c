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

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>
#include "ntloader.h"
#include "vdisk.h"
#include "cmdline.h"
#include "charset.h"
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
 * Extract files from ESP partition
 *
 * @v len		Initrd length
 * @v handle		Device handle
 * @ret ptr		Return initrd pointer
 */
static void *
efi_load_sfs_initrd (UINTN *len, EFI_HANDLE handle)
{
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    EFI_FILE_PROTOCOL *root;
    EFI_FILE_PROTOCOL *file;
    UINT64 size;
    void *initrd;
    EFI_STATUS efirc;
    wchar_t wpath[MAX_PATH + 1];

    *utf8_to_ucs2 (wpath, MAX_PATH + 1,
                   (uint8_t *) nt_cmdline->initrd_path) = L'\0';

    /* Open file system */
    efirc = bs->OpenProtocol (handle,
                              &efi_simple_file_system_protocol_guid,
                              (void *)&fs, efi_image_handle, NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (efirc != EFI_SUCCESS)
        die ("Could not open simple file system.\n");

    /* Open root directory */
    efirc = fs->OpenVolume (fs, &root);
    if (efirc != EFI_SUCCESS)
        die ("Could not open root directory.\n");

    /* Close file system */
    bs->CloseProtocol (handle, &efi_simple_file_system_protocol_guid,
                       efi_image_handle, NULL);

    efirc = root->Open (root, &file, wpath, EFI_FILE_MODE_READ, 0);
    if (efirc != EFI_SUCCESS)
        die ("Could not open %ls.\n", wpath);
    file->SetPosition (file, 0xFFFFFFFFFFFFFFFF);
    file->GetPosition (file, &size);
    file->SetPosition (file, 0);
    if (!size)
        die ("Could not get file size\n");
    DBG ("...found %ls size 0x%llx\n", wpath, size);
    *len = size;
    initrd = efi_allocate_pages (BYTES_TO_PAGES (*len), EfiLoaderData);
    efirc = file->Read (file, len, initrd);
    if (efirc != EFI_SUCCESS)
        die ("Could not read from file.\n");

    return initrd;
}

/**
 * Extract files from Linux initrd media
 *
 * @v len		Initrd length
 * @ret ptr		Return initrd pointer
 */
static void *
efi_load_lf2_initrd (UINTN *len)
{
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    EFI_HANDLE lf2_handle;
    EFI_LOAD_FILE2_PROTOCOL *lf2;
    EFI_DEVICE_PATH_PROTOCOL *dp =
        (EFI_DEVICE_PATH_PROTOCOL *) &efi_initrd_path;
    void *initrd;
    EFI_STATUS efirc;

    /* Locate initrd media device */
    efirc = bs->LocateDevicePath (&efi_load_file2_protocol_guid,
                                  &dp, &lf2_handle);
    if (efirc != EFI_SUCCESS)
        return NULL;
    DBG ("...found initrd media device\n");

    /* Get LoadFile2 protocol */
    efirc = bs->HandleProtocol (lf2_handle,
                                &efi_load_file2_protocol_guid,
                                (void **) &lf2);
    if (efirc != EFI_SUCCESS)
        die ("Could not get LoadFile2 protocol.\n");

    /* Get initrd size */
    efirc = lf2->LoadFile (lf2, dp, FALSE, len, NULL);
    if (*len == 0)
        die ("Could not get initrd size\n");

    /* Allocate memory */
    initrd = efi_allocate_pages (BYTES_TO_PAGES (*len), EfiLoaderData);

    /* Read initrd */
    efirc = lf2->LoadFile (lf2, dp, FALSE, len, initrd);
    if (efirc != EFI_SUCCESS)
        die ("Could not read initrd.\n");

    return initrd;
}

/**
 * Extract files from EFI file system
 *
 * @v handle		Device handle
 */
void efi_extract (EFI_HANDLE handle)
{
    UINTN initrd_len = 0;
    void *initrd;
    /* Extract files from initrd media */
    initrd = efi_load_lf2_initrd (&initrd_len);

    if (initrd == NULL)
        initrd = efi_load_sfs_initrd (&initrd_len, handle);

    if (initrd == NULL)
        die ("Could not load initrd\n");

    extract_initrd (initrd, initrd_len);
}
