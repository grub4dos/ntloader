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
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include "msdos.h"
#include "fsuuid.h"
#include "cmdline.h"
#include "ntloader.h"

static unsigned part_count = 0;

static int
check_mbr (void *disk, uint32_t start_lba, int (*disk_read)
(void *disk, uint64_t sector, size_t len, void *buf))
{
    int i;
    struct msdos_part_mbr mbr;
    uint64_t start_addr;

    if (!disk_read (disk, start_lba, sizeof (mbr), &mbr))
        return 0;
    if (mbr.signature != MSDOS_PART_SIGNATURE)
    {
        DBG ("MSDOS partition signature not found.\n");
        return 0;
    }

    for (i = 0; i < MSDOS_MAX_PARTITIONS; i++)
    {
        if (part_count >= 32)
        {
            DBG ("too many partitions.\n");
            return 0;
        }
        if (mbr.entries[i].type == MSDOS_PART_TYPE_GPT_DISK)
        {
            DBG ("found dummy mbr.\n");
            return 0;
        }
        if (! mbr.entries[i].length || msdos_part_is_empty (mbr.entries[i].type))
            continue;
        if (msdos_part_is_extended (mbr.entries[i].type))
        {
            if (check_mbr (disk, start_lba + mbr.entries[i].start, disk_read))
                return 1;
            continue;
        }
        DBG ("part %d (%d) ", part_count++, i);
        if (check_fsuuid (disk, start_lba + mbr.entries[i].start, disk_read))
        {
            start_addr = ((uint64_t) mbr.entries[i].start + start_lba) << 9;
            DBG ("MBR Start=0x%llx Signature=%04X\n", start_addr, mbr.unique_signature);
            memcpy (nt_cmdline->partid, &start_addr, sizeof (start_addr));
            nt_cmdline->partmap = 0x01;
            memcpy (nt_cmdline->diskid, &mbr.unique_signature, sizeof (uint32_t));
            return 1;
        }
    }
    return 0;
}

int
check_msdos_partmap (void *disk,
                     int (*disk_read) (void *disk, uint64_t sector,
                                       size_t len, void *buf))
{
    part_count = 0;
    return check_mbr (disk, 0, disk_read);
}
