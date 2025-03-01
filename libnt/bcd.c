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
#include "ntloader.h"
#include "reg.h"
#include "bcd.h"
#include "cmdline.h"
#include "charset.h"
#include "efi.h"

static void
bcd_replace_suffix (const wchar_t *src, const wchar_t *dst)
{
    uint8_t *p = nt_cmdline->bcd;
    uint32_t ofs;
    const size_t len = sizeof(wchar_t) * 5; // . E F I \0
    for (ofs = 0; ofs + len < nt_cmdline->bcd_length; ofs++)
    {
        if (memcmp (p + ofs, src, len) == 0)
        {
            memcpy (p + ofs, dst, len);
            DBG ("...patched BCD at %#x (%ls->%ls)\n", ofs, src, dst);
        }
    }
}

static void *
bcd_find_hive (hive_t *hive, HKEY objects,
               const wchar_t *guid, const wchar_t *keyname,
               uint32_t *len)
{
    HKEY entry, elements, key;
    void *data = NULL;
    uint32_t type;

    if (reg_find_key (hive, objects, guid, &entry) != REG_ERR_NONE)
        die ("Can't find HKEY %ls\n", guid);
    if (reg_find_key (hive, entry, BCD_REG_HKEY, &elements)
        != REG_ERR_NONE)
        die ("Can't find HKEY %ls\n", BCD_REG_HKEY);
    if (reg_find_key (hive, elements, keyname, &key)
        != REG_ERR_NONE)
        die ("Can't find HKEY %ls\n", keyname);
    if (reg_query_value (hive, key, BCD_REG_HVAL,
        (void **)&data, len, &type) != REG_ERR_NONE)
        die ("Can't find HVAL %ls\n", BCD_REG_HVAL);
    return data;
}

static void
bcd_delete_key (hive_t *hive, HKEY objects,
                const wchar_t *guid, const wchar_t *keyname)
{
    HKEY entry, elements, key;
    if (reg_find_key (hive, objects, guid, &entry) != REG_ERR_NONE)
        die ("Can't find HKEY %ls\n", guid);
    if (reg_find_key (hive, entry, BCD_REG_HKEY, &elements)
        != REG_ERR_NONE)
        die ("Can't find HKEY %ls\n", BCD_REG_HKEY);
    if (reg_find_key (hive, elements, keyname, &key)
        != REG_ERR_NONE)
        die ("Can't find HKEY %ls\n", keyname);
    if (reg_delete_key (hive, elements, key)
        != REG_ERR_NONE)
        die ("Can't delete HKEY %ls\n", keyname);
}

static inline void
bcd_patch_bool (hive_t *hive, HKEY objects,
                const wchar_t *guid, const wchar_t *keyname,
                uint8_t val)
{
    uint8_t *data;
    uint32_t len;
    data = bcd_find_hive (hive, objects, guid, keyname, &len);
    if (len != sizeof (uint8_t))
        die ("Invalid bool size %x\n", len);
    memcpy (data, &val, sizeof (uint8_t));
    DBG ("...patched %ls->%ls (%c)\n", guid, keyname, val ? 'y' : 'n');
}

static inline void
bcd_patch_u64 (hive_t *hive, HKEY objects,
               const wchar_t *guid, const wchar_t *keyname,
               uint64_t val)
{
    uint8_t *data;
    uint32_t len;
    data = bcd_find_hive (hive, objects, guid, keyname, &len);
    if (len != sizeof (uint64_t))
        die ("Invalid u64 size %x\n", len);
    memcpy (data, &val, sizeof (uint64_t));
    DBG ("...patched %ls->%ls (%llx)\n", guid, keyname, val);
}

static inline void
bcd_patch_sz (hive_t *hive, HKEY objects,
              const wchar_t *guid, const wchar_t *keyname,
              const char *str)
{
    uint16_t *data;
    uint32_t len;
    data = bcd_find_hive (hive, objects, guid, keyname, &len);
    memset (data, 0, len);
    utf8_to_ucs2 (data, len / sizeof (wchar_t), (uint8_t *) str);
    DBG ("...patched %ls->%ls (%ls)\n",
         guid, keyname, (wchar_t *) data);
}

static inline void
bcd_patch_szw (hive_t *hive, HKEY objects,
               const wchar_t *guid, const wchar_t *keyname,
               const wchar_t *str)
{
    uint16_t *data;
    uint32_t len;
    size_t wlen = wcslen (str) * sizeof (wchar_t);
    data = bcd_find_hive (hive, objects, guid, keyname, &len);
    if (wlen > len)
        die ("Invalid wchar size %zu\n", wlen);
    memset (data, 0, len);
    memcpy (data, str, wlen);
    DBG ("...patched %ls->%ls (%ls)\n", guid, keyname, str);
}

