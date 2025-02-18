<br />
<div align="center">
  <h2 align="center">NTloader</h2>
</div>
<br />

**Ntloader** is a bootloader for NT6+ WIM, VHD and VHDX images.

## Compatibility

**Ntloader** supports BIOS (i386) and UEFI (i386, x86_64, arm64) environments.  
It is recommended to use GRUB2 (>=2.12) or GRUB4DOS to boot NTloader.  

| Platform      | ntloader | ntloader.i386 | ntloader.arm64 |
| :------------ | :-: | :-: | :-: |
| BIOS (i386)   | yes | yes | no  |
| UEFI (i386)   | no  | yes | no  |
| UEFI (x86_64) | yes | no  | no  |
| UEFI (arm64)  | no  | no  | yes |

## Download

The source code is maintained in a git repository at https://github.com/grub4dos/ntloader.  
You can download the latest version of the binary from [GitHub Releases](https://github.com/grub4dos/ntloader/releases).  

<div style="page-break-after: always;"></div>

