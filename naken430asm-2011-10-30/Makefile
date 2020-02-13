include config.mak

default:
	$(CC) -c assembler.c $(CFLAGS)
	$(CC) -c elf_parse.c $(CFLAGS)
	$(CC) -c eval_expression.c $(CFLAGS)
	$(CC) -c file_output.c $(CFLAGS)
	$(CC) -c get_tokens.c $(CFLAGS)
	$(CC) -c ifdef_expression.c $(CFLAGS)
	$(CC) -c lookup_tables.c $(CFLAGS)
	$(CC) -c macros.c $(CFLAGS)
	$(CC) -c msp430_asm.c $(CFLAGS)
	$(CC) -c hex_parse.c $(CFLAGS)
	$(CC) -c msp430_disasm.c $(CFLAGS)
	$(CC) -c msp430_simulate.c $(CFLAGS)
	$(CC) -o naken430asm$(CONFIG_EXT) naken430asm.c assembler.o \
	    eval_expression.o file_output.o get_tokens.o ifdef_expression.o \
	    lookup_tables.o macros.o msp430_asm.o msp430_disasm.o \
	   -DINCLUDE_PATH="\"$(INCLUDE_PATH)\"" \
	   $(CFLAGS) $(LDFLAGS)
	$(CC) -o naken430util$(CONFIG_EXT) naken430util.c elf_parse.o \
	   hex_parse.o msp430_disasm.o msp430_simulate.o \
	   $(CFLAGS) $(LDFLAGS) $(LDFLAGS_UTIL)

util:
	$(CC) -c hex_parse.c $(CFLAGS)
	$(CC) -c jtag.c $(CFLAGS)
	$(CC) -c msp430_disasm.c $(CFLAGS)
	$(CC) -o naken430util naken430util.c jtag.o hex_parse.o \
	   msp430_disasm.o $(CFLAGS)

install:
	@mkdir -p $(INSTALL_BIN)
	@mkdir -p $(INSTALL_INCLUDES)/include
	@cp naken430asm $(INSTALL_BIN)
	@cp naken430util $(INSTALL_BIN)
	@cp -r include/*.inc $(INSTALL_INCLUDES)/include


uninstall:
	@echo "Scary stuff.  Just erase these things by hand."
	#@rm -f $(INSTALL_BIN)/naken430asm
	#@rm -f $(INSTALL_BIN)/naken430util
	#@rm -rf $(INSTALL_INCLUDES)

testing: launchpad_blink.asm
	msp430-as launchpad_blink.asm
	msp430-objcopy -F ihex a.out testing.hex

clean:
	@rm -f naken430asm naken430util *.exe *.o *.hex a.out *.lst *.ndbg *.elf
	@echo "Clean!"

dist-clean: clean
	@rm -f config.mak *.asm