static inline void
bcd_patch_dp (hive_t *hive, HKEY objects, uint32_t boottype,
              const wchar_t *guid, const wchar_t *keyname)
{
    uint8_t *data;
    uint32_t len, ofs;
    uint8_t sdi[] = GUID_BIN_RAMDISK;
    data = bcd_find_hive (hive, objects, guid, keyname, &len);
    memset (data, 0, len);
    switch (boottype)
    {
        case NTBOOT_WIM:
        case NTBOOT_RAM:
        {
            if (len < 0x028a)
                die ("WIM device path (%ls->%ls) length error (%x)\n",
                     guid, keyname, len);
            memcpy (data + 0x0000, sdi, sizeof (sdi));
            data[0x0014] = 0x01;
            data[0x0018] = 0x7a; data[0x0019] = 0x02; // len - 0x10
            data[0x0020] = 0x03;
            data[0x0038] = 0x01;
            data[0x003c] = 0x52; data[0x003d] = 0x02; // len - 0x38
            data[0x0040] = 0x05;
            ofs = 0x0044;
            break;
        }
        case NTBOOT_VHD:
        {
            if (len < 0x02bc)
                die ("VHD device path (%ls->%ls) length error (%x)\n",
                     guid, keyname, len);
            data[0x0010] = 0x08;
            data[0x0018] = 0xac; data[0x0019] = 0x02; // len - 0x10
            data[0x0024] = 0x02;
            data[0x0027] = 0x12;
            data[0x0028] = 0x1e;
            data[0x0036] = 0x8e; data[0x0037] = 0x02; // len - 0x2e
            data[0x003e] = 0x06;
            data[0x005e] = 0x66; data[0x005f] = 0x02; // len - 0x56
            data[0x0066] = 0x05;
            data[0x006a] = 0x01;
            data[0x006e] = 0x52; data[0x006f] = 0x02; // len - 0x6a
            data[0x0072] = 0x05;
            ofs = 0x0076;
            break;
        }
        case NTBOOT_WOS:
        case NTBOOT_REC:
        {
            if (len < 0x0058)
                die ("OS device path (%ls->%ls) length error (%x)\n",
                     guid, keyname, len);
            ofs = 0x0010;
            break;
        }
        default:
            die ("Unsupported boot type %x\n", boottype);
    }

    /* os device */
    if (nt_cmdline->fsuuid[0])
        data[ofs + 0x00] = 0x06; // 05=boot, 06=disk
    else
        data[ofs + 0x00] = 0x05;
    data[ofs + 0x08] = 0x48;
    memcpy (data + ofs + 0x10, nt_cmdline->partid, 16);
    data[ofs + 0x24] = nt_cmdline->partmap;
    memcpy (data + ofs + 0x28, nt_cmdline->diskid, 16);
    if (boottype == NTBOOT_WIM ||
        boottype == NTBOOT_VHD ||
        boottype == NTBOOT_RAM)
        utf8_to_ucs2 ((uint16_t *)(data + ofs + 0x48), MAX_PATH,
                      (uint8_t *)nt_cmdline->filepath);
    DBG ("...patched %ls->%ls (device%x)\n", guid, keyname, boottype);
}

