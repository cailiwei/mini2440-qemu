#!/bin/bash
#
# Run with script with 
# -sd <SD card image file> to have a SD card loaded
# -kernel <kernel uImage file> to have a kernel preloaded in RAM
#

base=$(dirname $0)

echo Starting in $base

name_nand="$base/mini2440_nand128.bin"

if [ ! -f "$name_nand" ]; then
	echo $0 : creating NAND empty image : "$name_nand"
	dd if=/dev/zero of="$name_nand" bs=528 count=127100
	echo "** NAND file created - make sure to 'nand scrub' in u-boot"
fi

#	-kernel /tftpboot/uImage
cmd="$base/../arm-softmmu/qemu-system-arm \
	-M mini2440 $* \
	-kernel $base/uImage \
	-serial stdio \
	-mtdblock "$name_nand" \
	-show-cursor \
	-usb -usbdevice keyboard -usbdevice mouse \
	-monitor telnet::5555,server,nowait"

echo $cmd
$cmd
