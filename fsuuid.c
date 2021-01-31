/*
 *  ntloader  --  Microsoft Windows NT6+ loader
 *  Copyright (C) 2021  A1ive.
 *
 *  ntloader is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  ntloader is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with ntloader.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <fsuuid.h>
#include <cmdline.h>
#include <ntboot.h>

int
check_fsuuid (void *disk, uint64_t lba,
              int (*disk_read) (void *disk, uint64_t sector, size_t len, void *buf))
{
  uint8_t vbr[512];
  char uuid[17] = " ";
  const char *fs = "-";
  struct fat_bpb *fat = (void *) vbr;
  struct exfat_bpb *exfat = (void *) vbr;
  struct ntfs_bpb *ntfs = (void *) vbr;
  if (!disk_read (disk, lba, sizeof (vbr), &vbr))
  {
    DBG ("invalid partition\n");
    return 0;
  }
  if (memcmp ((const char *) exfat->oem_name, "EXFAT", 5) == 0)
  {
    snprintf (uuid, 17, "%04x-%04x",
              (uint16_t) (exfat->num_serial >> 16), (uint16_t) exfat->num_serial);
    fs = "exFAT";
  }
  else if (memcmp ((const char *) ntfs->oem_name, "NTFS", 4) == 0)
  {
    snprintf (uuid, 17, "%016llx",(unsigned long long) ntfs->num_serial);
    fs = "NTFS ";
  }
  else if (memcmp ((const char *) fat->version_specific.fat12_or_fat16.fstype,
                   "FAT12", 5) == 0)
  {
    snprintf (uuid, 17, "%04x-%04x",
              (uint16_t) (fat->version_specific.fat12_or_fat16.num_serial >> 16),
              (uint16_t) fat->version_specific.fat12_or_fat16.num_serial);
    fs = "FAT12";
  }
  else if (memcmp ((const char *) fat->version_specific.fat12_or_fat16.fstype,
                   "FAT16", 5) == 0)
  {
    snprintf (uuid, 17, "%04x-%04x",
              (uint16_t) (fat->version_specific.fat12_or_fat16.num_serial >> 16),
              (uint16_t) fat->version_specific.fat12_or_fat16.num_serial);
    fs = "FAT16";
  }
  else if (memcmp ((const char *) fat->version_specific.fat32.fstype,
                   "FAT32", 5) == 0)
  {
    snprintf (uuid, 17, "%04x-%04x",
              (uint16_t) (fat->version_specific.fat32.num_serial >> 16),
              (uint16_t) fat->version_specific.fat32.num_serial);
    fs = "FAT32";
  }

  DBG ("%s %s\n", fs, uuid);
  if (strcasecmp (uuid, nt_cmdline->uuid) == 0)
    return 1;
  return 0;
}
