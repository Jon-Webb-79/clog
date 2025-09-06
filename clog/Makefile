# avrMakefile
# ================================================================================
# ================================================================================
# - Purpose: This file contains scripts necessary to compile and upload an
#            avr sketch
#
# Source Metadata
# - Author:  Jonathan A. Webb
# - Date:    February 25, 2022
# - Version: 1.0
# - Copyright: Copyright 2022, Jon Webb Inc.
# ================================================================================
# ================================================================================
# Re-title this document as Makefile

FILENAME   = filename here without .c extension
#FILENAME2 = Second filename
PORT       = /dev/ttyACM0
DEVICE     = atmega328p
PROGRAMMER = arduino
BAUD       = 115200
COMPILE    = avr-gcc -Wall -Os -mmcu=$(DEVICE)

default: compile upload clean

compile:
	$(COMPILE) -c $(FILENAME).c -o $(FILENAME).o
	#$(COMPILE) -c $(FILENAME2).c -o $(FILENAME).o
	$(COMPILE) -o $(FILENAME).elf $(FILENAME).o
	avr-objcopy -j .text -j .data -O ihex $(FILENAME).elf $(FILENAME).hex 
	avr-size --format=avr --mcu=$(DEVICE) $(FILENAME).elf

upload:
	avrdude -v -p $(DEVICE) -c $(PROGRAMMER) -P $(PORT) -b $(BAUD) -U flash:w:$(FILENAME).hex:i

clean:
	rm $(FILENAME).o
	rm $(FILENAME).elf
	rm $(FILENAME).hex
# ================================================================================
# ================================================================================
