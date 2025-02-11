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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <wchar.h>
#include "ntloader.h"
#include "cmdline.h"
#include "bcd.h"
#include "charset.h"

static struct nt_args args =
{
    .testmode = NTARG_BOOL_FALSE,
    .hires = NTARG_BOOL_UNSET, // wim = yes
    .hal = NTARG_BOOL_TRUE,
    .minint = NTARG_BOOL_UNSET, // wim = yes
    .novga = NTARG_BOOL_FALSE,
    .novesa = NTARG_BOOL_FALSE,
    .safemode = NTARG_BOOL_FALSE,
    .altshell = NTARG_BOOL_FALSE,

    .nx = NX_OPTIN,
    .pae = PAE_DEFAULT,
    .timeout = 0,
    .safeboot = SAFE_MINIMAL,
    .gfxmode = GFXMODE_1024X768,

    .loadopt = BCD_DEFAULT_CMDLINE,
    .winload = "",
    .sysroot = BCD_DEFAULT_SYSROOT,

    .boottype = NTBOOT_WOS,
    .fsuuid = "",
    .partid = { 0 },
    .diskid = { 0 },
    .partmap = 0x01,

    .initrd_pathw = L"\\initrd.cpio",
    .bcd = NULL,
    .bcd_length = 0,
    .bootmgr = NULL,
    .bootmgr_length = 0,
};

struct nt_args *nt_cmdline;

static void
convert_path (char *str, int backslash)
{
    char *p = str;
    while (*p)
    {
        if (*p == '/' && backslash)
            *p = '\\';
        if (*p == ':')
            *p = ' ';
        if (*p == '\r' || *p == '\n')
            *p = '\0';
        p++;
    }
}

static uint8_t
convert_bool (const char *str)
{
    uint8_t value = NTARG_BOOL_FALSE;
    if (! str ||
        strcasecmp (str, "yes") == 0 ||
        strcasecmp (str, "on") == 0 ||
        strcasecmp (str, "true") == 0 ||
        strcasecmp (str, "1") == 0)
        value = NTARG_BOOL_TRUE;
    return value;
}

/**
 * Process command line
 *
 * @v cmdline		Command line
 */
