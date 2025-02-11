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

#include <charset.h>

static inline int
is_utf8_trailing_octet (uint8_t c)
{
    /* 10 00 00 00 - 10 11 11 11 */
    return (c >= 0x80 && c <= 0xBF);
}

/* Convert UTF-8 to UCS-2.  */
uint16_t *
utf8_to_ucs2 (uint16_t *dest, size_t destsize,
              const uint8_t *src)
{
    size_t i;
    for (i = 0; src[0] && i < destsize; i++)
    {
        if (src[0] <= 0x7F)
        {
            *dest++ = 0x007F & src[0];
            src += 1;
        }
        else if (src[0] <= 0xDF
            && is_utf8_trailing_octet (src[1]))
        {
            *dest++ = ((0x001F & src[0]) << 6)
                | (0x003F & src[1]);
            src += 2;
        }
        else if (src[0] <= 0xEF
            && is_utf8_trailing_octet (src[1])
            && is_utf8_trailing_octet (src[2]))
        {
            *dest++ = ((0x000F & src[0]) << 12)
                | ((0x003F & src[1]) << 6)
                | (0x003F & src[2]);
                src += 3;
        }
        else
        {
            *dest++ = 0;
            break;
        }
    }
    return dest;
}

/* Convert UCS-2 to UTF-8.  */
uint8_t *
ucs2_to_utf8 (uint8_t *dest, const uint16_t *src,
              size_t size)
{
    size_t i;
    for (i = 0; i < size; i++)
    {
        if (src[i] <= 0x007F)
            *dest++ = src[i];
        else if (src[i] <= 0x07FF)
        {
            *dest++ = (src[i] >> 6) | 0xC0;
            *dest++ = (src[i] & 0x3F) | 0x80;
        }
        else if (src[i] >= 0xD800 && src[i] <= 0xDFFF)
        {
            *dest++ = 0;
            break;
        }
        else
        {
            *dest++ = (src[i] >> 12) | 0xE0;
            *dest++ = ((src[i] >> 6) & 0x3F) | 0x80;
            *dest++ = (src[i] & 0x3F) | 0x80;
        }
    }
    return dest;
}
