CC=arm-elf-gcc
LD=arm-elf-ld
AR=arm-elf-ar
AS=arm-elf-as
OBJCOPY=arm-elf-objcopy
OBJDUMP=arm-elf-objdump

CFLAGS=-nostartfiles -nostdlib -nodefaultlibs -g -O2
LDFLAGS=-Tloader.lds

all: loader128.bin loader256.bin

headers: loader128.bin loader256.bin
	gcc -o bin2c bin2c.c 
	./bin2c loader128_data < loader128.bin > ../loader128_data.h 
	./bin2c loader256_data < loader256.bin > ../loader256_data.h

loader128.elf: crt0.o loader128.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^

loader256.elf: crt0.o loader256.o
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^


loader128.o: loader.c
	${CC} ${CFLAGS} -DPAGE_SIZE=128 -c -o $@ $^ 

loader256.o: loader.c
	${CC} ${CFLAGS} -DPAGE_SIZE=256 -c -o $@ $^ 

%.bin: %.elf
	${OBJCOPY} -O binary $^ $@

clean:
	rm -f loader128.o loader256.o loader128.elf loader256.elf \
		loader128.bin loader256.bin crt0.o bin2c bin2c.exe