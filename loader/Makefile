# udisk firmware - loader
# $Id: Makefile 65 2013-11-17 17:53:53Z brad $

CROSS=/opt/tools/arm/gnuarm-3.4.3/bin/arm-elf-
MACH=-mcpu=arm7tdmi

DEFINES += -DDEBUG_VECTORS

CC=$(CROSS)gcc
LD=$(CROSS)ld
OBJDUMP=$(CROSS)objdump
OBJCOPY=$(CROSS)objcopy

ASM_SRC = start.S

C_SRC = startup.c debug.c main.c usb.c samba.c flash.c led.c cli.c usart.c printf.c support.c

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


TEXTADDR_ROM = 0x100000
	
firmware.elf: $(OBJ)
	$(LD) -o $@ -T rom.lds -Ttext $(TEXTADDR_ROM) -Map $*.map $(OBJ)

firmware.bin: firmware.elf
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -D $< >$@.txt
	cp $@ /tftpboot/firmware.bin

# temporary test image for running in ram
ram.elf: $(OBJ)
	$(LD) -o $@ -T ram.lds -Ttext 0x200000 -Map $*.map $(OBJ)

ram.bin: ram.elf
	$(OBJCOPY) -O binary $< $@
	$(OBJDUMP) -D $< >$@.txt
	cp $@ /tftpboot/ram


ram.lst:
	$(CC) -T'ram.lds' -Ttext 0x200000 -Wa,"-ahl=ram.lst" $(DEFINES) start.o $(C_SRC) -o ram.elf

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
	rm -f *.o firmware firmware.out fireware.elf firmware.dis firmware.map firmware.bin
	rm -f ram ram.out ram.elf ram.map ram.bin ram.bin.txt
	rm -f .depend
	rm -f *.lst

.depend: $(C_SRC)
	$(CC) -MM $(CFLAGS) $(C_SRC) > .depend
