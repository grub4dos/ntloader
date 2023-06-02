/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2021  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_CHARSET_HEADER
#define GRUB_CHARSET_HEADER 1

#include <stdint.h>

#define GRUB_UINT8_1_LEADINGBIT 0x80
#define GRUB_UINT8_2_LEADINGBITS 0xc0
#define GRUB_UINT8_3_LEADINGBITS 0xe0
#define GRUB_UINT8_4_LEADINGBITS 0xf0
#define GRUB_UINT8_5_LEADINGBITS 0xf8
#define GRUB_UINT8_6_LEADINGBITS 0xfc
#define GRUB_UINT8_7_LEADINGBITS 0xfe

#define GRUB_UINT8_1_TRAILINGBIT 0x01
#define GRUB_UINT8_2_TRAILINGBITS 0x03
#define GRUB_UINT8_3_TRAILINGBITS 0x07
#define GRUB_UINT8_4_TRAILINGBITS 0x0f
#define GRUB_UINT8_5_TRAILINGBITS 0x1f
#define GRUB_UINT8_6_TRAILINGBITS 0x3f

#define GRUB_UCS2_LIMIT 0x10000

/* Process one character from UTF8 sequence. 
   At beginning set *code = 0, *count = 0. Returns 0 on failure and
   1 on success. *count holds the number of trailing bytes.  */
static inline int
grub_utf8_process (uint8_t c, uint32_t *code, int *count)
{
  if (*count)
  {
    if ((c & GRUB_UINT8_2_LEADINGBITS) != GRUB_UINT8_1_LEADINGBIT)
    {
      *count = 0;
      /* invalid */
      return 0;
    }
    else
    {
      *code <<= 6;
      *code |= (c & GRUB_UINT8_6_TRAILINGBITS);
      (*count)--;
      /* Overlong.  */
      if ((*count == 1 && *code <= 0x1f) || (*count == 2 && *code <= 0xf))
      {
        *code = 0;
        *count = 0;
        return 0;
      }
      return 1;
    }
  }

  if ((c & GRUB_UINT8_1_LEADINGBIT) == 0)
  {
    *code = c;
    return 1;
  }
  if ((c & GRUB_UINT8_3_LEADINGBITS) == GRUB_UINT8_2_LEADINGBITS)
  {
    *count = 1;
    *code = c & GRUB_UINT8_5_TRAILINGBITS;
    /* Overlong */
    if (*code <= 1)
    {
      *count = 0;
      *code = 0;
      return 0;
    }
    return 1;
  }
  if ((c & GRUB_UINT8_4_LEADINGBITS) == GRUB_UINT8_3_LEADINGBITS)
  {
    *count = 2;
    *code = c & GRUB_UINT8_4_TRAILINGBITS;
    return 1;
  }
  if ((c & GRUB_UINT8_5_LEADINGBITS) == GRUB_UINT8_4_LEADINGBITS)
  {
    *count = 3;
    *code = c & GRUB_UINT8_3_TRAILINGBITS;
    return 1;
  }
  return 0;
}

/* Convert a (possibly null-terminated) UTF-8 string of at most SRCSIZE
   bytes (if SRCSIZE is -1, it is ignored) in length to a UCS-2 string.
   Return the number of characters converted. DEST must be able to hold
   at least DESTSIZE characters. If an invalid sequence is found, return -1.
   If SRCEND is not NULL, then *SRCEND is set to the next byte after the
   last byte used in SRC.  */
static inline size_t
grub_utf8_to_ucs2 (uint16_t *dest, size_t destsize,
                   const uint8_t *src, size_t srcsize,
                   const uint8_t **srcend)
{
  uint16_t *p = dest;
  int count = 0;
  uint32_t code = 0;

  if (srcend)
    *srcend = src;

  while (srcsize && destsize)
  {
    int was_count = count;
    if (srcsize != (size_t)-1)
      srcsize--;
    if (!grub_utf8_process (*src++, &code, &count))
    {
      code = '?';
      count = 0;
      /* Character c may be valid, don't eat it.  */
      if (was_count)
        src--;
    }
    if (count != 0)
      continue;
    if (code == 0)
      break;
    if (destsize < 2 && code >= GRUB_UCS2_LIMIT)
      break;
    if (code >= GRUB_UCS2_LIMIT)
    {
      *p++ = '?';
      destsize --;
    }
    else
    {
      *p++ = code;
      destsize--;
    }
  }

  if (srcend)
    *srcend = src;
  return p - dest;
}

/* Convert UCS-2 to UTF-8.  */
static inline uint8_t *
grub_ucs2_to_utf8 (uint8_t *dest, const uint16_t *src,
                   size_t size)
{
  while (size--)
  {
    uint16_t code = *src++;
    if (code <= 0x007F)
      *dest++ = code;
    else if (code <= 0x07FF)
    {
      *dest++ = (code >> 6) | 0xC0;
      *dest++ = (code & 0x3F) | 0x80;
    }
    else if (code >= 0xD800 && code <= 0xDFFF)
    {
      /* Error... */
      *dest++ = '?';
    }
    else
    {
      *dest++ = (code >> 12) | 0xE0;
      *dest++ = ((code >> 6) & 0x3F) | 0x80;
      *dest++ = (code & 0x3F) | 0x80;
    }
  }
  return dest;
}

#endif
