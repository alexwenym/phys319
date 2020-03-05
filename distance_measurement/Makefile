OBJECTS=main.o
DEVICE  = msp430g2553
INSTALL_DIR=$(HOME)/ti

GCC_DIR =  $(INSTALL_DIR)/bin
SUPPORT_FILE_DIRECTORY = $(INSTALL_DIR)/include

CC      = $(GCC_DIR)/msp430-elf-gcc
GDB     = $(GCC_DIR)/msp430-elf-gdb

CFLAGS = -I $(SUPPORT_FILE_DIRECTORY) -mmcu=$(DEVICE) -O2 -g 
LFLAGS = -L $(SUPPORT_FILE_DIRECTORY) -T $(DEVICE).ld

elf: ${OBJECTS}
	$(CC) $(CFLAGS) $(LFLAGS) $? -g -o main.elf
asm: 
	$(CC) $(CFLAGS) $(LFLAGS) $? -S main.c

#all: ${OBJECTS}
#	$(CC) $(CFLAGS) $(LFLAGS) $? -o main.elf

debug: all
	$(GDB) main.elf
