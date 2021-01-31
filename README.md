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
                Copyright (C) 2021 A1ive.
```

## Usage

### Boot Windows NT6+ WIM/VHD/VHDX

- GRUB4DOS
```
title Boot Windows NT6+ PE
uuid (hdx,y)
kernel /ntloader uuid=%?% file=/path/to/winpe.wim
initrd /initrd.lz1
```

- a1ive GRUB2 
```
menuentry "Boot Windows NT6+ PE" {
    probe -s dev_uuid -u (hdx,y);
    if [ "${grub_platform}" = "efi" ];
    then
        linuxefi /ntloader uuid=${dev_uuid} file=/path/to/winpe.wim;
        initrdefi /initrd.lz1;
    else
        linux16 /ntloader uuid=${dev_uuid} file=/path/to/winpe.wim;
        initrd16 /initrd.lz1;
    fi;
}
```
- GNU GRUB2
  
```
menuentry "Boot Windows NT6+ PE" {
    probe -s dev_uuid -u (hdx,y);
    if [ "${grub_platform}" = "efi" ];
    then
        chainloader /ntloader initrd=/initrd.lz1 uuid=${dev_uuid} file=/path/to/winpe.wim;
    else
        linux16 /ntloader uuid=${dev_uuid} file=/path/to/winpe.wim;
        initrd16 /initrd.lz1;
   fi;
}
```

### Boot Windows NT6+ OS

- GRUB4DOS
```
title Boot Windows NT6+ PE
uuid (hdx,y)
kernel /ntloader uuid=%?%
initrd /initrd.lz1
```

- a1ive GRUB2 
```
menuentry "Boot Windows NT6+ PE" {
    probe -s dev_uuid -u (hdx,y);
    if [ "${grub_platform}" = "efi" ];
    then
        linuxefi /ntloader uuid=${dev_uuid};
        initrdefi /initrd.lz1;
    else
        linux16 /ntloader uuid=${dev_uuid};
        initrd16 /initrd.lz1;
    fi;
}
```
- GNU GRUB2
  
```
menuentry "Boot Windows NT6+ PE" {
    probe -s dev_uuid -u (hdx,y);
    if [ "${grub_platform}" = "efi" ];
    then
        chainloader /ntloader initrd=/initrd.lz1 uuid=${dev_uuid};
    else
        linux16 /ntloader uuid=${dev_uuid};
        initrd16 /initrd.lz1;
   fi;
}
```

### Advanced options

- Disable debug messages
    You can disable debug messages by using the 'quiet' command-line option.
    For example:
```
kernel /ntloader uuid=%?% file=/xxx.vhd quiet
```
- Disable graphical boot messages
    You can force the Windows boot manager to display error messages
     in text mode by using the 'text' command-line option.
    For example:
```
kernel /ntloader uuid=%?% file=/xxx.vhd gui
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

You can use the following command to create initrd.cpio:
```
cd utils/rootfs
find * | cpio -o -H newc > ../initrd.cpio
cd ..
```
You can also use lznt1 to compress initrd to reduce its size:
```
python lznt1.py initrd.cpio initrd.lz1
```

## Bugs
MSDOS logical partitions are not supported.