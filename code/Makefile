# udisk firmware
# $Id: Makefile 70 2013-11-17 17:59:13Z brad $

CROSS=/opt/tools/arm/gnuarm-3.4.3/bin/arm-elf-
MACH=-mcpu=arm7tdmi

DEFINES += -DDEBUG_VECTORS

CC=$(CROSS)gcc
LD=$(CROSS)ld
OBJDUMP=$(CROSS)objdump
OBJCOPY=$(CROSS)objcopy

ASM_SRC = start.S

DSK = rl.c
#DSK = rk.c

C_SRC = startup.c debug.c main.c unibus.c $(DSK) \
	cli.c irq.c usart.c printf.c bus.c pio.c \
	disk.c ide.c tests.c boots.c

HDRS = main.h AT91SAM7S128.h

OBJ = $(ASM_SRC:.S=.o) $(C_SRC:.c=.o)

# extract the SVN rev # from main.c
SVN_REV = $(shell grep -m 1 Id main.c | cut -d ' ' -s -f 5)
DEFINES += -DSVN_REV=$(SVN_REV)

CFLAGS = $(DEFINES) $(INC) $(MACH) -g -O3
#CFLAGS = $(DEFINES) $(INC) $(MACH) -g

#
# targets
#

all: .depend firmware.bin ram.bin

ifeq (.depend, $(wildcard .depend))
include .depend
endif


TEXTADDR_ROM = 0x110000

firmware.out: $(OBJ) rom.o
	$(LD) -o $@ -T rom.lds -Ttext $(TEXTADDR_ROM) -Map $*.map $(OBJ) rom.o

firmware.bin: firmware.out
	$(OBJCOPY) -O binary $< $@
	cp $@ /tftpboot/firmware.bin

# temporary test image for running in ram
ram.out: $(OBJ) ram.o
	$(LD) -o $@ -T ram.lds -Ttext 0x200000 -Map $*.map $(OBJ) ram.o

ram.bin: ram.out
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -D $< >$@.txt
	cp $@ /tftpboot/ram


ram.lst:
	$(CC) -T'ram.lds' -Ttext 0x200000 -Wa,"-ahl=ram.lst" $(DEFINES) start.o $(C_SRC) -o ram.out

start.o: start.S
	$(CC) $(INC) $(DEFINES) -c start.S

startup.o: startup.c
	$(CC) $(CFLAGS) -c startup.c

debug.o: debug.c
	$(CC) $(CFLAGS) -c debug.c

.c.o: $(HDRS)
	$(CC) $(CFLAGS) -c $<

tags:
	etags $(SRC)

clean:
	rm -f *.o firmware firmware.out firmware.dis firmware.map firmware.bin
	rm -f ram ram.out ram.map ram.bin ram.bin.txt
	rm -f .depend
	rm -f *.lst

.depend: $(C_SRC)
	$(CC) -MM $(CFLAGS) $(C_SRC) > .depend
