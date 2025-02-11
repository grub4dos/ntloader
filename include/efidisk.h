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

#ifndef _EFIDISK_H
#define _EFIDISK_H

#include "efi.h"
#include "efi/Protocol/BlockIo.h"
#include "efi/Protocol/DevicePath.h"

struct efidisk_data
{
    char name[16];
    EFI_HANDLE handle;
    EFI_DEVICE_PATH_PROTOCOL *dp;
    EFI_DEVICE_PATH_PROTOCOL *ldp;
    EFI_BLOCK_IO_PROTOCOL *bio;
    struct efidisk_data *next;
};

extern int efidisk_read (void *disk, uint64_t sector, size_t len, void *buf);

extern void efidisk_iterate (void);
extern void efidisk_fini (void);
extern void efidisk_init (void);

#endif /* _EFIBLOCK_H */
