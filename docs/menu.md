## Getting Started

Extract `ntloader` and `initrd.cpio` onto the disk, and modify the bootloader menu to boot Windows from a FAT/NTFS/exFAT partition.  

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

In UEFI, the `chainloader` command should be used to load `ntloader` and pass the path to `initrd.cpio` to it.  
:warning: GRUB 2 cannot properly handle command lines containing non-ASCII characters, so file names must not include non-ASCII characters.  
:warning: `ntloader` and `initrd.cpio` must be on **the same ESP partition**.  

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

Use the `kernel` command to load `ntloader`.

```
title Boot Windows NT6+ WIM
find --set-root /path/to/winpe.wim
uuid ()
find --set-root /path/to/ntloader
kernel /path/to/ntloader uuid=%?_UUID% wim=/path/to/winpe.wim
initrd /path/to/initrd.cpio
```

### rEFInd

Use the `fsuuid.exe` tool to obtain the UUID of the partition where the WIM/VHD is located, and replace `PASTE_UUID_HERE` in the menu below with the obtained UUID.  
:warning: `ntloader` and `initrd.cpio` must be on **the same ESP partition**.  

```
menuentry "Boot Windows NT6+ VHD/VHDx" {
    loader /path/to/ntloader
    initrd /path/to/initrd.cpio
    options "uuid=PASTE_UUID_HERE vhd=/path/to/windows.vhd"
}
```

### limine

Use the `fsuuid.exe` tool to obtain the UUID of the partition where the WIM/VHD is located, and replace `PASTE_UUID_HERE` in the menu below with the obtained UUID.  
:warning: `ntloader` and `initrd.cpio` must be on **the same ESP partition**.  
:warning: Only UEFI is supported.

```
/Boot Windows NT6+ VHD/VHDx
    protocol: efi
    path: boot():/path/to/ntloader
    cmdline: uuid=PASTE_UUID_HERE vhd=/path/to/windows.vhd initrd=/path/to/initrd.cpio
```

<div style="page-break-after: always;"></div>

