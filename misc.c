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

#include <stdarg.h>
#include <stdio.h>
#include "ntboot.h"
#include "efi.h"

/**
 * Handle fatal errors
 *
 * @v fmt Error message format string
 * @v ... Arguments
 */
void die (const char *fmt, ...)
{
  EFI_RUNTIME_SERVICES *rs;
  va_list args;
  /* Print message */
  va_start (args, fmt);
  vprintf (fmt, args);
  va_end (args);
  /* Wait for keypress */
  printf ("Press a key to reboot...");
  getchar();
  printf ("\n");
  /* Reboot */
  if (efi_systab)
  {
    rs = efi_systab->RuntimeServices;
    rs->ResetSystem (EfiResetWarm, 0, 0, NULL);
    printf ("Failed to reboot\\n");
  }
  else
    reboot();
  /* Should be impossible to reach this */
  __builtin_unreachable();
}

/**
 * Pause before booting
 *
 */
void pause_boot (void)
{
  /* Wait for keypress, prompting unless inhibited */
  if (nt_cmdline->pause_quiet)
    getchar();
  else
  {
    printf ("Press any key to continue booting...");
    getchar();
    printf ("\n");
  }
}

static char nt_ascii_art[] =
"      ___       ___          ___   ___          ___     \n"
"     /\\__\\     /\\  \\        /\\__\\ /\\  \\        /\\  \\    \n"
"    /::|  |    \\:\\  \\      /:/  //::\\  \\      /::\\  \\   \n"
"   /:|:|  |     \\:\\  \\    /:/  //:/\\:\\  \\    /:/\\:\\  \\  \n"
"  /:/|:|  |__   /::\\  \\  /:/  //:/  \\:\\__\\  /::\\~\\:\\  \\ \n"
" /:/ |:| /\\__\\ /:/\\:\\__\\/:/__//:/__/ \\:|__|/:/\\:\\ \\:\\__\\\n"
" \\/__|:|/:/  //:/  \\/__/\\:\\  \\\\:\\  \\ /:/  /\\/_|::\\/:/  /\n"
"     |:/:/  //:/  /      \\:\\  \\\\:\\  /:/  /    |:|::/  / \n"
"     |::/  / \\/__/        \\:\\  \\\\:\\/:/  /     |:|\\/__/  \n"
"     /:/  /                \\:\\__\\\\::/__/      |:|  |    \n"
"     \\/__/                  \\/__/ ~~           \\|__|    \n"
"\n  NTloader " VERSION " -- Windows NT6+ OS/VHD/WIM loader"
"\n          Copyright (C) 2021 A1ive.\n";

void print_banner (void)
{
  printf ("%s", nt_ascii_art);
}

