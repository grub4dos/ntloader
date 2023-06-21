```
          ___       ___          ___   ___          ___     
         /\__\     /\  \        /\__\ /\  \        /\  \    
        /::|  |    \:\  \      /:/  //::\  \      /::\  \   
       /:|:|  |     \:\  \    /:/  //:/\:\  \    /:/\:\  \  
      /:/|:|  |__   /::\  \  /:/  //:/  \:\__\  /::\~\:\  \ 
     /:/ |:| /\__\ /:/\:\__\/:/__//:/__/ \:|__|/:/\:\ \:\__\
     \/__|:|/:/  //:/  \/__/\:\  \\:\  \ /:/  /\/_|::\/:/  /
         |:/:/  //:/  /      \:\  \\:\  /:/  /    |:|::/  / 
         |::/  / \/__/        \:\  \\:\/:/  /     |:|\/__/  
         /:/  /                \:\__\\::/__/      |:|  |    
         \/__/                  \/__/ ~~           \|__|    
        NTloader -- Windows NT6+ OS/VHD/WIM loader
                Copyright (C) 2023 A1ive.
```

## Usage

### Boot Windows NT6+ WIM/VHD/VHDX

- GRUB4DOS
```
title Boot Windows NT6+ PE
uuid (hdx,y)
kernel /ntloader uuid=%?_UUID% file=/path/to/winpe.wim
initrd /initrd.cpio
```

- GNU GRUB2
```
menuentry "Boot Windows NT6+ PE" {
    probe -s dev_uuid -u (hdx,y);
    if [ "${grub_platform}" = "efi" ];
    then
        chainloader /ntloader initrd=/initrd.cpio uuid=${dev_uuid} file=/path/to/winpe.wim;
    else
        linux16 /ntloader uuid=${dev_uuid} file=/path/to/winpe.wim;
        initrd16 /initrd.cpio;
   fi;
}
```

### Boot Windows NT6+ OS

- GRUB4DOS
```
title Boot Windows NT6+
uuid (hdx,y)
kernel /ntloader uuid=%?_UUID%
initrd /initrd.cpio
```

- GNU GRUB2
```
menuentry "Boot Windows NT6+" {
    probe -s dev_uuid -u (hdx,y);
    if [ "${grub_platform}" = "efi" ];
    then
        chainloader /ntloader initrd=/initrd.cpio uuid=${dev_uuid};
    else
        linux16 /ntloader uuid=${dev_uuid};
        initrd16 /initrd.cpio;
   fi;
}
```

### Advanced options

- Enable *Test Signing* mode
```
kernel /ntloader uuid=%?% file=/xxx.vhd testmode=1
```
- Use the highest graphical resolution
```
kernel /ntloader uuid=%?% file=/xxx.wim hires=1
```
- Disable hardware abstraction layer (HAL) and kernel detection.
```
kernel /ntloader uuid=%?% detecthal=0
```
- Load the Registry SYSTEM hive as a volatile hive (WinPE mode)
```
kernel /ntloader uuid=%?% minint=1
```
- Disable the use of VGA modes
```
kernel /ntloader uuid=%?% novga=1
```
- Disable VESA BIOS calls
```
kernel /ntloader uuid=%?% novesa=1
```
- Configure *Physical Address Extension* (PAE)
```
kernel /ntloader uuid=%?% pae=Enable|Disable|Default
```
- Configure *Data Execution Prevention* (DEP)
```
kernel /ntloader uuid=%?% nx=OptIn|OptOut|AlwaysOff|AlwaysOn
```
- Set Windows load options
```
kernel /ntloader uuid=%?% file=/xxx.vhd loadopt=XXX
```
- Specify the path of `winload.exe` or `winload.efi`
```
kernel /ntloader uuid=%?% winload=/Windows/System32/winload.efi
```
- Specify system root
```
kernel /ntloader uuid=%?% file=/xxx.vhd sysroot=/Windows
```
- Boot Windows 7 (UEFI)
```
kernel /ntloader uuid=%?% win7
```
- Disable debug messages
    You can disable debug messages by using the 'quiet' command-line option.
    For example:
```
kernel /ntloader uuid=%?% file=/xxx.vhd quiet
```
- Troubleshoot
    You can add the 'pause' command-line option for ntloader.
    
    This will instruct ntloader to wait for a keypress
    
     before booting Windows, to give you a further opportunity
    
     to observe any messages that may be displayed.
    
    For example:
```
kernel /ntloader uuid=%?% file=/xxx.vhd pause
```

## Compilation

You will need to have at least the following packages installed in order to build NTloader:

- gcc
- binutils
- make
- python3
- libz
- zip

You can use the following command to create initrd.cpio:
```
cd utils/rootfs
find * | cpio -o -H newc > ../initrd.cpio
```

## License

NTloader is released under the GPLv2+ License. See LICENSE.txt for details.

