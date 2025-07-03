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
#include "ntloader.h"
#include "cmdline.h"
#include "charset.h"
#include "efi.h"
#include "efifile.h"
#include "efiblock.h"
#include "efiboot.h"
#include "efidisk.h"
#include "efi/Protocol/LoadedImage.h"

/** SBAT section attributes */
#define __sbat __attribute__ ((section (".sbat"), aligned (512)))

/** SBAT metadata */
#define SBAT_CSV							\
	/* SBAT version */						\
	"sbat,1,SBAT Version,sbat,1,"					\
	"https://github.com/rhboot/shim/blob/main/SBAT.md"		\
	"\n"								\
	/* wimboot version */						\
	"wimboot," SBAT_GENERATION ",iPXE,wimboot," WIMBOOT_VERSION ","		\
	"https://ipxe.org/wimboot"					\
	"\n"								\
	/* ntloader version */						\
	"ntloader," SBAT_GENERATION ",a1ive,ntloader," VERSION ","		\
	"https://github.com/grub4dos/ntloader"		\
	"\n"

/** SBAT metadata (with no terminating NUL) */
const char sbat[ sizeof (SBAT_CSV) - 1 ] __sbat __nonstring = SBAT_CSV;

/**
 * Process command line
 *
 * @v loaded		Loaded image protocol
 */
static void efi_cmdline (EFI_LOADED_IMAGE_PROTOCOL *loaded)
{
    size_t cmdline_len = (loaded->LoadOptionsSize / sizeof (wchar_t));
    char *cmdline = efi_malloc (4 * cmdline_len + 1);

    /* Convert command line to ASCII */
    *ucs2_to_utf8 ((uint8_t *) cmdline,
                   loaded->LoadOptions, cmdline_len) = 0;

    /* Process command line */
    process_cmdline (cmdline);

    efi_free (cmdline);
}

/**
 * EFI entry point
 *
 * @v image_handle	Image handle
 * @v systab		EFI system table
 * @ret efirc		EFI status code
 */
EFI_STATUS EFIAPI efi_main (EFI_HANDLE image_handle,
                            EFI_SYSTEM_TABLE *systab)
{
    EFI_BOOT_SERVICES *bs;
    union
    {
        EFI_LOADED_IMAGE_PROTOCOL *image;
        void *interface;
    } loaded;
    EFI_HANDLE vdisk = NULL;
    EFI_HANDLE vpartition = NULL;
    EFI_STATUS efirc;

    /* Record EFI handle and system table */
    efi_image_handle = image_handle;
    efi_systab = systab;
    bs = systab->BootServices;

    /* Initialise stack cookie */
    init_cookie();

    efi_set_text_mode (1);

    /* Print welcome banner */
    printf ("\n\nntloader " VERSION "\n\n");

    /* Get loaded image protocol */
    efirc = bs->OpenProtocol (image_handle,
                              &efi_loaded_image_protocol_guid,
                              &loaded.interface,
                              image_handle, NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL);
    if (efirc != 0)
    {
        die ("Could not open loaded image protocol: %#lx\n",
              ((unsigned long) efirc));
    }

    /* Process command line */
    efi_cmdline (loaded.image);

    efidisk_init ();
    efidisk_iterate ();

    /* Extract files from file system */
    efi_extract (loaded.image->DeviceHandle);

    /* Install virtual disk */
    efi_install (&vdisk, &vpartition);

    /* Invoke boot manager */
    efi_boot (bootmgfw_path, vpartition);

    return 0;
}
