#!/bin/sh

src=$1
lst=`basename $1 .c`.lst

/opt/tools/arm/gnuarm-3.4.3/bin/arm-elf-gcc -DSVN_REV=29  -mcpu=arm7tdmi -g -O2  -g -Wa,-aldh -c ${src} >${lst}

