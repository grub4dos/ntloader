/*
 * ntloader  --  Microsoft Windows NT6+ loader
 * Copyright (C) 2023  A1ive.
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "fsuuid.h"
#include "cmdline.h"
#include "ntloader.h"

int
check_fsuuid (void *disk, uint64_t lba,
              int (*disk_read) (void *disk, uint64_t sector, size_t len, void *buf))
{
    union volume_boot_record vbr;
    char uuid[17] = " ";
    const char *fs __unused = "-";
    if (!disk_read (disk, lba, sizeof (vbr), &vbr))
    {
        DBG ("invalid partition\n");
        return 0;
    }
    if (memcmp (vbr.exfat.oem_name, "EXFAT", 5) == 0)
    {
        snprintf (uuid, 17, "%04x-%04x",
                  (uint16_t) (vbr.exfat.num_serial >> 16),
                  (uint16_t) vbr.exfat.num_serial);
        fs = "exFAT";
    }
    else if (memcmp (vbr.ntfs.oem_name, "NTFS", 4) == 0)
    {
        snprintf (uuid, 17, "%016llx", (unsigned long long) vbr.ntfs.num_serial);
        fs = "NTFS ";
    }
    else if (memcmp (vbr.fat.version.fat12_or_fat16.fstype, "FAT12", 5) == 0)
    {
        snprintf (uuid, 17, "%04x-%04x",
                  (uint16_t) (vbr.fat.version.fat12_or_fat16.num_serial >> 16),
                  (uint16_t) vbr.fat.version.fat12_or_fat16.num_serial);
        fs = "FAT12";
    }
    else if (memcmp (vbr.fat.version.fat12_or_fat16.fstype, "FAT16", 5) == 0)
    {
        snprintf (uuid, 17, "%04x-%04x",
                  (uint16_t) (vbr.fat.version.fat12_or_fat16.num_serial >> 16),
                  (uint16_t) vbr.fat.version.fat12_or_fat16.num_serial);
        fs = "FAT16";
    }
    else if (memcmp (vbr.fat.version.fat32.fstype, "FAT32", 5) == 0)
    {
        snprintf (uuid, 17, "%04x-%04x",
                  (uint16_t) (vbr.fat.version.fat32.num_serial >> 16),
                  (uint16_t) vbr.fat.version.fat32.num_serial);
        fs = "FAT32";
    }

    DBG ("%s %s\n", fs, uuid);
    if (strcasecmp (uuid, nt_cmdline->fsuuid) == 0)
        return 1;
    return 0;
}
