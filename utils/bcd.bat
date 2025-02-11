rd /s /q build
md build
cd build

set bcd=/store bcd

set guidwim={19260817-6666-8888-abcd-000000000000}
set guidvhd={19260817-6666-8888-abcd-000000000001}
set guidwin={19260817-6666-8888-abcd-000000000002}
set guidhib={19260817-6666-8888-abcd-000000000003}

set guidopt={19260817-6666-8888-abcd-100000000000}
set guidsbt={19260817-6666-8888-abcd-200000000000}

set guidhir={19260817-6666-8888-abcd-110000000000}
set guidlor={19260817-6666-8888-abcd-120000000000}

set guidram={19260817-6666-8888-abcd-101000000000}

set winload=\WINLOAD0000000000000000000000000000000000000000000000000000000
set windir=\WINDIR000000000000000000000000
set wincmd=DDISABLE_INTEGRITY_CHECKS000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

set winresume=\WINRESU0000000000000000000000000000000000000000000000000000000

set winhibr=\WINHIBR0000000000000000000000000000000000000000000000000000000

set wimfile=\000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.wim

set vhdfile=\000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000.vhd

bcdedit /createstore bcd
bcdedit %bcd% /create {bootmgr} /d "NTloader"
bcdedit %bcd% /set {bootmgr} locale en-us
bcdedit %bcd% /set {bootmgr} timeout 1
bcdedit %bcd% /set {bootmgr} displaybootmenu false

bcdedit %bcd% /create {globalsettings}
bcdedit %bcd% /set {globalsettings} optionsedit true
bcdedit %bcd% /set {globalsettings} advancedoptions true
bcdedit %bcd% /set {globalsettings} locale en-us

bcdedit %bcd% /create {bootloadersettings}

bcdedit %bcd% /create %guidlor% /inherit OSLOADER
bcdedit %bcd% /set %guidlor% graphicsresolution 1024x768

bcdedit %bcd% /create %guidhir% /inherit OSLOADER
bcdedit %bcd% /set %guidhir% highestmode true

bcdedit %bcd% /create %guidopt% /inherit OSLOADER
bcdedit %bcd% /set %guidopt% detecthal true
bcdedit %bcd% /set %guidopt% winpe true
bcdedit %bcd% /set %guidopt% testsigning true
bcdedit %bcd% /set %guidopt% pae default
bcdedit %bcd% /set %guidopt% nx OptIn
bcdedit %bcd% /set %guidopt% novesa true
bcdedit %bcd% /set %guidopt% novga true
bcdedit %bcd% /set %guidopt% nointegritychecks true
REM bcdedit %bcd% /set %guidopt% flightsigning true
REM bcdedit %bcd% /set %guidopt% integrityservices disable
REM bcdedit %bcd% /set %guidopt% isolatedcontext true
REM bcdedit %bcd% /set %guidopt% bootmenupolicy Legacy
bcdedit %bcd% /set %guidopt% loadoptions %wincmd%
bcdedit %bcd% /set %guidopt% inherit %guidhir%

bcdedit %bcd% /create %guidsbt% /inherit OSLOADER
bcdedit %bcd% /set %guidsbt% safeboot minimal
bcdedit %bcd% /set %guidsbt% safebootalternateshell yes

bcdedit %bcd% /create {ramdiskoptions}
bcdedit %bcd% /set {ramdiskoptions} ramdisksdidevice "boot"
bcdedit %bcd% /set {ramdiskoptions} ramdisksdipath \boot\boot.sdi

bcdedit %bcd% /create %guidram% /device
bcdedit %bcd% /set %guidram% ramdiskimageoffset 65536
bcdedit %bcd% /set %guidram% exportascd yes

bcdedit %bcd% /create %guidwim% /d "NT6+ WIM" /application OSLOADER
bcdedit %bcd% /set %guidwim% device ramdisk=[C:]%wimfile%,{ramdiskoptions}
bcdedit %bcd% /set %guidwim% osdevice ramdisk=[C:]%wimfile%,{ramdiskoptions}
bcdedit %bcd% /set %guidwim% systemroot %windir%
bcdedit %bcd% /set %guidwim% path %winload%
bcdedit %bcd% /set %guidwim% inherit %guidopt% %guidsbt%

bcdedit %bcd% /create %guidvhd% /d "NT6+ VHD" /application OSLOADER
bcdedit %bcd% /set %guidvhd% device vhd=[C:]%vhdfile%
bcdedit %bcd% /set %guidvhd% osdevice vhd=[C:]%vhdfile%
bcdedit %bcd% /set %guidvhd% systemroot %windir%
bcdedit %bcd% /set %guidvhd% path %winload%
bcdedit %bcd% /set %guidvhd% inherit %guidopt% %guidsbt%

bcdedit %bcd% /create {resumeloadersettings}

bcdedit %bcd% /create %guidhib% /d "NT6+ Resume" /application RESUME
bcdedit %bcd% /set %guidhib% device partition=C:
bcdedit %bcd% /set %guidhib% path %winresume%
bcdedit %bcd% /set %guidhib% filedevice partition=C:
bcdedit %bcd% /set %guidhib% filepath %winhibr%
bcdedit %bcd% /set %guidhib% inherit {resumeloadersettings}

bcdedit %bcd% /create %guidwin% /d "NT6+ OS" /application OSLOADER
bcdedit %bcd% /set %guidwin% device partition=C:
bcdedit %bcd% /set %guidwin% osdevice partition=C:
bcdedit %bcd% /set %guidwin% systemroot %windir%
bcdedit %bcd% /set %guidwin% path %winload%
bcdedit %bcd% /set %guidwin% inherit %guidopt% %guidsbt%
bcdedit %bcd% /set %guidwin% resumeobject %guidhib%

bcdedit %bcd% /set {bootmgr} displayorder %guidwim% /addlast
bcdedit %bcd% /default %guidwim%

cd ..