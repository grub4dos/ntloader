#ifndef _EFIBLOCK_H
#define _EFIBLOCK_H

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
 * EFI block device
 *
 */

#include "efi.h"
#include "efi/Protocol/BlockIo.h"
#include "efi/Protocol/DevicePath.h"

/**
 * Initialise device path
 *
 * @v name    Variable name
 * @v type    Type
 * @v subtype   Subtype
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
 * @v name    Variable name
 */
#define EFI_DEVPATH_END_INIT(name) \
  EFI_DEVPATH_INIT (name, END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE)

struct vdisk_file;

extern void efi_install (void);

extern void efi_boot (struct vdisk_file *file);

#endif /* _EFIBLOCK_H */
