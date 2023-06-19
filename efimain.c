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
 * EFI entry point
 *
 */

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ntboot.h>
#include <cmdline.h>
#include <lznt1.h>
#include <efi.h>
#include <efiblock.h>
#include <efidisk.h>
#include <bcd.h>
#include <charset.h>
#include <vdisk.h>
#include <cpio.h>
#include <efi/Protocol/BlockIo.h>
#include <efi/Protocol/DevicePath.h>
#include <efi/Protocol/GraphicsOutput.h>
#include <efi/Protocol/LoadedImage.h>
#include <efi/Protocol/LoadFile2.h>
#include <efi/Protocol/SimpleFileSystem.h>
#include <efi/Guid/LinuxEfiInitrdMedia.h>

/** SBAT section attributes */
#define __sbat __attribute__ ((section (".sbat"), aligned (512)))

/** SBAT metadata */
#define SBAT_CSV \
  /* SBAT version */ \
  "sbat,1,SBAT Version,sbat,1," \
  "https://github.com/rhboot/shim/blob/main/SBAT.md" \
  "\n" \
  /* wimboot version */ \
  "wimboot,1,iPXE,wimboot," WIMBOOT_VERSION "," \
  "https://ipxe.org/wimboot" \
  "\n" \
  /* ntloader version */ \
  "ntloader,1,a1ive,ntloader," VERSION "," \
  "https://github.com/grub4dos/ntloader" \
  "\n"

/** SBAT metadata (with no terminating NUL) */
const char sbat[sizeof (SBAT_CSV) - 1] __sbat = SBAT_CSV;

/** EFI system table */
EFI_SYSTEM_TABLE *efi_systab = 0;

/** EFI image handle */
EFI_HANDLE efi_image_handle = 0;

/** Block I/O protocol GUID */
EFI_GUID efi_block_io_protocol_guid
  = EFI_BLOCK_IO_PROTOCOL_GUID;

/** Device path protocol GUID */
EFI_GUID efi_device_path_protocol_guid
  = EFI_DEVICE_PATH_PROTOCOL_GUID;

/** Graphics output protocol GUID */
EFI_GUID efi_graphics_output_protocol_guid
  = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

/** Loaded image protocol GUID */
EFI_GUID efi_loaded_image_protocol_guid
  = EFI_LOADED_IMAGE_PROTOCOL_GUID;

/** Simple file system protocol GUID */
EFI_GUID efi_simple_file_system_protocol_guid
  = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

/** Load File 2 protocol GUID */
EFI_GUID efi_load_file2_protocol_guid
  = EFI_LOAD_FILE2_PROTOCOL_GUID;

static struct
{
  VENDOR_DEVICE_PATH vendor;
  EFI_DEVICE_PATH_PROTOCOL end;
} __attribute__ ((packed)) efi_initrd_path =
{
  {
    .Header = EFI_DEVPATH_INIT (efi_initrd_path.vendor,
                                MEDIA_DEVICE_PATH, MEDIA_VENDOR_DP),
    .Guid = LINUX_EFI_INITRD_MEDIA_GUID,
  },
  .end = EFI_DEVPATH_END_INIT (efi_initrd_path.end),
};

#if __x86_64__
  #define BOOT_FILE_NAME  "BOOTX64.EFI"
#elif __i386__
  #define BOOT_FILE_NAME  "BOOTIA32.EFI"
#else
  #error Unknown Processor Type
#endif

/** bootmgfw.efi file */
static struct vdisk_file *bootmgr;

/**
 * File handler
 *
 * @v name    File name
 * @v data    File data
 * @v len   Length
 * @ret rc    Return status code
 */
static int add_file (const char *name, void *data, size_t len)
{
  /* Check for special-case files */
  if ((strcasecmp (name, "bcdwin") == 0 && nt_cmdline->type == BOOT_WIN) ||
      (strcasecmp (name, "bcdwim") == 0 && nt_cmdline->type == BOOT_WIM) ||
      (strcasecmp (name, "bcdvhd") == 0 && nt_cmdline->type == BOOT_VHD))
  {
    DBG ("...found BCD file %s\n", name);
    vdisk_add_file ("BCD", data, len, read_mem_file);
    nt_cmdline->bcd_data = data;
    nt_cmdline->bcd_len = len;
    bcd_patch_data ();
    pause_boot ();
  }
  else if (strcasecmp (name, "bcd") == 0)
    DBG ("...skip BCD\n");
  else if (!(nt_cmdline->flag & NT_FLAG_WIN7) &&
           strcasecmp (name, BOOT_FILE_NAME) == 0)
  {
    DBG ("...found bootmgfw.efi file %s\n", name);
    bootmgr = vdisk_add_file (name, data, len, read_mem_file);
  }
  else if (nt_cmdline->flag & NT_FLAG_WIN7 &&
           strcasecmp (name, "win7.efi") == 0)
  {
    DBG ("..found win7 efi loader\n");
    bootmgr = vdisk_add_file ("bootx64.efi", data, len, read_mem_file);
  }
  else
    vdisk_add_file (name, data, len, read_mem_file);
  return 0;
}

static void
extract_initrd (void *initrd, uint32_t initrd_len)
{
  void *dst;
  ssize_t dst_len;
  size_t padded_len = 0;
  /* Extract files from initrd */
  if (initrd && initrd_len)
  {
    DBG ("initrd=%p+0x%x\n", initrd, initrd_len);
    dst_len = lznt1_decompress (initrd, initrd_len, NULL);
    if (dst_len < 0)
    {
      DBG ("...extracting initrd\n");
      cpio_extract (initrd, initrd_len, add_file);
    }
    else
    {
      DBG ("...extracting LZNT1-compressed initrd\n");
      dst = efi_allocate_pages (BYTES_TO_PAGES (dst_len));
      lznt1_decompress (initrd, initrd_len, dst);
      cpio_extract (dst, dst_len, add_file);
      initrd_len += padded_len;
      initrd = dst;
    }
  }
}

