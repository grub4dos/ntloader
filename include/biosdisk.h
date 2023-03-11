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

#ifndef _BIOSDISK_H
#define _BIOSDISK_H

#include <stdint.h>
#include <bootapp.h>

#define BIOSDISK_FLAG_LBA          1
#define BIOSDISK_FLAG_CDROM        2

/* Drive Parameters.  */
struct biosdisk_drp
{
  uint16_t size;
  uint16_t flags;
  uint32_t cylinders;
  uint32_t heads;
  uint32_t sectors;
  uint64_t total_sectors;
  uint16_t bytes_per_sector;
  /* ver 2.0 or higher */
  union
  {
    uint32_t EDD_configuration_parameters;
    /* Pointer to the Device Parameter Table Extension (ver 3.0+).  */
    uint32_t dpte_pointer;
  };
  /* ver 3.0 or higher */
  uint16_t signature_dpi;
  uint8_t length_dpi;
  uint8_t reserved[3];
  uint8_t name_of_host_bus[4];
  uint8_t name_of_interface_type[8];
  uint8_t interface_path[8];
  uint8_t device_path[16];
  uint8_t reserved2;
  uint8_t checksum;
  /* XXX: This is necessary, because the BIOS of Thinkpad X20
     writes a garbage to the tail of drive parameters,
     regardless of a size specified in a caller.  */
  uint8_t dummy[16];
} __attribute__ ((packed));

struct biosdisk_cdrp
{
  uint8_t size;
  uint8_t media_type;
  uint8_t drive_no;
  uint8_t controller_no;
  uint32_t image_lba;
  uint16_t device_spec;
  uint16_t cache_seg;
  uint16_t load_seg;
  uint16_t length_sec512;
  uint8_t cylinders;
  uint8_t sectors;
  uint8_t heads;
  uint8_t dummy[16];
} __attribute__ ((packed));

/* Disk Address Packet.  */
struct biosdisk_dap
{
  uint8_t length;
  uint8_t reserved;
  uint16_t blocks;
  uint32_t buffer;
  uint64_t block;
} __attribute__ ((packed));

struct biosdisk_data
{
  /* int13h data */
  int drive;
  unsigned long cylinders;
  unsigned long heads;
  unsigned long sectors;
  unsigned long flags;
  /* disk data */
  uint64_t total_sectors;
  unsigned int log_sector_size;
};

extern int biosdisk_read (void *disk, uint64_t sector, size_t len, void *buf);

extern void biosdisk_iterate (void);
extern void biosdisk_fini (void);
extern void biosdisk_init (void);

#endif
