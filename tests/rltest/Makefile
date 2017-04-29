#

MACRO11 = ../../macro11/macro11
DUMP11 = ../../macro11/dumpobj

all: rltest.img

%.img: %.mac
	rm -f a.out
	$(MACRO11) -o $*.obj -l $*.lst $<
	$(DUMP11) -r $*.obj $*.img


clean:
	rm -f *.mem *.img *.obj *.lst

