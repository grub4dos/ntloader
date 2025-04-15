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
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <wchar.h>
#include "vdisk.h"
#include "cpio.h"
#include "ntloader.h"
#include "bcd.h"
#include "cmdline.h"
#include "efi.h"

/* The console control protocol is not a part of the EFI spec,
 * but defined in Intel's Sample Implementation. */

static EFI_GUID efi_console_ctrl_guid =
{
    0xf42f7782, 0x12e, 0x4c12,
    { 0x99, 0x56, 0x49, 0xf9, 0x43, 0x4, 0xf7, 0x21 },
};

typedef enum
{
    EFI_SCREEN_TEXT,
    EFI_SCREEN_GRAPHICS,
    EFI_SCREEN_TEXT_MAX_VALUE
} EFI_SCREEN_MODE;

struct _EFI_CONSOLE_CTRL_PROTOCOL
{
    EFI_STATUS
    (EFIAPI *GetMode) (struct _EFI_CONSOLE_CTRL_PROTOCOL *this,
                       EFI_SCREEN_MODE *mode,
                       BOOLEAN *uga_exists,
                       BOOLEAN *std_in_locked);

    EFI_STATUS
    (EFIAPI *SetMode) (struct _EFI_CONSOLE_CTRL_PROTOCOL *this,
                       EFI_SCREEN_MODE mode);

    EFI_STATUS
    (EFIAPI *LockStdin) (struct _EFI_CONSOLE_CTRL_PROTOCOL *this,
                         CHAR16 *password);
};
typedef struct _EFI_CONSOLE_CTRL_PROTOCOL EFI_CONSOLE_CTRL_PROTOCOL;

int efi_set_text_mode (int on)
{
    EFI_STATUS rc;
    EFI_CONSOLE_CTRL_PROTOCOL *c;
    EFI_SCREEN_MODE mode, new_mode;
    EFI_BOOT_SERVICES *bs = efi_systab->BootServices;

    rc = bs->LocateProtocol(&efi_console_ctrl_guid,
                            NULL,
                            (void**) &c);
    if (rc != EFI_SUCCESS)
    {
        /* No console control protocol instance available, assume it is
         * already in text mode. */
        DBG ("Console control protocol not found, assume text mode.\n");
        return 1;
    }

    if (c->GetMode (c, &mode, 0, 0) != EFI_SUCCESS)
        return 0;

    new_mode = on ? EFI_SCREEN_TEXT : EFI_SCREEN_GRAPHICS;
    if (mode != new_mode)
        if (c->SetMode (c, new_mode) != EFI_SUCCESS)
            return 0;

    return 1;
}

void efi_cls (void)
{
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *conout = efi_systab->ConOut;
    INT32 orig_attr = conout->Mode->Attribute;
    conout->SetAttribute (conout, EFI_BLACK);
    conout->ClearScreen (conout);
    conout->SetAttribute (conout, orig_attr);
}
