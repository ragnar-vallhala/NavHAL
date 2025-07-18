CC = arm-none-eabi-gcc
CFLAGS = -Wall -O0 -mcpu=cortex-m4 -mthumb

all: main.elf

main.elf: main.c startup.s linker.ld
	$(CC) $(CFLAGS) -nostdlib -Tlinker.ld startup.s main.c -o main.elf

main.bin: main.elf
	arm-none-eabi-objcopy -O binary $< $@

flash: main.bin
	st-flash write main.bin 0x08000000
clean:
	rm -f *.elf
