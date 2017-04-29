#
#
#

VERILOG=cver

all: crun

crun: $(PARTS) udisk_test.v udisk_cpld.v
	$(VERILOG) udisk_test.v

display:
	./maketraces.sh >traces
	gtkwave udisk.vcd traces

