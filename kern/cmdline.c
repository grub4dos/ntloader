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

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <wchar.h>
#include "ntloader.h"
#include "cmdline.h"
#include "pmapi.h"
#include "bcd.h"

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
            snprintf (pm->fsuuid, 17, "%s", value);
        }
        else if (strcmp (key, "wim") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "wim");
            snprintf (pm->filepath, 256, "%s", value);
            convert_path (pm->filepath, 1);
            pm->boottype = NTBOOT_WIM;
        }
        else if (strcmp (key, "vhd") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "vhd");
            snprintf (pm->filepath, 256, "%s", value);
            convert_path (pm->filepath, 1);
            pm->boottype = NTBOOT_VHD;
        }
        else if (strcmp (key, "ram") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "ram");
            snprintf (pm->filepath, 256, "%s", value);
            convert_path (pm->filepath, 1);
            pm->boottype = NTBOOT_RAM;
        }
        else if (strcmp (key, "file") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "file");
            snprintf (pm->filepath, 256, "%s", value);
            convert_path (pm->filepath, 1);
            char *ext = strrchr (value, '.');
            if (ext && strcasecmp(ext, ".wim") == 0)
                pm->boottype = NTBOOT_WIM;
            else
                pm->boottype = NTBOOT_VHD;
        }
        else if (strcmp (key, "text") == 0)
        {
            pm->textmode = NTARG_BOOL_TRUE;
        }
        else if (strcmp (key, "testmode") == 0)
        {
            pm->testmode = convert_bool (value);
        }
        else if (strcmp (key, "hires") == 0)
        {
            pm->hires = convert_bool (value);
        }
        else if (strcmp (key, "detecthal") == 0)
        {
            pm->hal = convert_bool (value);
        }
        else if (strcmp (key, "minint") == 0)
        {
            pm->minint = convert_bool (value);
        }
        else if (strcmp (key, "novga") == 0)
        {
            pm->novga = convert_bool (value);
        }
        else if (strcmp (key, "novesa") == 0)
        {
            pm->novesa = convert_bool (value);
        }
        else if (strcmp (key, "altshell") == 0)
        {
            pm->altshell = convert_bool (value);
            pm->safemode = NTARG_BOOL_TRUE;
        }
        else if (strcmp (key, "exportascd") == 0)
        {
            pm->exportcd = convert_bool (value);
        }
        else if (strcmp (key, "f8") == 0)
        {
            pm->advmenu = NTARG_BOOL_TRUE;
            pm->textmode = NTARG_BOOL_TRUE;
        }
        else if (strcmp (key, "edit") == 0)
        {
            pm->optedit = NTARG_BOOL_TRUE;
            pm->textmode = NTARG_BOOL_TRUE;
        }
        else if (strcmp (key, "nx") == 0)
        {
            if (! value || strcasecmp (value, "OptIn") == 0)
                pm->nx = NX_OPTIN;
            else if (strcasecmp (value, "OptOut") == 0)
                pm->nx = NX_OPTOUT;
            else if (strcasecmp (value, "AlwaysOff") == 0)
                pm->nx = NX_ALWAYSOFF;
            else if (strcasecmp (value, "AlwaysOn") == 0)
                pm->nx = NX_ALWAYSON;
        }
        else if (strcmp (key, "pae") == 0)
        {
            if (! value || strcasecmp (value, "Default") == 0)
                pm->pae = PAE_DEFAULT;
            else if (strcasecmp (value, "Enable") == 0)
                pm->pae = PAE_ENABLE;
            else if (strcasecmp (value, "Disable") == 0)
                pm->pae = PAE_DISABLE;
        }
        else if (strcmp (key, "safemode") == 0)
        {
            pm->safemode = NTARG_BOOL_TRUE;
            if (! value || strcasecmp (value, "Minimal") == 0)
                pm->safeboot = SAFE_MINIMAL;
            else if (strcasecmp (value, "Network") == 0)
                pm->safeboot = SAFE_NETWORK;
            else if (strcasecmp (value, "DsRepair") == 0)
                pm->safeboot = SAFE_DSREPAIR;
        }
        else if (strcmp (key, "gfxmode") == 0)
        {
            pm->hires = NTARG_BOOL_NA;
            if (! value || strcasecmp (value, "1024x768") == 0)
                pm->gfxmode = GFXMODE_1024X768;
            else if (strcasecmp (value, "800x600") == 0)
                pm->gfxmode = GFXMODE_800X600;
            else if (strcasecmp (value, "1024x600") == 0)
                pm->gfxmode = GFXMODE_1024X600;
        }
        else if (strcmp (key, "imgofs") == 0)
        {
            char *endp;
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "imgofs");
            pm->imgofs = strtoul (value, &endp, 0);
            if (*endp)
                die ("Invalid imgofs \"%s\"\n", value);
        }
        else if (strcmp (key, "loadopt") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "loadopt");
            snprintf (pm->loadopt, 128, "%s", value);
            convert_path (pm->loadopt, 0);
        }
        else if (strcmp (key, "winload") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "winload");
            snprintf (pm->winload, 64, "%s", value);
            convert_path (pm->winload, 1);
        }
        else if (strcmp (key, "sysroot") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "sysroot");
            snprintf (pm->sysroot, 32, "%s", value);
            convert_path (pm->sysroot, 1);
        }
        else if (strcmp (key, "initrdfile") == 0 ||
            strcmp (key, "initrd") == 0)
        {
            if (! value || ! value[0])
                die ("Argument \"%s\" needs a value\n", "initrd");

            snprintf (pm->initrd_path, MAX_PATH + 1, "%s", value);
            convert_path (pm->initrd_path, 1);
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

    if (pm->hires == NTARG_BOOL_UNSET)
    {
        if (pm->boottype == NTBOOT_WIM)
            pm->hires = NTARG_BOOL_TRUE;
        else
            pm->hires = NTARG_BOOL_FALSE;
    }

    if (pm->minint == NTARG_BOOL_UNSET)
    {
        if (pm->boottype == NTBOOT_WIM)
            pm->minint = NTARG_BOOL_TRUE;
        else
            pm->minint = NTARG_BOOL_FALSE;
    }
    if (pm->winload[0] == '\0')
    {
        if (pm->boottype == NTBOOT_WIM)
            snprintf (pm->winload, 64, "%s", BCD_LONG_WINLOAD);
        else
            snprintf (pm->winload, 64, "%s", BCD_SHORT_WINLOAD);
    }
}
