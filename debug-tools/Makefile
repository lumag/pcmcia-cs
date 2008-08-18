#
# debug-tools/Makefile 1.44 2002/12/21 08:23:22 (David Hinds)
#

# Include site dependent options
include ../config.mk

XFLAGS = -O -pipe
CPPFLAGS += -I../modules
CC = $(UCC) $(UFLAGS)

SRCS  = dump_cis.c dump_cisreg.c pack_cis.c dump_exca.c
HDRS  = pack_cis.h
TOOLS = dump_cis dump_cisreg pack_cis dump_exca

ifdef CONFIG_PNP_BIOS
SRCS  += lspnp.c setpnp.c
TOOLS += lspnp setpnp
endif

ifdef CONFIG_ISA
SRCS  += dump_tcic.c
TOOLS += dump_tcic
endif

ifdef CONFIG_PCI
SRCS  += dump_cardbus.c
TOOLS += dump_cardbus
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
	@mkdir -p $(PREFIX)/sbin
	cp -f dump_cis pack_cis $(PREFIX)/sbin
ifdef CONFIG_PNP_BIOS
	cp -f lspnp setpnp $(PREFIX)/sbin
	@mkdir -p $(PREFIX)/usr/share
	cp -f pnp.ids $(PREFIX)/usr/share
endif

include ../rules.mk
