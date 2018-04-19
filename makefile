CC		= gcc
CFLAGS	= -DNO_CLIB -DNO_ZLIB
AS		= yasm
ASFLAGS = -felf64
LDFLAGS = -nostartfiles -nostdlib
ARCH	= amd64

llxterm: libpsf.o term.o $(ARCH)_syscalls.o crt.o mem.o llx.o
	$(CC) -g -o $@ $(LDFLAGS) $^ -fno-stack-protector

%.o: %.c
	$(CC) -g $(CFLAGS) -c $< -fno-stack-protector

%.o: %.s
	$(AS) $(ASFLAGS) $<

clean:
	rm *.o
