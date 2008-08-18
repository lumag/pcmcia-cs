#
# debug-tools/Makefile 1.38 2000/01/27 00:26:28 (David Hinds)
#

# Include site dependent options
include ../config.mk

CFLAGS = -O -Wall -Wstrict-prototypes -pipe
XFLAGS = -O -pipe
CPPFLAGS := $(CPPFLAGS) -I../modules

CC := $(CC) $(UFLAGS)

SRCS = dump_cis.c dump_cisreg.c pack_cis.c
HDRS = pack_cis.h
TOOLS = dump_cis dump_cisreg pack_cis

ifdef CONFIG_PNP_BIOS
SRCS := $(SRCS) lspnp.c setpnp.c
TOOLS := $(TOOLS) lspnp setpnp
endif

ifdef CONFIG_ISA
SRCS := $(SRCS) dump_tcic.c dump_i365.c 
TOOLS := $(TOOLS) dump_tcic dump_i365
endif

all:	$(SRCS) $(HDRS) $(TOOLS)

dump_cis: dump_cis.o

dump_i365: dump_i365.o

dump_tcic: dump_tcic.o

dump_cisreg: dump_cisreg.o

lspnp: lspnp.o

setpnp: setpnp.o

yacc_cis.o: yacc_cis.c
	$(CC) -MD $(XFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

lex_cis.o: lex_cis.c yacc_cis.h
	$(CC) -MD $(XFLAGS) $(CPPFLAGS) -c $<
	@mkdir -p .depfiles ; mv $*.d .depfiles

pack_cis: pack_cis.o lex_cis.o yacc_cis.o
	$(CC) $+ -o $@ -lm

parser: lex_cis.o yacc_cis.c
	$(CC) $(XFLAGS) $(CPPFLAGS) -DDEBUG -o $@ $+

clean:
	rm -f core core.* *.o *.s *.a *~ .depend .depfiles/*.d
	rm -f dump_i365 dump_tcic $(TOOLS)

install: $(TOOLS)
	mkdir -p $(PREFIX)/sbin
	cp -f dump_cis pack_cis $(PREFIX)/sbin
ifdef CONFIG_PNP_BIOS
	cp -f lspnp setpnp $(PREFIX)/sbin
	mkdir -p $(PREFIX)/usr/share
	cp -f pnp.ids $(PREFIX)/usr/share
endif

include ../rules.mk
