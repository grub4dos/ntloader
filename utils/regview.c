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

#include "reg.h"
#include "charset.h"

#define MAX_REG_NAME 256

static void *
load_regf (const char *path, size_t *len)
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

static reg_err_t
get_hkey (hive_t *h, HKEY parent, const char *name, HKEY *key)
{
    wchar_t wkey[MAX_REG_NAME];
    *utf8_to_ucs2 (wkey, MAX_REG_NAME, (uint8_t *) name) = L'\0';
    return reg_find_key (h, parent, wkey, key);
}

static void
print_keys (hive_t *h, HKEY key, wchar_t *wname, char *name)
{
    uint32_t i, value_type;
    reg_err_t err;
    for (i = 0, err = REG_ERR_NONE; err == REG_ERR_NONE; i++)
    {
        wname[0] = L'\0';
        err = reg_enum_keys (h, key, i, wname, MAX_REG_NAME);
        if (wname[0] == L'\0')
            continue;
        *ucs2_to_utf8 ((uint8_t *) name, wname, MAX_REG_NAME) = 0;
        printf ("[%s]\n", name);
    }
    for (i = 0, err = REG_ERR_NONE; err == REG_ERR_NONE; i++)
    {
        wname[0] = L'\0';
        err = reg_enum_values (h, key, i,
                               wname, MAX_REG_NAME, &value_type);
        if (wname[0] == L'\0')
            continue;
        *ucs2_to_utf8 ((uint8_t *) name, wname, MAX_REG_NAME) = 0;
        printf ("%s -> [%u]\n", name, value_type);
    }
}

int main (int argc, char *argv[])
{

    if (argc < 2)
    {
        fprintf (stderr, "Usage: %s REGF [KEY]\n", argv[0]);
        return EXIT_FAILURE;
    }

    HKEY root, key;
    hive_t hive;
    wchar_t wname[MAX_REG_NAME];
    char name[MAX_REG_NAME];
    hive.data = load_regf (argv[1], &hive.size);

    if (hive.data == NULL)
        return EXIT_FAILURE;

    if (reg_open_hive (&hive) != REG_ERR_NONE)
    {
        fprintf (stderr, "Error opening hive\n");
        free (hive.data);
        return EXIT_FAILURE;
    }

    reg_find_root (&hive, &root);
    printf ("[ROOT] = %u\n", root);

    if (argc >= 3)
    {
        if (get_hkey (&hive, root, argv[2], &key) == REG_ERR_NONE)
            print_keys (&hive, key, wname, name);
        else
            fprintf (stderr, "Error finding key '%s'\n", argv[2]);
    }
    else
        print_keys (&hive, root, wname, name);

    return 0;
}
