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

#ifndef _ACPI_H
#define _ACPI_H 1

#include <ntboot.h>
#include <stdint.h>

#define EFI_ACPI_TABLE_GUID \
  { 0xeb9d2d30, 0x2d88, 0x11d3, \
    { 0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define EFI_ACPI_20_TABLE_GUID  \
  { 0x8868e871, 0xe4f1, 0x11d3, \
    { 0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define ACPI_RSDP_SIGNATURE "RSD PTR "
#define ACPI_RSDP_SIGNATURE_SIZE 8

struct acpi_rsdp_v10
{
  uint8_t signature[ACPI_RSDP_SIGNATURE_SIZE];
  uint8_t checksum;
  uint8_t oemid[6];
  uint8_t revision;
  uint32_t rsdt_addr;
} __attribute__ ((packed));

struct acpi_rsdp_v20
{
  struct acpi_rsdp_v10 rsdpv1;
  uint32_t length;
  uint64_t xsdt_addr;
  uint8_t checksum;
  uint8_t reserved[3];
} __attribute__ ((packed));

struct acpi_table_header
{
  uint8_t signature[4];
  uint32_t length;
  uint8_t revision;
  uint8_t checksum;
  uint8_t oemid[6];
  uint8_t oemtable[8];
  uint32_t oemrev;
  uint8_t creator_id[4];
  uint32_t creator_rev;
} __attribute__ ((packed));

struct acpi_bgrt
{
  struct acpi_table_header header;
  // 2-bytes (16 bit) version ID. This value must be 1.
  uint16_t version;
  // 1-byte status field indicating current status about the table.
  // Bits[7:1] = Reserved (must be zero)
  // Bit [0] = Valid. A one indicates the boot image graphic is valid.
  uint8_t status;
  // 0 = Bitmap
  // 1 - 255  Reserved (for future use)
  uint8_t type;
  // physical address pointing to the firmware's in-memory copy of the image.
  uint64_t addr;
  // (X, Y) display offset of the top left corner of the boot image.
  // The top left corner of the display is at offset (0, 0).
  uint32_t x;
  uint32_t y;
} __attribute__ ((packed));

struct bmp_header
{
  // bmfh
  uint8_t bftype[2];
  uint32_t bfsize;
  uint16_t bfreserved1;
  uint16_t bfreserved2;
  uint32_t bfoffbits;
  // bmih
  uint32_t bisize;
  int32_t biwidth;
  int32_t biheight;
  uint16_t biplanes;
  uint16_t bibitcount;
  uint32_t bicompression;
  uint32_t bisizeimage;
  int32_t bixpelspermeter;
  int32_t biypelspermeter;
  uint32_t biclrused;
  uint32_t biclrimportant;
} __attribute__ ((packed));

extern void
acpi_load_bgrt (void *file, size_t file_len);

#endif
