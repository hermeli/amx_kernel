#!/bin/sh

make ARCH="arm" CROSS_COMPILE="$(echo $PWD)/../buildroot/output/host/usr/bin/arm-unknown-linux-uclibcgnueabi-"
make uImage ARCH="arm" CROSS_COMPILE="$(echo $PWD)/../buildroot/output/host/usr/bin/arm-unknown-linux-uclibcgnueabi-"
