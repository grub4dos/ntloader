#ifndef _CMDLINE_H
#define _CMDLINE_H

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
 * Command line
 *
 */

#include <stdint.h>
#include <bcd.h>

#define MAX_PATH 255

#define NT_FLAG_QUIET     (1 << 0)
#define NT_FLAG_PAUSE     (1 << 1)
#define NT_FLAG_WIN7      (1 << 2)

struct nt_args
{
  uint32_t flag;

  char uuid[17];
  char path[MAX_PATH + 1];

  char *initrd_path;

  char test_mode[6];
  char hires[6];
  char detecthal[6];
  char minint[6];
  char novesa[6];
  char novga[6];
  char nx[10];
  char pae[8];
  char loadopt[128];
  char winload[64];
  char sysroot[32];

  enum bcd_type type;
  struct bcd_disk_info info;
  void *bcd_data;
  uint32_t bcd_len;
  wchar_t path16[MAX_PATH + 1];
};

extern struct nt_args *nt_cmdline;

extern void process_cmdline (char *cmdline);

#endif /* _CMDLINE_H */
