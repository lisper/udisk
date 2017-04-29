#!/bin/sh

(while read line; do
  echo $line
done) <<EOF
<?xml version="1.0"?>
<!-- made with maketraces.sh -->

<config>
 <decors>
  <decor name="default">
   <trace-state-colors>
    <named-color name="font" color="#00ff00"/>
    <named-color name="low" color="#00ff00"/>
    <named-color name="high" color="#00ff00"/>
    <named-color name="x" color="#00ff00"/>
    <named-color name="xfill" color="#008000"/>
    <named-color name="trans" color="#00ff00"/>
    <named-color name="mid" color="#00ff00"/>
    <named-color name="vtrans" color="#00ff00"/>
    <named-color name="vbox" color="#00ff00"/>
    <named-color name="unloaded" color="#800000"/>
    <named-color name="analog" color="#00ff00"/>
    <named-color name="clip" color="#ff0000"/>
    <named-color name="req" color="#ff0000"/>
    <named-color name="ack" color="#008000"/>
    <named-color name="hbox" color="#ffffff"/>
   </trace-state-colors>
  </decor>
 </decors>

 <trace-groups>
  <trace-group name="default" decor="default">
  </trace-group>
 </trace-groups>

 <pane-colors>
  <named-color name="back" color="#181818"/>
  <named-color name="grid" color="#808080"/>
  <named-color name="mark" color="#0000ff"/>
  <named-color name="umark" color="#ffff00"/>
  <named-color name="pfont" color="#ffffff"/>
 </pane-colors>

 <markers>
  <marker name="primary" time="0 s"/>
 </markers>

 <traces>
EOF

# check for iverilog
iverilog=0

if  head udisk.vcd | grep -q Icarus ; then
    iverilog=1
fi

(while read -r signalname format; do
#
#iverilog
   if [ $iverilog == "1" ]; then
     if [ "${signalname:0:1}" == "\\" ]; then
        signalname=${signalname:1}
     fi
  fi
  if [ "$signalname" == "---" ]; then
      echo '  <separator name="">'
      echo '  </separator>'
  else
      signalname="udisk_test.$signalname"
      signalmode="oct"
      if [ "$format" != "" ]; then
	  signalmode="$format";
      fi
      echo '  <trace name="'$signalname'" mode="'$signalmode'" rjustified="yes">'
      echo '    <signal name="'$signalname'"/>'
      echo '  </trace>'
  fi
done) <<EOF
CLK
cpld.RESET_L
cpld.MSYN_IN
cpld.BBSY_IN
cpld.C0_IN
cpld.C1_IN
cpld.BUS_ADDR[17:0] bin
cpld.addr_out[17:0] bin
cpld.BUS_ADDR_DIR
cpld.BUS_DATA[15:0] hex
cpld.SSYN_OUT
cpld.ssyn_assert
cpld.addr_match
cpld.have_match1
cpld.have_match2
cpld.addr_match_1[17:2] hex
cpld.addr_match_2[17:2] hex
cpld.mask1[17:2] hex
cpld.mask2[17:2] hex
cpld.CPU_INT
---
cpld.state[2:0] hex
cpld.next_state[2:0] hex
---
cpld.CPU_RD
cpld.CPU_WR
cpld.cpu_addr[3:0] hex
cpld.CPU_D[15:0] hex
cpld.assert_pin[15:0] hex
---
cpld.cf_enable[1:0] binary
CF_CS0_N
CF_CS1_N
DISK_RESET_N
CF_IORD_N
CF_IOWR_N
---
EOF

echo " </traces>"
echo "</config>"
exit 0

