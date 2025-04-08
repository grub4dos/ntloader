# NTloader
![](https://img.shields.io/github/license/grub4dos/ntloader) ![](https://img.shields.io/github/stars/grub4dos/ntloader?style=flat) ![](https://img.shields.io/github/downloads/grub4dos/ntloader/total)

**Ntloader** is a bootloader for NT6+ WIM, VHD and VHDX images.

## Compatibility

**Ntloader** supports BIOS (i386) and UEFI (i386, x86_64, arm64) environments.  
It is recommended to use GRUB2 (>=2.12) or GRUB4DOS to boot NTloader.  

## Download

The source code is maintained in a git repository at https://github.com/grub4dos/ntloader.  
You can download the latest version of the binary from [GitHub Releases](https://github.com/grub4dos/ntloader/releases).  

## Getting Started

Extract `ntloader` and `initrd.cpio` onto the disk, and modify the bootloader menu to boot Windows from a FAT/NTFS/exFAT partition.  

```
# GRUB 2 (>= 2.12)
menuentry "Boot Windows NT6+ WIM" {
    search -s -f /path/to/ntloader
    search -s dev -f /path/to/winpe.wim
    probe -s dev_uuid -u $dev
    if [ "${grub_platform}" = "efi" ]; then
        linux /path/to/ntloader uuid=${dev_uuid} wim=/path/to/winpe.wim
        initrd /path/to/initrd.cpio
    else
        linux16 /path/to/ntloader uuid=${dev_uuid} wim=/path/to/winpe.wim
        initrd16 /path/to/initrd.cpio
   fi;
}

menuentry "Boot Windows NT6+ VHD/VHDx" {
    search -s -f /path/to/ntloader
    search -s dev -f /path/to/windows.vhd
    probe -s dev_uuid -u $dev
    if [ "${grub_platform}" = "efi" ]; then
        linux /path/to/ntloader uuid=${dev_uuid} vhd=/path/to/windows.vhd
        initrd /path/to/initrd.cpio
    else
        linux16 /path/to/ntloader uuid=${dev_uuid} vhd=/path/to/windows.vhd
        initrd16 /path/to/initrd.cpio
   fi;
}

menuentry "Boot Windows NT6+ on (hdx,y)" {
    search -s -f /path/to/ntloader
    probe -s dev_uuid -u (hdx,y)
    if [ "${grub_platform}" = "efi" ]; then
        linux /path/to/ntloader uuid=${dev_uuid}
        initrd /path/to/initrd.cpio
    else
        linux16 /path/to/ntloader uuid=${dev_uuid}
        initrd16 /path/to/initrd.cpio
   fi;
}
```

## Credits

- [wimboot](https://ipxe.org/wimboot) -- Windows Imaging Format bootloader
- [GRUB 2](https://www.gnu.org/software/grub/) -- GNU GRUB
- [Quibble](https://github.com/maharmstone/quibble) -- the custom Windows bootloader
- [EDK2](https://github.com/tianocore/edk2) -- EDK II Project
