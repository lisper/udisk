
#
BASE = $(shell pwd)
ISE = /opt/Xilinx/13.1/ISE_DS

export BASE
export ISE

clean:
	(cd cpld; rm -f *~)
	(cd cpld; rm -f *.log *.vcd)
	(cd ise; rm -f *.html *.nga *.gyd *.cxt *.mfd *.pnx *.vm6 *.csv *.rpt *.chk *.xrpt *.bld *.ngd *.stx *.syr *.ngc *.ngr *.tim *.tspec *.pad *.err)
	(cd ise; rm -rf xst/* _xmsgs/* xlnx_auto_0_xdb _ngo)

cpld:
	./make-cpld.sh
