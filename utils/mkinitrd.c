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
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include "cpio.h"

#ifdef _WIN32
#define lstat stat
#endif

static char
hex (uint8_t val)
{
    if (val < 10)
        return '0' + val;
    return 'a' + val - 10;
}

static void
set_field (char *var, uint32_t val)
{
    int i;
    char *ptr = var;
    for (i = 28; i >= 0; i -= 4)
        *ptr++ = hex((val >> i) & 0xf);
}

static int
write_header (FILE *out,
              const char *name, uint32_t mode, uint32_t fsize)
{
    struct cpio_header header;
    uint32_t len = (uint32_t)(strlen (name) + 1);
    memcpy (header.c_magic, CPIO_MAGIC, 6);
    set_field (header.c_ino, 0);
    set_field (header.c_mode, mode);
    set_field (header.c_uid, 0);
    set_field (header.c_gid, 0);
    set_field (header.c_nlink, 1);
    set_field (header.c_mtime, 0);
    set_field (header.c_filesize, fsize);
    set_field (header.c_maj, 0);
    set_field (header.c_min, 0);
    set_field (header.c_rmaj, 0);
    set_field (header.c_rmin, 0);
    set_field (header.c_namesize, len);
    set_field (header.c_chksum, 0);

    if (sizeof (header) != 110)
    {
        perror ("cpio header");
        return -1;
    }

    if(fwrite (&header, 1, sizeof (header), out) != sizeof (header))
    {
        perror ("fwrite header");
        return -1;
    }

    if(fwrite (name, 1, len, out) != len)
    {
        perror ("fwrite name");
        return -1;
    }

    size_t total = sizeof (header) + len;
    int pad = (4 - (total % 4)) % 4;
    if (pad > 0)
    {
        char zero[4] = {0, 0, 0, 0};
        if (fwrite (zero, 1, pad, out) != (size_t) pad)
        {
            perror ("fwrite header padding");
            return -1;
        }
    }
    return 0;
}

static int
copy_data (FILE *out, FILE *in, size_t fsize)
{
    size_t remaining = fsize;
    char buffer[4096];
    while(remaining > 0)
    {
        size_t to_read = remaining < sizeof(buffer) ?
                            remaining : sizeof(buffer);
        size_t r = fread (buffer, 1, to_read, in);
        if(r == 0)
        {
            if(ferror (in))
            {
                perror ("fread");
                return -1;
            }
            break;
        }
        if(fwrite (buffer, 1, r, out) != r)
        {
            perror ("fwrite file data");
            return -1;
        }
        remaining -= r;
    }

    int pad = (4 - (fsize % 4)) % 4;
    if (pad > 0)
    {
        char zero[4] = {0, 0, 0, 0};
        if (fwrite (zero, 1, pad, out) != (size_t) pad)
        {
            perror ("fwrite data padding");
            return -1;
        }
    }
    return 0;
}

static int
process_file (const char *path,
              const char *arcname,
              const struct stat *st,
              FILE *out)
{
    uint32_t fsize = (uint32_t) st->st_size;
    if (write_header (out, arcname, st->st_mode, fsize) != 0)
        return -1;

    FILE *in = fopen (path, "rb");
    if(!in)
    {
        perror (path);
        return -1;
    }
    if(copy_data (out, in, fsize) != 0)
    {
        fclose (in);
        return -1;
    }
    fclose (in);
    return 0;
}

static int
process_root (const char *dir, FILE *out)
{
    char full[PATH_MAX];
    DIR *d = opendir (dir);
    if (!d)
    {
        perror (dir);
        return -1;
    }
    struct dirent *de;
    while ((de = readdir (d)) != NULL)
    {
        if (strcmp(de->d_name, ".") == 0 ||
            strcmp(de->d_name, "..") == 0)
            continue;

        snprintf (full, PATH_MAX, "%s/%s", dir, de->d_name);

        struct stat st;
        if (lstat (full, &st) < 0)
        {
            perror (full);
            closedir (d);
            return -1;
        }

        if (S_ISREG (st.st_mode))
        {
            if (process_file (full, de->d_name, &st, out) != 0)
            {
                closedir (d);
                return -1;
            }
        }
    }
    closedir (d);
    return 0;
}

int main (int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf (stderr, "Usage: %s DIR OUT_FILE\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *dir = argv[1];
    const char *outfile = argv[2];

    FILE *out = fopen (outfile, "wb");
    if (!out)
    {
        perror (outfile);
        return EXIT_FAILURE;
    }

    if (process_root (dir, out) != 0)
    {
        fclose (out);
        return EXIT_FAILURE;
    }

    if (write_header (out, CPIO_TRAILER, 0, 0) != 0)
    {
        fclose (out);
        return EXIT_FAILURE;
    }
    fclose (out);
    return EXIT_SUCCESS;
}
