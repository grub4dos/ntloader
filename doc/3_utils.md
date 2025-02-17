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

### bmtool
`bmtool` is a program for extracting bootmgr.exe from bootmgr.  

### bcd.bat
`bcd.bat` is a batch script to create the BCD file.  
Do not edit it unless you know how NTloader works.  
:warning: Administrator privileges are required.  

<div style="page-break-after: always;"></div>
