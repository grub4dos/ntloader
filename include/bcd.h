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

#ifndef _BCD_BOOT_H
#define _BCD_BOOT_H 1

#include <ntloader.h>
#include <stdint.h>

#define GUID_WIMB L"{19260817-6666-8888-abcd-000000000000}"
#define GUID_VHDB L"{19260817-6666-8888-abcd-000000000001}"
#define GUID_WOSB L"{19260817-6666-8888-abcd-000000000002}"
#define GUID_HIBR L"{19260817-6666-8888-abcd-000000000003}"

#define GUID_OPTN L"{19260817-6666-8888-abcd-100000000000}"

#define GUID_BOOTMGR L"{9dea862c-5cdd-4e70-acc1-f32b344d4795}"
#define GUID_RAMDISK L"{ae5534e0-a924-466c-b836-758539a3ee3a}"
#define GUID_MEMDIAG L"{b2721d73-1db4-4c62-bf78-c548a880142d}"
#define GUID_OSNTLDR L"{466f5a88-0af2-4f76-9038-095b170dc21c}"
#define GUID_RELDRST L"{1afa9c49-16ab-4a5c-901b-212802da9460}"
#define GUID_BTLDRST L"{6efb52bf-1766-41db-a6b3-0ee5eff72bd7}"

#define GUID_BIN_RAMDISK \
    { \
        0xe0, 0x34, 0x55, 0xae, 0x24, 0xa9, 0x6c, 0x46, \
        0xb8, 0x36, 0x75, 0x85, 0x39, 0xa3, 0xee, 0x3a \
    }

#define BCD_REG_ROOT L"Objects"
#define BCD_REG_HKEY L"Elements"
#define BCD_REG_HVAL L"Element"

#define BCDOPT_APPDEV   L"11000001" // application device
#define BCDOPT_WINLOAD  L"12000002" // winload & winresume
#define BCDOPT_TITLE    L"12000004"
#define BCDOPT_LANG     L"12000005"
#define BCDOPT_CMDLINE  L"12000030"
#define BCDOPT_INHERIT  L"14000006" // options & safeboot
#define BCDOPT_GFXMODE  L"15000052" // graphicsresolution
#define BCDOPT_TESTMODE L"16000049" // testsigning
#define BCDOPT_HIGHRES  L"16000054" // highest resolution
#define BCDOPT_OSDDEV   L"21000001" // os device
#define BCDOPT_SYSROOT  L"22000002" // os root & hiberfil
#define BCDOPT_OBJECT   L"23000003" // resume / default object
#define BCDOPT_ORDER    L"24000001" // menu display order
#define BCDOPT_TIMEOUT  L"25000004" // menu timeout
#define BCDOPT_NX       L"25000020"
#define BCDOPT_PAE      L"25000021"
#define BCDOPT_SAFEMODE L"25000080" // safeboot
#define BCDOPT_DETHAL   L"26000010" // detect hal
#define BCDOPT_DISPLAY  L"26000020" // menu display ui
#define BCDOPT_WINPE    L"26000022" // minint winpe moe
#define BCDOPT_NOVESA   L"26000042"
#define BCDOPT_NOVGA    L"26000043"
#define BCDOPT_ALTSHELL L"26000081" // safeboot alternate shell
#define BCDOPT_SOS      L"26000091"
#define BCDOPT_SDIDEV   L"31000003" // sdi ramdisk device
#define BCDOPT_SDIPATH  L"32000004" // sdi ramdisk path
#define BCDOPT_IMGOFS   L"35000001" // ramdisk image offset
#define BCDOPT_EXPORTCD L"36000006" // ramdisk export as cd

#define NX_OPTIN     0x00
#define NX_OPTOUT    0x01
#define NX_ALWAYSOFF 0x02
#define NX_ALWAYSON  0x03

#define PAE_DEFAULT  0x00
#define PAE_ENABLE   0x01
#define PAE_DISABLE  0x02

#define SAFE_MINIMAL  0x00
#define SAFE_NETWORK  0x01
#define SAFE_DSREPAIR 0x02

#define GFXMODE_1024X768 0x00
#define GFXMODE_800X600  0x01
#define GFXMODE_1024X600 0x02

#define BCD_DEFAULT_CMDLINE "/DISABLE_INTEGRITY_CHECKS"

#define BCD_LONG_WINLOAD "\\Windows\\System32\\boot\\winload.efi"

#define BCD_SHORT_WINLOAD "\\Windows\\System32\\winload.efi"

#define BCD_DEFAULT_WINRESUME "\\Windows\\System32\\winresume.efi"

#define BCD_DEFAULT_SYSROOT "\\Windows"

#define BCD_DEFAULT_HIBERFIL "\\hiberfil.sys"

struct bcd_disk_info
{
  uint32_t infotype; // 5-BOOT, 6-PART
  uint32_t pad1;
  uint32_t infolen; // 0x48
  uint32_t pad2;
  uint8_t partid[16];
  uint32_t pad3;
  uint32_t partmap; // 0-GPT, 1-MBR
  uint8_t diskid[16];
  uint8_t pad4[16];
} __attribute__ ((packed));

extern void bcd_patch_data (void);

#endif
