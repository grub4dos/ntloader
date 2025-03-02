#ifndef _EFI_H
#define _EFI_H

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
 * EFI definitions
 *
 */

/* EFIAPI definition */
#if __x86_64__
#define EFIAPI __attribute__ ((ms_abi))
#else
#define EFIAPI
#endif

/* EFI headers rudely redefine NULL */
#undef NULL

#include "efi/Uefi.h"
#include "efi/Protocol/LoadedImage.h"
#include "efi/Protocol/DevicePath.h"

/**
 * Initialise device path
 *
 * @v name		Variable name
 * @v type		Type
 * @v subtype		Subtype
 */
#define EFI_DEVPATH_INIT(name, type, subtype) \
{ \
    .Type = (type), \
    .SubType = (subtype), \
    .Length[0] = (sizeof (name) & 0xff), \
    .Length[1] = (sizeof (name) >> 8), \
}

/**
 * Initialise device path end
 *
 * @v name		Variable name
 */
#define EFI_DEVPATH_END_INIT(name) \
    EFI_DEVPATH_INIT (name, \
                      END_DEVICE_PATH_TYPE, \
                      END_ENTIRE_DEVICE_PATH_SUBTYPE)

extern EFI_SYSTEM_TABLE *efi_systab;
extern EFI_HANDLE efi_image_handle;

extern EFI_GUID efi_block_io_protocol_guid;
extern EFI_GUID efi_device_path_protocol_guid;
extern EFI_GUID efi_loaded_image_protocol_guid;
extern EFI_GUID efi_simple_file_system_protocol_guid;
extern EFI_GUID efi_load_file2_protocol_guid;
extern EFI_GUID efi_gop_guid;

extern void efi_free_pages (void *ptr, UINTN pages);
extern void *efi_allocate_pages (UINTN pages, EFI_MEMORY_TYPE type);

#endif /* _EFI_H */
