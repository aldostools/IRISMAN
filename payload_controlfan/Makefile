PPU_CC = ppu-gcc
PPU_OBJCOPY = ppu-objcopy

all: ps3_controlfan.bin

%.o : %.S
	$(PPU_CC) -c $< -o $@

%.bin : %.o
	$(PPU_OBJCOPY) -O binary $< $@
	cp -f $@ ../datas/$@

clean:
	rm -f *.o *.bin
