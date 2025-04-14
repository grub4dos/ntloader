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

void efi_cls (void)
{
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *conout = efi_systab->ConOut;
    INT32 orig_attr = conout->Mode->Attribute;
    conout->SetAttribute (conout, EFI_BLACK);
    conout->ClearScreen (conout);
    conout->SetAttribute (conout, orig_attr);
}
