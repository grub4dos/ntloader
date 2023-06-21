#!/bin/sh
mkdir -p raw
cd utils/rootfs
find * | cpio -o -H newc > ../initrd.cpio
cd ../..
cp utils/initrd.cpio raw/
cp ntloader raw/
cp ntloader.i386 raw/
cp README.md raw/
cp LICENSE.txt raw/
zip -9 -r -j ntloader.zip raw
