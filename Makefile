DEVICE=attiny85
DEFINES=-DF_CPU=1000000
LFUSE=0x62
HFUSE=0xdf
EFUSE=0xff
FUSES= -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m

default:
	avr-gcc -g -Wall -Os -mmcu=$(DEVICE) $(DEFINES) -c USI_TWI_Master.c
	avr-gcc -g -Wall -Os -mmcu=$(DEVICE) $(DEFINES) -c main.c
	avr-gcc -g -mmcu=$(DEVICE) -o main.elf main.o USI_TWI_Master.o
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-size --format=avr --mcu=$(DEVICE) main.elf

install:
	avrdude -c usbtiny -p $(DEVICE) -U flash:w:main.hex:i

fuse:
	avrdude -c usbtiny -p $(DEVICE) $(FUSES)
