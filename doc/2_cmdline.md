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