static void
efi_load_sfs_initrd (EFI_HANDLE handle, void **initrd, size_t *initrd_len)
{
  EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
  EFI_FILE_PROTOCOL *root;
  EFI_FILE_PROTOCOL *file;
  UINT64 size;
  CHAR16 wname[256];
  EFI_STATUS efirc;

  /* Open file system */
  efirc = bs->OpenProtocol (handle, &efi_simple_file_system_protocol_guid,
                            (void *)&fs, efi_image_handle, NULL,
                            EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (efirc != EFI_SUCCESS)
    die ("Could not open simple file system.\n");

  /* Open root directory */
  efirc = fs->OpenVolume (fs, &root);
  if (efirc != EFI_SUCCESS)
    die ("Could not open root directory.\n");

  /* Close file system */
  bs->CloseProtocol (handle, &efi_simple_file_system_protocol_guid,
                     efi_image_handle, NULL);
  memset (wname, 0, sizeof (wname));
  utf8_to_ucs2 (wname, MAX_PATH, (uint8_t *)nt_cmdline->initrd_path);
  efirc = root->Open (root, &file, wname, EFI_FILE_MODE_READ, 0);
  if (efirc != EFI_SUCCESS)
    die ("Could not open %ls.\n", wname);
  file->SetPosition (file, 0xFFFFFFFFFFFFFFFF);
  file->GetPosition (file, &size);
  file->SetPosition (file, 0);
  if (!size)
    die ("Could not get initrd size\n");
  *initrd_len = size;
  *initrd = efi_allocate_pages (BYTES_TO_PAGES (*initrd_len));
  efirc = file->Read (file, (UINTN *)initrd_len, *initrd);
  if (efirc != EFI_SUCCESS)
    die ("Could not read from file.\n");
}

static void
efi_load_lf2_initrd (void **initrd, size_t *initrd_len)
{
  EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
  EFI_HANDLE lf2_handle;
  EFI_LOAD_FILE2_PROTOCOL *lf2;
  EFI_DEVICE_PATH_PROTOCOL *dp = (EFI_DEVICE_PATH_PROTOCOL *) &efi_initrd_path;
  UINTN size = 0;
  EFI_STATUS efirc;

  /* Locate initrd media device */
  efirc = bs->LocateDevicePath (&efi_load_file2_protocol_guid, &dp, &lf2_handle);
  if (efirc != EFI_SUCCESS)
    die ("Could not locate initrd media.\n");

  /* Get LoadFile2 protocol */
  efirc = bs->HandleProtocol (lf2_handle, &efi_load_file2_protocol_guid,
                              (void **)&lf2);
  if (efirc != EFI_SUCCESS)
    die ("Could not get LoadFile2 protocol.\n");

  /* Get initrd size */
  efirc = lf2->LoadFile (lf2, dp, FALSE, &size, NULL);
  if (size == 0)
    die ("Could not get initrd size\n");

  *initrd_len = size;
  *initrd = efi_allocate_pages (BYTES_TO_PAGES (*initrd_len));
  efirc = lf2->LoadFile (lf2, dp, FALSE, (UINTN *)initrd_len, *initrd);
  if (efirc != EFI_SUCCESS)
    die ("Could not read initrd.\n");
}

EFI_STATUS EFIAPI efi_main (EFI_HANDLE image_handle,EFI_SYSTEM_TABLE *systab)
{
  EFI_BOOT_SERVICES *bs;
  EFI_LOADED_IMAGE_PROTOCOL *loaded;
  EFI_STATUS efirc;
  size_t cmdline_len = 0;
  char *cmdline;
  void *initrd = NULL;
  size_t initrd_len = 0;

  efi_image_handle = image_handle;
  efi_systab = systab;
  bs = systab->BootServices;

  /* Initialise stack cookie */
  init_cookie ();

  /* Print welcome banner */
  cls ();
  print_banner ();
  /* Get loaded image protocol */
  efirc = bs->OpenProtocol (image_handle, &efi_loaded_image_protocol_guid,
                            (void **)&loaded, image_handle, NULL,
                            EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (efirc != EFI_SUCCESS)
    die ("Could not open loaded image protocol\n");

  cmdline_len = (loaded->LoadOptionsSize / sizeof (wchar_t));
  cmdline = efi_malloc (4 * cmdline_len + 1);

  /* Convert command line to ASCII */
  *ucs2_to_utf8 ((uint8_t *) cmdline, loaded->LoadOptions, cmdline_len) = 0;

  /* Process command line */
  process_cmdline (cmdline);
  DBG ("systab=%p image_handle=%p\n", systab, image_handle);
  if (nt_cmdline->initrd_path)
    efi_load_sfs_initrd (loaded->DeviceHandle, &initrd, &initrd_len);
  else
    efi_load_lf2_initrd (&initrd, &initrd_len);

  efidisk_init ();
  efidisk_iterate ();

  pause_boot ();

  extract_initrd (initrd, initrd_len);

  efi_free (cmdline);
  if (! bootmgr)
    die ("FATAL: no bootmgfw.efi\n");

  /* Install virtual disk */
  efi_install ();
  /* Invoke boot manager */
  efi_boot (bootmgr);

  return EFI_SUCCESS;
}
