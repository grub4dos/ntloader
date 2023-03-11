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

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <msdos.h>
#include <gpt.h>
#include <fsuuid.h>
#include <cmdline.h>
#include <ntboot.h>

static uint8_t gpt_magic[8] = GPT_HEADER_MAGIC;

static const gpt_part_guid_t gpt_part_type_empty = GPT_PART_TYPE_EMPTY;

int
check_gpt_partmap (void *disk,
                   int (*disk_read) (void *disk, uint64_t sector,
                                     size_t len, void *buf))
{
  uint8_t sector[1024];
  struct gpt_header gpt;
  struct gpt_part_entry entry;
  uint32_t i;

  /* Read the GPT header */
  if (!disk_read (disk, 1, sizeof (gpt), &gpt))
    return 0;

  if (memcmp (gpt.magic, gpt_magic, sizeof (gpt_magic)) != 0)
  {
    DBG ("GPT magic not found\n");
    return 0;
  }

  for (i = 0; i < gpt.maxpart; i++)
  {
    uint64_t entry_ofs = i * gpt.entry_size;
    uint64_t entry_lba = entry_ofs >> 9;
    entry_ofs -= (entry_lba << 9);
    entry_lba += gpt.partitions;
    if (!disk_read (disk, entry_lba, 512, sector) ||
        !disk_read (disk, entry_lba + 1, 512, sector + 512))
      return 0;
    memcpy (&entry, sector + entry_ofs, sizeof (entry));

    if (memcmp (&gpt_part_type_empty, &entry.type, sizeof (entry.type)) == 0)
      continue;

    DBG ("part %d ", i);
    if (check_fsuuid (disk, entry.start, disk_read))
    {
      DBG ("GPT LBA=%lld\n", (unsigned long long)entry.start);
      memcpy (nt_cmdline->info.partid, &entry.guid, 16);
      nt_cmdline->info.partmap = 0x00;
      memcpy (nt_cmdline->info.diskid, &gpt.guid, 16);
      return 1;
    }
  }
  return 0;
}
