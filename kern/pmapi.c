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
#include <stdarg.h>
#include <stdio.h>

#include "bcd.h"
#include "pmapi.h"

#ifdef __x86_64__
extern struct nt_args i386(pm_struct);
#else
struct nt_args pm_struct =
{
    .textmode = NTARG_BOOL_FALSE,
    .testmode = NTARG_BOOL_FALSE,
    .hires = NTARG_BOOL_UNSET, // wim = yes
    .hal = NTARG_BOOL_TRUE,
    .minint = NTARG_BOOL_UNSET, // wim = yes
    .novga = NTARG_BOOL_FALSE,
    .novesa = NTARG_BOOL_FALSE,
    .safemode = NTARG_BOOL_FALSE,
    .altshell = NTARG_BOOL_FALSE,
    .exportcd = NTARG_BOOL_FALSE,
    .advmenu = NTARG_BOOL_FALSE,
    .optedit = NTARG_BOOL_FALSE,

    .nx = NX_OPTIN,
    .pae = PAE_DEFAULT,
    .timeout = 0,
    .safeboot = SAFE_MINIMAL,
    .gfxmode = GFXMODE_1024X768,
    .imgofs = 65536,

    .loadopt = BCD_DEFAULT_CMDLINE,
    .winload = "",
    .sysroot = BCD_DEFAULT_SYSROOT,

    .boottype = NTBOOT_WOS,
    .fsuuid = "",
    .partid = { 0 },
    .diskid = { 0 },
    .partmap = 0x01,

    .initrd_path = "\\initrd.cpio",
    .bcd = NULL,
    .bcd_length = 0,
    .bootmgr = NULL,
    .bootmgr_length = 0,
};
#endif

struct nt_args *pm = &i386(pm_struct);

/**
 * Handle fatal errors
 *
 * @v fmt	Error message format string
 * @v ...	Arguments
 */
void die (const char *fmt, ...)
{
    va_list args;

    /* Print message */
    va_start (args, fmt);
    vprintf (fmt, args);
    va_end (args);

    /* Wait for keypress */
    printf ("Press a key to reboot...");
    getchar ();
    printf ("\n");

    /* Reboot system */
    pm->_reboot ();

    /* Should be impossible to reach this */
    __builtin_unreachable ();
}