void process_cmdline (char *cmdline)
{
    char *tmp = cmdline;
    char *key;
    char *value;
    char chr;

    nt_cmdline = &args;

    /* Do nothing if we have no command line */
    if ((cmdline == NULL) || (cmdline[0] == '\0'))
        return;

    /* Show command line */
    printf ("Command line: \"%s\"\n", cmdline);

    /* Parse command line */
    while (*tmp)
    {

        /* Skip whitespace */
        while (isspace (*tmp))
            tmp++;

        /* Find value (if any) and end of this argument */
        key = tmp;
        value = NULL;
        while ((chr = *tmp))
        {
            if (isspace (chr))
            {
                *(tmp++) = '\0';
                break;
            }
            else if ((chr == '=') && (! value))
            {
                *(tmp++) = '\0';
                value = tmp;
            }
            else
                tmp++;
        }

        /* Process this argument */
        if (strcmp (key, "uuid") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "uuid");
            snprintf (args.fsuuid, 17, "%s", value);
        }
        else if (strcmp (key, "wim") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "wim");
            snprintf (args.filepath, 256, "%s", value);
            convert_path (args.filepath, 1);
            args.boottype = NTBOOT_WIM;
        }
        else if (strcmp (key, "vhd") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "vhd");
            snprintf (args.filepath, 256, "%s", value);
            convert_path (args.filepath, 1);
            args.boottype = NTBOOT_VHD;
        }
        else if (strcmp (key, "file") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "file");
            snprintf (args.filepath, 256, "%s", value);
            convert_path (args.filepath, 1);
            char *ext = strrchr (value, '.');
            if (ext && strcasecmp(ext, ".wim") == 0)
                args.boottype = NTBOOT_WIM;
            else
                args.boottype = NTBOOT_VHD;
        }
        else if (strcmp (key, "testmode") == 0)
        {
            args.testmode = convert_bool (value);
        }
        else if (strcmp (key, "hires") == 0)
        {
            args.hires = convert_bool (value);
        }
        else if (strcmp (key, "detecthal") == 0)
        {
            args.hal = convert_bool (value);
        }
        else if (strcmp (key, "minint") == 0)
        {
            args.minint = convert_bool (value);
        }
        else if (strcmp (key, "novga") == 0)
        {
            args.novga = convert_bool (value);
        }
        else if (strcmp (key, "novesa") == 0)
        {
            args.novesa = convert_bool (value);
        }
        else if (strcmp (key, "altshell") == 0)
        {
            args.altshell = convert_bool (value);
            args.safemode = NTARG_BOOL_TRUE;
        }
        else if (strcmp (key, "nx") == 0)
        {
            if (! value || strcasecmp (value, "OptIn") == 0)
                args.nx = NX_OPTIN;
            else if (strcasecmp (value, "OptOut") == 0)
                args.nx = NX_OPTOUT;
            else if (strcasecmp (value, "AlwaysOff") == 0)
                args.nx = NX_ALWAYSOFF;
            else if (strcasecmp (value, "AlwaysOn") == 0)
                args.nx = NX_ALWAYSON;
        }
        else if (strcmp (key, "pae") == 0)
        {
            if (! value || strcasecmp (value, "Default") == 0)
                args.pae = PAE_DEFAULT;
            else if (strcasecmp (value, "Enable") == 0)
                args.pae = PAE_ENABLE;
            else if (strcasecmp (value, "Disable") == 0)
                args.pae = PAE_DISABLE;
        }
        else if (strcmp (key, "safemode") == 0)
        {
            args.safemode = NTARG_BOOL_TRUE;
            if (! value || strcasecmp (value, "Minimal") == 0)
                args.safeboot = SAFE_MINIMAL;
            else if (strcasecmp (value, "Network") == 0)
                args.safeboot = SAFE_NETWORK;
            else if (strcasecmp (value, "DsRepair") == 0)
                args.safeboot = SAFE_DSREPAIR;
        }
        else if (strcmp (key, "gfxmode") == 0)
        {
            args.hires = NTARG_BOOL_NA;
            if (! value || strcasecmp (value, "1024x768") == 0)
                args.gfxmode = GFXMODE_1024X768;
            else if (strcasecmp (value, "800x600") == 0)
                args.gfxmode = GFXMODE_800X600;
            else if (strcasecmp (value, "1024x600") == 0)
                args.gfxmode = GFXMODE_1024X600;
        }
        else if (strcmp (key, "loadopt") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "loadopt");
            snprintf (args.loadopt, 128, "%s", value);
            convert_path (args.loadopt, 0);
        }
        else if (strcmp (key, "winload") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "winload");
            else
            {
                snprintf (args.winload, 64, "%s", value);
                convert_path (args.winload, 1);
            }
        }
        else if (strcmp (key, "sysroot") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "sysroot");
            else
            {
                snprintf (args.sysroot, 32, "%s", value);
                convert_path (args.sysroot, 1);
            }
        }
        else if (strcmp (key, "initrdfile") == 0 ||
            strcmp (key, "initrd") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "initrd");

            char initrd_path[MAX_PATH + 1];
            snprintf (initrd_path, MAX_PATH + 1, "%s", value);
            convert_path (initrd_path, 1);
            utf8_to_ucs2 (args.initrd_pathw, MAX_PATH + 1,
                          (uint8_t *) initrd_path);
        }
        else
        {
            /* Ignore unknown arguments */
        }

        /* Undo modifications to command line */
        if (chr)
            tmp[-1] = chr;
        if (value)
            value[-1] = '=';
    }

    if (args.hires == NTARG_BOOL_UNSET)
    {
        if (args.boottype == NTBOOT_WIM)
            args.hires = NTARG_BOOL_TRUE;
        else
            args.hires = NTARG_BOOL_FALSE;
    }

    if (args.minint == NTARG_BOOL_UNSET)
    {
        if (args.boottype == NTBOOT_WIM)
            args.minint = NTARG_BOOL_TRUE;
        else
            args.minint = NTARG_BOOL_FALSE;
    }
    if (args.winload[0] == '\0')
    {
        if (args.boottype == NTBOOT_WIM)
            snprintf (args.winload, 64, "%s", BCD_LONG_WINLOAD);
        else
            snprintf (args.winload, 64, "%s", BCD_SHORT_WINLOAD);
    }
}
