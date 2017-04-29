#!/bin/bash

echo $ISE
#exit 0

mkdir -p $BASE/ise/xst/projnav.tmp

source $ISE/settings32.sh && cd ise && ls -l *.xst && \
 xst -intstyle ise -ifn "$BASE/ise/udisk_cpld.xst" -ofn "$BASE/ise/udisk_cpld.syr" && \
 ngdbuild -intstyle ise -dd _ngo -uc $BASE/cpld/udisk_cpld.ucf -p xcr3256xl-TQ144-12 udisk_cpld.ngc udisk_cpld.ngd  && \
 cpldfit -intstyle ise -p xcr3256xl-12-TQ144 -ofmt vhdl -optimize density -htmlrpt -loc on -slew fast -init low -inputs 32 -unused pullup -terminate float -pterms 28 -nofbnand udisk_cpld.ngd && \
 XSLTProcess udisk_cpld_build.xml && \
 tsim -intstyle ise udisk_cpld udisk_cpld.nga && \
 hprep6 -i udisk_cpld

# taengine -intstyle ise -f udisk_cpld -w --format html1 -l udisk_cpld_html/tim/timing_report.htm &&

