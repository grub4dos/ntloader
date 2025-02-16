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

<div style="page-break-after: always;"></div>

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

`ntloader` and `initrd.cpio` must be on **the same ESP partition**.  
In UEFI, the `chainloader` command should be used to load `ntloader` and pass the path to `initrd.cpio` to it.  
:warning: GRUB 2 cannot properly handle command lines containing non-ASCII characters, so file names must not include non-ASCII characters.  

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

`ntloader` and `initrd.cpio` must be on **the same ESP partition**.  
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

## Kernel parameters

The maximum size of the command line without the terminating zero is 4096 bytes.  
Boolean values can be expressed as `yes` and `no`, `true` and `false`, `on` and `off`, or `1` and `0`.  
Spaces in file paths should be replaced with `:`.  
Slashes in file paths are automatically converted to backslashes.  

### uuid
```
uuid=XXXXXXXX
```
The filesystem UUID of the partition where the WIM/VHD is located.  

### initrd
```
initrd=/path/to/initrd.cpio
```
The path to the initrd file when booting with GRUB2 (<2.12) under UEFI.  
Must be in the same ESP partition with `ntloader`.  

### wim
```
wim=/path/to/winpe.wim
```
The path to the WIM file.  

### vhd
```
vhd=/path/to/windows.vhd
```
The path to the VHD/VHDX file.  

### ram
```
ram=/path/to/ramos.vhd
```
The path to the VHD/VHDX file for RAMDISK boot.  

### file
```
file=/path/to/xxx.wim
file=/path/to/xxx.vhd
```
The path to the WIM/VHD/VHDX file. Bootloader will automatically detect the file type.  

### testmode
```
testmode=yes|no
```
Boolean value, enable or disable test signing mode. Default is `no`.  

### hires
```
hires=yes|no
```
Boolean value, enable or disable forced highest resolution.  
Default is "yes" for WIM files; otherwise, default is "no".  

### detecthal
```
detecthal=yes|no
```
Boolean value, enable or disable HAL detection. Default is `yes`.  

### minint
```
minint=yes|no
```
Boolean value, enable or disable WINPE mode.  
Default is `yes` for WIM files; otherwise, default is `no`.  

### novga
```
novga=yes|no
```
Boolean value, enable or disable the use of VGA modes entirely. Default is `no`.  

### novesa
```
novesa=yes|no
```
Boolean value, enable or disable the use of VESA modes entirely. Default is `no`.  

### altshell
```
altshell=yes|no
```
Boolean value, enable or disable the use of the alternative shell in safe mode. Default is `no`.  

### exportascd
```
exportascd=yes|no
```
Boolean value, enable or disable treating the RAMDISK as an ISO in RAMDISK boot mode. Default is `no`.  

### nx
```
nx=OptIn|OptOut|AlwaysOff|AlwaysOn
```
Set the Data Execution Prevention (DEP) policy. Default is `OptIn`.  

### pae
```
pae=Default|Enable|Disable
```
Enables or disables Physical Address Extension (PAE). Default is `Default`.

### safemode
```
safemode=Minimal|Network|DsRepair
```
Boot into safe mode if this parameter is set.  

### gfxmode
```
gfxmode=1024x768|800x600|1024x600
```
Set the graphics resolution.  

### imgofs
```
imgofs=XXXXXX
```
Set the partition offset of the VHD/VHDX file in RAMDISK boot mode. Default is `65536`.  

### loadopt
```
loadopt=XXXXXX
```
Set the load options for Windows.  
For available options, see [Boot Options](https://www.geoffchappell.com/notes/windows/boot/editoptions.htm).  

### winload
```
winload=XXXXXX
```
Set the path to `winload.efi` or `winload.exe`.  
Default is `/Windows/System32/Boot/winload.efi` for WIM; otherwise, default is `/Windows/System32/winload.efi`.  
Ntloader will automatically correct the file extension.  

### sysroot
```
sysroot=XXXXXX
```
Set the path to the system root directory. Default is `/Windows`.  

<div style="page-break-after: always;"></div>

## Utilities

### fsuuid
`fsuuid` is a tool to obtain the UUID of the partition where the WIM/VHD is located.  
Useful for rEFInd and other bootloaders that do not support retrieving the filesystem UUID.  
:warning: Administrator privileges are required.  
```
# Get the UUID by drive letter (Windows)
fsuuid.exe D
# Get the UUID by volume path (Windows)
fsuuid.exe \\?\Volume{XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX}
# List all volume paths (Windows)
fsuuid.exe -l
# Get the UUID by device path (Linux)
fsuuid /dev/sdxY
```

### mkinitrd
`mkinitrd` is a tool to create the `initrd.cpio` file.  
You can use it to create a custom initrd file using bootmgr from other Windows versions.  
You can also use `cpio` to create the initrd file.  
```
# Create initrd.cpio with files from rootfs directory
mkinitrd.exe rootfs initrd.cpio
# Create initrd.cpio using linux utilities 'find' and 'cpio'
find * | cpio -o -H newc > ../initrd.cpio
```

### bcd.bat
`bcd.bat` is a batch script to create the BCD file.  
Do not edit it unless you know how NTloader works.  
:warning: Administrator privileges are required.  

<div style="page-break-after: always;"></div>

## Compilation

gcc, gcc-mingw-w64 and gcc-aarch64-linux-gnu are required to compile NTloader.  

### Compile NTloader
```
# Build everything
make
# Build arm64 binary only
make ntloader.arm64
```

### Compile fsuuid
```
make fsuuid
make fsuuid.exe
```

### Compile mkinitrd
```
make mkinitrd
make mkinitrd.exe
```

### Build initrd.cpio
```
make initrd.cpio
```

<div style="page-break-after: always;"></div>

## Licence

NTloader is free, open-source software licensed under the [GNU GPLv3](https://www.gnu.org/licenses/gpl-3.0.txt).  

## Credits

- [wimboot](https://ipxe.org/wimboot) -- Windows Imaging Format bootloader
- [GRUB 2](https://www.gnu.org/software/grub/) -- GNU GRUB
- [Quibble](https://github.com/maharmstone/quibble) -- the custom Windows bootloader
- [EDK2](https://github.com/tianocore/edk2) -- EDK II Project
