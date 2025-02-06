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

#ifndef MSDOS_PART_HEADER
#define MSDOS_PART_HEADER  1

#include <stdint.h>

/* The signature.  */
#define MSDOS_PART_SIGNATURE        0xaa55

/* This is not a flag actually, but used as if it were a flag.  */
#define MSDOS_PART_TYPE_HIDDEN_FLAG    0x10

/* DOS partition types.  */
#define MSDOS_PART_TYPE_NONE              0
#define MSDOS_PART_TYPE_FAT12             1
#define MSDOS_PART_TYPE_FAT16_LT32M       4
#define MSDOS_PART_TYPE_EXTENDED          5
#define MSDOS_PART_TYPE_FAT16_GT32M       6
#define MSDOS_PART_TYPE_NTFS              7
#define MSDOS_PART_TYPE_FAT32             0xb
#define MSDOS_PART_TYPE_FAT32_LBA         0xc
#define MSDOS_PART_TYPE_FAT16_LBA         0xe
#define MSDOS_PART_TYPE_WIN95_EXTENDED    0xf
#define MSDOS_PART_TYPE_PLAN9             0x39
#define MSDOS_PART_TYPE_LDM               0x42
#define MSDOS_PART_TYPE_EZD               0x55
#define MSDOS_PART_TYPE_MINIX             0x80
#define MSDOS_PART_TYPE_LINUX_MINIX       0x81
#define MSDOS_PART_TYPE_LINUX_SWAP        0x82
#define MSDOS_PART_TYPE_EXT2FS            0x83
#define MSDOS_PART_TYPE_LINUX_EXTENDED    0x85
#define MSDOS_PART_TYPE_VSTAFS            0x9e
#define MSDOS_PART_TYPE_FREEBSD           0xa5
#define MSDOS_PART_TYPE_OPENBSD           0xa6
#define MSDOS_PART_TYPE_NETBSD            0xa9
#define MSDOS_PART_TYPE_HFS               0xaf
#define MSDOS_PART_TYPE_GPT_DISK          0xee
#define MSDOS_PART_TYPE_LINUX_RAID        0xfd

#define MSDOS_MAX_PARTITIONS  4

/* The partition entry.  */
struct msdos_part_entry
{
    /* If active, 0x80, otherwise, 0x00.  */
    uint8_t flag;

    /* The head of the start.  */
    uint8_t start_head;

    /* (S | ((C >> 2) & 0xC0)) where S is the sector of the start and C
     *     is the cylinder of the start. Note that S is counted from one.  */
    uint8_t start_sector;

    /* (C & 0xFF) where C is the cylinder of the start.  */
    uint8_t start_cylinder;

    /* The partition type.  */
    uint8_t type;

    /* The end versions of start_head, start_sector and start_cylinder,
     *     respectively.  */
    uint8_t end_head;
    uint8_t end_sector;
    uint8_t end_cylinder;

    /* The start sector. Note that this is counted from zero.  */
    uint32_t start;

    /* The length in sector units.  */
    uint32_t length;
} __attribute__ ((packed));

/* The structure of MBR.  */
struct msdos_part_mbr
{
    char dummy1[11];/* normally there is a short JMP instuction(opcode is 0xEB) */
    uint16_t bytes_per_sector;/* seems always to be 512, so we just use 512 */
    uint8_t sectors_per_cluster;/* non-zero, the power of 2, i.e., 2^n */
    uint16_t reserved_sectors;/* FAT=non-zero, NTFS=0? */
    uint8_t number_of_fats;/* NTFS=0; FAT=1 or 2  */
    uint16_t root_dir_entries;/* FAT32=0, NTFS=0, FAT12/16=non-zero */
    uint16_t total_sectors_short;/* FAT32=0, NTFS=0, FAT12/16=any */
    uint8_t media_descriptor;/* range from 0xf0 to 0xff */
    uint16_t sectors_per_fat;/* FAT32=0, NTFS=0, FAT12/16=non-zero */
    uint16_t sectors_per_track;/* range from 1 to 63 */
    uint16_t total_heads;/* range from 1 to 256 */
    uint32_t hidden_sectors;/* any value */
    uint32_t total_sectors_long;/* FAT32=non-zero, NTFS=0, FAT12/16=any */
    uint32_t sectors_per_fat32;/* FAT32=non-zero, NTFS=any, FAT12/16=any */
    uint64_t total_sectors_long_long;/* NTFS=non-zero, FAT12/16/32=any */
    char dummy2[392];
    uint32_t unique_signature;
    uint8_t unknown[2];

    /* Four partition entries.  */
    struct msdos_part_entry entries[4];

    /* The signature 0xaa55.  */
    uint16_t signature;
} __attribute__ ((packed));

static inline int
msdos_part_is_empty (int type)
{
    return (type == MSDOS_PART_TYPE_NONE);
}

static inline int
msdos_part_is_extended (int type)
{
    return (type == MSDOS_PART_TYPE_EXTENDED ||
    type == MSDOS_PART_TYPE_WIN95_EXTENDED ||
    type == MSDOS_PART_TYPE_LINUX_EXTENDED);
}

extern int
check_msdos_partmap (void *disk,
                     int (*disk_read) (void *disk, uint64_t sector,
                                       size_t len, void *buf));

static inline void
lba_to_chs (uint32_t lba, uint8_t *cl, uint8_t *ch,
            uint8_t *dh)
{
    uint32_t cylinder, head, sector;
    uint32_t sectors = 63, heads = 255, cylinders = 1024;

    sector = lba % sectors + 1;
    head = (lba / sectors) % heads;
    cylinder = lba / (sectors * heads);

    if (cylinder >= cylinders)
    {
        *cl = *ch = 0xff;
        *dh = 0xfe;
        return;
    }

    *cl = sector | ((cylinder & 0x300) >> 2);
    *ch = cylinder & 0xFF;
    *dh = head;
}

#endif /* ! MSDOS_PART_HEADER */
