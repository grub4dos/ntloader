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
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

#include "lznt1.h"
#include "xca.h"

#define BOOTMGR_MIN_LEN 16384

static void *
load_bootmgr (const char *path, size_t *len)
{
    FILE *fp = fopen (path, "rb");
    if (!fp)
    {
        fprintf (stderr, "Error opening file '%s': %s\n",
                 path, strerror (errno));
        return NULL;
    }

    if (fseek (fp, 0, SEEK_END) != 0)
    {
        fprintf (stderr, "Error seeking file '%s': %s\n",
                 path, strerror(errno));
        fclose (fp);
        return NULL;
    }
    long fsize = ftell (fp);
    if (fsize <= 0)
    {
        fprintf (stderr, "Error getting file size '%s': %s\n",
                 path, strerror (errno));
        fclose (fp);
        return NULL;
    }

    rewind (fp);

    char *buffer = malloc (fsize);
    if (!buffer)
    {
        fprintf (stderr, "Memory allocation failed\n");
        fclose (fp);
        return NULL;
    }

    size_t read_size = fread (buffer, 1, fsize, fp);
    if (read_size != (size_t) fsize)
    {
        fprintf (stderr, "Error reading file '%s'\n", path);
        free (buffer);
        fclose (fp);
        return NULL;
    }

    fclose (fp);
    *len = (size_t) fsize;
    return buffer;
}

static int
write_data (const char *filename, const void *data, size_t size)
{
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL)
    {
        fprintf (stderr, "cannot open %s\n", filename);
        return -1;
    }

    size_t bytes_written = fwrite (data, 1, size, fp);
    if (bytes_written != size)
    {
        fprintf (stderr, "cannot write to %s\n", filename);
        fclose (fp);
        return -1;
    }

    fclose (fp);
    return 0;
}

static int is_empty_pgh (const void *pgh)
{
    const uint32_t *dwords = pgh;
    return ((dwords[0] | dwords[1] | dwords[2] | dwords[3]) == 0);
}

static int
decompress_bootmgr (const char *out, void *data, size_t len)
{
    const uint8_t *cdata;
    size_t offset;
    size_t cdata_len;
    ssize_t (* decompress) (const void *, size_t, void *);
    ssize_t udata_len;
    void *udata = NULL;

    fprintf (stdout, "bootmgr @%p [%zu]\n", data, len);

    /* Look for an embedded compressed bootmgr.exe on an
     * eight-byte boundary.
     */
    for (offset = BOOTMGR_MIN_LEN;
         offset < (len - BOOTMGR_MIN_LEN);
         offset += 0x08)
    {

        /* Initialise checks */
        decompress = NULL;
        cdata = (uint8_t *) data + offset;
        cdata_len = len - offset;

        /* Check for an embedded LZNT1-compressed bootmgr.exe.
         * Since there is no way for LZNT1 to compress the
         * initial "MZ" bytes of bootmgr.exe, we look for this
         * signature starting three bytes after a paragraph
         * boundary, with a preceding tag byte indicating that
         * these two bytes would indeed be uncompressed.
         */
        if (((offset & 0x0f) == 0x00) &&
            ((cdata[0x02] & 0x03) == 0x00) &&
            (cdata[0x03] == 'M') &&
            (cdata[0x04] == 'Z'))
        {
            fprintf (stdout,
                     "checking for LZNT1 bootmgr.exe at +0x%zx\n",
                     offset);
            decompress = lznt1_decompress;
        }

        /* Check for an embedded XCA-compressed bootmgr.exe.
         * The bytes 0x00, 'M', and 'Z' will always be
         * present, and so the corresponding symbols must have
         * a non-zero Huffman length.  The embedded image
         * tends to have a large block of zeroes immediately
         * beforehand, which we check for.  It's implausible
         * that the compressed data could contain substantial
         * runs of zeroes, so we check for that too, in order
         * to eliminate some common false positive matches.
         */
        if (((cdata[0x00] & 0x0f) != 0x00) &&
            ((cdata[0x26] & 0xf0) != 0x00) &&
            ((cdata[0x2d] & 0x0f) != 0x00) &&
            (is_empty_pgh (cdata - 0x10)) &&
            (! is_empty_pgh ((cdata + 0x400))) &&
            (! is_empty_pgh ((cdata + 0x800))) &&
            (! is_empty_pgh ((cdata + 0xc00))))
        {
            fprintf (stdout,
                     "checking for XCA bootmgr.exe at +0x%zx\n",
                     offset);
            decompress = xca_decompress;
        }

        /* If we have not found a possible bootmgr.exe, skip
         * to the next offset.
         */
        if (! decompress)
            continue;

        /* Find length of decompressed image */
        udata_len = decompress (cdata, cdata_len, NULL);
        if (udata_len < 0)
        {
            /* May be a false positive signature match */
            continue;
        }

        /* Extract decompressed image to memory */
        fprintf (stdout, "extracting embedded bootmgr.exe\n");
        udata = malloc (udata_len);
        if (! udata)
        {
            fprintf (stderr, "out of memory\n");
            return -1;
        }
        decompress (cdata, cdata_len, udata);
        break;
    }

    if (udata == NULL)
    {
        fprintf (stderr, "no embedded bootmgr.exe found\n");
        return -1;
    }
    int rc = write_data (out, udata, udata_len);
    free (udata);
    return rc;
}

int main (int argc, char *argv[])
{

    if (argc < 2)
    {
        fprintf (stderr, "Usage: %s BOOTMGR [BOOTMGR.EXE]\n",
                 argv[0]);
        return EXIT_FAILURE;
    }

    size_t len;
    const char *out = argc >= 3 ? argv[2] : "bootmgr.exe";
    void *data = load_bootmgr (argv[1], &len);

    if (data == NULL)
        return EXIT_FAILURE;

    int rc = decompress_bootmgr (out, data, len);

    free (data);
    return rc;
}
