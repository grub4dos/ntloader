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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>
#include <ntboot.h>
#include <bcd.h>
#include <cmdline.h>
#include <charset.h>
#include <efi.h>

#define BCD_DP_MAGIC "GNU GRUB2 NTBOOT"


static void
bcd_print_hex (const void *data, size_t len)
{
  const uint8_t *p = data;
  size_t i;
  for (i = 0; i < len; i++)
    DBG2 ("%02x ", p[i]);
}

static void
bcd_replace_hex (const void *search, uint32_t search_len,
                 const void *replace, uint32_t replace_len, int count)
{
  uint8_t *p = nt_cmdline->bcd_data;
  uint32_t offset;
  int cnt = 0;
  for (offset = 0; offset + search_len < nt_cmdline->bcd_len; offset++)
  {
    if (memcmp (p + offset, search, search_len) == 0)
    {
      cnt++;

      DBG2 ("0x%08x ", offset);
      bcd_print_hex (search, search_len);
      DBG2 ("\n---> ");
      bcd_print_hex (replace, replace_len);
      DBG2 ("\n");

      memcpy (p + offset, replace, replace_len);
      DBG ("...patched BCD at %#x len %d\n", offset, replace_len);
      if (count && cnt == count)
        break;
    }
  }
}

static void
bcd_patch_path (void)
{
  const char *search = "\\PATH_SIGN";
  char *p;
  size_t len;
  len = 2 * (strlen (nt_cmdline->path) + 1);
  /* replace '/' to '\\' */
  p = nt_cmdline->path;
  while (*p)
  {
    if (*p == '/')
      *p = '\\';
    if (*p == ':')
      *p = ' ';
    p++;
  }
  /* UTF-8 to UTF-16le */
  grub_utf8_to_utf16 (nt_cmdline->path16, len,
                      (uint8_t *)nt_cmdline->path, -1, NULL);

  bcd_replace_hex (search, strlen (search), nt_cmdline->path16, len, 2);
}

void
bcd_patch_data (void)
{
  static const wchar_t a[] = L".exe";
  static const wchar_t b[] = L".efi";
  if (nt_cmdline->type != BOOT_WIN)
    bcd_patch_path ();

  bcd_replace_hex (BCD_DP_MAGIC, strlen (BCD_DP_MAGIC),
                   &nt_cmdline->info, sizeof (struct bcd_disk_info), 2);
  if (efi_systab)
    bcd_replace_hex (a, 8, b, 8, 0);
  else
    bcd_replace_hex (b, 8, a, 8, 0);
}
