/*
 * Copyright (C) 2012 Michael Brown <mbrown@fensystems.co.uk>.
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
 * Standard Input/Output
 *
 */

#include <stdio.h>
#include <string.h>
#include "bootapp.h"
#include "ntloader.h"
#include "efi.h"

/**
 * Print character to console
 *
 * @v character		Character to print
 */
int putchar (int character)
{
    /* Convert LF to CR,LF */
    if (character == '\n')
        putchar ('\r');

    /* Print character to EFI/BIOS console as applicable */
#ifdef __i386__
    struct bootapp_callback_params params;
    memset (&params, 0, sizeof (params));
    params.vector.interrupt = 0x10;
    params.eax = (0x0e00 | character);
    params.ebx = 0x0007;
    call_interrupt (&params);
#else
    wchar_t wbuf[2];
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *conout = efi_systab->ConOut;
    wbuf[0] = character;
    wbuf[1] = 0;
    conout->OutputString (conout, wbuf);
#endif

    return 0;
}

/**
 * Get character from console
 *
 * @ret character	Character
 */
int getchar (void)
{
    int character;

#ifdef __i386__
    struct bootapp_callback_params params;
    memset (&params, 0, sizeof (params));
    params.vector.interrupt = 0x16;
    call_interrupt (&params);
    character = params.al;
#else
    EFI_INPUT_KEY key;
    UINTN index;
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL *conin = efi_systab->ConIn;
    bs->WaitForEvent (1, &conin->WaitForKey, &index);
    conin->ReadKeyStroke (conin, &key);
    character = key.UnicodeChar;
#endif

    return character;
}