void
bcd_patch_data (void)
{
    const wchar_t *entry_guid;
    HKEY root, objects;
    hive_t hive =
    {
        .size = nt_cmdline->bcd_length,
        .data = nt_cmdline->bcd,
    };

    /* Open BCD hive */
    if (reg_open_hive (&hive) != REG_ERR_NONE)
        die ("BCD hive load error.\n");
    reg_find_root (&hive, &root);
    if (reg_find_key (&hive, root, BCD_REG_ROOT, &objects)
        != REG_ERR_NONE)
        die ("Can't find HKEY %ls\n", BCD_REG_ROOT);
    DBG ("BCD hive load OK.\n");

    /* Check entry type */
    switch (nt_cmdline->boottype)
    {
        case NTBOOT_VHD:
            entry_guid = GUID_VHDB;
            break;
        case NTBOOT_WOS:
            entry_guid = GUID_WOSB;
            break;
        case NTBOOT_WIM:
        case NTBOOT_RAM:
        default:
            entry_guid = GUID_WIMB;
    }

    /* Patch Objects->{BootMgr} */
    bcd_patch_szw (&hive, objects, GUID_BOOTMGR,
                   BCDOPT_OBJECT, entry_guid); // default object
    bcd_patch_szw (&hive, objects, GUID_BOOTMGR,
                   BCDOPT_ORDER, entry_guid); // menu display order
    bcd_patch_u64 (&hive, objects, GUID_BOOTMGR,
                   BCDOPT_TIMEOUT, nt_cmdline->timeout); // timeout

    /* Patch Objects->{Resume} */
    bcd_patch_dp (&hive, objects, NTBOOT_REC,
                  GUID_HIBR, BCDOPT_APPDEV); // app device
    bcd_patch_dp (&hive, objects, NTBOOT_REC,
                  GUID_HIBR, BCDOPT_OSDDEV); // os device
    bcd_patch_sz (&hive, objects, GUID_HIBR,
                  BCDOPT_WINLOAD, BCD_DEFAULT_WINRESUME); // resume
    bcd_patch_sz (&hive, objects, GUID_HIBR,
                  BCDOPT_SYSROOT, BCD_DEFAULT_HIBERFIL); // hiberfil

    /* Patch Objects->{Ramdisk} */
    if (nt_cmdline->boottype == NTBOOT_RAM)
    {
        bcd_delete_key (&hive, objects, GUID_RAMDISK, BCDOPT_SDIDEV);
        bcd_delete_key (&hive, objects, GUID_RAMDISK, BCDOPT_SDIPATH);
        bcd_patch_bool (&hive, objects, GUID_RAMDISK,
                        BCDOPT_EXPORTCD, nt_cmdline->exportcd);
        bcd_patch_u64 (&hive, objects, GUID_RAMDISK,
                       BCDOPT_IMGOFS, nt_cmdline->imgofs);
    }
    else
    {
        bcd_delete_key (&hive, objects, GUID_RAMDISK, BCDOPT_EXPORTCD);
        bcd_delete_key (&hive, objects, GUID_RAMDISK, BCDOPT_IMGOFS);
    }

    /* Patch Objects->{Options} */
    bcd_patch_sz (&hive, objects, GUID_OPTN,
                  BCDOPT_CMDLINE, nt_cmdline->loadopt);
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_TESTMODE, nt_cmdline->testmode);
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_DETHAL, nt_cmdline->hal);
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_WINPE, nt_cmdline->minint);
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_NOVGA, nt_cmdline->novga);
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_NOVESA, nt_cmdline->novesa);
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_ADVOPT, nt_cmdline->advmenu);
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_OPTEDIT, nt_cmdline->optedit);
    bcd_patch_bool (&hive, objects, GUID_OPTN,
                    BCDOPT_TEXT, nt_cmdline->textmode);
    bcd_patch_u64 (&hive, objects, GUID_OPTN,
                   BCDOPT_NX, nt_cmdline->nx);
    bcd_patch_u64 (&hive, objects, GUID_OPTN,
                   BCDOPT_PAE, nt_cmdline->pae);

    /* Patch Objects->{Resolution} */
    if (nt_cmdline->hires == NTARG_BOOL_NA)
    {
        bcd_delete_key (&hive, objects, GUID_OPTN, BCDOPT_HIGHRES);
        bcd_patch_u64 (&hive, objects, GUID_OPTN,
                       BCDOPT_GFXMODE, nt_cmdline->gfxmode);
    }
    else
    {
        bcd_delete_key (&hive, objects, GUID_OPTN, BCDOPT_GFXMODE);
        bcd_patch_bool (&hive, objects, GUID_OPTN,
                        BCDOPT_HIGHRES, nt_cmdline->hires);
    }

    if (nt_cmdline->safemode)
    {
        bcd_patch_u64 (&hive, objects, GUID_OPTN,
                       BCDOPT_SAFEMODE, nt_cmdline->safeboot);
        bcd_patch_bool (&hive, objects, GUID_OPTN,
                        BCDOPT_ALTSHELL, nt_cmdline->altshell);
    }
    else
    {
        bcd_delete_key (&hive, objects, GUID_OPTN, BCDOPT_SAFEMODE);
        bcd_delete_key (&hive, objects, GUID_OPTN, BCDOPT_ALTSHELL);
    }

    /* Patch Objects->{Entry} */
    bcd_patch_dp (&hive, objects, nt_cmdline->boottype,
                  entry_guid, BCDOPT_APPDEV); // app device
    bcd_patch_dp (&hive, objects, nt_cmdline->boottype,
                  entry_guid, BCDOPT_OSDDEV); // os device
    bcd_patch_sz (&hive, objects, entry_guid,
                  BCDOPT_WINLOAD, nt_cmdline->winload);
    bcd_patch_sz (&hive, objects, entry_guid,
                  BCDOPT_SYSROOT, nt_cmdline->sysroot);

    if (efi_systab)
        bcd_replace_suffix (L".exe", L".efi");
    else
        bcd_replace_suffix (L".efi", L".exe");
}
