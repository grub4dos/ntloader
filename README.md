<br />
<div align="center">
  <h2 align="center">NTloader</h2>
  <div align="center">
    <img src="https://img.shields.io/github/license/grub4dos/ntloader">
  </div>
</div>
<br />

**Ntloader** is a bootloader for NT6+ WIM, VHD and VHDX images.

## Compatibility

**Ntloader** supports BIOS (i386) and UEFI (i386, x86_64, arm64) environments.  
The hybrid binary (`ntloader`) is compatible with BIOS and x86_64 UEFI. `ntloader.i386` is compatible with BIOS and i386 UEFI. `ntloader.arm64` is compatible with ARM64 UEFI.  
It is recommended to use GRUB2 (>=2.12) and GRUB4DOS to boot NTloader.  

## Download

The source code is maintained in a git repository at https://github.com/grub4dos/ntloader.  
You can download the latest version of the binary from [GitHub Releases](https://github.com/grub4dos/ntloader/releases).  

## Licence

NTloader is free, open-source software licensed under the [GNU GPLv3](https://www.gnu.org/licenses/gpl-3.0.txt).

<div style="page-break-after: always;"></div>

## Getting Started

Extract `ntloader` and `initrd.cpio` to the disk, and modify the bootloader menu to boot Windows from FAT/NTFS/exFAT partition.  

### GRUB 2 (>= 2.12)
```
menuentry "Boot Windows NT6+ WIM" {
    search -s --f /path/to/ntloader
    search -s dev --f /path/to/winpe.wim
    probe -s dev_uuid -u $dev
    if [ "${grub_platform}" = "efi" ]; then
        linux /path/to/ntloader uuid=${dev_uuid} wim=/path/to/winpe.wim
        initrd /path/to/initrd.cpio
    else
        linux16 /ntloader uuid=${dev_uuid} wim=/path/to/winpe.wim
        initrd16 /initrd.cpio
   fi;
}

menuentry "Boot Windows NT6+ VHD/VHDx" {
    search -s --f /path/to/ntloader
    search -s dev --f /path/to/windows.vhd
    probe -s dev_uuid -u $dev
    if [ "${grub_platform}" = "efi" ]; then
        linux /path/to/ntloader uuid=${dev_uuid} vhd=/path/to/windows.vhd
        initrd /path/to/initrd.cpio
    else
        linux16 /ntloader uuid=${dev_uuid} vhd=/path/to/windows.vhd
        initrd16 /initrd.cpio
   fi;
}

menuentry "Boot Windows NT6+ on (hdx,y)" {
    search -s --f /path/to/ntloader
    probe -s dev_uuid -u (hdx,y)
    if [ "${grub_platform}" = "efi" ]; then
        linux /path/to/ntloader uuid=${dev_uuid}
        initrd /path/to/initrd.cpio
    else
        linux16 /ntloader uuid=${dev_uuid}
        initrd16 /initrd.cpio
   fi;
}
```

<div style="page-break-after: always;"></div>

### GRUB 2 (< 2.12)

`ntloader` and `initrd.cpio` must be placed in **the same ESP partition**.  
In UEFI, the chainloader command should be used to load `ntloader` and pass the path of `initrd.cpio` to it.  

```
menuentry "Boot Windows NT6+ WIM" {
    search -s --f /path/to/ntloader
    search -s dev --f /path/to/winpe.wim
    probe -s dev_uuid -u $dev
    if [ "${grub_platform}" = "efi" ]; then
        chainloader /path/to/ntloader initrd=/path/to/initrd.cpio uuid=${dev_uuid} wim=/path/to/winpe.wim
    else
        linux16 /ntloader uuid=${dev_uuid} wim=/path/to/winpe.wim
        initrd16 /initrd.cpio
   fi;
}
```

### GRUB4DOS / GRUB4EFI

Use the kernel command to load `ntloader`.

```
title Boot Windows NT6+ WIM
find --set-root /path/to/winpe.wim
uuid ()
find --set-root /path/to/ntloader
kernel /path/to/ntloader uuid=%?_UUID% wim=/path/to/winpe.wim
initrd /path/to/initrd.cpio
```

### rEFInd

`ntloader` and `initrd.cpio` must be placed in **the same ESP partition**.  
Use the `fsuuid.exe` tool included in the archive to obtain the UUID of the partition where the WIM/VHD is located.  
For example, if the VHD path is `D:\path\to\windows.vhd`, run
```bat
fsuuid.exe D
```
to obtain the UUID of that partition, and replace `PASTE_UUID_HERE` in the menu below with the obtained UUID.  

```
menuentry "Boot Windows NT6+ VHD/VHDx" {
    loader /path/to/ntloader
    initrd /path/to/initrd.cpio
    options "uuid=PASTE_UUID_HERE vhd=/path/to/windows.vhd"
}
```

<div style="page-break-after: always;"></div>

## Credits

- [wimboot](https://ipxe.org/wimboot) -- Windows Imaging Format bootloader
- [GRUB 2](https://www.gnu.org/software/grub/) -- GNU GRUB
- [Quibble](https://github.com/maharmstone/quibble) -- the custom Windows bootloader
- [EDK2](https://github.com/tianocore/edk2) -- EDK II Project
