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

#ifndef _BCD_BOOT_H
#define _BCD_BOOT_H 1

#include <stdint.h>

struct bcd_disk_info
{
  uint8_t partid[16];
  uint32_t unknown;
  uint32_t partmap;
  uint8_t diskid[16];
} __attribute__ ((packed));

enum bcd_type
{
  BOOT_WIN,
  BOOT_WIM,
  BOOT_VHD,
};

extern void bcd_patch_data (void);

#endif
